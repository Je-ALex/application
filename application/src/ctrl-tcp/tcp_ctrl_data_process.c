/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */


#include "../../inc/tcp_ctrl_data_process.h"
#include "../../inc/tcp_ctrl_device_status.h"
#include "../../inc/tcp_ctrl_queue.h"
#include "../../inc/tcp_ctrl_api.h"


/*
 * 定义两个结构体
 * 存取前状态，存储当前状态
 *
 * 创建查询线程，轮训比对，有差异，就返回新状态
 */
extern Pconference_status con_status;
extern pclient_node list_head;
extern pclient_node conference_head;
extern Pmodule_info node_queue;
/*
 * tcp_ctrl_source_dest_setting
 * tcp控制模块数据源地址和目标地址设置
 *
 * in：
 * @s_fd 源套接字号
 * @d_fd 目标套接字号
 * out：
 * @Pframe_type
 *
 * return：
 * @error
 * @success
 */
int tcp_ctrl_source_dest_setting(int s_fd,int d_fd,Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cinfo;
	Pconference_info info;

	int i,j;
	i = j = 0;

	type->d_id = type->s_id = 0;

	tmp = node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		info = tmp->data;
		if(info->fd == d_fd)
		{
			i++;

			type->d_id = info->con_data.id;
		}
		if(info->fd == s_fd)
		{
			j++;
			type->s_id = info->con_data.id;
		}
		if(i>0 && j>0)
			break;
		tmp = tmp->next;
	}
	tmp = node_queue->list_head->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == d_fd
				&& cinfo->client_name == PC_CTRL)
		{
			i++;
			type->d_id = 0xFFFF;
		}
		if(cinfo->client_fd == s_fd
				&& cinfo->client_name == PC_CTRL)
		{
			j++;
			type->s_id = 0xFFFF;
		}
		if(i>0 || j>0)
			break;
		tmp = tmp->next;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_data_char_to_int
 * tcp控制模块数据源地址和目标地址设置
 *
 * in：
 * @s_fd 源套接字号
 * @d_fd 目标套接字号
 * out：
 * @Pframe_type
 *
 * return：
 * @error
 * @success
 */
int tcp_ctrl_data_char_to_int(int* value,char* buf)
{

	int tmp = 0;
	tmp = buf[0] << 24;
	tmp = tmp + (buf[1] << 16);
	tmp = tmp + (buf[2] << 8);
	tmp = tmp + (buf[3] << 0);

	*value = tmp;

	return SUCCESS;
}



/*
 * tcp_ctrl_add_client
 * 终端信息录入函数
 *
 * 生成设备连接信息文本文件，采用结构体方式进行存储
 * 应用在设置参数时，需要读取文本文件中的fd信息，进行参数设置
 *
 */
int tcp_ctrl_add_client(void* value)
{
	pclient_node tmp = NULL;
	Pclient_info pinfo;

	FILE* file;
	int ret;

	char* connection_name = "connection.info";
	/*
	 * 连接信息存入链表
	 */
	list_add(node_queue->list_head,value);

	/*
	 * 存入到文本文件
	 * 更新文本信息
	 */
	file = fopen(connection_name,"w+");
	tmp = node_queue->list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			printf("fd:%d,ip:%s,id:%d\n",pinfo->client_fd,
					inet_ntoa(pinfo->cli_addr.sin_addr),pinfo->client_name);

			ret = fwrite(pinfo,sizeof(client_info),1,file);
			perror("fwrite");
			if(ret != 1)
				return ERROR;

		}
		tmp = tmp->next;
		usleep(10000);
	}
	fclose(file);

	return SUCCESS;
}


/*
 * tcp_ctrl_delete_client
 * 客户端 删除函数
 * 删除链表中的连接信息
 * 先通过查找链表，定位离线客户端的位置，在调用链表删除函数，删除链表中的设备
 *
 * 更新文本文件
 */
