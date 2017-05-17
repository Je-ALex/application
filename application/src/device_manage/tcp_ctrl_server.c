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

#include "wifi_sys_init.h"
#include "wifi_sys_list.h"
#include "wifi_sys_queue.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "client_heart_manage.h"
#include "client_connect_manage.h"
#include "tcp_ctrl_api.h"
#include "sys_uart_init.h"


//互斥锁和信号量
sys_info sys_in;
//链表和结点
Pglobal_info node_queue;


/*
 * tcp_ctrl_local_addr_init
 * TCP服务端socket初始化
 *
 * 输入
 * 端口号
 * 输出
 * 无
 *
 * 返回值：
 * 成功-套接字号
 * 失败-错误码
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
    	printf("%s-%s-%d socket failed\n",__FILE__,__func__,__LINE__);

        return SOCKERRO;
    }
    //设置sock属性，地址复用
    int opt=1;
    setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof(opt));

    ret = bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin));
    if (ret == -1)
    {
        printf("%s-%s-%d bind failed\n",__FILE__,__func__,__LINE__);
        perror("bind");
        return BINDERRO;
    }
    ret = listen(sock_fd, 20);
    if (ret)
    {
    	printf("%s-%s-%d listen failed\n",__FILE__,__func__,__LINE__);

        return LISNERRO;
    }

    return sock_fd;

}


/*
 * tcp_ctrl_set_noblock
 * 设置非阻塞模式
 *
 * 输入
 * 套接字号
 * 输出
 * 无
 *
 * 返回值：
 * 成功-SUCCESS
 * 失败-错误码
 */
static int tcp_ctrl_set_noblock(int fd)
{
    int fl = fcntl(fd,F_GETFL);
    if(fl<0)
    {
        printf("%s-%s-%d F_GETFL failed\n",__FILE__,__func__,__LINE__);
        return SETOPTERRO;
    }
    if(fcntl(fd,F_SETFL,fl | O_NONBLOCK))
    {
        printf("%s-%s-%d F_SETFL failed\n",__FILE__,__func__,__LINE__);
        return SETOPTERRO;
    }
    return SUCCESS;
}


/*
 * tcp_ctrl_org_data_analysis
 * tcp原始数据包解析
 *
 * 此函数主要是为了解决接收数据出现粘黏的情况
 * 采用循环解析数据包的方式，再将解析后的数据送进队列
 *
 * 输入
 * fd
 * buf
 * len
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_org_data_analysis(int fd,unsigned char* buf,int len)
{
	int frame_len = 0;//帧长
	unsigned char frame_data[512] = {0};//帧数组

	int org_index = 0;//字节号
	int tmp_len = len;//原始数据长度
	unsigned char* tmp_buf = buf;//临时指针

	unsigned char check_sum = 0;//校验和
	int i = 0;

	if(tmp_len <= 0)
	{
		sys_mutex_lock(CTRL_TCP_RQUEUE_MUTEX);
		tcp_ctrl_tprecv_enqueue(&fd,frame_data,&frame_len);
		sys_mutex_unlock(CTRL_TCP_RQUEUE_MUTEX);

	}else
	{

		while(tmp_len>0)
		{

	//		printf("org_data %d:",fd);
	//		for(i=0;i<tmp_len;i++)
	//		{
	//			printf("%x ",tmp_buf[i]);
	//		}
	//		printf("\n");

			//判断从第0个字节开始的数据头是否合法，是数据头，则解析后续内容，并送入队列
			if (tmp_buf[0] == 'D' && tmp_buf[1] == 'S'
					&& tmp_buf[2] == 'D' && tmp_buf[3] == 'S')
			{
				//解析一帧数据包的长度
				org_index = 5;
				frame_len = tmp_buf[org_index++] & 0xff;
				frame_len = frame_len << 8;
				frame_len = frame_len + (tmp_buf[org_index] & 0xff);
				//判断长度是否合法
				if(frame_len == 0 || frame_len > tmp_len)
					return ERROR;
				//计算帧的校验和
				for(i=0;i<frame_len-1;i++)
				{
					if(i<tmp_len)
					{
						check_sum += tmp_buf[i];
					}

				}
				//比较校验和的值
				if(tmp_buf[frame_len-1] != check_sum)
					return ERROR;
				//数据帧拷贝到入队数组
				memcpy(frame_data,tmp_buf,frame_len);

				//指针指向第二帧数据头，长度修正
				tmp_buf = &tmp_buf[frame_len];
				tmp_len -= frame_len;
				//修正临时变量
				org_index = check_sum = 0;

	//			printf("tmp_len-%d frame_len-%d:",tmp_len,frame_len);
	//			printf(" %d:",fd);
	//			for(i=0;i<frame_len;i++)
	//			{
	//				printf("%x ",frame_data[i]);
	//			}
	//			printf("\n");

				//帧数据入队
				sys_mutex_lock(CTRL_TCP_RQUEUE_MUTEX);
				tcp_ctrl_tprecv_enqueue(&fd,frame_data,&frame_len);
				sys_mutex_unlock(CTRL_TCP_RQUEUE_MUTEX);

			}else
			{
				//帧头内容不对，长度减一，指针后移一个字节
				tmp_len--;
				tmp_buf = &tmp_buf[1];
			}
		}
	}

	return SUCCESS;
}


/*
 * wifi_sys_ctrl_tcp_recv
 * 主机系统TCP数据接收线程函数
 *
 * 系统控制指令均通过TCP进行数据传输，接收数据通过原始处理后，送入信息处理队列
 *
 *
 * 1、初始化socket套接字，设置相关参数
 * 2、初始化epoll，对套接字进行维护检测
 *
 * 通过对TCP的逻辑处理，判断连接设备的运行状况，进行关闭或进行下一步的数据处理
 *
 * 输入
 * 无
 * 输出
 * 无
 * 返回值
 * 无
 *
 */
