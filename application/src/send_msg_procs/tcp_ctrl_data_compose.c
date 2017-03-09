/*
 * tcp_ctrl_module_api.c
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */



#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_data_process.h"



extern Pglobal_info node_queue;
extern sys_info sys_in;

/*
 * tcp_ctrl_data_shift
 * 移位函数
 * 将高位移至低字节，低位移至高字节
 */
int tcp_ctrl_data_short2char(unsigned short value,unsigned char* r_value)
{

	r_value[0] =(unsigned char) (( value >> 8 ) & 0xff);
	r_value[1] =(unsigned char) ( value & 0xff);


	return SUCCESS;
}
/*
 * tcp_ctrl_data_int2char
 * 移位函数
 * 将高位移至低字节，低位移至高字节
 */
int tcp_ctrl_data_int2char(unsigned int value,unsigned char* r_value)
{

	r_value[0] =(unsigned char) (( value >> 24 ) & 0xff);
	r_value[1] =(unsigned char) ( value >> 16 & 0xff);
	r_value[2] =(unsigned char) ( value >> 8 & 0xff);
	r_value[3] =(unsigned char) ( value & 0xff);

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

	tcp_ctrl_data_short2char(type->s_id,id_msg);
	memcpy(&result_buf[tc_index],id_msg,sizeof(short));
	tc_index = tc_index+sizeof(short);

	tcp_ctrl_data_short2char(type->d_id,id_msg);
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
 * tcp_ctrl_edit_conference_content
 * 会议型数据内容组包
 * 分成控制类，查询类
 *
 *
 *
 * @Pframe_type帧信息
 * @buf输出数据包
 *
 * 控制类消息中，分为普通的id席别等信息的设置，议题单独进行下发，下发会议结果
 *
 * 议题：
 * name | code | 议题号  | 类型   | 人数   # 姓名长度  | 人员序号   |  姓名     # 议题长度   | 议题内容
 * 0x05	  0x0a   char  char	 char  char    char    string  char   string
 *
 * 投票结果：
 * name | code | 分类   | 数量  |
 * 0x06	  0x0a  char  ushort
 *
 * 选举结果：
 * name | code | 人员编号   | 数量  |
 * 0x07	  0x0a    char  ushort
 *
 * 计分结果：
 * name | code  | 分数    |
 * 0x08	  0x0a   char
 *
 * return：
 * @ERROR
 * @SUCCESS
 *
 */
static int tcp_ctrl_edit_conference_content(Pframe_type type,unsigned char* buf)
{
	int i;
	int tmp_index = 0;
	unsigned char data[sizeof(short)] = {0};

//	printf("%s,%d\n",__func__,__LINE__);

	switch (type->msg_type)
	{

		case WRITE_MSG:
		{

			switch(type->name_type[0])
			{

			//议题单独进行下发
			case WIFI_MEETING_CON_SUBJ:
				/*
				 * 议题信息
				 * name-0x03
				 * code-0x0a
				 * 姓名编码+数据格式编码+内容长度+内容
				 */
//				if(strlen(type->con_data.subj[0]) > 0)
//				{
//					buf[tmp_index++] = WIFI_MEETING_CON_SUBJ;
//					buf[tmp_index++] = WIFI_MEETING_STRING;
//					buf[tmp_index++] = strlen(type->con_data.subj[0]);
//					memcpy(&buf[tmp_index],type->con_data.subj[0],strlen(type->con_data.subj[0]));
//					tmp_index = tmp_index+strlen(type->con_data.subj[0]);
//
//				}

				if(strlen(type->con_data.subj[0]) > 0)
				{
					buf[tmp_index++] = WIFI_MEETING_CON_SUBJ;
					buf[tmp_index++] = WIFI_MEETING_STRING;
					memcpy(&buf[tmp_index],type->con_data.subj[0],strlen(type->con_data.subj[0]));
					tmp_index = tmp_index+strlen(type->con_data.subj[0]);
				}
				type->data_len = tmp_index;
				break;

			case WIFI_MEETING_CON_VOTE:
				buf[tmp_index++] = type->name_type[0];
				buf[tmp_index++] = type->code_type[0];

				//赞成
				buf[tmp_index++] = WIFI_MEETING_CON_V_ASSENT;
				tcp_ctrl_data_short2char(type->con_data.v_result.assent,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//反对
				buf[tmp_index++] = WIFI_MEETING_CON_V_NAY;
				tcp_ctrl_data_short2char(type->con_data.v_result.nay,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//弃权
				buf[tmp_index++] = WIFI_MEETING_CON_V_WAIVER;
				tcp_ctrl_data_short2char(type->con_data.v_result.waiver,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//超时
				buf[tmp_index++] = WIFI_MEETING_CON_V_TOUT;
				tcp_ctrl_data_short2char(type->con_data.v_result.timeout,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);

				type->data_len = tmp_index;
				break;
			case WIFI_MEETING_CON_ELEC:
				buf[tmp_index++] = type->name_type[0];
				buf[tmp_index++] = type->code_type[0];

				for(i=1;i<=conf_status_get_elec_totalp(0);i++)
				{
					buf[tmp_index++] = i;
					tcp_ctrl_data_short2char(type->con_data.elec_rsult.ele_id[i],data);
					memcpy(&buf[tmp_index],data,sizeof(short));
					tmp_index = tmp_index + sizeof(short);
				}

				type->data_len = tmp_index;
				break;
			case WIFI_MEETING_CON_SCORE:
				buf[tmp_index++] = type->name_type[0];
				buf[tmp_index++] = type->code_type[0];

				buf[tmp_index++] = type->con_data.src_result.score_r;
				type->data_len = tmp_index;
				break;

			default:
				/*
				 * 下发的控制消息
				 *
				 * 拷贝座位号信息
				 * name-0x01
				 * code-0x04
				 * 进行移位操作，高位保存第字节
				 */
				if(type->con_data.id > 0)
				{
					unsigned char data[sizeof(short)] = {0};
					buf[tmp_index++]=WIFI_MEETING_CON_ID;
					buf[tmp_index++]=WIFI_MEETING_USHORT;
					tcp_ctrl_data_short2char(type->con_data.id,data);
					memcpy(&buf[tmp_index],data,sizeof(short));
					tmp_index = tmp_index + sizeof(short);

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
				 * 议题数量
				 * name-0x05
				 * code-0x01
				 */
				if(type->con_data.seat > 0)
				{
					buf[tmp_index++]=WIFI_MEETING_CON_SUBJ;
					buf[tmp_index++]=WIFI_MEETING_CHAR;
					buf[tmp_index++] = type->con_data.sub_num;
				}
				type->data_len = tmp_index;

				break;

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
	for(i=0;i<tmp_index;i++)
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
int tcp_ctrl_edit_event_content(Pframe_type type,unsigned char* buf)
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;

	int tmp_index = 0;
	unsigned char data[2] = {0};
	unsigned char value[4] = {0};

//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	if(type->code_type[0] < WIFI_MEETING_ERROR)
	{
		switch(type->name_type[0])
		{
		case WIFI_MEETING_EVT_SSID:
		{
			if(type->name_type[1] == WIFI_MEETING_EVT_KEY)
			{
				buf[tmp_index++] = type->name_type[0];
				buf[tmp_index++] = type->code_type[0];
				buf[tmp_index++] = strlen(type->evt_data.unet_info.ssid);
				memcpy(&buf[tmp_index],type->evt_data.unet_info.ssid,strlen(type->evt_data.unet_info.ssid));

				tmp_index = tmp_index+strlen(type->evt_data.unet_info.ssid);

				buf[tmp_index++] = type->name_type[1];
				buf[tmp_index++] = type->code_type[1];
				buf[tmp_index++] = strlen(type->evt_data.unet_info.key);
				memcpy(&buf[tmp_index],type->evt_data.unet_info.key,strlen(type->evt_data.unet_info.key));

				tmp_index = tmp_index+strlen(type->evt_data.unet_info.key);
			}
			break;
		}
		case WIFI_MEETING_EVT_RP_TO_PC:
		{
			buf[tmp_index++] = type->name_type[0];
			buf[tmp_index++] = type->code_type[0];
			buf[tmp_index++] = node_queue->sys_list[CONNECT_LIST]->size-1;

			if(node_queue->sys_list[CONNECT_LIST]->size-1 == 0)
			{
				printf("%s-%s-%d not find unit\n",__FILE__,__func__,__LINE__);

				//sockfd进行封装
				buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_FD;
				tcp_ctrl_data_short2char(0,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//id号进行封装
				buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_ID;
				tcp_ctrl_data_short2char(0,data);
				memcpy(&buf[tmp_index],data,sizeof(short));
				tmp_index = tmp_index + sizeof(short);
				//直接上传网络地址字节序
				buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_IP;
				tcp_ctrl_data_int2char(0,value);
				memcpy(&buf[tmp_index],value,sizeof(int));
				tmp_index = tmp_index + sizeof(int);
				//单元机席别信息
				buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_SEAT;
				buf[tmp_index++] = 0;
				//单元机电量信息
				buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_POWER;
				buf[tmp_index++] = 0;
			}else{
				tmp = node_queue->sys_list[CONNECT_LIST]->next;
				while(tmp != NULL)
				{
					pinfo = tmp->data;
					if(pinfo->client_name != PC_CTRL)
					{
						//sockfd进行封装
						buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_FD;
						tcp_ctrl_data_short2char(pinfo->mac_addr,data);
						memcpy(&buf[tmp_index],data,sizeof(short));
						tmp_index = tmp_index + sizeof(short);
						//id号进行封装
						buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_ID;
						tcp_ctrl_data_short2char(pinfo->id,data);
						memcpy(&buf[tmp_index],data,sizeof(short));
						tmp_index = tmp_index + sizeof(short);
						//直接上传网络地址字节序
						buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_IP;
						tcp_ctrl_data_int2char((int)pinfo->cli_addr.sin_addr.s_addr,value);
						memcpy(&buf[tmp_index],value,sizeof(int));
						tmp_index = tmp_index + sizeof(int);
						//单元机席别信息
						buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_SEAT;
						buf[tmp_index++] = pinfo->seat;
						//单元机电量信息
						buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_POWER;
						buf[tmp_index++] = pinfo->client_power;

					}
					tmp = tmp->next;
				}
			}
			break;
		}
		case WIFI_MEETING_EVT_UNIT_ONOFF_LINE:
		{
			buf[tmp_index++] = type->name_type[0];
			buf[tmp_index++] = type->code_type[0];

			buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_FD;
			tcp_ctrl_data_short2char(type->evt_data.unet_info.sockfd,data);
			memcpy(&buf[tmp_index],data,sizeof(short));
			tmp_index = tmp_index + sizeof(short);

			buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_ID;
			tcp_ctrl_data_short2char(type->s_id,data);
			memcpy(&buf[tmp_index],data,sizeof(short));
			tmp_index = tmp_index + sizeof(short);

			buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_IP;
			tcp_ctrl_data_int2char(type->evt_data.unet_info.ip,value);
			memcpy(&buf[tmp_index],value,sizeof(int));
			tmp_index = tmp_index + sizeof(int);

			//单元机席别信息
			buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_SEAT;
			buf[tmp_index++] = type->con_data.seat;
			//单元机电量信息
			buf[tmp_index++] = WIFI_MEETING_EVT_RP_TO_PC_POWER;
			buf[tmp_index++] = type->evt_data.electricity;

			type->s_id = HOST_ID;
			break;
		}
		case WIFI_MEETING_EVT_AD_PORT:
		{
			buf[tmp_index++] = type->name_type[0];
			buf[tmp_index++] = type->code_type[0];
			if(type->spk_port > 0){
				buf[tmp_index++] = WIFI_MEETING_EVT_AD_PORT_SPK;
				tcp_ctrl_data_short2char(type->spk_port ,data);
			}else if(type->brd_port > 0){
				buf[tmp_index++] = WIFI_MEETING_EVT_AD_PORT_LSN;
				tcp_ctrl_data_short2char(type->brd_port ,data);
			}
			memcpy(&buf[tmp_index],data,sizeof(short));
			tmp_index = tmp_index + sizeof(short);
			break;
		}
		case WIFI_MEETING_EVT_HOST_STATUS:
		{
			buf[tmp_index++] = type->name_type[0];
			buf[tmp_index++] = type->code_type[0];

			buf[tmp_index++] = WIFI_MEETING_EVT_HOST_MIC;
			buf[tmp_index++] = conf_info_get_mic_mode();
			buf[tmp_index++] = WIFI_MEETING_EVT_HOST_SPK;
			buf[tmp_index++] = conf_status_get_spk_num();
			buf[tmp_index++] = WIFI_MEETING_EVT_HOST_SND;
			buf[tmp_index++] = conf_status_get_snd_effect();
			buf[tmp_index++] = WIFI_MEETING_EVT_HOST_CTRACK;
			if(conf_status_get_camera_track() == 0)
				buf[tmp_index++] = 2;
			else
				buf[tmp_index++] = conf_status_get_camera_track();

			break;
		}
		default:
		{
			buf[tmp_index++] = type->name_type[0];
			buf[tmp_index++] = type->code_type[0];
			buf[tmp_index++] = type->evt_data.value;
			break;
		}
		}
	}else{
		buf[tmp_index++] = type->name_type[0];
		buf[tmp_index++] = type->code_type[0];
		buf[tmp_index++] = type->evt_data.value;
	}


	type->data_len = tmp_index;

	return SUCCESS;
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
	Pconference_list con_list;

	char find_fd = -1;
	unsigned char buf[1024] = {0};
	unsigned char s_buf[2048] = {0};

	int pos = 0;

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

	/*
	 * 若果有消息体，基本上是需要进行群发的
	 * 没有消息体的时候，可能会是需要群发，可能需要单独发
	 */
	if(msg == NULL)
	{
		//通过上报的事件类型，进行区分
		switch(type->data_type)
		{
		case EVENT_DATA:
			tcp_ctrl_edit_event_content(type,buf);
			tcp_ctrl_frame_compose(type,buf,s_buf);
			pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
			tcp_ctrl_tpsend_enqueue(type,s_buf);
			pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
			break;
		case CONFERENCE_DATA:
			switch(type->name_type[0])
			{
			case WIFI_MEETING_CON_SUBJ:
			case WIFI_MEETING_CON_VOTE:
			case WIFI_MEETING_CON_ELEC:
			case WIFI_MEETING_CON_SCORE:
				tcp_ctrl_edit_conference_content(type,buf);

				tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
				while(tmp!=NULL)
				{
					con_list = tmp->data;
					//下发给所有单元机
					if((con_list->con_data.id > 0) &&
							(con_list->con_data.id != PC_ID))
					{
						type->fd = con_list->fd;
						tcp_ctrl_source_dest_setting(-1,type->fd,type);

						tcp_ctrl_frame_compose(type,buf,s_buf);

						pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
						tcp_ctrl_tpsend_enqueue(type,s_buf);
						pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);

						usleep(1000);
					}
					tmp=tmp->next;
				}
				break;
			default:
				tcp_ctrl_edit_conference_content(type,buf);
				tcp_ctrl_frame_compose(type,buf,s_buf);
				pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
				tcp_ctrl_tpsend_enqueue(type,s_buf);
				pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
				break;
			}
			break;
		default:
			printf("%s-%s-%d not leagal data type\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}else{

		//通过上报的事件类型，进行区分
		switch(type->evt_data.status)
		{
		case WIFI_MEETING_EVENT_POWER_OFF_ALL:
		case WIFI_MEETING_EVENT_SUBJECT_OFFSET:
		case WIFI_MEETING_EVENT_CHECKIN_START:
		case WIFI_MEETING_EVENT_CHECKIN_END:
		case WIFI_MEETING_EVENT_VOTE_START:
		case WIFI_MEETING_EVENT_VOTE_END:
		case WIFI_MEETING_EVENT_VOTE_RESULT:
		case WIFI_MEETING_CONF_ELECTION_START:
		case WIFI_MEETING_CONF_ELECTION_END:
		case WIFI_MEETING_CONF_ELECTION_RESULT:
		case WIFI_MEETING_CONF_SCORE_START:
		case WIFI_MEETING_CONF_SCORE_END:
		case WIFI_MEETING_CONF_SCORE_RESULT:
		case WIFI_MEETING_EVENT_CON_MAG_START:
		case WIFI_MEETING_EVENT_CON_MAG_END:
		//上位机下发数据
		case WIFI_MEETING_CONF_PC_CMD_ALL:
		case WIFI_MEETING_EVENT_PC_CMD_ALL:
//			tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
//			while(tmp!=NULL)
//			{
//				con_list = tmp->data;
//				//下发给所有单元机
//				if((con_list->con_data.id > 0) &&
//						(con_list->con_data.id != PC_ID))
//				{
//					type->fd = con_list->fd;
//					tcp_ctrl_source_dest_setting(-1,type->fd,type);
//
//					tcp_ctrl_frame_compose(type,msg,s_buf);
//
//					pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
//					tcp_ctrl_tpsend_enqueue(type,s_buf);
//					pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
//					pos++;
//					usleep(1000);
//				}
//				tmp=tmp->next;
//			}
			tmp = node_queue->sys_list[CONNECT_LIST]->next;
			while(tmp!=NULL)
			{
				pinfo = tmp->data;
				//下发给所有单元机
				if(pinfo->client_name == UNIT_CTRL)
				{
					type->fd = pinfo->client_fd;
					tcp_ctrl_source_dest_setting(-1,type->fd,type);

					tcp_ctrl_frame_compose(type,msg,s_buf);

					pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
					tcp_ctrl_tpsend_enqueue(type,s_buf);
					pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
					pos++;
					usleep(1000);
				}
				tmp=tmp->next;
			}
			if(pos == 0)
				return ERROR;
			break;
		//以下只是进行单独的发送
		case WIFI_MEETING_EVENT_DEVICE_HEART:
		case WIFI_MEETING_EVENT_SPK_ALLOW:
		case WIFI_MEETING_EVENT_SPK_VETO:
		case WIFI_MEETING_EVENT_SPK_REQ_SPK:
		case WIFI_MEETING_EVENT_SPK_ALOW_SND:
		case WIFI_MEETING_EVENT_SPK_VETO_SND:
		case WIFI_MEETING_EVENT_SPK_CLOSE_REQ:
		case WIFI_MEETING_EVENT_UNIT_SYS_TIME:
		//上位机下发数据
		case WIFI_MEETING_EVENT_PC_CMD_SIGNAL:
		case WIFI_MEETING_CONF_PC_CMD_SIGNAL:
		case WIFI_MEETING_HOST_REP_TO_PC:

			tcp_ctrl_frame_compose(type,msg,s_buf);

			pthread_mutex_lock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
			tcp_ctrl_tpsend_enqueue(type,s_buf);
			pthread_mutex_unlock(&sys_in.sys_mutex[CTRL_TCP_SQUEUE_MUTEX]);
			break;

		default:
			printf("%s-%s-%d not leagal status\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

	}

	return SUCCESS;
}
