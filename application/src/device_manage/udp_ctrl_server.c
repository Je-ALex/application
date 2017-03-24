/*
 *
 *  Created on: 2016年11月10日
 *      Author: leon
 *
 * udp_ctrl_ser.c
 * 设备发现UDP广播服务
 *
 * 主要是设备发现的UDP部分
 * UDP广播包的组包，UDP套接字的初始化，广播包的发送
 *
 */

#include "wifi_sys_init.h"
#include "udp_ctrl_server.h"
#include "tcp_ctrl_device_status.h"

int status = 0;



/*
 * udp_data_frame
 * UDP设备发现广播包组包协议
 *
 * 消息分为数据报头，帧信息，帧长度，帧内容和校验和
 *
 */
static void udp_data_frame(unsigned char* str,int* len)
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

}

//static void sig_handler()
//{
//	sys_net_status_set(1);
//
//}

/*
 * wifi_sys_ctrl_udp_server
 * 系统网络设备发现广播线程函数
 *
 * 主要是对设备发现包进行广播，用于网络上设备进行TCP连接
 *
 */
void* wifi_sys_ctrl_udp_server(void* p)
{
	int sock;
	int ret = 0;
	int opt = 1;
    int len;
    unsigned char msg[100] = {0};

	pthread_detach(pthread_self());

	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0)
	{
		 printf("%s-%s-%d init socket failed\n",__FILE__,__func__,__LINE__);
		 pthread_exit(0);
	}

	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			(char *)&opt, sizeof(opt));
	if(ret)
	{
		printf("%s-%s-%d setsockopt error\n",__FILE__,__func__,__LINE__);
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
		printf("%s-%s-%d bind failed\n",__FILE__,__func__,__LINE__);
		pthread_exit(0);
	}

    udp_data_frame(msg,&len);


    /*
     * UDP数据广播，1秒广播一次
     */
	while(1){

		if(sys_net_status_get() == 1)
			goto ERR;

		ret = 0;
		ret = sendto(sock,msg,len,0,
				(struct sockaddr*)&addr_serv,sizeof(addr_serv));

		if(ret < 0){
			status++;
			printf("sendto fail number = %d\n",status);

//			close(sock);
//			pthread_exit(0);
		}else{
//				for(i=0;i<len;i++)
//				{
//					printf("%02X ",msg[i]);
//				}
//				printf("send ok\n");
		}
		sleep(1);
	}
ERR:
	close(sock);
	pthread_exit(0);
}