int tcp_ctrl_delete_client(int fd)
{

	pclient_node tmp = NULL;
	pclient_node tmp2 = NULL;
	pclient_node del = NULL;

	Pclient_info pinfo;
	Pconference_info cinfo;

	FILE* file;

	int ret;
	int status = 0;
	int pos = 0;

	/*
	 * 删除链接信息链表中的客户端
	 */
	tmp = node_queue->list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == fd)
		{
			/*
			 * 上位机下线
			 */
			if(pinfo->client_name == PC_CTRL)
			{
				node_queue->con_status->pc_status = ERROR;

			}
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}

	if(status > 0)
	{
		list_delete(node_queue->list_head,pos,&del);
		pinfo = del->data;
		printf("close socket and remove ctrl ip %s\n",
				inet_ntoa(pinfo->cli_addr.sin_addr));
		free(pinfo);
		free(del);
	}else{

		printf("there is no client in the list\n");
		return ERROR;
	}

	/*
	 * 删除链接信息文本中的信息
	 */
	file = fopen("connection.info","w+");
	tmp = node_queue->list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			printf("fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));

			ret = fwrite(pinfo,sizeof(client_info),1,file);
			perror("fwrite");
			if(ret != 1)
				return ERROR;

		}
		tmp = tmp->next;
		usleep(10000);
	}
	fclose(file);

	/*
	 * 下线消息告知主机或上位机
	 */
	//上报上位机部分
	frame_type tmp_type;
	memset(&tmp_type,0,sizeof(frame_type));
	if(node_queue->con_status->pc_status)
	{
		tmp_type.fd = node_queue->con_status->pc_status;
		tmp_type.msg_type = OFFLINE_REQ;
		tmp_type.data_type = EVENT_DATA;
		tmp_type.dev_type = HOST_CTRL;
		tmp_type.s_id = fd;
		tmp_type.d_id = node_queue->con_status->pc_status;
		tcp_ctrl_source_dest_setting(fd,tmp_type.fd,&tmp_type);
		tcp_ctrl_module_edit_info(&tmp_type,NULL);
	}

	//告知主机部分
	tmp_type.fd = fd;
	tcp_ctrl_msg_send_to(&tmp_type,NULL,WIFI_MEETING_EVENT_OFFLINE_REQ);

	/*
	 * 会议信息链表
	 */
	status = 0;
	pos = 0;
	/*
	 * 删除会议信息链表中的数据
	 */
	tmp2 = node_queue->conference_head->next;
	while(tmp2 != NULL)
	{
		cinfo = tmp2->data;
		if(cinfo->fd == fd)
		{
			status++;
			break;
		}
		pos++;
		tmp2 = tmp2->next;
	}
	if(status > 0)
	{
		list_delete(node_queue->conference_head,pos,&del);
		cinfo = del->data;
		printf("close socket %d \n",
				cinfo->fd);
		free(cinfo);
		free(del);
	}else{

		printf("there is no data in conference list\n");
		return ERROR;
	}



	return SUCCESS;
}


/*
 * tcp_ctrl_refresh_client_list
 * 控制模块客户端连接管理
 *
 * 通过msg_type来判断是否是宣告在线消息
 * 再通过fd判断连接存储情况，如果是新的客户端则存入
 */
int tcp_ctrl_refresh_client_list(const unsigned char* msg,Pframe_type type)
{

	/*
	 * 终端信息录入结构体
	 */
	pclient_node tmp = NULL;
	Pclient_info info;
	Pclient_info pinfo;
	Pconference_info new_data_info;

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	int state = 0;

	printf("%s\n",__func__);

	/*
	 * 宣告上线消息类型
	 */
	if(type->msg_type == ONLINE_REQ)
	{

		/*
		 * 将新连接终端信息录入结构体中
		 * 这里可以将fd和IP信息存入到本地链表和文件中
		 *
		 * 区分上线的设备类型，单元机还是主机
		 *
		 */
		info = (Pclient_info)malloc(sizeof(client_info));
		memset(info,0,sizeof(client_info));

		info->client_fd = type->fd;
		getpeername(info->client_fd,(struct sockaddr*)&cli_addr,&clilen);
		info->cli_addr = cli_addr;
		info->clilen = clilen;

		if(type->dev_type == PC_CTRL){
			info->client_name = PC_CTRL;
			node_queue->con_status->pc_status = type->fd;//PC_CTRL;

		}else{
			info->client_name = UNIT_CTRL;
		}
		/*
		 * 解析msg，共5个字节，席别和ID
		 * 在上线请求时，会携带单元机的ID信息，没有就默认为0
		 * 如果接收到的上线请求消息有id，则将该设备更新至会议信息链表中
		 *
		 */
		if(type->s_id > 0)
		{
			info->id = type->s_id;
			info->seat = msg[0];
		}else{

			//暂时不知道怎么处理
		}
		/*
		 * 检查该客户端是否已经存在
		 *
		 * 链表如果为空，则不需要进行检查，直接存储
		 * 链表不为空，则通过读取再比对，进行存储
		 *
		 */
		tmp = node_queue->list_head->next;
		do
		{
			if(tmp == NULL)
			{
				state++;
				tcp_ctrl_add_client(info);
				break;
			}else{
				pinfo = tmp->data;
				if(pinfo->client_fd == info->client_fd)
				{
					state++;
					printf("the client is exist\n");
					return ERROR;
				}
			}
			tmp = tmp->next;
		}while(tmp != NULL);

		if(state == 0)
		{
			tcp_ctrl_add_client(info);
		}
		/*
		 * 保存至会议信息链表
		 * 解析msg，共5个字节，席别和ID
		 * 在上线请求时，会携带单元机的ID信息，没有就默认为0
		 * 如果接收到的上线请求消息有id，则将该设备更新至会议信息链表中
		 *
		 */
		if(type->s_id > 0 && type->dev_type != PC_CTRL)
		{
			new_data_info = (Pconference_info)malloc(sizeof(conference_info));
			memset(new_data_info,0,sizeof(conference_info));
			new_data_info->fd = type->fd;
			new_data_info->con_data.id = type->s_id;
			new_data_info->con_data.seat = msg[0];
			tcp_ctrl_refresh_conference_list(new_data_info);
		}else{

			//暂时不知道怎么处理
		}

	/*
	 * 发送至本地上报队列
	 */
	tcp_ctrl_msg_send_to(type,msg,WIFI_MEETING_EVENT_ONLINE_REQ);
	}

	return SUCCESS;
}


