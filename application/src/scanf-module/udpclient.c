#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include <linux/socket.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<errno.h>


pthread_t c_id;
#define PORT 50001

static void* client(void* p)
{
	int sock;

	//在recvfrom中使用的对方主机地址
	struct sockaddr_in fromAddr;

	unsigned int fromLen;
	char recvBuffer[128] = {0};

	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0)
	{
	 printf("创建套接字失败了.\r\n");
	 pthread_exit(0);
	}

    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = htonl(INADDR_ANY);
    addrto.sin_port = htons(PORT);

	const int opt = -1;

	//设置该套接字为广播类型，
//	int nb = 0;
//	nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
//			(char *)&opt, sizeof(opt));
//	if(nb == -1)
//	{
//		printf("set socket error...\n");
//		pthread_exit(0);
//	}

    if(bind(sock,(struct sockaddr *)&(addrto),
    		sizeof(struct sockaddr_in)) == -1)
    {
        printf("bind error...\n");
        pthread_exit(0);
    }

	while(1)
	{
		printf("start recv\n");

		fromLen = sizeof(fromAddr);
		if(recvfrom(sock,recvBuffer,sizeof(recvBuffer),
				0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen)<0)
		{
			 printf("()recvfrom()函数使用失败了\n");
			 close(sock);
			 pthread_exit(0);
		}
		printf("recvfrom() result:%s\n",recvBuffer);
		printf("server ip:port-->%s:%d\n",inet_ntoa(fromAddr.sin_addr),
				ntohs(fromAddr.sin_port));

	}

	close(sock);

}
int test_client()
{

	void*status;

	pthread_create(&c_id,NULL,client,NULL);

	pthread_join(c_id,&status);

	return 0;
}


