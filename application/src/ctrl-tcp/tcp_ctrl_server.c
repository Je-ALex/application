/*
 *
 *	控制类消息的TCP线程 主要功能是接收和处理tcp控制类消息
 *	上位机的通讯部分
 *	单元机的通讯部分
 *	从服务端角度出发，接收客户端连接请求后，需要知道的客户端的消息，如下
 *	1.TCP中客户端的fd，后续下发指令需要用到
 *	2.客户端的IP/MAC地址，主要是识别具体的单元机
 *
 *	在TCP控制模块主要处理的工作，如下，
 *	1.逻辑控制类管理，如单元机关机
 *	2.会议类管理，扫描，ID设置，席别设置，签到
 *
 *	综上，服务端需要绑定fd和对应的ID（MAC）,控制消息才能准确下发
 *
 *  Created on: 2016年9月29日
 *      Author: leon
 */
#include "../../header/tcp_ctrl_server.h"

#include "../../header/tcp_ctrl_data_process.h"

#include "../../header/tcp_ctrl_list.h"

#include "../../header/tcp_ctrl_module_api.h"

int port = 8080;

pthread_t s_id;
pthread_mutex_t mutex;


pclient_node list_head;

/*
 * 服务端socket初始化
 *
 * 返回值
 * 错误码和成功后的端口号
 *
 */
static int tc_local_addr_init()
{
	struct sockaddr_in sin;
	int sock_fd;
	int ret;

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock_fd)
    {
        perror("call to socket");
        return SOCKERRO;
    }
    //reuse socket
    int opt=1;
    setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    ret = bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin));
    if (-1 == ret)
    {
        perror("call to bind");
        return BINDERRO;
    }
    ret = listen(sock_fd, 20);
    if (-1 == ret)
    {
        perror("call to listen");
        return LISNERRO;
    }

    return sock_fd;

}


/*
 * 设置socket_fd为非阻塞模式
 */
static void tc_set_noblock(int fd)
{
    int fl=fcntl(fd,F_GETFL);
    if(fl<0)
    {
        perror("fcntl");
        pthread_exit(0);
    }
    if(fcntl(fd,F_SETFL,fl|O_NONBLOCK))
    {
        perror("fcntl");
        pthread_exit(0);
    }
}


/*
 * 接收消息的初步处理
 * 分类：
 * 上位机通讯部分
 * 单元机通讯部分
 *
 * 参数：
 * 客户端FD
 * 接收到的消息
 *
 *
 */
static void tc_process_rev_msg(int* cli_fd, unsigned const char* value, int* length)
{

	/*
	 * 检查数据包是否是完整的数据包
	 * 数据头的检测
	 * 数据长度的检测
	 * 校验和的检测
	 */
	Pframe_type type;
	int i;
	char* handlbuf = NULL;

	type = (Pframe_type)malloc(sizeof(frame_type));
	memset(type,0,sizeof(frame_type));

	handlbuf = tc_frame_analysis(cli_fd,value,length,type);

	printf("type->dev_type = %d\n",type->dev_type);
	/*
	 * 设备类型分类
	 */
	switch(type->dev_type)
	{
		case PC_CTRL:
			printf("process the pc data\n");
			tc_from_pc(handlbuf,type);
			break;

		case HOST_CTRL:
			printf("process the unit data\n");
			tc_from_unit(handlbuf,type);
			break;

	}

	free(type);
	free(handlbuf);

}
/*
 * TCP控制模块线程
 * 主要是初始化TCP端子，设置服务端模式，采用epoll进行多终端管理
 * 客户端连接后 ，生成一个唯一的fd，保存在
 */
