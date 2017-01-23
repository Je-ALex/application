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


#include "tcp_ctrl_server.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_device_manage.h"

#include "tcp_ctrl_list.h"
#include "tcp_ctrl_queue.h"
#include "tcp_ctrl_api.h"
#include "scanf_md_udp.h"
#include "sys_uart_init.h"


//互斥锁和信号量
sys_info sys_in;
//链表和结点
Pglobal_info node_queue;


/*
 * tcp_ctrl_local_addr_init.c
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
static int tcp_ctrl_set_noblock(int fd)
{
    int fl=fcntl(fd,F_GETFL);
    if(fl<0)
    {
        perror("fcntl");
        return SETOPTERRO;
    }
    if(fcntl(fd,F_SETFL,fl|O_NONBLOCK))
    {
        perror("fcntl");
        return SETOPTERRO;
    }
    return SUCCESS;
}

/*
 * tcp_ctrl_process_recv_msg
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
static int tcp_ctrl_process_recv_msg(int* cli_fd, unsigned char* value, int* length)
{

	/*
	 * 检查数据包是否是完整的数据包
	 * 数据头的检测
	 * 数据长度的检测
	 * 校验和的检测
	 */
	Pframe_type tmp_type;

	int ret = 0;
	int status = 0;
	unsigned char* handlbuf = NULL;

	tmp_type = (Pframe_type)malloc(sizeof(frame_type));
	memset(tmp_type,0,sizeof(frame_type));

	ret = tcp_ctrl_frame_analysis(cli_fd,value,length,tmp_type,&handlbuf);

#if 0
	printf("type->msg_type = %d\n"
			"type->data_type = %d\n"
			"type->dev_type = %d\n",tmp_type->msg_type,tmp_type->data_type,tmp_type->dev_type);
#endif

	if(ret != SUCCESS)
	{
		printf("tcp_ctrl_frame_analysis failed\n");
		free(tmp_type);
		free(handlbuf);
		return ret;
	}

	/*
	 * 在连接信息链表中检查端口合法性
	 */
	if(tmp_type->msg_type != ONLINE_REQ)
	{
		status = conf_status_check_client_connect_legal(tmp_type);

		if(status < 1)
		{
			printf("client is not legal connect\n");
			free(tmp_type);
			free(handlbuf);
			return CONECT_NOT_LEGAL;
		}
	}

	/*
	 * 设备类型分类
	 */
	switch(tmp_type->dev_type)
	{
		case PC_CTRL:
			tcp_ctrl_from_pc(handlbuf,tmp_type);
			break;

		case UNIT_CTRL:
			tcp_ctrl_from_unit(handlbuf,tmp_type);
			break;

	}

	free(tmp_type);
	free(handlbuf);

	return SUCCESS;
}



/*
 * wifi_sys_ctrl_tcp_recv
 * 设备控制模块TCP数据接收模块
 *
 * 采用epoll模式，维护多终端
 * 接收客户端的通讯数据
 *
 */
void* wifi_sys_ctrl_tcp_recv(void* p)
{
	struct epoll_event event;
    struct epoll_event* wait_event;
	struct sockaddr_in cli_addr;

	int sockfd;
	int epoll_fd;
	int clilen = sizeof(cli_addr);

    int n;
    int newfd;
    int ctrl_ret,wait_ret;
	int maxi = 0;
	int len = 0;
	int ret = 0;
	unsigned char buf[1024] = {0};

	printf("%s,%d\n",__func__,__LINE__);
	pthread_detach(pthread_self());
    /*
     * 控制模块接收服务器套接字初始化
     */
    sockfd = tcp_ctrl_local_addr_init(CTRL_TCP_PORT);
    if(sockfd < 0)
    {
    	printf("wifi_sys_ctrl_tcp_recv failed\n");
    	pthread_exit(0);
    }
    ret = tcp_ctrl_set_noblock(sockfd);
    if(ret < 0)
    {
    	printf("tcp_ctrl_set_noblock failed\n");
    	pthread_exit(0);
    }

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

	wait_event = calloc(1024,sizeof(struct epoll_event));


	while(1)
	{
//		printf("epoll_wait...\n");
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
					&& (EPOLLIN == (wait_event[n].events & (EPOLLIN|EPOLLERR))))
			{
				pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_MUTEX]);
				newfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								(socklen_t*)&clilen);
				if (newfd < 0) {
					perror("accept");
					continue;
				}else
				{

					ret = tcp_ctrl_set_noblock(newfd);
				    if(ret < 0)
				    {
				    	printf("tcp_ctrl_set_noblock failed\n");
				    	pthread_exit(0);
				    }
					// 打印客户端的 ip 和端口
					printf("%d---------------------------\n",newfd);
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

					pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_MUTEX]);
				}

			}else
			{

				pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_MUTEX]);
				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);
				pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_MUTEX]);

				//客户端关闭连接
				if(len < 0)// && errno != EAGAIN)
				{
					printf("wait_event[n].data.fd--%d offline\n",wait_event[n].data.fd);

					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							wait_event[n].data.fd,&event);
					if( ctrl_ret < 0)
					{
					 fprintf(stderr, "delete socket '%d' from epoll failed! %s\n",
							 wait_event[n].data.fd, strerror(errno));
					}
					/*
					 * 删除链表中节点信息与文本中的信息
					 * fd参数
					 */
					pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
					dmanage_delete_info(wait_event[n].data.fd);
					pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
					printf("size--%d\n",node_queue->sys_list[CONNECT_LIST]->size);

					close(wait_event[n].data.fd);
					maxi--;



				}else if(len == 0)//客户端关闭连接
				{
					printf("client %d offline\n",wait_event[n].data.fd);
					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							wait_event[n].data.fd,&event);
					if( ctrl_ret < 0)
					{
					 fprintf(stderr, "delete socket '%d' from epoll failed! %s\n",
							 wait_event[n].data.fd, strerror(errno));
					}
					pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
					dmanage_delete_info(wait_event[n].data.fd);
					pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
					printf("size--%d\n",node_queue->sys_list[CONNECT_LIST]->size);

					close(wait_event[n].data.fd);
					maxi--;
				}else
				{
//					getpeername(wait_event[n].data.fd,(struct sockaddr*)&cli_addr,
//							&clilen);
//					printf("client %s:%d\n",inet_ntoa(cli_addr.sin_addr),
//							ntohs(cli_addr.sin_port));

//					printf("EPOLL %d:",wait_event[n].data.fd);
//					for(i=0;i<len;i++)
//					{
//						printf("%x ",buf[i]);
//					}
//					printf("\n");

					//通过返回值进行处理
					pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_RQUEUE_MUTEX]);
					tcp_ctrl_tprecv_enqueue(&wait_event[n].data.fd,buf,&len);
					pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_RQUEUE_MUTEX]);
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


