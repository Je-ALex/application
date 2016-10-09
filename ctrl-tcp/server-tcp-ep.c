/*
 * epool-test.c
 *
 *	控制类消息的TCP线程
 *	上位机的通讯部分
 *	单元机的通讯部分
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

#define MAXLINE 800
int port = 8080;

pthread_t s_id;

#define OPEN_MAX 1000

/*
 * 连接主机的设备信息
 * 主要有
 * socket fd
 * client addr的信息
 * client mac的信息
 */
typedef struct {

	struct epoll_event event;   // 告诉内核要监听什么事件
	int fd;
	struct sockaddr_in cli_addr;
	int clilen;
	char clint_mac[24];

}Client_Info;

enum{

	SOCKERRO = -3,
	BINDERRO,
	LISNERRO,
	SUCCESS = 0,

};

typedef enum{

	PC_CTRL = 1,
	HOST_CTRL,
	UNIT_CTRL,

}Machine_Type;
typedef enum{

	EVT_DATA = 1,
	UNIT_DATA,
	CONFERENCE_DATA,

}Data_Type;
typedef enum{

	CTRL_DATA = 1,
	REQUEST_DATA,
	REPLY_DATA,
}Message_Type;


typedef struct {

	char msg_type;
	char data_type;
	char dev_type;

}Frame_Type;



static int local_addr_init()
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

static void set_noblock(int fd)
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
static void frame_compose(char* result_data,char* data,int* send_len,int len)
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
static void frame_analysis(const char* buf,int* len,char* handlbuf,Frame_Type* frame_type)
{

	int i;
	int package_len = 0;
	unsigned char sum;

	int length = *len;
	char* buffer = handlbuf;

	/*
	 * print the receive data
	 */
	for(i=0;i<*len;i++)
	{
		printf("%x ",buf[i]);
	}
	printf("\n");
	printf("length: %d\n",length);

	/*
	 * 验证数据头是否正确buf[0]~buf[3]
	 */
	//printf("%c,%c,%c,%c\n",buf[0],buf[1],buf[2],buf[3]);
	while (buf[0] != 'D' || buf[1] != 'S'
			|| buf[2] != 'D' || buf[3] != 'S') {

		printf( "%s--not legal headers---%d\n", __FUNCTION__, __LINE__);
		return;
	}
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
	 * 计算数据长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[5] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + buf[6] & 0xff;

	printf("package_len: %d\n",package_len);

	if(length < 0x0e || (length !=package_len))
	{
		printf( "%s--not legal length---%d\n  ", __FUNCTION__, __LINE__);

		return;
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
		return;
	}

	/*
	 * 保存有效数据
	 * 将有效数据另存为一个内存区域
	 */
	buffer = calloc(2,sizeof(char));
	memcpy(buffer,buf+11,2);

	printf("buffer: ");
	for(i=0;i<strlen(buffer);i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");
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
static void process_rev_msg(int* cli_fd, const char* value, int* length)
{


	/*
	 * 检查数据包是否是完整的数据包
	 * 数据头的检测
	 * 数据长度的检测
	 * 校验和的检测
	 */
	Frame_Type frame_type;

	char* handlbuf;

	frame_analysis(value,length,handlbuf,&frame_type);


	switch(frame_type.dev_type)
	{
		case PC_CTRL:
			printf("process the pc data\n");
			//process_ctrl_msg_from_pc(cli_fd,handlbuf,&frame_type);
			break;

	}

}

static void* control_tcp_recv(void* p)
{
	int sockfd;
	int epoll_fd;
	struct epoll_event event;   // 告诉内核要监听什么事件
    struct epoll_event* wait_event; //内核监听完的结果

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

    int n;
    int connfd;
    int ctrl_ret,wait_ret;

	int len = 0;
	char buf[1024] = "";

    /*
     *socket init
     */
    sockfd = local_addr_init();
    set_noblock(sockfd);


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

    /* Buffer where events are returned */
//    wait_event = calloc (MAXEVENTS, sizeof wait_event);

	//epoll相应参数准备
//	int fd[OPEN_MAX];
	int i = 0, maxi = 0;
//	memset(fd,-1, sizeof(fd));
//	fd[0] = sockfd;
//	printf("sockfd %d \n",sockfd);


	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */
	char ask_buf[] = {0x26,0x09,0x01};
	char send_buf[1024] = {0};
	int send_len;

	frame_compose(send_buf,ask_buf,&send_len,sizeof(ask_buf));
	printf("send_len %d \n",send_len);

	int conc_num = 0;

	wait_event = calloc(1000,sizeof(struct epoll_event));

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

				connfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								&clilen);
				if (connfd < 0) {
					perror("accept");
					continue;
				}
				else
				{
					conc_num++;
					set_noblock(connfd);
					// 打印客户端的 ip 和端口
					printf("%d----------------------------------------------\n",conc_num);
					printf("client ip=%s,port=%d\n", inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					/*
					 * 设备上线，发送一个信息获取
					 */
					send(connfd, send_buf, send_len, 0);
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
					maxi++;
				}

			} else {

				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);

				printf("%s  len = %d,%d\n",__func__,len,__LINE__);
				//客户端关闭连接
				if(len < 0 && errno != EAGAIN)
				{
//					if(errno == ECONNRESET)//tcp连接超时、RST
//					{
//						close(fd[i]);
//						fd[i] = -1;
//					}
//					else
//						perror("read error:");
//					printf("wait_event[n].data.fd--%d\n",wait_event[n].data.fd);
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

					process_rev_msg(&wait_event[n].data.fd,buf,&len);
					//将测试数据返回发送
					send(wait_event[n].data.fd, buf, len, 0);
				}
			}//end if

		}//end for

