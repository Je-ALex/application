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
#include "../../header/tcp_ctrl_data_compose.h"
#include "../../header/tcp_ctrl_data_process.h"
#include "../../header/tcp_ctrl_list.h"
#include "../../header/tcp_ctrl_queue.h"

int port = 8080;

pthread_t s_id;
pthread_mutex_t mutex;
sem_t bin_sem;

/*
 * 1、连接信息链表
 * 2、会议参数链表
 * 3、终端会议状态链表
 */
pclient_node list_head;

pclient_node confer_head;

Plinkqueue status_queue;




/*
 * tc_local_addr_init.c
 * 服务端socket初始化
 *
 * 返回值
 * 错误码和成功后的端口号
 *
 */
static int tcp_ctrl_local_addr_init()
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
 * tc_set_noblock.c
 * 设置socket_fd为非阻塞模式
 */
static void tcp_ctrl_set_noblock(int fd)
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
 * tcp_ctrl_add_client.c
 * 终端信息录入函数
 *
 * 将客户端信息增加本地链表，然后将数据信息录入文本文件，供QT使用
 *
 *
 */
static int tcp_ctrl_add_client(void* value)
{
	pclient_node tmp = NULL;
	Pclient_info pinfo;
	FILE* file;
	int ret;


	/*
	 * 增加到链表中
	 */
	list_add(list_head,value);

	/*
	 * 存入到文本文件
	 */
	file = fopen("connect_info.txt","w+");
	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			printf("fd:%d,ip:%s,id:%d\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr),pinfo->client_name);

			ret = fwrite(pinfo,sizeof(client_info),1,file);
			perror("fwrite");
			if(ret != 1)
				return ERROR;

		}
		tmp = tmp->next;
		usleep(10000);
	}
	fclose(file);

	return SUCCESS;
}


/*
 * tcp_ctrl_delete_client.c
 * 客户端 删除函数
 *
 * 通过fd来判别链表中需要移除的客户端
 * 更新文本文件
 */
static int tcp_ctrl_delete_client(int fd)
{
	pclient_node tmp = NULL;
	pclient_node tmp2 = NULL;

	Pclient_info pinfo;
	Pframe_type tmp_type;

	pclient_node del = NULL;

	FILE* file;
	int ret;
	int status = 0;
	int pos = 0;

	/*
	 * 删除会议信息链表中的数据
	 */
	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == fd)
		{
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}

	if(status > 0)
	{
		list_delete(list_head,pos,&del);
		pinfo = del->data;
		printf("close socket and remove ctrl ip %s\n",
				inet_ntoa(pinfo->cli_addr.sin_addr));
		free(pinfo);
		free(del);
	}else{

		printf("there is no data in the list\n");
	}

	status = 0;
	pos = 0;
	/*
	 * 删除会议信息链表中的数据
	 */
	tmp2 = confer_head->next;
	while(tmp2 != NULL)
	{
		tmp_type = tmp2->data;
		if(tmp_type->fd == fd)
		{
			status++;
			break;
		}
		pos++;
		tmp2 = tmp2->next;
	}
	if(status > 0)
	{
		list_delete(confer_head,pos,&del);
		tmp_type = del->data;
		printf("close socket %d \n",
				tmp_type->fd);
		free(tmp_type);
		free(del);
	}else{

		printf("there is no data in the list\n");
	}

//	file = fopen("info.txt","w+");
//	while(tmp != NULL)
//	{
//		pinfo = tmp->data;
//		if(pinfo->client_fd > 0)
//		{
//			printf("fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));
//
//			ret = fwrite(pinfo,sizeof(client_info),1,file);
//			perror("fwrite");
//			if(ret != 1)
//				return ERROR;
//
//		}
//		tmp = tmp->next;
//		usleep(10000);
//	}
//	fclose(file);

	return SUCCESS;
}


/*
 * tcp_ctrl_refresh_client_list.c
 * 控制模块客户端连接管理
 *
 * 通过msg_type来判断是否是宣告在线消息
 * 再通过fd判断连接存储情况，如果是新的客户端则存入
 */