/*
 * wifi_sys_ctrl_tcp_send
 * 设备控制模块TCP数据下发线程
 *
 * 发送消息进入发送队列，进行数据下发
 *
 */
void* wifi_sys_ctrl_tcp_send(void* p)
{

	int i,ret;
	Plinknode node;
	Pctrl_tcp_rsqueue tmp;

	pthread_detach(pthread_self());

	while(1)
	{

		sem_wait(&sys_in.sys_sem[CTRL_TCP_SEND_SEM]);

		pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);

		ret = out_queue(node_queue->sys_queue[CTRL_TCP_SEND_QUEUE],&node);

		if(ret == 0)
		{
			tmp = node->data;
//			if(sys_debug_get_switch())
//			{
				if(tmp->msg[4] != 0x87)
				{
					printf("%s-tmp->socket_fd[%d] : \n",
							__func__,tmp->socket_fd);
					for(i=0;i<tmp->len;i++)
					{
						printf("%x ",tmp->msg[i]);
					}
					printf("\n");
				}
//			}

			msleep(1);
			pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_MUTEX]);
			write(tmp->socket_fd,tmp->msg, tmp->len);
			pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_MUTEX]);

			free(tmp->msg);
			free(tmp);
			free(node);

		}else{
			printf("%s dequeue error\n",__func__);

		}
		pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);


	}
	free(tmp);

}


/*
 * wifi_sys_ctrl_tcp_procs_data
 * 设备控制模块TCP接收数据处理线程
 *
 * 将消息从队列中读取，进行处理后发送
 *
 */
void* wifi_sys_ctrl_tcp_procs_data(void* p)
{
	Plinknode node;
	Pctrl_tcp_rsqueue tmp;
	int ret;
	pthread_detach(pthread_self());

	while(1)
	{
		sem_wait(&sys_in.sys_sem[CTRL_TCP_RECV_SEM]);
//		printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

		pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_RQUEUE_MUTEX]);
		ret = out_queue(node_queue->sys_queue[CTRL_TCP_RECV_QUEUE],&node);

		if(ret == 0)
		{
			msleep(1);
			tmp = node->data;
			ret = tcp_ctrl_process_recv_msg(&tmp->socket_fd,tmp->msg,&tmp->len);

			free(tmp->msg);
			free(tmp);
			free(node);
		}else{
			printf("%s dequeue error\n",__func__);

		}
		pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_RQUEUE_MUTEX]);

	}

}

/*
 * wifi_sys_ctrl_tcp_heart_sta
 * 设备管理，心跳检测
 * 1、轮询查找当前设备连接信息中的心跳状态
 * 2.判断连接信息，如果状态为0，则表示端口异常，需要关闭
 */
void* wifi_sys_ctrl_tcp_heart_state(void* p)
{
	frame_type type;
	memset(&type,0,sizeof(frame_type));

	pthread_detach(pthread_self());

	while(1)
	{
		if(!conf_status_get_connected_len())
		{
			sleep(1);
//			conf_status_set_snd_effect(i);
//			i++;
//			if(i == 10)
//				i =0;
			conf_status_set_sys_timestamp(1);
			continue;
		}else{
			conf_status_set_sys_timestamp(5);
			sleep(5);
			dmanage_get_communication_heart(&type);
			if(type.fd)
			{
				printf("%s-%s-%d client connect err,close it\n",__FILE__,__func__,__LINE__);
				pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
				dmanage_delete_info(type.fd);
				pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
				close(type.fd);
				type.fd = 0;
			}
		}

	}

}

