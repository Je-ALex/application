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


#include "../../inc/tcp_ctrl_server.h"

#include "../../inc/device_ctrl_module_udp.h"
#include "../../inc/tcp_ctrl_data_compose.h"
#include "../../inc/tcp_ctrl_data_process.h"
#include "../../inc/tcp_ctrl_device_status.h"
#include "../../inc/tcp_ctrl_list.h"
#include "../../inc/tcp_ctrl_queue.h"
#include "../../inc/tcp_ctrl_api.h"

//互斥锁和信号量
sys_info sys_in;
//链表和结点
Pmodule_info node_queue;



/*
 * tc_local_addr_init.c
 * 服务端socket初始化
 *
 * 返回值
 * 错误码和成功后的端口号
 *
 */
static int tcp_ctrl_local_addr_init(int port)
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
 * tcp_ctrl_module_init
 * 设备控制模块的链表和队列初始化
 *
 */
static int tcp_ctrl_module_init()
{

	node_queue = (Pmodule_info)malloc(sizeof(module_info));
	memset(node_queue,0,sizeof(module_info));
	/*
	 * 终端连接信息链表
	 */
	node_queue->list_head = list_head_init();
	if(node_queue->list_head != NULL)
		printf("list_head init success\n");
	else
		return ERROR;
	/*
	 * 会议数据链表
	 */
	node_queue->conference_head = list_head_init();
	if(node_queue->conference_head != NULL)
		printf("conference_head init success\n");
	else
		return ERROR;
	/*
	 * 创建本地消息队列
	 */
	node_queue->report_queue = queue_init();
	if(node_queue->report_queue != NULL)
		printf("report_queue init success\n");
	else
		return ERROR;
	/*
	 * 创建pc消息队列
	 */
	node_queue->report_pc_queue = queue_init();
	if(node_queue->report_pc_queue != NULL)
		printf("report_pc_queue init success\n");
	else
		return ERROR;

	/*
	 * 创建tcp发送消息队列
	 */
	node_queue->tcp_send_queue = queue_init();
	if(node_queue->tcp_send_queue != NULL)
		printf("tcp_send_queue init success\n");
	else
		return ERROR;
	/*
	 * 相关参数初始化
	 */
	node_queue->con_status = (Pconference_status)malloc(sizeof(conference_status));
	memset(node_queue->con_status,0,sizeof(conference_status));

	return SUCCESS;
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
	pclient_node tmp = NULL;
	Pclient_info tmp_type;

	int i;
	int ret = 0;
	int status = 0;
	unsigned char* handlbuf = NULL;

	type = (Pframe_type)malloc(sizeof(frame_type));
	memset(type,0,sizeof(frame_type));

	ret = tcp_ctrl_frame_analysis(cli_fd,value,length,type,&handlbuf);

#if TCP_DBG

	printf("type->msg_type = %d\n"
			"type->data_type = %d\n"
			"type->dev_type = %d\n",type->msg_type,type->data_type,type->dev_type);
#endif

	if(ret)
	{
		printf("%s failed\n",__func__);
		return;
	}

	/*
	 * 在连接信息链表中检查端口合法性
	 */
	tmp=node_queue->list_head->next;
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
			tcp_ctrl_from_pc(handlbuf,type);
			break;

		case UNIT_CTRL:
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

	int len = 0;
	unsigned char buf[1024] = {0};

	printf("%s,%d\n",__func__,__LINE__);
    /*
     *socket init
     */
    sockfd = tcp_ctrl_local_addr_init(CTRL_PORT);
    if(sockfd < 0)
    	pthread_exit(0);

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
	 * 队列等初始化
	 */
	tcp_ctrl_module_init();

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
				pthread_mutex_lock(&sys_in.tcp_mutex);
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

					event.events = EPOLLIN | EPOLLET;
					event.data.fd = newfd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &event) < 0) {
						fprintf(stderr, "add socket '%d' to epoll failed %s\n",
								newfd, strerror(errno));
						pthread_exit(0);
					}
					maxi++;

					pthread_mutex_unlock(&sys_in.tcp_mutex);
				}

			} else {

				pthread_mutex_lock(&sys_in.tcp_mutex);
				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);
				pthread_mutex_unlock(&sys_in.tcp_mutex);

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

					printf("size--%d\n",node_queue->list_head->size);

					maxi--;
				}
				else
				{
					getpeername(wait_event[n].data.fd,(struct sockaddr*)&cli_addr,
							&clilen);
					printf("client %s:%d\n",inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					tcp_ctrl_process_rev_msg(&wait_event[n].data.fd,buf,&len);

				}
			}//end if

		}//end for

	}//end while

	free(wait_event);
    close(sockfd);
    free(node_queue->con_status);
    free(node_queue);

    pthread_exit(0);



}