int tcp_ctrl_refresh_client_list(Pframe_type frame_type)
{

	/*
	 * 终端信息录入结构体
	 */
	Pclient_info info;
	Pclient_info pinfo;

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	pclient_node tmp = NULL;

	int state = 0;

	printf("%s\n",__func__);
	/*
	 * 宣告上线消息类型
	 */
	if(frame_type->msg_type == ONLINE_REQ)
	{
		/*
		 *
		 *将新连接终端信息录入结构体中
		 *这里可以将fd和IP信息存入到本地链表和文件中
		 */
		info = (Pclient_info)malloc(sizeof(client_info));
		memset(info,0,sizeof(client_info));
		info->client_fd = frame_type->fd;
		getpeername(info->client_fd,(struct sockaddr*)&cli_addr,
									&clilen);
		info->cli_addr = cli_addr;
		info->clilen = clilen;

		if(frame_type->dev_type == PC_CTRL){
			info->client_name = PC_CTRL;
		}else{
			info->client_name = UNIT_CTRL;
		}

		tmp = list_head->next;

		/*
		 * 检查该客户端是否已经存在
		 *
		 * 链表如果为空，则不需要进行检查，直接存储
		 * 链表不为空，则通过读取再比对，进行存储
		 *
		 */
		do
		{
			if(tmp == NULL)
			{
				state++;
				tcp_ctrl_add_client(info);
//				list_add(list_head,info);
				break;
			}else{
				pinfo = tmp->data;
				if(pinfo->client_fd == info->client_fd)
				{
					state++;
					printf("the client is exist\n");
				}
			}
			tmp = tmp->next;
			usleep(10000);
		}while(tmp != NULL);


		if(state == 0)
		{
//			list_add(list_head,info);
			tcp_ctrl_add_client(info);
		}

	}

	return 0;
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
static void tcp_ctrl_process_rev_msg(int* cli_fd, unsigned char* value, int* length)
{

	/*
	 * 检查数据包是否是完整的数据包
	 * 数据头的检测
	 * 数据长度的检测
	 * 校验和的检测
	 */
	Pframe_type type;
	int i;
	int status = 0;
	unsigned char* handlbuf = NULL;

	pclient_node tmp = NULL;
	Pclient_info tmp_type;



	type = (Pframe_type)malloc(sizeof(frame_type));
	memset(type,0,sizeof(frame_type));

	tcp_ctrl_frame_analysis(cli_fd,value,length,type,&handlbuf);

	printf("type->dev_type = %d\n"
			"type->msg_type = %d\n"
			"type->data_type = %d\n",type->dev_type,type->msg_type,type->data_type);

	/*
	 * 在连接信息链表中检查端口合法性
	 */
	tmp=list_head->next;
	if(type->msg_type != ONLINE_REQ)
	{
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;

			if(tmp_type->client_fd == *cli_fd)
			{
				status++;
				break;
			}
			tmp=tmp->next;
		}

		if(status < 1)
		{
			printf("client is not legal connect\n");
			return;
		}
	}

	/*
	 * 设备类型分类
	 */
	switch(type->dev_type)
	{
		case PC_CTRL:
			printf("process the pc data\n");
			tcp_ctrl_from_pc(handlbuf,type);
			break;

		case UNIT_CTRL:
			printf("process the unit data\n");
			tcp_ctrl_from_unit(handlbuf,type);
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
static void* tcp_control_module(void* p)
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
    sockfd = tcp_ctrl_local_addr_init();
    tcp_ctrl_set_noblock(sockfd);


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
	 * 终端连接信息链表
	 */
	list_head = list_head_init();
	/*
	 * 会议数据链表
	 */
	confer_head = list_head_init();
	/*
	 * 创建消息队列
	 */
	status_queue = queue_init();

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

					tcp_ctrl_set_noblock(newfd);
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
					 * 删除链表中节点信息与文本中的信息
					 * fd参数
					 */
					tcp_ctrl_delete_client(wait_event[n].data.fd);

				}
				else if(len == 0)//客户端关闭连接
				{
					printf("client %d offline\n",wait_event[n].data.fd);

					tcp_ctrl_delete_client(wait_event[n].data.fd);

					close(wait_event[n].data.fd);

					printf("size--%d\n",list_head->size);

					maxi--;
				}
				else
				{
					getpeername(wait_event[n].data.fd,(struct sockaddr*)&cli_addr,
							&clilen);
					printf("client %s:%d\n",inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					tcp_ctrl_process_rev_msg(&wait_event[n].data.fd,buf,&len);

					//将测试数据返回发送
					//send(wait_event[n].data.fd, buf, len, 0);
				}
			}//end if

		}//end for

	}//end while

	free(wait_event);
    close(sockfd);
    pthread_exit(0);

    return;


}