/*
 * 数据包解析函数，将有效数据提出到handlbuf中
 */
int tcp_ctrl_frame_analysis(int* fd,unsigned char* buf,int* len,Pframe_type frame_type,
		unsigned char** ret_msg)
{

	int i;
	int tc_index = 0;
	unsigned short package_len = 0;
	unsigned char sum = 0;
	int length = *len;
	unsigned char* buffer = NULL;
#if TCP_DBG
	printf("length: %d\n",length);
	/*
	 * print the receive data
	 */
	printf("receive from：");
	for(i=0;i<length;i++)
	{
		printf("%x ",buf[i]);
	}
	printf("\n");
#endif

	printf("receive from：");
	for(i=0;i<length;i++)
	{
		printf("%x ",buf[i]);
	}
	printf("\n");
	/*
	 * 验证数据头是否正确buf[0]~buf[3]
	 */
	while (buf[tc_index++] != 'D' || buf[tc_index++] != 'S'
			|| buf[tc_index++] != 'D' || buf[tc_index++] != 'S') {

		printf( "%s not legal headers\n", __FUNCTION__);
		return ERROR;
	}
	/*
	 * buf[4]
	 * 判断数据类型
	 * 消息类型(0xf0)4.5-4.7：控制消息(0x01)，请求消息(0x02)，应答消息(0x03)
	 * 数据类型(0x0c)4.2-4.4：事件型数据(0x01)，会议型单元参数(0x02)，会议型会议参数(0x03)
	 * 设备类型数据(0x03)4.0-4.1：上位机发送(0x01)，主机发送(0x02)，单元机发送(0x03)
	 */
	frame_type->msg_type = (buf[tc_index] & MSG_TYPE) >> 4;
	frame_type->data_type = (buf[tc_index] & DATA_TYPE) >> 2;
	frame_type->dev_type = buf[tc_index] & MACHINE_TYPE;
	tc_index++;

	/*
	 * 计算帧总长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[tc_index++] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + buf[tc_index++] & 0xff;


	/* fixme
	 * 长度最小不小于16(帧信息字节)+1(data字节，最小为一个名称码) = 13
	 */
	if(length < 0x10 || (length != package_len))
	{
		printf( "%s not legal length\n", __FUNCTION__);

		return ERROR;
	}
	unsigned char id_msg[4] = {0};

	memcpy(id_msg,&buf[tc_index],sizeof(int));
	tcp_ctrl_data_char_to_int(&frame_type->s_id,id_msg);
	tc_index = tc_index+sizeof(int);

	memcpy(id_msg,&buf[tc_index],sizeof(int));
	tcp_ctrl_data_char_to_int(&frame_type->d_id,id_msg);
	tc_index = tc_index+sizeof(int);


	/*
	 * 校验和的验证
	 */
	for(i = 0;i<package_len-1 ;i++)
	{
		sum += buf[i];
	}
	if(sum != buf[package_len-1])
	{
		printf( "%s not legal checksum\n", __FUNCTION__);

		return ERROR;
	}

	frame_type->fd = *fd;

	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节源地址，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 15 - 1
	 */
	int data_len = package_len - 0x10;
	frame_type->data_len = data_len;

	/*
	 * 保存有效数据
	 * 将有数据另存为一个内存区域
	 */
	buffer = (unsigned char*)calloc(data_len,sizeof(char));

	memcpy(buffer,buf+tc_index,data_len);
#if TCP_DBG
	printf("buffer: ");
	for(i=0;i<data_len;i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");
#endif
	*ret_msg = buffer;

	return SUCCESS;
}


/*
 * tcp_ctrl_msg_send_to
 * 处理后的消息进行何种处理，是发送给上位机还是在主机进行显示
 * 通过判断是否有上位机进行分别处理，有上位机的情况，主机就不进行显示提示
 *
 */
int tcp_ctrl_msg_send_to(Pframe_type frame_type,const unsigned char* msg,int value)
{

	pclient_node tmp = NULL;
	Pclient_info tmp_type;
	/*
	 * 查询是否有上位机连接，若有，将投票信息上报给上位机
	 */
	if(node_queue->con_status->pc_status == PC_CTRL && msg != NULL)
	{
		tmp=node_queue->list_head->next;
		while(tmp != NULL)
		{
			tmp_type = tmp->data;

			if(tmp_type->client_name == PC_CTRL)
			{
				frame_type->fd = tmp_type->client_fd;
				tcp_ctrl_module_edit_info(frame_type,msg);

				break;
			}
			tmp = tmp->next;
		}
	}else{
		printf("there is no pc\n");

		/*
		 * 将信息发送至消息队列
		 */
		tcp_ctrl_report_enqueue(frame_type,value);

	}



	return SUCCESS;
}
/*
 * tcp_ctrl_uevent_request_spk
 * 发言请求函数
 *
 * 主机将请求消息发送给主席单元，主席单元显示几号单元机请求发言
 *
 * 主机界面显示请求发言，如果主席单元处理请求后，显示关闭
 *
 */
int tcp_ctrl_uevent_request_spk(Pframe_type frame_type,const unsigned char* msg)
{

	pclient_node tmp = NULL;
	Pconference_info tmp_type;

	int pos = 0;
	int tmp_fd = 0;

	tmp_fd = frame_type->fd;

	/*
	 * 赋值id和席别
	 */
	tmp=node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->fd == frame_type->d_id)
		{
			frame_type->con_data.id = frame_type->d_id;
			frame_type->con_data.seat=tmp_type->con_data.seat;
			break;
		}
		tmp=tmp->next;
	}

	switch(frame_type->evt_data.value)
	{
	/*
	 * 主机收到主席单元的允许指令，目标地址为具体允许的单元机ID，源地址为主席单元
	 * 主机下发允许指令，修改源地址为主机，目标地址不变
	 */
	case WIFI_MEETING_EVT_SPK_ALLOW:
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id == frame_type->d_id)
			{
				printf("allow the speaking\n");
				frame_type->fd=tmp_type->fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}
		tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
		if(pos>0){
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		/*
		 * 将事件信息发送至消息队列
		 * 告知应用哪个机器发言
		 */
//		frame_type->fd = tmp_fd;
		tcp_ctrl_report_enqueue(frame_type,WIFI_MEETING_EVENT_SPK_ALLOW);
		break;
	case WIFI_MEETING_EVT_SPK_VETO:
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id == frame_type->d_id)
			{
				printf("vote the speaking\n");
				frame_type->fd=tmp_type->fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}
		tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
		if(pos>0){
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		/*
		 * 将事件信息发送至消息队列
		 * 告知应用哪个机器
		 */
//		frame_type->fd = tmp_fd;
		tcp_ctrl_report_enqueue(frame_type,WIFI_MEETING_EVENT_SPK_VETO);
		break;
	case WIFI_MEETING_EVT_SPK_ALOW_SND:
	case WIFI_MEETING_EVT_SPK_VETO_SND:
	case WIFI_MEETING_EVT_SPK_REQ_SND:
		break;
	case WIFI_MEETING_EVT_SPK_REQ_SPK:
		/*
		 * 请求消息发给主席单元
		 */
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
			{
				printf("find the chariman\n");
				frame_type->fd=tmp_type->fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}

		tcp_ctrl_source_dest_setting(tmp_fd,frame_type->fd,frame_type);

		if(pos>0){
			tcp_ctrl_module_edit_info(frame_type,msg);
		}

		/*
		 * 请求消息发送给上位机和主机显示
		 */
		frame_type->fd = tmp_fd;
		tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_EVENT_SPK_REQ_SPK);

		break;
	default:
		printf("there is legal value\n");
		return ERROR;
	}


	return SUCCESS;

}