void read_file(){

//	Pclient_info pinfo;
//	int i;
//	int ret;
//	FILE* file;
//
//	pinfo = malloc(sizeof(client_info));
//
//	file = fopen("connection.info","r");
//
//	while(1)
//	{
//		ret = fread(pinfo,sizeof(client_info),1,file);
//		perror("fread");
//		if(ret ==0)
//			return;
//		if(pinfo == NULL)
//			break;
//
//		printf("fd:%d,ip:%s,id:%d\n",pinfo->client_fd,
//				inet_ntoa(pinfo->cli_addr.sin_addr),pinfo->client_name);
//
//		usleep(100000);
//	}
//	free(pinfo);
//	fclose(file);

	pclient_node tmp = NULL;
	Pclient_info info;


	tmp = node_queue->list_head->next;
	printf("%s\n",__func__);
	while(tmp != NULL)
	{
		info = tmp->data;

		if(info->client_fd > 0)
		{
			printf("fd--%d\n",info->client_fd);
		}
		tmp = tmp->next;
	}
}



int print_conference_list()
{
	pclient_node tmp = NULL;
	Pconference_info info;
	int ret;

	tmp = node_queue->conference_head->next;

	printf("%s\n",__func__);

	while(tmp != NULL)
	{
		info = tmp->data;

		if(info->fd > 0)
		{
			printf("fd--%d,id--%d,seat--%d,name--%s  subject-%s\n",info->fd,
					info->con_data.id,info->con_data.seat,info->con_data.name,info->con_data.subj[0]);
		}
		tmp = tmp->next;
	}

//	Pconference_info confer_info = (Pconference_info)malloc(sizeof(conference_info));
//	memset(confer_info,0,sizeof(conference_info));
//
//	FILE* file;
//
//	file = fopen("connection.info","r");
//
//	while(1)
//	{
//		ret = fread(confer_info,sizeof(conference_info),1,file);
//		perror("fread");
//		if(ret ==0)
//			return;
//		if(confer_info == NULL)
//			break;
//
//		printf("fd:%d,id:%d,seat:%d\n",confer_info->fd,
//			confer_info->con_data.id,confer_info->con_data.seat);
//
//		usleep(100000);
//	}
//	free(confer_info);
//	fclose(file);


	return SUCCESS;
}


static void* control_tcp_send(void* p)
{

	int ret;
	int i = 0;
	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */

	int s,fd,id,seat,value;

	Prun_status event_tmp;

	Pconference_info confer_info;
	host_info list;
	memset(&list,0,sizeof(host_info));

	char file_name[32] = {0};

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
		printf("6 set_the_event_parameter_power\n");
		printf("7 get_the_event_parameter_power\n");

		scanf("%d",&s);

		switch(s)
		{
			case 1:

				ret=get_client_connected_info(file_name);
				printf("file name %s,scanf_client size--%d\n",file_name,ret);
				break;
			case 2:
				read_file();
				print_conference_list();
				break;
			case 3:
				printf("s=%d,input the fd,id,seat\n",s);
				scanf("%d",&fd);
				scanf("%d",&id);
				scanf("%d",&seat);
				set_the_conference_parameters(fd,id,seat,0,0);
				break;
			case 4:
				printf("s=%d,input the fd\n",s);
				scanf("%d",&fd);
				get_the_conference_parameters(fd);
				break;
			case 5:

				set_the_conference_vote_result();
				break;
			case 6:
				printf("s=%d,input the fd and value\n",s);
				scanf("%d",&fd);
				scanf("%d",&value);
				set_the_event_parameter_power(fd,value);
				break;
			case 7:
				printf("s=%d,input the fd\n",s);
				scanf("%d",&fd);
				get_the_event_parameter_power(fd);
				break;
			case 8:
				printf("enter the queue..\n");
				event_tmp = (Prun_status)malloc(sizeof(run_status));
				memset(event_tmp,0,sizeof(run_status));
				event_tmp->socket_fd = 7;
				event_tmp->id = i;
				event_tmp->seat = 1;
				event_tmp->value = WIFI_MEETING_EVT_SPK_REQ_SPK;
				enter_queue(node_queue->report_queue,event_tmp);
				sem_post(&sys_in.local_report_sem);
				i++;
				break;
			case 9:
			{
				confer_info = (Pconference_info)malloc(sizeof(conference_info));

				memset(confer_info,0,sizeof(frame_type));

				unsigned char* name = "湖山电器有限责任公司";
				unsigned char* sub = "WIFI无线会议系统zhegehaonan";
				confer_info->fd = fd;
				confer_info->con_data.id = id;
				confer_info->con_data.seat = seat;
				memcpy(confer_info->con_data.name,name,strlen(name));
				memcpy(confer_info->con_data.subj[0],sub,strlen(sub));

				tcp_ctrl_refresh_conference_list(confer_info);
				break;
			}

			case 10:
				get_the_host_factory_infomation(&list);
				get_the_host_network_info(&list);
				break;
			case 11:

				break;

			default:
				break;
		}

	    printf("\tversion: %s\n",list.version );
	    printf("\thost_model: %s\n",list.host_model );
	    printf("\tfactory_information: %s\n",list.factory_information );

	    printf("\tmac: %s\n",list.mac );
	    printf("\tIP: %s\n", list.local_ip);
	    printf("\tNetmask: %s\n",list.netmask );

	}


}