void* wifi_sys_ctrl_tcp_recv(void* p)
{
	//声明epoll结构体和相关变量
	struct epoll_event event;
    struct epoll_event* wait_event;
	int epoll_fd = -1;

    //声明socket结构体相关参数
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	int listenfd = -1,newfd = -1;

    //相关参数变量
    int n;
    int ctrl_ret = -1,wait_ret = -1;
	int maxi = 0;//监听最大值
	int len = 0;//接收数据长度
	int ret = 0;
	unsigned char buf[1024] = {0};//接收buffer的大小

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	//线程非阻塞
	pthread_detach(pthread_self());

	//初始化套接字
	listenfd = tcp_ctrl_local_addr_init(CTRL_TCP_PORT);
    if(listenfd < 0)
    {
    	printf("%s-%s-%d wifi_sys_ctrl_tcp_recv failed\n",__FILE__,__func__,__LINE__);
    	pthread_exit(0);
    }
    //设置非阻塞
    ret = tcp_ctrl_set_noblock(listenfd);
    if(ret < 0)
    {
    	printf("%s-%s-%d tcp_ctrl_set_noblock failed\n",__FILE__,__func__,__LINE__);
    	pthread_exit(0);
    }

    //创建epoll描述
    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0){
        printf("%s-%s-%d epoll_create1 failed\n",__FILE__,__func__,__LINE__);
        pthread_exit(0);
    }
    //设置sock的属性，接收边沿触发
    event.data.fd = listenfd;
    event.events = EPOLLIN | EPOLLET;

	//事件注册函数，将监听套接字描述符 sockfd 加入监听事件
    ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &event);
    if(-1 == ctrl_ret){
        printf("%s-%s-%d epoll_ctl failed\n",__FILE__,__func__,__LINE__);
        pthread_exit(0);
    }
    //申请事件集空间
	wait_event = (struct epoll_event*)calloc(1024,sizeof(struct epoll_event));

	while(1)
	{
		// 监视并等待多个文件描述符的属性变化（是否可读）
        // 没有属性变化，这个函数会阻塞，直到有变化才往下执行，(-1)这里没有设置超时
		wait_ret = epoll_wait(epoll_fd, wait_event, maxi+1, -1);
		if(wait_ret == -1)
		{
			printf("%s-%s-%d epoll_wait failed\n",__FILE__,__func__,__LINE__);
			break;
		}
		//轮询wait_ret个文件描述符的状态
		for (n = 0; n < wait_ret; ++n)
		{
			//listen套接字有状态变化，则通过accept函数创建新的连接，通过epoll_ctl添加到监测池
			if (wait_event[n].data.fd == listenfd
					&& (EPOLLIN == (wait_event[n].events & (EPOLLIN|EPOLLERR))))
			{
				newfd = accept(listenfd, (struct sockaddr *) &cli_addr,
								(socklen_t*)&clilen);
				if (newfd < 0) {
					printf("%s-%s-%d accept failed\n",__FILE__,__func__,__LINE__);
					continue;
				}else
				{
					ret = tcp_ctrl_set_noblock(newfd);
				    if(ret < 0)
				    {
				    	printf("%s-%s-%d tcp_ctrl_set_noblock failed\n",__FILE__,__func__,__LINE__);
				    	pthread_exit(0);
				    }

					printf("%d---------------------------\n",newfd);
					printf("client ip=%s,port=%d\n", inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					//将套接字号添加到epoll监听事件之中
					event.events = EPOLLIN | EPOLLET;
					event.data.fd = newfd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &event) < 0) {

						printf("%s-%s-%d epoll_ctl failed\n",__FILE__,__func__,__LINE__);
						pthread_exit(0);
					}
					maxi++;
				}

			}else if(wait_event[n].events & EPOLLIN)
			{
				sys_mutex_lock(CTRL_TCP_MUTEX);
				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);
				sys_mutex_unlock(CTRL_TCP_MUTEX);

				//客户端关闭连接
				if(len <= 0)// && errno != EAGAIN)
				{
					printf("client %d offline,len=%d\n",wait_event[n].data.fd,len);

					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							wait_event[n].data.fd,&event);
					if( ctrl_ret < 0)
					{
						printf("%s-%s-%d epoll_ctl failed\n",__FILE__,__func__,__LINE__);
					}
					close(wait_event[n].data.fd);
					maxi--;
				}else
				{
//					getpeername(wait_event[n].data.fd,(struct sockaddr*)&cli_addr,
//							(socklen_t*)&clilen);
//					printf("\n client %s: ",inet_ntoa(cli_addr.sin_addr));
//					int i;
////					printf("\nEPOLL %d:",wait_event[n].data.fd);
//					for(i=0;i<len;i++)
//					{
//						printf("%x ",buf[i]);
//					}
//					printf("\n");
				}

				tcp_ctrl_org_data_analysis(wait_event[n].data.fd,buf,len);

			}else if(wait_event[n].events & EPOLLERR)
			{
				printf("%d client %d offline\n",__LINE__,wait_event[n].data.fd);
				ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
						wait_event[n].data.fd,&event);
				if( ctrl_ret < 0)
				{
					printf("%s-%s-%d epoll_ctl failed\n",__FILE__,__func__,__LINE__);
				}
				close(wait_event[n].data.fd);
				maxi--;

				len = 0;
				tcp_ctrl_org_data_analysis(wait_event[n].data.fd,buf,len);

			}//end if

		}//end for

	}//end while

	free(wait_event);
	close(epoll_fd);
    close(listenfd);
    pthread_exit(0);

}