//		//监测sockfd(监听套接字)是否存在连接
//		if(( sockfd == wait_event.data.fd )
//            && ( EPOLLIN == wait_event.events & EPOLLIN ))
//		{
//
//			printf("main sock listen the connect\n");
//			//从tcp完成连接中提取客户端
//			int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
//			if(connfd < 0)
//			{
//				perror("accept this time");
//
//
//			}else{
//				conc_num++;
//				set_noblock(connfd);
//				// 打印客户端的 ip 和端口
//				printf("%d----------------------------------------------\n",conc_num);
//				printf("client ip=%s,port=%d\n", inet_ntoa(cli_addr.sin_addr),
//						ntohs(cli_addr.sin_port));
//
//
//				/*
//				 * 设备上线，发送一个信息获取
//				 */
//				send(connfd, send_buf, send_len, 0);
//				/*
//				 * print the data
//				 */
//				for(i =0;i<send_len;i++)
//				{
//					printf("%02x ",send_buf[i]);
//				}
//				printf("\n");
//			}
//
//			//将提取到的connfd放入fd数组中，以便下面轮询客户端套接字
//			for(i=1; i<OPEN_MAX; i++)
//			{
//				if(fd[i] < 0)
//				{
//					fd[i] = connfd;
//					event.data.fd = connfd; //监听套接字
//					event.events = EPOLLIN | EPOLLET; // 表示对应的文件描述符可以读
//
//					//事件注册函数，将监听套接字描述符 connfd 加入监听事件
//					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &event);
//					if(-1 == ctrl_ret){
//						perror("epoll_ctl");
//						pthread_exit(0);
//					}
//					break;
//				}
//			}
//
//			//maxi更新
//			if(i > maxi)
//				maxi = i;
//
//			//如果没有就绪的描述符，就继续epoll监测，否则继续向下看
//			if(--wait_ret <= 0)
//				continue;
//		}else{
//			;
//		}
//
//		//继续响应就绪的描述符
//		for(i=1; i<=maxi; i++)
//		{
//			printf("0 client %d \n",fd[0]);
//			printf("%d client %d \n",i,fd[i]);
//			printf("%d wait_event.data.fd %d \n",i,wait_event.data.fd);
//			if(fd[i] < 0)
//				continue;
//
//			if(( fd[i] == wait_event.data.fd )
//            && ( EPOLLIN == wait_event.events & (EPOLLIN|EPOLLERR)))
//			{
//
//				int len = 0;
//				char buf[1024] = "";
//
//				len = recv(fd[i], buf, sizeof(buf), 0);
//
//				printf("%s  len = %d,%d\n",len,__func__,__LINE__);
//				//接受客户端数据
//				if(len < 0)
//				{
//					if(errno == ECONNRESET)//tcp连接超时、RST
//					{
//						close(fd[i]);
//						fd[i] = -1;
//					}
//					else
//						perror("read error:");
//
//				}
//				else if(len == 0)//客户端关闭连接
//				{
//					printf("client %d offline\n",fd[i]);
//					close(fd[i]);
//					fd[i] = -1;
//				}
//				else
//				{
//					getpeername(fd[i],(struct sockaddr*)&cli_addr,&clilen);
//					printf("client %s:%d\n",inet_ntoa(cli_addr.sin_addr),
//							ntohs(cli_addr.sin_port));
//
//					process_rev_msg(&fd[i],buf,&len);
//					//将测试数据返回发送
//					send(fd[i], buf, len, 0);
//				}
//				//所有的就绪描述符处理完了，就退出当前的for循环，继续poll监测
//				if(--wait_ret <= 0)
//					break;
//			}
//			else
//			{
//				/*
//				 * 客户端连接异常，清除异常连接
//				 */
//				printf("tcp connect abnormal eixt\n");
////				for(i=1; i<=maxi; i++)
////				{
////					if((fd[i] != wait_event.data.fd) && (fd[i] > 0))
////					{
////						event.data.fd = fd[i]; //监听套接字
////						event.events = EPOLLIN | EPOLLET; // 表示对应的文件描述符可以读
////						ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd[i] , &event);
////						if(-1 == ctrl_ret){
////							perror("epoll_ctl");
////							pthread_exit(0);
////						}
////	//					close(fd[i]);
////
////					}
////				}
////				maxi--;
//
//			}
//
//		}

	}//end while

	free(wait_event);
    close(sockfd);
    pthread_exit(0);


}


int main(void)
{
	void*status;

	pthread_create(&s_id,NULL,control_tcp_recv,NULL);

	pthread_join(s_id,&status);

    return 0;
}
