/*
 * tcp_ctrl_module_api.c
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */



#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_list.h"

extern Pmodule_info node_queue;
extern sys_info sys_in;

/*
 * tcp_ctrl_data_shift
 * 移位函数
 * 将高位移至低字节，低位移至高字节
 */
int tcp_ctrl_data_short_to_char(unsigned short value,unsigned char* r_value)
{

	r_value[0] =(unsigned char) (( value >> 8 ) & 0xff);

	r_value[1] =(unsigned char) ( value & 0xff);


	return SUCCESS;
}
/*
 * 主机发送数据组包函数
 *
 * in：@Pframe_type @params
 *
 * 帧数信息：Message_Type Data_Type Machine_Type
 * 数据内容：buf
 *
 * out：
 * 发送数据帧
 *
 * 返回值：
 * 成功，失败
 */
int tcp_ctrl_frame_compose(Pframe_type type,const unsigned char* params,unsigned char* result_buf)
{

	unsigned char msg,data,machine,info;
	int length = 0;
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

	int tc_index = 0;

	msg=data=machine=info=0;

	if(result_buf == NULL)
		return ERROR;

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
	msg = (msg << 4) & MSG_TYPE;

	data = type->data_type;
	data = (data << 2) & DATA_TYPE;

	machine = type->dev_type;
	machine = machine & MACHINE_TYPE;

	info = msg+data+machine;
#if TCP_DBG
	printf("msg = %x\n",msg);
	printf("data = %x\n",data);
	printf("machine = %x\n",machine);
	printf("info = %x\n",info);
#endif
	/*
	 * 存储第四字节的信息
	 */
	result_buf[tc_index++] = info;

	/*
	 * LENGTH
	 * 数据长度加上固定长度
	 * 固定长度为4个头字节+1个消息类+2个长度+4个源地址+4个目标地址+1个校验和，共16个固定字节
	 */
	length = type->data_len + 0x10;
	result_buf[tc_index++] = (length >> 8) & 0xff;
	result_buf[tc_index++] = length & 0xff;

	/*
	 * Destination address
	 * 分为源地址和目标地址，用ID号
	 */

	unsigned char id_msg[sizeof(short)] = {0};

	tcp_ctrl_data_short_to_char(type->s_id,id_msg);
	memcpy(&result_buf[tc_index],id_msg,sizeof(short));
	tc_index = tc_index+sizeof(short);

	tcp_ctrl_data_short_to_char(type->d_id,id_msg);
	memcpy(&result_buf[tc_index],id_msg,sizeof(short));
	tc_index = tc_index+sizeof(short);

	/*
	 * 保留四个字节
	 */
	tc_index = tc_index+sizeof(int);

	/*
	 * 具体数据内容
	 * 根据数据长度拷贝
	 */
	memcpy(&result_buf[tc_index],params,type->data_len);

	tc_index = tc_index + type->data_len;

	int i;
	unsigned char sum = 0;
	/*
	 * 计算校验和
	 */
	for(i=0;i<tc_index;i++)
	{
		sum += result_buf[i];
	}
	result_buf[tc_index] = (unsigned char) (sum & 0xff);

	type->frame_len = tc_index+1;


//	printf("tcp_ctrl_frame_compose result data: ");
//	for(i=0;i<tc_index+1;i++)
//	{
//		printf("%x ",result_buf[i]);
//	}
//	printf("\n");

	return SUCCESS;
}



/*
 * tcp_ctrl_edit_conference_info
 *
 * 会议类数据
 * 分成控制类，查询类
 *
 * in:@Pframe_type
 * out: buf
 */
