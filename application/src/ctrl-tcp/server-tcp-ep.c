/*
 * server-tcp-ep.c
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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>


#include "../../header/ctrl-tcp.h"

int port = 8080;
pthread_t s_id;
pthread_mutex_t mutex;

Server_Info fd_info[1000];
Client_Info* info = 0;


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
 * 主机发送数据组包函数
 * 主要功能
 * 计算校验和
 */
static void tc_frame_compose(char* result_data,char* data,int* send_len,int len)
{

	int i;
	int length = 0;
	unsigned char sum = 0;

	if(result_data == NULL)
		return;
	/*
	 * HEAD
	 */
	result_data[0] = 'D';
	result_data[1] = 'S';
	result_data[2] = 'D';
	result_data[3] = 'S';
	/*
	 * TYPE
	 */
	result_data[4] = data[0];
	/*
	 * DATA
	 */
	memcpy(result_data+11,data+1,2);

	/*
	 * LENGTH
	 * 数据长度加上固定长度
	 * 固定长度为4个头字节+2个长度+4个目标地址+1个校验和
	 */
	length = len + 11;
	*send_len=length;

	result_data[5] = (length >> 8) & 0xff;
	result_data[6] = length & 0xff;

	/*
	 * calculate the checksum
	 */
	for(i =0;i<length;i++)
	{
		sum += result_data[i];
	}
	result_data[length - 1] = sum&0xff;

	/*
	 * print the data
	 */
	for(i =0;i<length;i++)
	{
		printf("%02x ",result_data[i]);
	}
	printf("\n");

}
static void tc_frame_analysis(const char* buf,int* len,char* handlbuf,Frame_Type* frame_type)
{

	int i;
	int package_len = 0;
	unsigned char sum;

	int length = *len;
	char* buffer = handlbuf;

	/*
	 * print the receive data
	 */

//	for(i=0;i<*len;i++)
//	{
//		printf("%x ",buf[i]);
//	}
//	printf("\n");


	printf("length: %d\n",length);

	/*
	 * 验证数据头是否正确buf[0]~buf[3]
	 */
	//printf("%c,%c,%c,%c\n",buf[0],buf[1],buf[2],buf[3]);
//	fixme
//	while (buf[0] != 'D' || buf[1] != 'S'
//			|| buf[2] != 'D' || buf[3] != 'S') {
//
//		printf( "%s--not legal headers---%d\n", __FUNCTION__, __LINE__);
//		return;
//	}
	/*
	 * buf[4]
	 * 判断数据类型
	 * 消息类型(0xe0)4.5-4.7：控制消息(0x01)，请求消息(0x02)，应答消息(0x03)
	 * 数据类型(0x1c)4.2-4.4：事件型数据(0x01)，会议型单元参数(0x02)，会议型会议参数(0x03)
	 * 设备类型数据(0x03)4.0-4.1：上位机发送(0x01)，主机发送(0x02)，单元机发送(0x03)
	 */
	frame_type->msg_type = buf[4] & 0xe0;
	frame_type->data_type = buf[4] & 0x1c;
	frame_type->dev_type = buf[4] & 0x03;

	/*
	 * 计算帧总长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[5] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + buf[6] & 0xff;

	printf("package_len: %d\n",package_len);

	if(length < 0x0e || (length !=package_len))
	{
		printf( "%s--not legal length---%d\n  ", __FUNCTION__, __LINE__);
//fixme
		//return;
	}

	/*
	 * 校验和的验证
	 */
	for(i = 0;i<package_len;i++)
	{
		sum = buf[i];
	}
	if(sum != buf[package_len-1])
	{
		printf( "%s---check sum not legal %d\n", __FUNCTION__, __LINE__);
		//fixme
		//return;
	}

	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 12
	 */
	int data_len = package_len -12;

	/*
	 * 保存有效数据
	 * 将有数据另存为一个内存区域
	 */
//	buffer = calloc(data_len,sizeof(char));
//	memcpy(buffer,buf+11,data_len);

	buffer = calloc(length,sizeof(char));
	memcpy(buffer,buf,length);

	printf("buffer: ");
	for(i=0;i<strlen(buffer);i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");
}

static void tc_process_ctrl_msg_from_reply()
{

}

static void tc_process_ctrl_msg_from_unit(int* cli_fd,char* handlbuf,Frame_Type* frame_type)
{

	printf("%s",handlbuf);

	switch(frame_type->msg_type)
	{

		case REQUEST_DATA:
			//tc_process_ctrl_msg_from_request();
			break;


		case REPLY_DATA:
			tc_process_ctrl_msg_from_reply();
			break;

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
static void tc_process_rev_msg(int* cli_fd, const char* value, int* length)
{


	/*
	 * 检查数据包是否是完整的数据包
	 * 数据头的检测
	 * 数据长度的检测
	 * 校验和的检测
	 */
	Frame_Type frame_type;

	char* handlbuf;


	printf("receive from %d---%s\n",*cli_fd,value);

	tc_frame_analysis(value,length,handlbuf,&frame_type);


	switch(frame_type.dev_type)
	{
		case PC_CTRL:
			printf("process the pc data\n");
			//process_ctrl_msg_from_pc(cli_fd,handlbuf,&frame_type);
			break;

		case UNIT_CTRL:
			printf("process the unit data\n");
			tc_process_ctrl_msg_from_unit(cli_fd,handlbuf,&frame_type);
			break;

	}

}
int conc_num = 0;
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
    int connfd;
    int ctrl_ret,wait_ret;

	int maxi = 0;

    /*
     *
     */
	int len = 0;
	char buf[1024] = "";

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

	memset(fd_info,-1,sizeof(fd_info));

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
				connfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								&clilen);
				if (connfd < 0) {
					perror("accept");
					continue;
				}
				else
				{

					tc_set_noblock(connfd);
					// 打印客户端的 ip 和端口
					printf("%d----------------------------------------------\n",conc_num);
					printf("client ip=%s,port=%d\n", inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					/*
					 * 设备上线，发送一个信息获取
					 */
//					send(connfd, send_buf, send_len, 0);
//					/*
//					 * print the data
//					 */
//					for(i =0;i<send_len;i++)
//					{
//						printf("%02x ",send_buf[i]);
//					}
//					printf("\n");
					event.events = EPOLLIN | EPOLLET;
					event.data.fd = connfd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &event) < 0) {
						fprintf(stderr, "add socket '%d' to epoll failed %s\n",
								connfd, strerror(errno));
						pthread_exit(0);
					}

					for(i = 0;i<maxi+1;i++)
					{
						if(fd_info[i].fd < 0)
						{
							fd_info[i].fd = connfd;
						}

						printf("fd_info[%d].fd-->%d \n",i,fd_info[i].fd);
					}
					maxi++;
					conc_num++;
					pthread_mutex_unlock(&mutex);
				}

			} else {

				pthread_mutex_lock(&mutex);
				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);
				pthread_mutex_unlock(&mutex);

				printf("%s  len = %d,%d\n",__func__,len,__LINE__);
				//客户端关闭连接
				if(len < 0 && errno != EAGAIN)
				{
//					printf("wait_event[n].data.fd--%d\n",wait_event[n].data.fd);

					fd_info[n].fd = -1;

					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							wait_event[n].data.fd,&event);
					if( ctrl_ret < 0)
					{
					 fprintf(stderr, "delete socket '%d' from epoll failed! %s\n",
							 wait_event[n].data.fd, strerror(errno));
					}
					maxi--;
					conc_num--;
					close(wait_event[n].data.fd);

				}
				else if(len == 0)//客户端关闭连接
				{
					printf("client %d offline\n",wait_event[n].data.fd);
					fd_info[n].fd = -1;
					close(wait_event[n].data.fd);

					maxi--;
					conc_num--;
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
static void scanf_client(Client_Info* info)
{
	int i;

	char get_client_mac[] = "get mac addr";

	info->clilen = sizeof(info->cli_addr);

	info = (Client_Info*)calloc(conc_num,sizeof(Client_Info));

	for(i=0;i<conc_num;i++)
	{
		getpeername(fd_info[i].fd,(struct sockaddr*)&info->cli_addr,
									&info->clilen);
		printf("client %s:%d\n",inet_ntoa(info->cli_addr.sin_addr),
									ntohs(info->cli_addr.sin_port));
	}
	for(i=0;i<conc_num;i++)
	{
		if(fd_info[i].fd > 0)
		{
			pthread_mutex_lock(&mutex);
			write(fd_info[i].fd, get_client_mac, sizeof(get_client_mac));
			pthread_mutex_unlock(&mutex);
		}
	}


}
static void* control_tcp_send(void* p)
{

	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */
	char ask_buf[] = {0x26,0x09,0x01};
	char send_buf[1024] = {0};
	int send_len;
	char str;
	int control;

	int i;
	tc_frame_compose(send_buf,ask_buf,&send_len,sizeof(ask_buf));
	printf("%s send_len %d \n",__func__,send_len);



	while(1)
	{
		/*
		 * 模拟对单元机的控制
		 * ‘S’扫描单元机
		 * ‘I’ID设置
		 * ‘X’席别
		 *
		 */
		str = fgetc(stdin);
		if(str != 'o')
		{
			printf("str--%c\n",str);
			switch(str)
			{
				case 'S':
					scanf_client(info);
					break;
				case 'I':

					break;
				case 'X':

					break;

				default:
					break;
			}
		}
//		usleep(10000);


	}
	free(info);


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