static void* control_tcp_queue(void* p)
{


	Prun_status event_tmp;
	event_tmp = (Prun_status)malloc(sizeof(run_status));


	while(1)
	{


		get_unit_running_status(&event_tmp);
		printf("fd--%d,id--%d,seat--%d,value--%d,queue_size--%d\n",
				event_tmp->socket_fd,event_tmp->id,event_tmp->seat,
				event_tmp->value,node_queue->report_queue->size);


	}
	free(event_tmp);
}

static void* control_tcp_send_queue(void* p)
{

	int i;
	Ptcp_send tmp;
	tmp = (Ptcp_send)malloc(sizeof(tcp_send));

	while(1)
	{

		printf("%s,%d\n",__func__,__LINE__);
		tcp_ctrl_tpsend_dequeue(&tmp);
		printf("queue_size--%d ",node_queue->tcp_send_queue->size);
		printf("send to %d msg：",tmp->socket_fd);
		for(i=0;i<tmp->len;i++)
		{
			printf("%x ",tmp->msg[i]);
		}
		printf("\n");

		pthread_mutex_lock(&sys_in.tcp_mutex);
		write(tmp->socket_fd, tmp->msg, tmp->len);
		pthread_mutex_unlock(&sys_in.tcp_mutex);

	}
	free(tmp);
}


/*
 * init_control_tcp_module
 * 设备控制模块初始化接口
 * 此接口可以作为API接口供外部使用
 *
 */
int init_control_tcp_module()
{
	pthread_t udp_t;
	pthread_t tcp_rcv;
	pthread_t tcp_send;
	pthread_t queue_t;

	void*status;
	int res;

	pthread_mutex_init(&sys_in.tcp_mutex, NULL);
	pthread_mutex_init(&sys_in.report_queue_mutex, NULL);
	pthread_mutex_init(&sys_in.pc_queue_mutex, NULL);
	pthread_mutex_init(&sys_in.tpsend_queue_mutex, NULL);

	res = sem_init(&sys_in.local_report_sem, 0, 0);
	if (res != 0)
	{
	 perror("local_report_sem initialization failed");
	}
	res = sem_init(&sys_in.pc_queue_sem, 0, 0);
	if (res != 0)
	{
	 perror("Semaphore initialization failed");
	}
	res = sem_init(&sys_in.tpsend_queue_sem, 0, 0);
	if (res != 0)
	{
	 perror("Semaphore tpsend_queue_sem initialization failed");
	}

	printf("%s,%d\n",__func__,__LINE__);

	//UDP广播线程
	pthread_create(&udp_t,NULL,udp_server,NULL);
	//TCP控制模块数据接收线程
	pthread_create(&tcp_rcv,NULL,tcp_control_module,NULL);
	//TCP控制模块数据发送线程
	pthread_create(&tcp_send,NULL,control_tcp_send_queue,NULL);

	//测试线程
//	pthread_create(&tcp_send,NULL,control_tcp_send,NULL);
//	pthread_create(&queue_t,NULL,control_tcp_queue,NULL);


//	pthread_join(udp_t,&status);
	pthread_join(tcp_rcv,&status);
//	pthread_join(tcp_send,&status);
//	pthread_join(queue_t,&status);

	return 0;
}


int main(void)
{

	init_control_tcp_module();


    return 0;
}
