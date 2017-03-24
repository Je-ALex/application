/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2017年3月16日
 *      Author: leon
 */


#include "wifi_sys_list.h"
#include "wifi_sys_queue.h"

#include "tcp_ctrl_server.h"
#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"

#include "client_heart_manage.h"
#include "client_connect_manage.h"

#include "tcp_ctrl_api.h"



extern sys_info sys_in;
extern Pglobal_info node_queue;

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
int tcp_ctrl_data_char2short(unsigned short* value,unsigned char* buf)
{

	unsigned short tmp = 0;
	tmp = (buf[0] & 0xff) << 8 ;
	tmp = tmp + (buf[1] & 0xff);

	*value = tmp;

	return SUCCESS;
}

/*
 * tcp_ctrl_msg_send_to
 * 处理后的消息进行何种处理，是发送给上位机还是在主机进行显示
 * 通过判断是否有上位机进行分别处理，有上位机的情况，主机就不进行显示提示
 *
 */
int tcp_ctrl_msg_send_to(Pframe_type type,const unsigned char* msg,int value)
{
	pclient_node tmp = NULL;
	Pclient_info pinfo = NULL;

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	int status = 0;

	int tmp_fd = type->fd;

	//本地状态上报qt
	tcp_ctrl_report_enqueue(type,value);

	/*
	 * 查询是否有上位机连接，若有，将信息上报给上位机
	 * 单元机上线后，需要上报上位机，上报内容为fd和ip地址
	 * msg重新组包
	 */
	if(type->dev_type == UNIT_CTRL && conf_status_get_pc_staus() > 0)
	{

		tmp=node_queue->sys_list[CONNECT_LIST]->next;;
		while(tmp != NULL)
		{
			pinfo = tmp->data;

			if(pinfo->client_fd == type->fd)
			{
				tmp_fd = pinfo->mac_addr;
				type->s_id = pinfo->id;
				printf("%s-%s-%d,%d find it\n",__FILE__,__func__,__LINE__
						,tmp_fd);
				break;
			}
			tmp = tmp->next;
		}

		/*
		 * 单元机上线和下线再有上位机的情况下
		 * 需要上报fd、id、ip、席别、电量
		 */
		if(type->msg_type == ONLINE_REQ ||
				type->msg_type == OFFLINE_REQ)
		{
			getpeername(type->fd,(struct sockaddr*)&cli_addr,
					(socklen_t*)&clilen);

			type->name_type[0] = WIFI_MEETING_EVT_UNIT_ONOFF_LINE;
			type->code_type[0] = WIFI_MEETING_STRING;

			type->evt_data.unet_info.sockfd = tmp_fd;
			type->evt_data.unet_info.ip = cli_addr.sin_addr.s_addr;

			status++;
		}

		tmp=node_queue->sys_list[CONNECT_LIST]->next;;
		while(tmp != NULL)
		{
			pinfo = tmp->data;

			if((pinfo->client_name == PC_CTRL) &&
					(conf_status_get_pc_staus() == pinfo->client_fd))
			{
				type->evt_data.status = WIFI_MEETING_HOST_REP_TO_PC;
				//上报上位机的源地址统一为sockfd
				type->d_id = PC_ID;
				type->fd = pinfo->client_fd;
				if(status > 0)
				{
					tcp_ctrl_module_edit_info(type,NULL);
				}else{
					type->s_id = tmp_fd;
					tcp_ctrl_module_edit_info(type,msg);
				}
				break;
			}
			tmp = tmp->next;
		}
	}


	return SUCCESS;
}



/*
 * tcp_ctrl_frame_analysis
 * 数据包解析函数，将数据信息保存到帧信息结构体
 *
 * 详细解析数据包的数组组成，将信息分类存储在结构体中，消息内容则传递给指针
 *
 * 输入
 * fd套接字号 buf原始数据包 len数据包长度 frame_type存储结构体 ret_msg指针
 *
 * 返回值
 * 成功
 * 失败-错误码
 *
 */
