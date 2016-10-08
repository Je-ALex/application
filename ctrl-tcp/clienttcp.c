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

int main(int argc, char *argv[])
{
  struct sockaddr_in pin;
  int sock_fd;
  char buf[MAXLINE];
//  char str[MAXLINE];
  int n,i=1;

  signal(SIGPIPE,SIG_IGN);

  bzero(&pin, sizeof(pin));
  pin.sin_family = AF_INET;
  inet_pton(AF_INET, "192.168.10.65", &pin.sin_addr);
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

	i++;
	snprintf(s2,sizeof(int),"%x",i);

	strcat(str,s2);

    write(sock_fd, str, strlen(str)+1);
    n=read(sock_fd, buf, MAXLINE);

    if (0 == n)
      printf("the othere side has been closed.\n");
    else
      printf("receive from server:%s\n",buf);


    memset(str+3,0,4);
    usleep(500);

  }
  close(sock_fd);

  return 0;
}