/*
 * wifi_sys_ctrl_tcp_send
 * 主机系统TCP数据发送线程函数
 *
 * 将数据发送队列中的数据取出后，进行发送，然后清空队列发送队列元素的信息
 *
 * 输入
 * 无
 * 输出
 * 无
 * 返回值
 * 无
 */
void* wifi_sys_ctrl_tcp_send(void* p)
{
	int i,ret;
	Plinknode node = NULL;
	Pctrl_tcp_rsqueue tmp = NULL;

	pthread_detach(pthread_self());


	while(1)
	{
		sys_semaphore_wait(CTRL_TCP_SEND_SEM);

		sys_mutex_lock(CTRL_TCP_SQUEUE_MUTEX);

		ret = out_queue(node_queue->sys_queue[CTRL_TCP_SEND_QUEUE],&node);

		if(ret == 0)
		{
			tmp = node->data;
//			if(sys_debug_get_switch())
//			{
				if(tmp->msg[4] != 0x86)
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

			sys_mutex_lock(CTRL_TCP_MUTEX);
			send(tmp->socket_fd,tmp->msg, tmp->len,0);
			sys_mutex_unlock(CTRL_TCP_MUTEX);

			free(tmp->msg);
			free(tmp);
			free(node);

			tmp->msg = NULL;
			tmp = NULL;
			node = NULL;

		}else{
			printf("%s dequeue error\n",__func__);

		}
		sys_mutex_unlock(CTRL_TCP_SQUEUE_MUTEX);

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
			printf("fd--%d,id--%d,seat--%d,name--%s\n",info->fd,
					info->ucinfo.id,info->ucinfo.seat,info->ucinfo.name);
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