static void* control_tcp_recv(void* p)
{
	int sockfd;
	int epoll_fd;

	struct epoll_event event;   // 告诉内核要监听什么事件
    struct epoll_event* wait_event; //内核监听完的结果

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

    int i,n;
    int newfd;
    int ctrl_ret,wait_ret;
	int maxi = 0;

    /*
     *
     */
	int len = 0;
	unsigned char buf[1024] = {0};

    /*
     *socket init
     */
    sockfd = tc_local_addr_init();
    tc_set_noblock(sockfd);


    /*
     * epoll event init
     * 除了参数size被忽略外,此函数和epoll_create完全相同
     */
    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0){
        perror ("epoll_create");
        pthread_exit(0);
    }
    /*
     * epoll监听套接字
     * epoll事件参数设置
     */
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;

	//事件注册函数，将监听套接字描述符 sockfd 加入监听事件
    ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event);
    if(-1 == ctrl_ret){
        perror("epoll_ctl");
        pthread_exit(0);
    }

	wait_event = calloc(1000,sizeof(struct epoll_event));


	/*
	 * 终端信息录入结构体
	 */
	Pclient_info info;

	list_head = list_head_init();

	while(1)
	{
		printf("epoll_wait...\n");
		// 监视并等待多个文件（标准输入，udp套接字）描述符的属性变化（是否可读）
        // 没有属性变化，这个函数会阻塞，直到有变化才往下执行，这里没有设置超时
		wait_ret = epoll_wait(epoll_fd, wait_event, maxi+1, -1);
		if(wait_ret == -1)
		{
			perror("epoll_wait");
			break;
		}
		for (n = 0; n < wait_ret; ++n)
		{

			if (wait_event[n].data.fd == sockfd
					&& ( EPOLLIN == wait_event[n].events & (EPOLLIN|EPOLLERR)))
			{
				pthread_mutex_lock(&mutex);
				newfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								&clilen);
				if (newfd < 0) {
					perror("accept");
					continue;
				}
				else
				{

					tc_set_noblock(newfd);
					// 打印客户端的 ip 和端口
					printf("%d----------------------------------------------\n",newfd);
					printf("client ip=%s,port=%d\n", inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));


					/*
					 * 设备上线，发送一个信息获取
					 */
//					send(newfd, send_buf, send_len, 0);
//					/*
//					 * print the data
//					 */
//					for(i =0;i<send_len;i++)
//					{
//						printf("%02x ",send_buf[i]);
//					}
//					printf("\n");
					event.events = EPOLLIN | EPOLLET;
					event.data.fd = newfd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &event) < 0) {
						fprintf(stderr, "add socket '%d' to epoll failed %s\n",
								newfd, strerror(errno));
						pthread_exit(0);
					}
					maxi++;

					/*
					 *将新连接终端信息录入结构体中
					 *这里可以将fd和IP信息存入
					 */
					info = (Pclient_info)malloc(sizeof(client_info));
					memset(info,0,sizeof(client_info));
					info->client_fd = newfd;
					info->cli_addr = cli_addr;
					info->clilen = clilen;

					list_add(list_head,info);

					pthread_mutex_unlock(&mutex);
				}

			} else {

				pthread_mutex_lock(&mutex);
				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);
				pthread_mutex_unlock(&mutex);

				//客户端关闭连接
				if(len < 0 && errno != EAGAIN)
				{
//					printf("wait_event[n].data.fd--%d\n",wait_event[n].data.fd);

					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							wait_event[n].data.fd,&event);
					if( ctrl_ret < 0)
					{
					 fprintf(stderr, "delete socket '%d' from epoll failed! %s\n",
							 wait_event[n].data.fd, strerror(errno));
					}
					maxi--;
					close(wait_event[n].data.fd);

					/*
					 * 删除链表中节点信息
					 * fd参数
					 */
					list_delete(list_head,wait_event[n].data.fd);

				}
				else if(len == 0)//客户端关闭连接
				{
					printf("client %d offline\n",wait_event[n].data.fd);


					list_delete(list_head,wait_event[n].data.fd);
					close(wait_event[n].data.fd);
					maxi--;
				}
				else
				{
					getpeername(wait_event[n].data.fd,(struct sockaddr*)&cli_addr,
							&clilen);
					printf("client %s:%d\n",inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					tc_process_rev_msg(&wait_event[n].data.fd,buf,&len);

					//将测试数据返回发送
					//send(wait_event[n].data.fd, buf, len, 0);
				}
			}//end if

		}//end for

	}//end while

	free(wait_event);
    close(sockfd);
    pthread_exit(0);


}




/*
 * 扫描信息主要是MAC地址，IP地址信息等
 */