/*
 * tcp_ctrl_uevent_request_vote
 * 处理单元机投票实时信息
 * 开始投票，结束投票
 * 赞成，反对，弃权，超时
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_vote(Pframe_type frame_type,const unsigned char* msg)
{
	pclient_node tmp = NULL;
	Pconference_info tmp_type;
	int value = 0;
	int tmp_fd = 0;

	tmp_fd = frame_type->fd;

	/*
	 * 开始投票和结束投票的指令需要下发给所有单元
	 */
	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_VOT_START:
	{

	/*
	 * 请求消息发给所有单元
	 */
	tmp=node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->con_data.id)
		{
			frame_type->fd = tmp_type->fd;
			tcp_ctrl_source_dest_setting(-1,tmp_type->fd,frame_type);
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		tmp=tmp->next;
		usleep(10000);
	}


	/*
	 * 请求消息发送给上位机和主机显示
	 */
	frame_type->fd = tmp_fd;
	value = WIFI_MEETING_EVENT_VOTE_START;
		break;
	}
	case WIFI_MEETING_EVT_VOT_END:
	{
		/*
		 * 请求消息发给所有单元
		 */
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id)
			{
				tcp_ctrl_source_dest_setting(-1,tmp_type->fd,frame_type);
				tcp_ctrl_module_edit_info(frame_type,msg);
			}
			tmp=tmp->next;
			usleep(10000);
		}

		/*
		 * 请求消息发送给上位机和主机显示
		 */
		frame_type->fd = tmp_fd;
		value = WIFI_MEETING_EVENT_VOTE_END;
		break;
	}
	case WIFI_MEETING_EVT_VOT_ASSENT:
		node_queue->con_status->v_result.assent++;
		value = WIFI_MEETING_CONF_REQ_VOTE_ASSENT;
		break;
	case WIFI_MEETING_EVT_VOT_NAY:
		node_queue->con_status->v_result.nay++;
		value = WIFI_MEETING_CONF_REQ_VOTE_NAY;
		break;
	case WIFI_MEETING_EVT_VOT_WAIVER:
		node_queue->con_status->v_result.waiver++;
		value = WIFI_MEETING_CONF_REQ_VOTE_WAIVER;
		break;
	case WIFI_MEETING_EVT_VOT_TOUT:
		node_queue->con_status->v_result.timeout++;
		value = WIFI_MEETING_CONF_REQ_VOTE_TIMEOUT;
		break;
	default:
		printf("there is legal value\n");
		return ERROR;
	}

	tcp_ctrl_msg_send_to(frame_type,msg,value);

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_subject
 * 会议议题控制管理
 *
 * 会议中主席单元会选择进行第几个议题，主机接收到具体的议题号，下发给单元机和上报给上位机
 *
 */
