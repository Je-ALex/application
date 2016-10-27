
#include "../../inc/device_ctrl_module_udp.h"
#include "../../inc/tcp_ctrl_server.h"



/*
 * 数据头为两个字节
 * 一个字节消息类型
 * 一个字节表示数据长度
 * 一个保留字节
 * 变长数据内容
 * 校验和
 *
 */
void udp_data_frame(unsigned char* str,int* len)
{

	unsigned char* data = str;
	int i;
	unsigned char sum = 0;
	/*
	 * header is stable
	 */
	data[0] = 'D';
	data[1] = 'S';

	/*
	 *message as reserved(0x00)
	 *message as broadcast(0x10)
	 *message as singledata(0x20)
	 */
	data[2] = 0x10;

	/*
	 * length is strlen of frame
	 * data[3]
	 */

    /*
     *data[4] reserved
     */
	/*
	 * data content is variable
	 * at here is tcp port
	 */
	data[5] = (unsigned char)((CTRL_PORT >> 8) & 0xff);
	data[6] = (unsigned char)(CTRL_PORT & 0xff);

	/*
	 * checksum
	 */
	for(i=0;;i++)
	{
		sum +=data[i];

		if((data[i] | data[i+1] |
				data[i+2]) < 1)
		{
			break;
		}
	}

	printf("%d \n",i);

	/*
	 * length is strlen of frame
	 * data[3]
	 */
	data[3] = i + 1;

	/*
	 * checksum
	 */
	data[i] = sum + data[3];
	*len = data[3];
}

/*
 * udp线程
 * 主要是服务端的控制
 * 用全局结构体来共享数据信息，加锁
 */
void* udp_server(void* p)
{
	int sock;
	const int opt = 1;
	int ret = 0;

	printf("%s,%d\n",__func__,__LINE__);
	/*
	 * new a udp socket
	 */
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0)
	{
		 printf("init socket failed\n");
		 pthread_exit(0);
	}
	/*
	 * set the socket proprrty as a broadcast
	 */
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			(char *)&opt, sizeof(opt));
	if(ret == -1)
	{
		printf("set socket error...\n");
		pthread_exit(0);
	}
	/*
	 *configuration the socket  parameter
	 */
	struct sockaddr_in addr_serv;
	memset(&addr_serv,0,sizeof(addr_serv));
	addr_serv.sin_family=AF_INET;
	addr_serv.sin_addr.s_addr=htonl(INADDR_BROADCAST);
	addr_serv.sin_port = htons(DEF_PORT);
	/*
	 * bind the socket
	 */
    if(bind(sock,(struct sockaddr *)&(addr_serv),
    		sizeof(struct sockaddr_in)) == -1)
    {
        printf("bind error...\n");
        pthread_exit(0);
    }

    int i,len;
    unsigned char msg[100] = {0};
    udp_data_frame(msg,&len);

    /*
     * UDP broadcast data
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
//			for(i=0;i<8;i++)
//			{
//				printf("%02X ",msg[i]);
//			}
//			printf("send ok\n");
		}
		sleep(1);

	}
}

/*
 * UDP socket
 * 主要是主机UDP服务端功能模块
 * 主要功能：
 * 1、创建套接字，返回创建结果
 * 2、广播数据包，等待客户端响应，返回结果
 *
 */
int device_ctrl_module_udp(int port)
{



	void*status;
	pthread_t s_id;


	pthread_create(&s_id,NULL,udp_server,NULL);

	pthread_join(s_id,&status);

	return 0;
}