void read_file(){

	Pclient_info pinfo;
	int i;
	int ret;
	FILE* file;

	pinfo = malloc(sizeof(client_info));

	file = fopen("connect_info.conf","r");

	for(i=0;i<list_head->size;i++){

		ret = fread(pinfo,sizeof(client_info),1,file);
		perror("fread");
		if(ret ==0)
			return;
		printf("fd:%d,ip:%s,id:%d\n",pinfo->client_fd,
				inet_ntoa(pinfo->cli_addr.sin_addr),pinfo->client_name);

		usleep(100000);
	}

	fclose(file);
}

void tcp_strlen()
{
	char* str1="01234567890";

	int i;

	i=strlen(str1);
	printf("i---%d\n",i);



}
static void* control_tcp_send(void* p)
{

	int ret;
	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */

	int s,fd,id,seat,value;


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
		printf("------------------------------------\n");
		printf("1 get_the_client_connect_info\n");
		printf("2 read_file\n");
		printf("3 set_the_conference_parameters\n");
		printf("4 get_the_conference_parameters\n");
		printf("5 set_the_conference_vote_result\n");

		scanf("%d",&s);

		if(s == 3 || s == 4 )
		{
			printf("s=%d,input the fd,id,seat\n",s);
			scanf("%d",&fd);
			scanf("%d",&id);
			scanf("%d",&seat);

		}
		if(s >5 )
		{
			printf("s=%d,input the fd and value\n",s);
			scanf("%d",&fd);
			scanf("%d",&value);

		}
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
				ret=get_the_client_connect_info();
				printf("scanf_client size--%d\n",ret);
				break;
			case 2:
				read_file();
				break;
			case 3:
				set_the_conference_parameters(fd,id,seat,0,0);
				break;
			case 4:
				get_the_conference_parameters(fd);
				break;
			case 5:
				set_the_conference_vote_result();
				break;
			case 6:
				set_the_event_parameter_power(fd,value);
				break;
			case 7:
				get_the_event_parameter_power(fd);
				break;
			case 8:

				break;
			case 9:

				break;
			case 0x0a:

				break;
			case 0x0b:

				break;
			case 0x0c:

				break;
			case 0x0d:

				break;
			case 0x0e:

				break;

			default:
				break;
		}

	}


}
static void* control_tcp_queue(void* p)
{

	int ret;
	Pqueue_event event_tmp;

	while(1)
	{

		sem_wait(&bin_sem);
		printf("get the value from queue\n");
		event_tmp = out_queue(status_queue);
//		if(ret == 0)
//		{
			printf("%s--%d\n",__func__,event_tmp->socket_fd);
//	//		refresh_the_status();
//			free(event_tmp);
//		}

		sleep(1);
	}

}
int control_tcp_module()
{

	void*status;
	int res;
	pthread_mutex_init(&mutex, NULL);

	res = sem_init(&bin_sem, 0, 0);
	if (res != 0)
	{
	 perror("Semaphore initialization failed");
	}

	pthread_create(&s_id,NULL,tcp_control_module,NULL);

	pthread_create(&s_id,NULL,control_tcp_send,NULL);

	pthread_create(&s_id,NULL,control_tcp_queue,NULL);

	pthread_join(s_id,&status);

	return 0;
}
int main(void)
{

	control_tcp_module();


    return 0;
}