int tcp_ctrl_uevent_request_subject(Pframe_type frame_type,const unsigned char* msg)
{
	pclient_node tmp = NULL;
	Pconference_info tmp_type;
	int value = 0;
	int state = 0;

	int tmp_fd = 0;
	tmp_fd = frame_type->fd;
	/*
	 * 请求消息发给主席单元
	 */
	tmp=node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->fd == frame_type->fd)
		{
			if(tmp_type->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
			{
				state++;
				printf("tcp_ctrl_uevent_request_subject find the chariman\n");
				break;
			}

		}
		tmp=tmp->next;
	}
	if(state > 0)
	{
		//偏移量为0x13
		value = frame_type->evt_data.value;
	}else{
		printf("the subject is not chariman send\n");
		return ERROR;
	}

	/*
	 * 请求消息发给所有单元
	 */
	tmp=node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->con_data.id)
		{
			frame_type->fd = tmp_type->fd;
			tcp_ctrl_source_dest_setting(-1,tmp_type->fd,frame_type);
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		tmp=tmp->next;
		usleep(10000);
	}

	/*
	 * 判断议题号
	 */
	switch(value)
	{
	case 1:
		value = WIFI_MEETING_EVENT_SUBJECT_ONE;
		break;
	case 2:
		value = WIFI_MEETING_EVENT_SUBJECT_TWO;
		break;
	case 3:
		value = WIFI_MEETING_EVENT_SUBJECT_THREE;
		break;
	case 4:
		value = WIFI_MEETING_EVENT_SUBJECT_FOUR;
		break;
	case 5:
		value = WIFI_MEETING_EVENT_SUBJECT_FIVE;
		break;
	case 6:
		value = WIFI_MEETING_EVENT_SUBJECT_SIX;
		break;
	case 7:
		value = WIFI_MEETING_EVENT_SUBJECT_SEVEN;
		break;
	case 8:
		value = WIFI_MEETING_EVENT_SUBJECT_EIGHT;
		break;
	case 9:
		value = WIFI_MEETING_EVENT_SUBJECT_NINE;
		break;
	case 0x0a:
		value = WIFI_MEETING_EVENT_SUBJECT_TEN;
		break;
	}
	/*
	 * 请求消息发送给上位机和主机显示
	 */
	frame_type->fd = tmp_fd;
	tcp_ctrl_msg_send_to(frame_type,msg,value);

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_service
 * 服务请求
 * 主机接收端服务请求，将请求在主机界面显示或上传上位机
 *
 */