static int tcp_ctrl_frame_analysis(int* fd,unsigned char* buf,int* len,Pframe_type frame_type,
		unsigned char** ret_msg)
{

	int i;
	int tc_index = 0;
	unsigned short package_len = 0;
	unsigned char sum = 0;
	int length = *len;
	unsigned char* buffer = NULL;

//	if(sys_debug_get_switch())
//	{
		if(buf[4] != 0x87)
		{
			printf("receive from %d:",*fd);
			for(i=0;i<length;i++)
			{
				printf("%x ",buf[i]);
			}
			printf("\n");
		}
//	}

	/*
	 * 验证数据头是否正确buf[0]~buf[3]
	 */
	if (buf[tc_index++] != 'D' || buf[tc_index++] != 'S'
			|| buf[tc_index++] != 'D' || buf[tc_index++] != 'S') {

		printf("%s-%s-%d not legal headers\n",__FILE__,__func__,__LINE__);
		return BUF_HEAD_ERR;
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
	package_len = package_len + (buf[tc_index++] & 0xff);


	/* fixme
	 * 长度最小不小于16(帧信息字节)+1(data字节，最小为一个名称码) = 13
	 */
	if(length < PKG_LEN || (length != package_len))
	{
		printf("%s-%s-%d not legal length\n",__FILE__,__func__,__LINE__);
		return BUF_LEN_ERR;
	}
	unsigned char id_msg[2] = {0};

//	memcpy(id_msg,&buf[tc_index],sizeof(short));
	tcp_ctrl_data_char2short(&frame_type->s_id,&buf[tc_index]);
	tc_index = tc_index+sizeof(short);

//	memcpy(id_msg,&buf[tc_index],sizeof(short));
	tcp_ctrl_data_char2short(&frame_type->d_id,&buf[tc_index]);
	tc_index = tc_index+sizeof(short);

	/*
	 * fixme 保留四个字节
	 */
	tc_index = tc_index+sizeof(int);

	/*
	 * 校验和的验证
	 */
	for(i=0;i<package_len-1 ;i++)
	{
		sum += buf[i];
	}
	if(sum != buf[package_len-1])
	{
		printf("%s-%s-%d not legal checksum\n",__FILE__,__func__,__LINE__);
		return BUF_CHECKS_ERR;
	}

	frame_type->fd = *fd;

	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节源地址，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 15 - 1
	 */
	int data_len = package_len - PKG_LEN;
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
 * tcp_ctrl_process_recv_msg
 * 接收消息的初步处理
 * 解析原始数据包后，分别进行数据处理
 * 分类：
 * 上位机通讯部分
 * 单元机通讯部分
 *
 * 输入
 * @cli_fd套接字号, buf数据包,length包长度
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int tcp_ctrl_process_recv_msg(int* cli_fd, unsigned char* buf, int* length)
{

	/*
	 * 帧信息内容结构
	 */
	Pframe_type tmp_type;

	int ret = 0;
	int status = 0;
	unsigned char* handlbuf = NULL;

	tmp_type = (Pframe_type)malloc(sizeof(frame_type));
	memset(tmp_type,0,sizeof(frame_type));

	ret = tcp_ctrl_frame_analysis(cli_fd,buf,length,tmp_type,&handlbuf);

#if 0
	printf("type->msg_type = %d\n"
			"type->data_type = %d\n"
			"type->dev_type = %d\n"
			"type->s_id = %d\n"
			"type->d_id = %d\n",
			tmp_type->msg_type,tmp_type->data_type,tmp_type->dev_type,
			tmp_type->s_id,tmp_type->d_id);
#endif

	if(ret != SUCCESS)
	{
		printf("%s-%s-%d tcp_ctrl_frame_analysis failed\n",__FILE__,__func__,__LINE__);

		free(tmp_type);
		free(handlbuf);

		return ret;
	}

	/* 不是上线消息类型，需要判断合法性
	 * 在连接信息链表中检查端口合法性
	 */
	if(tmp_type->msg_type != ONLINE_REQ)
	{
		status = conf_status_check_client_connect_legal(tmp_type);

		if(status < 1)
		{
			printf("%s-%s-%d client is not legal connect\n",__FILE__,__func__,__LINE__);

			free(tmp_type);
			free(handlbuf);

			return CONECT_NOT_LEGAL;
		}
	}

	/*
	 * 设备类型分类
	 */
	switch(tmp_type->dev_type)
	{
		case PC_CTRL:
			tcp_ctrl_from_pc(handlbuf,tmp_type);
			break;

		case UNIT_CTRL:
			tcp_ctrl_from_unit(handlbuf,tmp_type);
			break;

	}

	free(tmp_type);
	free(handlbuf);

	return SUCCESS;
}

/*
 * wifi_sys_ctrl_tcp_procs_data
 * TCP接收数据处理线程
 *
 * 将数据消息从队列中取出，进行数据解析处理
 *
 */
void* wifi_sys_ctrl_tcp_procs_data(void* p)
{
	Plinknode node;
	Pctrl_tcp_rsqueue tmp;
	int ret;
	pthread_detach(pthread_self());

	while(1)
	{
		sem_wait(&sys_in.sys_sem[CTRL_TCP_RECV_SEM]);

		pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_RQUEUE_MUTEX]);
		ret = out_queue(node_queue->sys_queue[CTRL_TCP_RECV_QUEUE],&node);

		if(ret == 0)
		{

			tmp = node->data;
			ret = tcp_ctrl_process_recv_msg(&tmp->socket_fd,tmp->msg,&tmp->len);

			free(tmp->msg);
			free(tmp);
			free(node);
		}else{
			printf("%s-%s-%d dequeue error\n",__FILE__,__func__,__LINE__);
		}
		pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_RQUEUE_MUTEX]);
	}

}