/*
 * TODO 测试用
 */

void read_file(char* fname)
{

	Pclient_info pinfo;
	int ret;
	FILE* file;

	pinfo = malloc(sizeof(client_info));

	file = fopen(fname,"r");

	while(1)
	{
		ret = fread(pinfo,sizeof(client_info),1,file);
		perror("fread");
		if(ret ==0)
			return;
		if(pinfo == NULL)
			break;

		printf("fd:%d,ip:%s-ip-%s,client:%d\n",pinfo->client_fd,
				inet_ntoa(pinfo->cli_addr.sin_addr),pinfo->ip,
				pinfo->client_name);
		printf("ip-%s,client:%x\n",
				inet_ntoa(pinfo->cli_addr.sin_addr),pinfo->cli_addr.sin_addr.s_addr);
		usleep(100000);
	}
	free(pinfo);
	fclose(file);
}

int print_conference_list()
{
	pclient_node tmp = NULL;
	Pconference_list info;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;


	while(tmp != NULL)
	{
		info = tmp->data;

		if(info->fd > 0)
		{
			printf("fd--%d,id--%d,seat--%d,name--%s  conf_name-%s\n",info->fd,
					info->con_data.id,info->con_data.seat,info->con_data.name,info->con_data.conf_name);
		}
		tmp = tmp->next;
	}

	return SUCCESS;
}

void* control_tcp_send(void* p)
{

	int ret;
	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */

	int s,fd,id,seat,value;


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
		printf("######################################\n");
		printf("1 扫描单元机\n");
		printf("2 设置会议ID参数\n");
		printf("3 获取单元机会议参数\n");
		printf("4 设置最大发言人数\n");
		printf("5 获取最大发言人数\n");
		printf("6 设置话筒模式\n");
		printf("7 获取话筒模式\n");
		printf("8 开始签到\n");
		printf("9 结束签到\n");
		printf("10 恢复出厂设置\n");
		printf("11 网络信息\n");
		printf("12 关于本机\n");
		printf("13 关机\n");
		printf("14 摄像跟踪\n");
		printf("######################################\n");

		scanf("%d",&s);

		switch(s)
		{
			case 1:
				ret=unit_info_get_connected_info(file_name);
				read_file(file_name);
				print_conference_list();
				printf("file name \"%s\",scanf_client size--%d\n",file_name,ret);
				break;
			case 2:
				printf("s=%d,input the fd,id,seat\n",s);
				scanf("%d",&fd);
				scanf("%d",&id);
				scanf("%d",&seat);
				conf_info_set_conference_params(fd,id,seat,0,0);
				break;
			case 3:
				printf("s=%d,input the fd\n",s);
				scanf("%d",&fd);
				conf_info_get_the_conference_params(fd);
				break;
			case 4:
				printf("s=%d,input the spk numt\n",s);
				scanf("%d",&value);
				conf_info_set_spk_num(value);
				break;
			case 5:
				ret = conf_info_get_spk_num();
				printf("conf_info_get_spk_num value=%d\n",ret);
				break;
			case 6:
				printf("s=%d,input the mic mode\n",s);
				scanf("%d",&value);
				conf_info_set_mic_mode(value);
				break;
			case 7:
				conf_info_get_mic_mode();
				break;
			case 8:
				break;
			case 9:
				break;
			case 10:
				host_info_reset_factory_mode();
				break;
			case 11:
				host_info_get_network_info(&list);
			    printf("\tmac: %s\n",list.mac_addr );
			    printf("\tIP: %s\n", list.ip_addr);
			    printf("\tNetmask: %s\n",list.netmask );
				break;
			case 12:
				host_info_get_factory_info(&list);
			    printf("\tversion: %s\n",list.version );
			    printf("\thost_model: %s\n",list.host_model );
			    printf("\tfactory_information: %s\n",list.factory_info);
				break;
			case 13:
				unit_info_set_device_power_off();
				break;
			case 14:
				printf("sys_uart_video_set\n");
				printf("s=%d,input the value\n",s);
				scanf("%d",&value);
				scanf("%d",&id);
				sys_uart_video_set(id,value);
				break;
			case 15:
				printf("subject set\n");
				conf_info_set_conference_sub_params();
				break;
			default:
				break;
		}

	}

}

void* control_tcp_queue(void* p)
{

	Prun_status event_tmp;
	event_tmp = (Prun_status)malloc(sizeof(run_status));

	while(1)
	{

		unit_info_get_running_status(event_tmp);
		printf("%s-%s-fd:%d,id:%d,seat:%d,value:%d\n",
				__FILE__,__func__,event_tmp->socket_fd,
				event_tmp->id,event_tmp->seat,event_tmp->value);
	}
	free(event_tmp);
}