static void scanf_client()
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;
	int i;
	char get_client_mac[] = "mac";


	tmp = list_head->next;

	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			pthread_mutex_lock(&mutex);
			write(pinfo->client_fd, get_client_mac, sizeof(get_client_mac));
			pthread_mutex_unlock(&mutex);
		}
		tmp = tmp->next;

	}

}

void delete_client(int num)
{

	list_delete(list_head,num);

}
void view_client_info()
{

	pclient_node tmp = NULL;

	Pclient_info pinfo;

	tmp = list_head->next;

	printf("print the info\n");

	while(tmp != NULL)
	{
		pinfo = tmp->data;

		printf("client_fd--%d ,client_id--%d ,"
		"client_mac--%s ,cli_addr--%s port--%d",pinfo->client_fd,pinfo->client_id,
		pinfo->client_mac,inet_ntoa(pinfo->cli_addr.sin_addr),ntohs(pinfo->cli_addr.sin_port));
		tmp = tmp->next;
		printf("\n");
	}

}


static void* control_tcp_send(void* p)
{

	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */
	unsigned int id_num = 0;
	int c_seat = 0;
	char c_name[32] = {0};
	char c_subj[32] = {0};
	int s,fd;

	frame_type data_type;
	memset(&data_type,0,sizeof(data_type));

	while(1)
	{
		/*
		 * 模拟对单元机的控制
		 * ‘S’扫描单元机,0是获取mac
		 * ‘I’ID设置
		 * ‘X’席别
		 *
		 */
		scanf("%d",&s);
		if(s > 0)
		{
			scanf("%d",&fd);
//			scanf("%d",&data_type.msg_type);
//			//查询
//			if(data_type.msg_type == 2)
//			{
//				scanf("%d",&data_type.data_type);
//				scanf("%d",&data_type.name_type[0]);
//			}

		}
//		data_type.fd = fd;
//		data_type.dev_type = HOST_CTRL;
//
//		printf("s=%d,fd=%d,msg=%d,dt=%d,na=%d\n",s,data_type.fd,data_type.msg_type,
//				data_type.data_type,data_type.name_type[0]);

//		if(data_type.msg_type == Write_msg)
//		{
//			printf("please input the ID:\n");
//			scanf("%d",&id_num);
//			printf("get input ID:%x\n",id_num);
//
//			printf("please input the seat:\n");
//			scanf("%d",&c_seat);
//			printf("get input seat:%x\n",c_seat);
//
//			printf("please input the name:\n");
//			scanf("%s",c_name);
//			printf("get input name:%s\n",c_name);
//
//			printf("please input the subject:\n");
//			scanf("%s",c_subj);
//			printf("get input subject:%s\n",c_subj);
//
//			/*
//			 * 确定此API的帧信息
//			 * 会议类参数
//			 */
//			data_type.con_data.id = id_num;
//			data_type.con_data.seat = c_seat;
//			memcpy(&data_type.con_data.name,c_name,strlen(c_name));
//			memcpy(&data_type.con_data.subj,c_subj,strlen(c_subj));
//		}
		/*
		 * 主要分为事件类和会议类
		 * 再其次是写信息，读信息，查询，请求信息
		 * 用会议类数据举例
		 * 0x01--id
		 * 0x02--seat
		 * 0x03--name
		 * 0x04--subject
		 * 0x05--vote(暂不)
		 *
		 * 每个内容对应4个接口，但是这里先举例常用的两个
		 * 读和写
		 * 会议类数据建议是一次设置完或一次查询完
		 */
		switch(s)
		{
			case 1:
				scanf_client();
				break;
			case 2:
//				edit_client_info(&data_type);
				set_the_conference_parameters(fd,0,0,0,0);

				break;
			default:
				break;
		}



	}


}

int control_tcp_module()
{

	void*status;
	pthread_mutex_init(&mutex, NULL);


	pthread_create(&s_id,NULL,control_tcp_recv,NULL);


	pthread_create(&s_id,NULL,control_tcp_send,NULL);

	pthread_join(s_id,&status);

	return 0;
}
int main(void)
{

	control_tcp_module();


    return 0;
}
