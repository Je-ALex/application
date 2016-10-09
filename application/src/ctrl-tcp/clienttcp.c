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

#define IP "192.168.10.71"
//#define IP "1270.0.0.1"


static void* control_tcp_cli(void* p)
{
  struct sockaddr_in pin;
  int sock_fd;
  char buf[20] = {0};
  int n,i;

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
	 char str[] ={"DS "};
	 char s2[4];
  //NULL != fgets(str,MAXLINE, stdin)
  while(1)
  {

//	i++;
//	snprintf(s2,sizeof(int),"%x",i);
//
//	strcat(str,s2);

    //write(sock_fd, str, strlen(str)+1);
    n=read(sock_fd, buf, sizeof(buf));
    printf("the %d thread receive\n",num);
    if (n == 0)
    {
        printf("the othere side has been closed.\n");

    }
    else
    {
    	printf("receive from server: ");
    	for(i=0;i<n;i++)
    	   printf("%x ",buf[i]);
    }
    printf("\n");

    write(sock_fd, buf, n);
//    memset(str+3,0,4);
//    sleep(1);

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
		//pthread_detach(s_id); // 线程分离，结束时自动回收资源
		usleep(100000);
		printf("thread number %d\n",i);
	}
	pthread_join(s_id,&status);

    return 0;
}
