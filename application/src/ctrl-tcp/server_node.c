/*
 * server_node.c
 *
 *  Created on: 2016年10月10日
 *      Author: leon
 */


/*
 * server-tcp-ep.c
 *
 *	控制类消息的TCP线程 主要功能是接收和处理tcp控制类消息
 *	上位机的通讯部分
 *	单元机的通讯部分
 *	从服务端角度出发，接收客户端连接请求后，需要知道的客户端的消息，如下
 *	1.TCP中客户端的fd，后续下发指令需要用到
 *	2.客户端的IP/MAC地址，主要是识别具体的单元机
 *
 *	在TCP控制模块主要处理的工作，如下，
 *	1.逻辑控制类管理，如单元机关机
 *	2.会议类管理，扫描，ID设置，席别设置，签到
 *
 *	综上，服务端需要绑定fd和对应的ID（MAC）,控制消息才能准确下发
 *
 *  Created on: 2016年9月29日
 *      Author: leon
 */


#include "../../header/ctrl-tcp.h"
#include "../../header/tc_list.h"

int port = 8080;
pthread_t s_id;
pthread_mutex_t mutex;


pclient_node list_head;

static int tc_local_addr_init()
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

static void tc_set_noblock(int fd)
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
 * 首先确认组包消息的格式
 * 输入：
 * 帧数据类型：消息类型，数据类型，设备类型
 * 帧数据内容：控制类数据类型，会议型数据
 *
 * 输出：
 * 发送数据包内容，发送数据包长度
 *
 * 返回值：
 * 成功，失败
 */
