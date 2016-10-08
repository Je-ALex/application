/*
 * servertcp.c
 *
 *  Created on: 2016年9月28日
 *      Author: leon
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include <pthread.h>

#define MAXLINE 800
int port = 8080;

pthread_t s_id;

typedef struct{


};

enum{


	SOCKERRO = -3,
	BINDERRO,
	LISNERRO,
	SUCCESS = 0,

};


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

static void* control_tcp(void* p)
{

	struct sockaddr_in pin;
	int listen_fd;
	int conn_fd;
	int nready;
	int maxi;
	int max;
	int client[FD_SETSIZE];
	int address_size = sizeof(pin);
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	int i;
	int len;
	int n;
	int ret;

	listen_fd = local_addr_init();

	while(1)
	{
		char cli_ip[INET_ADDRSTRLEN] = "";	   // 用于保存客户端IP地址
		struct sockaddr_in client_addr;		   // 用于保存客户端地址
		socklen_t cliaddr_len = sizeof(client_addr);   // 必须初始化!!!

		//获得一个已经建立的连接
		connfd = accept(sockfd, (struct sockaddr*)&client_addr, &cliaddr_len);
		if(connfd < 0)
		{
			perror("accept this time");
			continue;
		}else{
			// 打印客户端的 ip 和端口
			inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);
			printf("----------------------------------------------\n");
			printf("client ip=%s,port=%d\n", cli_ip,ntohs(client_addr.sin_port));
		}




	}


	while(1)
	{
		   printf("Accepting connections...\n");
		   conn_fd = accept(listen_fd, (struct sockaddr *)&pin, &address_size);

		   printf("you ip is %s at port %d\n",
		   					 inet_ntop(AF_INET, &pin.sin_addr,str,sizeof(str)),
		   					 ntohs(pin.sin_port));

	        while(1)
	        {
				memset(buf,'\0',MAXLINE);
readagain:
				ret = read(conn_fd,buf,MAXLINE);
				printf("I read %d Byte!\n",ret);

				if (-1 == ret){
					if (errno == EINTR){
						goto readagain;
					}else{
						perror("call to read");
						pthread_exit(0);
					}
				} else if (0 == ret){
					printf("the other side has been closed.\n");
					break;
				}


				printf("you ip is %s at port %d:%s\n",
					 inet_ntop(AF_INET, &pin.sin_addr,str,sizeof(str)),
					 ntohs(pin.sin_port),buf);

				len = strlen(buf);
				for (i = 0; i < len; i++)
				{
					buf[i] = toupper(buf[i]);
				}

writeagain:
				ret = write(conn_fd, buf, len+1);
				printf("I write %d Byte!\n",ret);

				if (-1 == ret){
					if (errno == EINTR){
						goto writeagain;
					} else {
						perror("call to write!");
						break;
					}
				}
	        }// end while


			ret = close(conn_fd);
			if (-1 == ret){
				perror("call close");
				pthread_exit(0);
			}


	    }
	    close(listen_fd);
}


int main(void)
{
	void*status;

	pthread_create(&s_id,NULL,control_tcp,NULL);

	pthread_join(s_id,&status);


    return 0;
}