int tcp_ctrl_uevent_request_service(Pframe_type frame_type,const unsigned char* msg)
{

	int value = 0;
	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_SER_WATER:
		value = WIFI_MEETING_EVENT_SERVICE_WATER;
		break;
	case WIFI_MEETING_EVT_SER_PEN:
		value = WIFI_MEETING_EVENT_SERVICE_PEN;
		break;
	case WIFI_MEETING_EVT_SER_PAPER:
		value = WIFI_MEETING_EVENT_SERVICE_PAPER;
		break;
	default:
		printf("there is legal value\n");
		return ERROR;

	}
	tcp_ctrl_msg_send_to(frame_type,msg,value);

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_checkin
 * 处理单元机签到功能
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_checkin(Pframe_type frame_type,const unsigned char* msg)
{
	pclient_node tmp = NULL;
	Pconference_info tmp_type;
	int value = 0;
	int tmp_fd = 0;

	tmp_fd = frame_type->fd;

	/*
	 * 开始签到和结束签到的指令需要下发给所有单元
	 */
	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_CHECKIN_START:
		/*
		 * 请求消息发给所有单元
		 */
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id)
			{
				frame_type->fd = tmp_type->fd;
				tcp_ctrl_source_dest_setting(-1,tmp_type->fd,frame_type);
				tcp_ctrl_module_edit_info(frame_type,msg);
			}
			tmp=tmp->next;
			usleep(10000);
		}

		/*
		 * 请求消息发送给上位机和主机显示
		 */
		frame_type->fd = tmp_fd;
		value = WIFI_MEETING_EVENT_CHECKIN_START;
		break;
	case WIFI_MEETING_EVT_CHECKIN_END:
		/*
		 * 请求消息发给所有单元
		 */
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id)
			{
				frame_type->fd = tmp_type->fd;
				tcp_ctrl_source_dest_setting(-1,tmp_type->fd,frame_type);
				tcp_ctrl_module_edit_info(frame_type,msg);
			}
			tmp=tmp->next;
			usleep(10000);
		}
		/*
		 * 请求消息发送给上位机和主机显示
		 */
		frame_type->fd = tmp_fd;
		value = WIFI_MEETING_EVENT_CHECKIN_END;
		break;
	case WIFI_MEETING_EVT_CHECKIN_SELECT:
		value = WIFI_MEETING_EVENT_CHECKIN_SELECT;
		break;
	}


	tcp_ctrl_msg_send_to(frame_type,msg,value);


	return SUCCESS;
}