static void tc_frame_compose(Pframe_type type,char* params,unsigned char* result_buf)
{

	unsigned char msg,data,machine,info;
	int length = 0;
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

	int tc_index = 0;

	msg=data=machine=info=0;

	if(result_buf == NULL)
		return;

	/*
	 * HEAD
	 */
	result_buf[tc_index++] = 'D';
	result_buf[tc_index++] = 'S';
	result_buf[tc_index++] = 'D';
	result_buf[tc_index++] = 'S';

	/*
	 * edit the frame information to the buf
	 * 三类info的地址分别是
	 * xxx 	xxx 	xx
	 * 0xe0	0x1c	0x03
	 * 每个type分别进行移位运算后进行位与，得到第四字节的信息
	 *
	 */
	msg = type->msg_type;
	printf("msg = %x\n",msg);
	msg = (msg << 5) & 0xe0;
	printf("msg = %x\n",msg);

	data = type->data_type;
	printf("data = %x\n",data);
	data = (data << 2) & 0x1c;
	printf("data = %x\n",data);

	machine = type->dev_type;
	printf("machine = %x\n",machine);
	machine = machine & 0x03;
	printf("machine = %x\n",machine);

	info = msg+data+machine;
	printf("info = %x\n",info);

	/*
	 * 存储第四字节的信息
	 */
	result_buf[tc_index++] = info;

	/*
	 * LENGTH
	 * 数据长度加上固定长度
	 * 固定长度为4个头字节+1个消息类+2个长度+4个目标地址+1个校验和，共12个固定字节
	 */
	length = type->data_len + 12;
	result_buf[tc_index++] = (length >> 8) & 0xff;
	result_buf[tc_index++] = length & 0xff;

	getpeername(type->fd,(struct sockaddr*)&cli_addr,
								&clilen);

	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 0 ) & 0xff);
	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 8 ) & 0xff);
	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 16  ) & 0xff);
	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 24  ) & 0xff);

	/*
	 * 数据拷贝
	 */
	memcpy(&result_buf[tc_index],params,type->data_len);

	tc_index = tc_index + type->data_len;

	int i;
	unsigned char sum = 0;
	/*
	 * 计算校验和
	 */
	for(i =0;i<tc_index;i++)
	{
		sum += result_buf[i];
	}
	result_buf[tc_index] = (unsigned char) (sum & 0xff);

	type->frame_len = tc_index+1;

	printf("result data: ");
	for(i=0;i<tc_index+1;i++)
	{
		printf("%x ",result_buf[i]);
	}
	printf("\n");


}
static void tc_frame_analysis(int* fd,const char* buf,int* len,char* handlbuf,Pframe_type frame_type)
{

	int i;
	int package_len = 0;
	unsigned char sum;

	int length = *len;
	char* buffer = handlbuf;

	/*
	 * print the receive data
	 */

//	for(i=0;i<*len;i++)
//	{
//		printf("%x ",buf[i]);
//	}
//	printf("\n");


	printf("length: %d\n",length);

	/*
	 * 验证数据头是否正确buf[0]~buf[3]
	 */
	//printf("%c,%c,%c,%c\n",buf[0],buf[1],buf[2],buf[3]);
//	fixme
//	while (buf[0] != 'D' || buf[1] != 'S'
//			|| buf[2] != 'D' || buf[3] != 'S') {
//
//		printf( "%s--not legal headers---%d\n", __FUNCTION__, __LINE__);
//		return;
//	}
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
	 * 计算帧总长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[5] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + buf[6] & 0xff;

	printf("package_len: %d\n",package_len);

	if(length < 0x0e || (length !=package_len))
	{
		printf( "%s not legal length\n", __FUNCTION__);
//fixme
		//return;
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
		printf( "%s check sum not legal\n", __FUNCTION__);
		//fixme
		//return;
	}

	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 12
	 */
	int data_len = package_len -12;

	/*
	 * 保存有效数据
	 * 将有数据另存为一个内存区域
	 */
//	buffer = calloc(data_len,sizeof(char));
//	memcpy(buffer,buf+11,data_len);

	buffer = calloc(length,sizeof(char));
	memcpy(buffer,buf,length);


	printf("buffer: ");
	for(i=0;i<sizeof(buffer);i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");

	tc_process_ctrl_msg_from_reply(fd,buffer,frame_type);
}

static void tc_process_ctrl_msg_from_reply(int* client_fd,char* msg,Pframe_type frame_type)
{
	pclient_node tmp;
	Pclient_info pinfo;

	tmp = list_head->next;

	printf("goto %s\n",__func__);

	printf("%s\n",msg);



	while(tmp != NULL)
	{
		pinfo = tmp->data;

		if(pinfo->client_fd == *client_fd)
		{
			memcpy(pinfo->client_mac,msg,sizeof(msg));
		}

		tmp=tmp->next;
	}

	free(msg);

}

static void tc_process_ctrl_msg_from_unit(int* cli_fd,char* handlbuf,Pframe_type frame_type)
{

	printf("%s",handlbuf);

	switch(frame_type->msg_type)
	{

		case Request_msg:
			//tc_process_ctrl_msg_from_request();
			break;


		case Reply_msg:
			tc_process_ctrl_msg_from_reply(cli_fd,handlbuf,frame_type);
			break;

	}


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
static void tc_process_rev_msg(int* cli_fd, const char* value, int* length)
{

	/*
	 * 检查数据包是否是完整的数据包
	 * 数据头的检测
	 * 数据长度的检测
	 * 校验和的检测
	 */
	Pframe_type type;

	char* handlbuf;

	type = (Pframe_type)malloc(sizeof(frame_type));

	memset(type,0,sizeof(frame_type));

	printf("receive from %d---%s\n",*cli_fd,value);

	tc_frame_analysis(cli_fd,value,length,handlbuf,type);


	switch(type->dev_type)
	{
		case PC_CTRL:
			printf("process the pc data\n");
			//process_ctrl_msg_from_pc(cli_fd,handlbuf,type);
			break;

		case UNIT_CTRL:
			printf("process the unit data\n");
			tc_process_ctrl_msg_from_unit(cli_fd,handlbuf,type);
			break;

	}
	free(type);

}
/*
 * TCP控制模块线程
 * 主要是初始化TCP端子，设置服务端模式，采用epoll进行多终端管理
 * 客户端连接后 ，生成一个唯一的fd，保存在
 */
static void* control_tcp_recv(void* p)
{
	int sockfd;
	int epoll_fd;

	struct epoll_event event;   // 告诉内核要监听什么事件

    struct epoll_event* wait_event; //内核监听完的结果

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

    int i,n;
    int newfd;
    int ctrl_ret,wait_ret;

	int maxi = 0;

    /*
     *
     */
	int len = 0;
	char buf[1024] = "";

    /*
     *socket init
     */
    sockfd = tc_local_addr_init();
    tc_set_noblock(sockfd);


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

	wait_event = calloc(1000,sizeof(struct epoll_event));


	/*
	 * 终端信息录入结构体
	 */
	Pclient_info info;

	list_head = list_head_init();

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
				pthread_mutex_lock(&mutex);
				newfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								&clilen);
				if (newfd < 0) {
					perror("accept");
					continue;
				}
				else
				{

					tc_set_noblock(newfd);
					// 打印客户端的 ip 和端口
					printf("%d----------------------------------------------\n",newfd);
					printf("client ip=%s,port=%d\n", inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));


					/*
					 * 设备上线，发送一个信息获取
					 */
//					send(newfd, send_buf, send_len, 0);
//					/*
//					 * print the data
//					 */
//					for(i =0;i<send_len;i++)
//					{
//						printf("%02x ",send_buf[i]);
//					}
//					printf("\n");
					event.events = EPOLLIN | EPOLLET;
					event.data.fd = newfd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &event) < 0) {
						fprintf(stderr, "add socket '%d' to epoll failed %s\n",
								newfd, strerror(errno));
						pthread_exit(0);
					}
					maxi++;

					/*
					 *将新连接终端信息录入结构体中
					 *这里可以将fd和IP信息存入
					 */
					info = (Pclient_info)malloc(sizeof(client_info));
					memset(info,0,sizeof(client_info));
					info->client_fd = newfd;
					info->cli_addr = cli_addr;
					info->clilen = clilen;

					list_add(list_head,info);

					pthread_mutex_unlock(&mutex);
				}

			} else {

				pthread_mutex_lock(&mutex);
				len = recv(wait_event[n].data.fd, buf, sizeof(buf), 0);
				pthread_mutex_unlock(&mutex);

				printf("%s  len = %d,%d\n",__func__,len,__LINE__);
				//客户端关闭连接
				if(len < 0 && errno != EAGAIN)
				{
//					printf("wait_event[n].data.fd--%d\n",wait_event[n].data.fd);


					ctrl_ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL,
							wait_event[n].data.fd,&event);
					if( ctrl_ret < 0)
					{
					 fprintf(stderr, "delete socket '%d' from epoll failed! %s\n",
							 wait_event[n].data.fd, strerror(errno));
					}
					maxi--;
					close(wait_event[n].data.fd);

					/*
					 * 删除链表中节点信息
					 * fd参数
					 */
					list_delete(list_head,wait_event[n].data.fd);

				}
				else if(len == 0)//客户端关闭连接
				{
					printf("client %d offline\n",wait_event[n].data.fd);


					list_delete(list_head,wait_event[n].data.fd);
					close(wait_event[n].data.fd);
					maxi--;
				}
				else
				{
					getpeername(wait_event[n].data.fd,(struct sockaddr*)&cli_addr,
							&clilen);
					printf("client %s:%d\n",inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));

					tc_process_rev_msg(&wait_event[n].data.fd,buf,&len);

					//将测试数据返回发送
					//send(wait_event[n].data.fd, buf, len, 0);
				}
			}//end if

		}//end for

	}//end while

	free(wait_event);
    close(sockfd);
    pthread_exit(0);


}