static int tcp_ctrl_edit_conference_content(Pframe_type type,unsigned char* buf)
{
	int i;
	int tmp_index = 0;
	unsigned char data[sizeof(short)] = {0};

	printf("%s,%d\n",__func__,__LINE__);

	switch (type->msg_type)
	{

		case WRITE_MSG:
		{

			/*
			 * 会议类下发数据中将投票结果单独进行下发
			 */
			if(type->name_type[0] == WIFI_MEETING_CON_VOTE){

				buf[tmp_index++] = type->name_type[0];
				buf[tmp_index++] = type->code_type[0];

				//赞成
				buf[tmp_index++] = WIFI_MEETING_CON_V_ASSENT;
				tcp_ctrl_data_short_to_char(type->con_data.v_result.assent,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//反对
				buf[tmp_index++] = WIFI_MEETING_CON_V_NAY;
				tcp_ctrl_data_short_to_char(type->con_data.v_result.nay,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//弃权
				buf[tmp_index++] = WIFI_MEETING_CON_V_WAIVER;
				tcp_ctrl_data_short_to_char(type->con_data.v_result.waiver,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//超时
				buf[tmp_index++] = WIFI_MEETING_CON_V_TOUT;
				tcp_ctrl_data_short_to_char(type->con_data.v_result.timeout,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);

				type->data_len = tmp_index;

			}else{

				/*
				 * 下发的控制消息
				 *
				 * 拷贝座位号信息
				 * name-0x01
				 * code-0x05
				 * 进行移位操作，高位保存第字节
				 */
				if(type->con_data.id > 0)
				{
					unsigned char data[sizeof(short)] = {0};
					buf[tmp_index++]=WIFI_MEETING_CON_ID;
					buf[tmp_index++]=WIFI_MEETING_USHORT;
					tcp_ctrl_data_short_to_char(type->con_data.id,data);
					memcpy(&buf[tmp_index],data,sizeof(short));
					tmp_index = tmp_index + sizeof(short);
//					for(i=0;i<tmp_index;i++)
//					{
//						printf("%x ",buf[i]);
//
//					}
				}
				/*
				 * 席别信息
				 * name-0x02
				 * code-0x01
				 */
				if(type->con_data.seat > 0)
				{
					buf[tmp_index++]=WIFI_MEETING_CON_SEAT;
					buf[tmp_index++]=WIFI_MEETING_CHAR;
					buf[tmp_index++] = type->con_data.seat;
				}
				/*
				 * 姓名信息
				 * name-0x03
				 * code-0x0a
				 * 姓名编码+数据格式编码+内容长度+内容
				 */
				if(strlen(type->con_data.name) > 0)
				{
					buf[tmp_index++] = WIFI_MEETING_CON_NAME;
					buf[tmp_index++] = WIFI_MEETING_STRING;
					buf[tmp_index++] = strlen(type->con_data.name);
					memcpy(&buf[tmp_index],type->con_data.name,strlen(type->con_data.name));
					tmp_index = tmp_index+strlen(type->con_data.name);
				}
				/*
				 * 会议名称
				 * name-0x04
				 * code-0x0a
				 * 名称编码+数据格式编码+内容长度+内容
				 */
				if(strlen(type->con_data.conf_name) > 0)
				{
					buf[tmp_index++] = WIFI_MEETING_CON_CNAME;
					buf[tmp_index++] = WIFI_MEETING_STRING;
					buf[tmp_index++] = strlen(type->con_data.conf_name);
					memcpy(&buf[tmp_index],type->con_data.conf_name,strlen(type->con_data.conf_name));
					tmp_index = tmp_index+strlen(type->con_data.conf_name);
				}
				/*
				 * 议题信息
				 * name-0x03
				 * code-0x0a
				 * 姓名编码+数据格式编码+内容长度+内容
				 */
				if(strlen(type->con_data.subj[0]) > 0)
				{
					buf[tmp_index++] = WIFI_MEETING_CON_SUBJ;
					buf[tmp_index++] = WIFI_MEETING_STRING;
					buf[tmp_index++] = strlen(type->con_data.subj[0]);
					memcpy(&buf[tmp_index],type->con_data.subj[0],strlen(type->con_data.subj[0]));
					tmp_index = tmp_index+strlen(type->con_data.subj[0]);

				}
				type->data_len = tmp_index;

			}
			break;
		}
		/*
		 * 会议类查询指令
		 *
		 * 会议类的查询是全状态查询
		 */
		case READ_MSG:
		{

			type->data_len = tmp_index;
		}
			break;
		default:
			printf("%s,message type is not legal\n",__func__);
			return ERROR;
	}
#if TCP_DBG
	printf("tcp_ctrl_edit_conference_content buf: ");
	for(i=0;i<num;i++)
	{
		printf("%x ",buf[i]);

	}
	printf("\n");
#endif
	return SUCCESS;
}

/*
 * tcp_ctrl_edit_event_content
 * 事件类数据
 *
 * 在此之前的API函数已经将信息进行细分，这里不用细分type
 * 只需将结构体中的值取出即可
 *
 * in:@Pframe_type
 * out: buf
 */
void tcp_ctrl_edit_event_content(Pframe_type type,unsigned char* buf)
{
	int i;
	int tc_index = 0;
	unsigned char data[4] = {0};

	printf("%s,%d\n",__func__,__LINE__);
	if(type->name_type[0] == WIFI_MEETING_EVT_SSID
			&& type->name_type[1] == WIFI_MEETING_EVT_KEY )
	{
		buf[tc_index++] = type->name_type[0];
		buf[tc_index++] = type->code_type[0];
		buf[tc_index++] = strlen(type->evt_data.ssid);
		memcpy(&buf[tc_index],type->evt_data.ssid,strlen(type->evt_data.ssid));
		tc_index = tc_index+strlen(type->evt_data.ssid);

		buf[tc_index++] = type->name_type[1];
		buf[tc_index++] = type->code_type[1];
		buf[tc_index++] = strlen(type->evt_data.password);
		memcpy(&buf[tc_index],type->evt_data.password,strlen(type->evt_data.password));
		tc_index = tc_index+strlen(type->evt_data.ssid);

		tc_index = tc_index+strlen(type->evt_data.password);

	}else{

		buf[tc_index++] = type->name_type[0];
		buf[tc_index++] = type->code_type[0];
		buf[tc_index++] = type->evt_data.value;

	}

	type->data_len = tc_index;

	perror("tcp_ctrl_edit_event_content");
	for(i=0;i<tc_index;i++)
	{
		printf("%x ",buf[i]);

	}
	printf("\n");

}
/*
 * tcp_ctrl_module_edit_info
 * 下发数据编码
 *
 * 事件型信息(0x01)的设置
 * 会议型信息(0x02)的设置
 *
 *
 * 编辑终端属性，设置id，seats等
 *
 * 输入：
 * @type结构体
 * --下发的消息类型(控制消息，请求消息，应答消息，查询消息)
 * --下发的数据内容(事件型和会议型)
 *
 * 参照通讯协议中会议类参数信息进行编码
 *
 * 事件型数据内容
 * 名称		编码格式	数据格式	值
 * power	0x01	char	开(0x01)关(0x00)
 * mic		0x02	char 	fifo(0x01)标准(0x02)自由(0x03)
 * speak	0x03	char
 * ...
 *
 * 会议型数据内容
 * 名称		编码格式	数据格式	值
 * seat		0x01	char	1,2,3
 * id		0x02	char 	1,2,3...
 * name		0x03	char	"hushan"
 * ...
 *
 */
int tcp_ctrl_module_edit_info(Pframe_type type,const unsigned char* msg)
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;

	char find_fd = -1;
	unsigned char buf[512] = {0};
	unsigned char s_buf[1024] = {0};
	int i;
	int ret = 0;

	/*
	 * 在连接信息链表中
	 * 判断是否有此客户端
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
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
		return ERROR;
	}
#if 0
	/*
	 * 发送的数据如果是会议投票结果则单独处理
	 */
	if(type->data_type == CONFERENCE_DATA &&
			type->name_type[0] == WIFI_MEETING_CON_VOTE)
	{

		ret = tcp_ctrl_edit_conference_content(type,buf);

		if(ret)
			return ERROR;

		tmp = list_head->next;
		while(tmp != NULL)
		{
			pinfo = tmp->data;
			type->fd = pinfo->client_fd;
			tcp_ctrl_frame_compose(type,buf,s_buf);

			/*
			 * 投票结果下发给所有单元机
			 */
			if(pinfo->client_fd > 0 && pinfo->client_name != PC_CTRL)
			{
				pthread_mutex_lock(&mutex);
				write(pinfo->client_fd, s_buf, type->frame_len);
				pthread_mutex_unlock(&mutex);
				usleep(10000);
			}
			tmp = tmp->next;

		}

	}else{
#endif
		if(msg == NULL)
		{
				/*
				 *
				 * 根据type参数，确定数据的内容和格式
				 *
				 * 判断是会议类还是事件类
				 */
				switch(type->data_type)
				{
					case CONFERENCE_DATA:
						tcp_ctrl_edit_conference_content(type,buf);
						break;

					case EVENT_DATA:
						tcp_ctrl_edit_event_content(type,buf);
						break;

				}
				/*
				 * 发送消息组包
				 * 对数据内容进行封装，增数据头等信息
				 */

				tcp_ctrl_frame_compose(type,buf,s_buf);
		}else{
			/*
			 * 接收到数据透传
			 */
			tcp_ctrl_frame_compose(type,msg,s_buf);
		}
		pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
		tcp_ctrl_tpsend_enqueue(type,s_buf);
		pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);

#if 0
	}
#endif
	return SUCCESS;
}