/*
 * tcp_ctrl_unit_request_msg
 * 单元机请求消息处理函数
 * 请求消息再细分为事件类和会议类
 *
 * 处理接收到的消息，将消息内容进行解析，存取到全局结构体中@new_uint_data
 *
 * 事件类已增加发言，投票，签到，服务
 *
 * 会议类暂时没有
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 */
int tcp_ctrl_unit_request_msg(const unsigned char* msg,Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_info tmp_type;
	int tc_index = 0;
	int num = 0;
	int i;

	/*
	 * 解析msg，共5个字节，席别和ID
	 * 在上线请求时，会携带单元机的ID信息，没有就默认为0
	 * s_id源ID，d_id目标ID
	 */
	int s_id = 0;
	int d_id = 0;
	unsigned char id_msg[4] = {0};

	/*
	 * 赋值id和席别
	 */
	tmp=node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->fd == frame_type->fd)
		{
			frame_type->con_data.id = frame_type->s_id;
			frame_type->con_data.seat=tmp_type->con_data.seat;
			break;
		}
		tmp=tmp->next;
	}

	switch(frame_type->data_type)
	{

	case EVENT_DATA:
	{
		frame_type->name_type[0] = msg[0];
		frame_type->code_type[0] = msg[1];
		frame_type->evt_data.value = msg[2];

		switch(frame_type->name_type[0])
		{

			/*
			 * 发言请求，主机收到后需要显示，还要将此信息发送给主席单元
			 */
			case WIFI_MEETING_EVT_SPK:
				tcp_ctrl_uevent_request_spk(frame_type,msg);
				break;
			/*
			 * 投票结果请求上报
			 */
			case WIFI_MEETING_EVT_VOT:
				tcp_ctrl_uevent_request_vote(frame_type,msg);
				break;
			/*
			 * 议题管理
			 */
			case WIFI_MEETING_EVT_SUB:
				tcp_ctrl_uevent_request_subject(frame_type,msg);
				break;
			/*
			 * 服务请求
			 */
			case WIFI_MEETING_EVT_SER:
				tcp_ctrl_uevent_request_service(frame_type,msg);
				break;
			/*
			 * 签到管理
			 */
			case WIFI_MEETING_EVT_CHECKIN:
				tcp_ctrl_uevent_request_checkin(frame_type,msg);
				break;
			default:
				printf("there is legal value\n");
				return ERROR;

		}
	}
		break;

	case CONFERENCE_DATA:
		break;

	default:
		printf("there is legal value\n");
		return ERROR;
	}


	return 0;

}
/*
 * tcp_ctrl_unit_reply_conference
 * 单元机会议类应答消息处理函数
 * 消息分为控制类应答和查询类应答
 * 消息会首先在主机做出处理，如果有上位机，还需通过上位机进行处理
 *
 * 处理接收到的消息，将消息内容进行解析
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 */
int tcp_ctrl_unit_reply_conference(const unsigned char* msg,Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_info tmp_type;

	Pconference_info confer_info;
	int tc_index = 0;
	int i;
	int ret = 0;
	int str_len = 0;

//	printf("msg: ");
//	for(i=0;i<frame_type->data_len;i++)
//	{
//		printf("0x%02x ",msg[i]);
//	}
//	printf("\n");



	switch(frame_type->msg_type)
	{
	/*
	 * 控制类应答是设置失败，单元机返回的
	 * 所以，在接收到此消息后，主机要进行显示，还要将信息上报给上位机
	 */
	case W_REPLY_MSG:
	{
		/*
		 * 赋值id和席别
		 */
		tmp=node_queue->conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->fd == frame_type->fd)
			{
				frame_type->con_data.id = frame_type->s_id;
				frame_type->con_data.seat=tmp_type->con_data.seat;
				break;
			}
			tmp=tmp->next;
		}
		/*
		 * 会议类控制应答，单元机只返回失败情况0xe3
		 * 上报消息进入队列
		 */
		if(msg[tc_index++] == WIFI_MEETING_ERROR)
		{
			switch(msg[tc_index++])
			{
			case TCP_C_ERR_SUCCESS:
				tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_CONF_WREP_SUCCESS);
				break;
			case TCP_C_ERR_HEAD:
			case TCP_C_ERR_LENGTH:
			case TCP_C_ERR_CHECKSUM:
				tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_CONF_WREP_ERR);
				break;
			}
		}
	}
		break;
	case R_REPLY_MSG:
	{
		/*
		 * 会议查询类应答，单元机应答全状态
		 * 对于单主机情况，主机只关心ID 和席别
		 * 有上位机情况，将应答信息转发给上位机
		 */
		if(msg[tc_index] == WIFI_MEETING_CON_ID)
		{
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];

			frame_type->con_data.id = (msg[tc_index++] << 24);
			frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 16);
			frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 8);
			frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);


		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){
			 tc_index--;
			 frame_type->name_type[0] = msg[tc_index];
			 tc_index++;
			 frame_type->code_type[0] = msg[tc_index++];
			 frame_type->con_data.seat = msg[tc_index++];
			 printf(" frame_type->con_data.seat--%d", frame_type->con_data.seat);

		 }

		if(msg[tc_index++] == WIFI_MEETING_CON_NAME){
			 tc_index--;
			 frame_type->name_type[0] = msg[tc_index];
			 tc_index++;
			 frame_type->code_type[0] = msg[tc_index++];
			 str_len = msg[tc_index++];
			 memcpy(frame_type->con_data.name,&msg[tc_index],str_len);

			 tc_index = tc_index+msg[tc_index-1];



		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_SUBJ){
			 tc_index--;
			 frame_type->name_type[0] = msg[tc_index];
			 tc_index++;
			 frame_type->code_type[0] = msg[tc_index++];
			 str_len = msg[tc_index++];
			 memcpy(frame_type->con_data.subj[0],&msg[tc_index],str_len);
			 tc_index = tc_index+msg[tc_index-1];


		 }

		/*
		 * 更新conference_head链表的内容
		 *
		 */
		confer_info = (Pconference_info)malloc(sizeof(conference_info));
		memset(confer_info,0,sizeof(conference_info));
		confer_info->fd = frame_type->fd;
		confer_info->con_data.id =  frame_type->con_data.id;
		confer_info->con_data.seat =  frame_type->con_data.seat;
		memcpy(confer_info->con_data.name,frame_type->con_data.name,strlen(frame_type->con_data.name));
		memcpy(confer_info->con_data.subj[0],frame_type->con_data.subj[0],strlen(frame_type->con_data.subj[0]));

		/*
		 * 增加到会议信息链表中
		 */
		ret = tcp_ctrl_refresh_conference_list(confer_info);
		if(ret)
			return ERROR;
		/*
		 * 消息进行处理
		 * 如果有PC就直接将数据传递给PC
		 * 单主机的话，就需要上报给应用
		 */
		tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_CONF_RREP);
	}
		break;
	}



	return 0;

}



/*
 * tcp_ctrl_unit_reply_event
 * 事件类应答消息处理
 *
 * 事件类应答不用区分应答类型，单元机应答的内容为固定格式
 * 消息内容为name+code+ID
 *
 */