/*
 * 扫描信息主要是MAC地址，IP地址信息等
 */
static void scanf_client()
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;
	int i;
	char get_client_mac[] = "mac";


	tmp = list_head->next;

	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			pthread_mutex_lock(&mutex);
			write(pinfo->client_fd, get_client_mac, sizeof(get_client_mac));
			pthread_mutex_unlock(&mutex);
		}
		tmp = tmp->next;

	}

}

void delete_client(int num)
{

	list_delete(list_head,num);

}
void view_client_info()
{

	pclient_node tmp = NULL;

	Pclient_info pinfo;

	tmp = list_head->next;

	printf("print the info\n");

	while(tmp != NULL)
	{
		pinfo = tmp->data;

		printf("client_fd--%d ,client_id--%d ,"
		"client_mac--%s ,cli_addr--%s port--%d",pinfo->client_fd,pinfo->client_id,
		pinfo->client_mac,inet_ntoa(pinfo->cli_addr.sin_addr),ntohs(pinfo->cli_addr.sin_port));
		tmp = tmp->next;
		printf("\n");
	}

}

/*
 * 会议类数据
 * 分成控制类，查询类
 */
void edit_conference_info(Pframe_type type,unsigned char* buf)
{
	int i;

	/*
	 * 指针表
	 */
	int num = 0;

	memset(buf,0,sizeof(buf));

	/*
	 * 下发的控制消息
	 */
//	switch (type->msg_type)
//	{
//
//		case Ctrl_msg:
//		{
//			/*
//			 * 拷贝座位号信息
//			 * name-0x01
//			 * code-0x05
//			 * 进行移位操作，高位保存第字节
//			 */
//			if(type->con_data.id > 0)
//			{
//				unsigned char data = 0;
//				buf[num++]=WIFI_MEETING_CON_ID;
//				buf[num++]=WIFI_MEETING_INT;
//				data =(unsigned char) (( type->con_data.id >> 24 ) & 0xff);
//				buf[num++] = data;
//				data =(unsigned char) (( type->con_data.id >> 16 ) & 0xff);
//				buf[num++] = data;
//				data =(unsigned char) (( type->con_data.id >> 8 ) & 0xff);
//				buf[num++] = data;
//				data =(unsigned char) (( type->con_data.id >> 0 ) & 0xff);
//				buf[num++] = data;
//	//			for(i=0;i<num;i++)
//	//			{
//	//				printf("%x ",buf[i]);
//	//
//	//			}
//			}
//			/*
//			 * 席别信息
//			 * name-0x02
//			 * code-0x01
//			 */
//			if(type->con_data.seat > 0)
//			{
//				buf[num++]=WIFI_MEETING_CON_SEAT;
//				buf[num++]=WIFI_MEETING_CHAR;
//				buf[num++] = type->con_data.seat;
//			}
//			/*
//			 * 姓名信息
//			 * name-0x03
//			 * code-0x0a
//			 * 姓名编码+数据格式编码+内容长度+内容
//			 */
//			if(strlen(type->con_data.name) > 0)
//			{
//				buf[num++] = WIFI_MEETING_CON_NAME;
//				buf[num++] = WIFI_MEETING_STRING;
//				buf[num++] = strlen(type->con_data.name);
//				memcpy(&buf[num],type->con_data.name,strlen(type->con_data.name));
//				num = num+strlen(type->con_data.name);
//			}
//			/*
//			 * 议题信息
//			 * name-0x03
//			 * code-0x0a
//			 * 姓名编码+数据格式编码+内容长度+内容
//			 */
//			if(strlen(type->con_data.subj) > 0)
//			{
//				buf[num++] = WIFI_MEETING_CON_SUBJ;
//				buf[num++] = WIFI_MEETING_STRING;
//				buf[num++] = strlen(type->con_data.subj);
//				memcpy(&buf[num],type->con_data.subj,strlen(type->con_data.subj));
//				num = num+strlen(type->con_data.subj);
//
//			}
//			type->data_len = num;
//
//			break;
//		}
//		case Inquire_msg:
//		{
//			if(type->name_type > 0)
//			{
//				buf[num++] = type->name_type;
//
//				buf[num++] = WIFI_MEETING_STRING;
//				buf[num++] = strlen(type->con_data.subj);
//
//			}
//
//		}
//			break;
//	}
	if(type->msg_type == Write_msg)
	{
		/*
		 * 拷贝座位号信息
		 * name-0x01
		 * code-0x05
		 * 进行移位操作，高位保存第字节
		 */
		if(type->con_data.id > 0)
		{
			unsigned char data = 0;
			buf[num++]=WIFI_MEETING_CON_ID;
			buf[num++]=WIFI_MEETING_INT;
			data =(unsigned char) (( type->con_data.id >> 24 ) & 0xff);
			buf[num++] = data;
			data =(unsigned char) (( type->con_data.id >> 16 ) & 0xff);
			buf[num++] = data;
			data =(unsigned char) (( type->con_data.id >> 8 ) & 0xff);
			buf[num++] = data;
			data =(unsigned char) (( type->con_data.id >> 0 ) & 0xff);
			buf[num++] = data;
//			for(i=0;i<num;i++)
//			{
//				printf("%x ",buf[i]);
//
//			}
		}
		/*
		 * 席别信息
		 * name-0x02
		 * code-0x01
		 */
		if(type->con_data.seat > 0)
		{
			buf[num++]=WIFI_MEETING_CON_SEAT;
			buf[num++]=WIFI_MEETING_CHAR;
			buf[num++] = type->con_data.seat;
		}
		/*
		 * 姓名信息
		 * name-0x03
		 * code-0x0a
		 * 姓名编码+数据格式编码+内容长度+内容
		 */
		if(strlen(type->con_data.name) > 0)
		{
			buf[num++] = WIFI_MEETING_CON_NAME;
			buf[num++] = WIFI_MEETING_STRING;
			buf[num++] = strlen(type->con_data.name);
			memcpy(&buf[num],type->con_data.name,strlen(type->con_data.name));
			num = num+strlen(type->con_data.name);
		}
		/*
		 * 议题信息
		 * name-0x03
		 * code-0x0a
		 * 姓名编码+数据格式编码+内容长度+内容
		 */
		if(strlen(type->con_data.subj) > 0)
		{
			buf[num++] = WIFI_MEETING_CON_SUBJ;
			buf[num++] = WIFI_MEETING_STRING;
			buf[num++] = strlen(type->con_data.subj);
			memcpy(&buf[num],type->con_data.subj,strlen(type->con_data.subj));
			num = num+strlen(type->con_data.subj);

		}
		type->data_len = num;
	}
	/*
	 * 下发的查询信息
	 */
	else if(type->msg_type == Read_msg)
	{

		if(type->name_type > 0)
		{
			buf[num++] = type->name_type;
		}
		type->data_len = num;
	}
	/*
	 * 下发的请求信息
	 */
	else if(type->msg_type == Request_msg){

	}
	/*
	 * 下发的应答信息
	 */
	else if(type->msg_type == Reply_msg){

	}else{

		printf("%s,message type is not legal\n",__func__);
	}

	for(i=0;i<num;i++)
	{
		printf("%x ",buf[i]);

	}
	printf("\n");

}

