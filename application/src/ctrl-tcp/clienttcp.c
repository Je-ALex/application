/*
 * clienttcp.c
 *
 *  Created on: 2016年9月28日
 *      Author: leon
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXLINE 80
int port = 8080;
pthread_t s_id;

//#define IP "192.168.10.61"
#define IP "127.0.0.1"


static void* control_tcp_cli(void* p)
{
  struct sockaddr_in pin;
  int sock_fd;
  unsigned char buf[256] = {0};
  int n,i;
  unsigned char buf_name[256] = {0};
  unsigned char buf_sub[256] = {0};
  int num = (int)p;

  bzero(&pin, sizeof(pin));
  pin.sin_family = AF_INET;
  inet_pton(AF_INET, IP, &pin.sin_addr);
  pin.sin_port = htons(port);

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  n=connect(sock_fd, (void *)&pin, sizeof(pin));

  if (-1 == n)
  {
     perror("call connect");
     exit(1);
  }
  char str[19] ={0x44,0x53,0x44, 0x53, 0x67, 0x00, 0x13, 00, 00, 00, 00, 0x0C,
		  00, 00, 00, 00, 00,00,0xB4};
//  char* str2 = "44 53 44 53 65 00 13 00 00 00 00 0C 00 00 00 00 00 00 B2";
  //NULL != fgets(str,MAXLINE, stdin)

  while(1)
  {

	write(sock_fd, str, 19);

	memset(buf,0,sizeof(buf));
	memset(buf_name,0,sizeof(buf_name));
	memset(buf_sub,0,sizeof(buf_sub));


	n=recv(sock_fd, buf, sizeof(buf), 0);

    if (n == 0)
    {
        printf("the othere side has been closed.\n");
        pthread_exit(0);
    }
    else
    {
    	printf("the %d receive from server: \n",num);

    	memcpy(buf_name,&buf[23],buf[22]);
    	printf("%s\n ",buf_name);

    	memcpy(buf_sub,&buf[23+buf[22]+3],buf[23+buf[22]+2]);
    	printf("%s\n ",buf_sub);


    	for(i=0;i<n;i++)
    	   printf("%x ",buf[i]);
    }
    printf("\n");

    sleep(1);

  }
  close(sock_fd);

  return 0;
}

int main(void)
{
	void*status;
	int i;
	for(i=0;i<5;i++)
	{
		pthread_create(&s_id, NULL, (void *)control_tcp_cli, (void*)i);  //创建线程
//		pthread_detach(s_id); // 线程分离，结束时自动回收资源
		usleep(100000);
		printf("thread number %d\n",i);
	}
	pthread_join(s_id,&status);

    return 0;
}