int tcp_ctrl_unit_reply_event(const unsigned char* msg,Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_info tmp_type;

	int value = 0;

	frame_type->name_type[0] = msg[0];
	frame_type->code_type[0] = msg[1];
	frame_type->evt_data.value = msg[2];

	/*
	 * 赋值id和席别
	 */
	frame_type->con_data.id = frame_type->s_id;
	tmp=node_queue->conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->con_data.id == frame_type->s_id)
		{
			frame_type->con_data.seat=tmp_type->con_data.seat;
			break;
		}
		tmp=tmp->next;
	}

	switch(frame_type->name_type[0])
	{
	/*
	 * 单元机电源
	 */
	case WIFI_MEETING_EVT_PWR:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_PWR_ON:
			value = WIFI_MEETING_EVENT_POWER_ON;
			break;
		case WIFI_MEETING_EVT_PWR_OFF:
			value = WIFI_MEETING_EVENT_POWER_OFF;
			break;
		}
		break;
	}
	case WIFI_MEETING_EVT_MIC:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_MIC_FIFO:
			value = WIFI_MEETING_EVENT_MIC_FIFO;
			break;
		case WIFI_MEETING_EVT_MIC_STAD:
			value = WIFI_MEETING_EVENT_MIC_STAD;
			break;
		case WIFI_MEETING_EVT_MIC_FREE:
			value = WIFI_MEETING_EVENT_MIC_FREE;
			break;
		}

		break;
	}

	case WIFI_MEETING_EVT_SPK:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_SPK_ALLOW:
			value = WIFI_MEETING_EVENT_SPK_ALLOW;
			break;
		case WIFI_MEETING_EVT_SPK_VETO:
			value = WIFI_MEETING_EVENT_SPK_VETO;
			break;
		case WIFI_MEETING_EVT_SPK_ALOW_SND:
			value = WIFI_MEETING_EVENT_SPK_ALOW_SND;
			break;
		case WIFI_MEETING_EVT_SPK_VETO_SND:
			value = WIFI_MEETING_EVENT_SPK_VETO_SND;
			break;
		case WIFI_MEETING_EVT_SPK_REQ_SPK:
			value = WIFI_MEETING_EVENT_SPK_REQ_SPK;
			break;
		}

		break;
	}

	}

	tcp_ctrl_msg_send_to(frame_type,msg,value);
	return SUCCESS;
}
/*
 * tcp_ctrl_from_unit
 * 设备控制模块单元机数据处理函数
 *
 * in：
 * @handlbuf解析后的数据内容
 * @Pframe_type帧信息内容
 *
 * 函数先进行消息类型判别，分为请求类和控制应答、查询应答类
 * 在应答类消息中有细分为事件型和会议型
 *
 */
int tcp_ctrl_from_unit( const unsigned char* handlbuf,Pframe_type frame_type)
{
	int i;
	int tc_index = 0;

#if TCP_DBG
	printf("handlbuf: ");
	for(i=0;i<frame_type->data_len;i++)
	{
		printf("0x%02x ",handlbuf[i]);
	}
	printf("\n");

#endif



	/*
	 * 消息类型分类
	 * 具体细分为控制类应答和查询类应答
	 */
	switch(frame_type->msg_type)
	{
		/*
		 * 请求类消息
		 */
		case REQUEST_MSG:
		{
			tcp_ctrl_unit_request_msg(handlbuf,frame_type);
			break;
		}
		/*
		 * 控制类应答
		 * 查询类应答
		 */
		case W_REPLY_MSG:
		case R_REPLY_MSG:
		{
			if(frame_type->data_type == CONFERENCE_DATA)
			{
				tcp_ctrl_unit_reply_conference(handlbuf,frame_type);
			}
			else if(frame_type->data_type == EVENT_DATA){
				tcp_ctrl_unit_reply_event(handlbuf,frame_type);
			}
			break;
		}
		/*
		 * 上线请求消息
		 */
		case ONLINE_REQ:
			tcp_ctrl_refresh_client_list(handlbuf,frame_type);
			break;
	}

	return 0;
}




int tcp_ctrl_pc_read_msg(const unsigned char* msg,Pframe_type frame_type)
{

	switch(frame_type->data_type)
	{
	case EVENT_DATA:
		frame_type->name_type[0] = msg[0];
		frame_type->code_type[0] = msg[1];
		frame_type->evt_data.value = msg[2];
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_ALL_STATUS:

			break;

		}
		break;
	case CONFERENCE_DATA:
		break;

	}


	return SUCCESS;
}

/*
 * tcp_ctrl_from_pc
 * 上位机消息数据
 * 1、上位机上线后先发送宣告在线消息，主机保存上位机连接信息至链表
 * 2、宣告上线后，上位机会发查询(单元机扫描功能)信息(全状态)，获取主机和单元机的状态@client_info
 *
 */
int tcp_ctrl_from_pc(const unsigned char* handlbuf,Pframe_type frame_type)
{
	int i;
	int tc_index = 0;

	printf("handlbuf: ");
	for(i=0;i<sizeof(handlbuf);i++)
	{
		printf("0x%02x ",handlbuf[i]);
	}
	printf("\n");


	switch(frame_type->msg_type)
	{
		case WRITE_MSG:
			break;
		case READ_MSG:
			tcp_ctrl_pc_read_msg(handlbuf,frame_type);
			break;
		case REQUEST_MSG:
			break;
		case ONLINE_REQ:
			tcp_ctrl_refresh_client_list(handlbuf,frame_type);
			break;

	}
	return 0;
}