/*
 * 会议类类信息(0x02)设置
 *
 * 此API是事件类数据统一接口
 *
 * 编辑终端属性，设置id，seats等
 *
 * 输入：
 * @fd--设置终端的fd
 * @id--分配的id
 * @seats--分配的席位,主席机，客席机，旁听机
 * @name--对应终端的姓名，单主机情况是不需要设置姓名
 * @msg_ty结构体--下发的消息类型(控制消息，请求消息，应答消息，查询消息)
 */
//
static void edit_client_info(Pframe_type type)
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;

//	type = malloc(sizeof(frame_type));

	char find_fd = -1;

	unsigned int id_num = 0;
	int c_seat = 0;
	char c_name[32] = {0};
	char c_subj[32] = {0};

	unsigned char buf[256] = {0};
	unsigned char s_buf[256] = {0};

	if(type->msg_type == Write_msg)
	{
		printf("please input the ID:\n");
		scanf("%d",&id_num);
		printf("get input ID:%x\n",id_num);

		printf("please input the seat:\n");
		scanf("%d",&c_seat);
		printf("get input seat:%x\n",c_seat);

		printf("please input the name:\n");
		scanf("%s",c_name);
		printf("get input name:%s\n",c_name);

		printf("please input the subject:\n");
		scanf("%s",c_subj);
		printf("get input subject:%s\n",c_subj);

		/*
		 * 确定此API的帧信息
		 * 会议类参数
		 */
		type->con_data.id = id_num;
		type->con_data.seat = c_seat;
		memcpy(&type->con_data.name,c_name,strlen(c_name));
		memcpy(&type->con_data.subj,c_subj,strlen(c_subj));
	}

	/**************************************
	 * 将数据信息进行组包
	 * 参照通讯协议中会议类参数信息进行编码
	 *
	 * 名称		编码格式	数据格式	值
	 * seat		0x01	char	1,2,3
	 * id		0x02	char 	1,2,3...
	 * name		0x03	char	"hushan"
	 ************************************/
	int i;
	/*
	 * 判断是否有此客户端
	 */
	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == type->fd)
		{
			find_fd = 1;
			break;
		}
		tmp = tmp->next;

	}
	if(find_fd < 0)
	{
		printf("please input the right fd..\n");
		return;
	}
	/*
	 * 会议类数据再次处理
	 * 根据type参数，确定数据的内容和格式
	 */
	switch(type->data_type)
	{
		case CONFERENCE_DATA:
			edit_conference_info(type,buf);
			break;

		case EVENT_DATA:

			break;

	}
	/*
	 * 发送消息组包
	 * 对数据内容进行封装，增数据头等信息
	 */
	tc_frame_compose(type,buf,s_buf);

	tmp = list_head->next;

	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == type->fd)
		{
			pthread_mutex_lock(&mutex);
			write(pinfo->client_fd, s_buf, type->frame_len);
			pthread_mutex_unlock(&mutex);
			break;
		}
		tmp = tmp->next;

	}

