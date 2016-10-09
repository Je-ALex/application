/*
 * client.c
 *
 *  Created on: 2016Äê7ÔÂ7ÈÕ
 *      Author: leon
 */


#include <stdio.h>
#include <sys/types.h>//socket():bind();listen():accept();listen();accept();connect();
#include <sys/socket.h>//socket():bind();listen():accept();inet_addr();listen():accept();connect();
#include <arpa/inet.h>//htons();inet_addr():
#include <netinet/in.h>//inet_addr():
#include <strings.h>//bzero();
#include <stdlib.h>//atoi();exit();
#include <unistd.h>//close():
#include <string.h>

#include "../wav_parser.h"
#include "../sndwav_common.h"


#define port 8080
#define ip 192.168.70
int tcp_client(SNDPCMContainer_t *sndpcm)
{

	uint8_t *data = sndpcm->data_buf;


    int sockfd;
    struct sockaddr_in c_addr;
    char buf[N] = {0};

    size_t n;
    socklen_t c_len;

    c_len = sizeof(c_addr);

    /*  creat socket  */
    if(-1 == (sockfd = socket(AF_INET,SOCK_DGRAM,0)))
    {
        perror("socket");
        exit(-1);
    }

    /*    connect    */
    bzero(&c_addr, sizeof(c_addr));
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(port);
    c_addr.sin_addr.s_addr = inet_addr(ip);

    connect(sockfd,(struct sockaddr *)&c_addr, c_len);
    while(1){

		printf("buf:%s",data);
		if(-1 == (n = sendto(sockfd, data, strlen(data), 0, (struct sockaddr *)&c_addr, c_len)))
		{
			perror("sendto");
			exit(-1);
		}
		if(strncmp(data, "quit", 4) == 0)
				break;
		printf("n = %d\n",n);
    }
    close(sockfd);
    exit(0);



}
