
#include "scanf_md_udp.h"



/*
 * udp_data_frame
 * UDP设备发现广播包组包协议
 *
 * 消息分为数据报头，帧信息，帧长度，帧内容和校验和
 *
 *
 */
void udp_data_frame(unsigned char* str,int* len)
{

	unsigned char* data = str;
	int i;
	unsigned char sum = 0;
	unsigned char udp_index = 0;

	/*
	 * data[0-3]帧头
	 * DSDS
	 */
	data[udp_index++] = 'D';
	data[udp_index++] = 'S';
	data[udp_index++] = 'D';
	data[udp_index++] = 'S';

	/*
	 * data[4]
	 * 消息类型
	 */
	data[udp_index++] = (UDP_BRD << 4);

    /*
     * data[5] 数据包长度
     *
     */
	udp_index++;
    /*
     * data[6] 保留
     *
     */
	udp_index++;
	/*
	 * data[7-8]
	 * 控制模块的端口号信息
	 */
	data[udp_index++] = (unsigned char)((CTRL_TCP_PORT >> 8) & 0xff);
	data[udp_index++] = (unsigned char)(CTRL_TCP_PORT & 0xff);

	/*
	 * data[5] 数据包长度
	 *
	 */
	data[5] = ++udp_index;

	/*
	 * 计算检验和
	 */
	for(i=0;i<udp_index-1;i++)
	{
		sum +=data[i];
	}
	/*
	 * 最后一位校验和
	 */
	data[i] = sum;
	*len = ++i;

//	printf("0x%.2x \n",i);
}



/*
 * wifi_sys_ctrl_udp_server
 * udp线程，设备发现
 *
 * 主要是服务端的控制，数据包的广播
 *
 */
void* wifi_sys_ctrl_udp_server(void* p)
{
	int sock;
	int ret = 0;
	int opt = 1;
    int len;
    unsigned char msg[100] = {0};

	printf("%s,%d\n",__func__,__LINE__);

	/*
	 * 创建socke套接字
	 */
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0)
	{
		 printf("init socket failed\n");
		 pthread_exit(0);
	}
	/*
	 * 设置套接字的属性
	 */
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			(char *)&opt, sizeof(opt));
	if(ret)
	{
		printf("setsockopt error...\n");
		pthread_exit(0);
	}
	/*
	 * 配置udp服务端参数
	 */
	struct sockaddr_in addr_serv;
	memset(&addr_serv,0,sizeof(addr_serv));
	addr_serv.sin_family=AF_INET;
	addr_serv.sin_addr.s_addr=htonl(INADDR_BROADCAST);
	addr_serv.sin_port = htons(CTRL_BROADCAST_PORT);
	/*
	 * 绑定套接字号
	 */
	if(bind(sock,(struct sockaddr *)&(addr_serv),
			sizeof(struct sockaddr_in)) == -1)
	{
		printf("bind error...\n");
		pthread_exit(0);
	}

    udp_data_frame(msg,&len);

    /*
     * UDP数据广播，没秒钟广播一次
     */
	while(1){

		ret = 0;
		ret = sendto(sock,msg,len,0,
				(struct sockaddr*)&addr_serv,sizeof(addr_serv));

		if(ret < 0){
			printf("sendto fail\r\n");
			close(sock);
			pthread_exit(0);
		}else{

			{
//				for(i=0;i<len;i++)
//				{
//					printf("%02X ",msg[i]);
//				}
//				printf("send ok\n");
			}

		}
		sleep(1);

	}
	close(sock);
	pthread_exit(0);
}