//	free(type);

}


static void* control_tcp_send(void* p)
{

	/*
	 * 设备连接后，发送一个获取mac的数据单包
	 * 控制类，事件型，主机发送数据
	 */
	char ask_buf[] = {0x26,0x09,0x01};
	char send_buf[1024] = {0};
	int send_len;

	unsigned int id_num = 0;
	int c_seat = 0;
	char c_name[32] = {0};
	char c_subj[32] = {0};
	int s,fd;

	printf("%s send_len %d \n",__func__,send_len);

	frame_type data_type;
	memset(&data_type,0,sizeof(data_type));

	while(1)
	{
		/*
		 * 模拟对单元机的控制
		 * ‘S’扫描单元机,0是获取mac
		 * ‘I’ID设置
		 * ‘X’席别
		 *
		 */
		scanf("%d",&s);
		if(s > 0)
		{
			scanf("%d",&fd);
			scanf("%d",&data_type.msg_type);
			//查询
			if(data_type.msg_type == 2)
			{
				scanf("%d",&data_type.data_type);
				scanf("%d",&data_type.name_type);
			}

		}
		data_type.fd = fd;
		data_type.dev_type = HOST_CTRL;

		printf("s=%d,fd=%d,msg=%d,dt=%d,na=%d\n",s,data_type.fd,data_type.msg_type,
				data_type.data_type,data_type.name_type);

		if(data_type.msg_type == Write_msg)
		{
			printf("please input the ID:\n");
			scanf("%d",&id_num);
			printf("get input ID:%x\n",id_num);

			printf("please input the seat:\n");
			scanf("%d",&c_seat);
			printf("get input seat:%x\n",c_seat);

			printf("please input the name:\n");
			scanf("%s",c_name);
			printf("get input name:%s\n",c_name);

			printf("please input the subject:\n");
			scanf("%s",c_subj);
			printf("get input subject:%s\n",c_subj);

			/*
			 * 确定此API的帧信息
			 * 会议类参数
			 */
			data_type.con_data.id = id_num;
			data_type.con_data.seat = c_seat;
			memcpy(&data_type.con_data.name,c_name,strlen(c_name));
			memcpy(&data_type.con_data.subj,c_subj,strlen(c_subj));
		}
		/*
		 * 主要分为事件类和会议类
		 * 再其次是写信息，读信息，查询，请求信息
		 * 用会议类数据举例
		 * 0x01--id
		 * 0x02--seat
		 * 0x03--name
		 * 0x04--subject
		 * 0x05--vote(暂不)
		 *
		 * 每个内容对应4个接口，但是这里先举例常用的两个
		 * 读和写
		 * 会议类数据建议是一次设置完或一次查询完
		 */
		switch(s)
		{
			case 1:
				scanf_client();
				break;
			case 2:
				edit_client_info(&data_type);
				break;
			default:
				break;
		}



	}


}

int control_tcp_module()
{

	void*status;
	pthread_mutex_init(&mutex, NULL);


	pthread_create(&s_id,NULL,control_tcp_recv,NULL);


	pthread_create(&s_id,NULL,control_tcp_send,NULL);

	pthread_join(s_id,&status);

	return 0;
}
int main(void)
{

	control_tcp_module();


    return 0;
}
