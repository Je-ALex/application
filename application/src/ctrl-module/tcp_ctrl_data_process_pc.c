/*
 * tcp_ctrl_data_process_pc.c
 *
 *  Created on: 2016年11月16日
 *      Author: leon
 */

#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_device_manage.h"

extern Pglobal_info node_queue;
extern sys_info sys_in;
/***************
 *
 * PC PART START
 *
 ***************/


/*
 * tcp_ctrl_pc_write_msg_con
 * 处理上位机下发的控制类会议类消息
 *
 * 会议类中，上位机下发的属于控制类的有ID,席别,姓名,会议名称和议题
 * 主机需要解析出iD等信息保存子会议链表
 * 议题不同做解析直接下发
 *
 */
static int tcp_ctrl_pc_write_msg_con(const unsigned char* msg,Pframe_type frame_type)
{
	int tc_index = 0;
	int ret = 0;
	int str_len = 0;
	int value = 0;


	/*
	 * 上位机下发会议参数
	 * ID,席别,单元姓名,会议名称，议题数量
	 */
	if(msg[tc_index] == WIFI_MEETING_CON_ID){

		frame_type->name_type[0] = msg[tc_index++];
		frame_type->code_type[0] = msg[tc_index++];

		frame_type->con_data.id = (msg[tc_index++] << 8);
		frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);

		if(msg[tc_index] == WIFI_MEETING_CON_SEAT){
				frame_type->name_type[0] = msg[tc_index++];
				frame_type->code_type[0] = msg[tc_index++];
				frame_type->con_data.seat = msg[tc_index++];

		}

		if(msg[tc_index] == WIFI_MEETING_CON_NAME){
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];
			str_len = msg[tc_index++];
			memcpy(frame_type->con_data.name,&msg[tc_index],str_len);

			tc_index = tc_index+str_len;

		}
		if(msg[tc_index] == WIFI_MEETING_CON_CNAME){
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];
			str_len = msg[tc_index++];
			memcpy(frame_type->con_data.conf_name,&msg[tc_index],str_len);

			tc_index = tc_index+str_len;
		}
		if(msg[tc_index] == WIFI_MEETING_CON_SUBJ){

			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];
			frame_type->con_data.sub_num = msg[tc_index++];

			value = WIFI_MEETING_CONF_PC_CMD_SIGNAL;
		}

	 }else if(msg[tc_index] == WIFI_MEETING_CON_SUBJ){

			/*
			 * 下发给会议单元
			 */
		 	value = WIFI_MEETING_CONF_PC_CMD_ALL;
		 	frame_type->evt_data.status = value;
			tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
			ret = tcp_ctrl_module_edit_info(frame_type,msg);
			if(ret)
				return ERROR;
	 }

	/*
	 * 解析数据格式，主机只保存id等信息
	 * 如果正确则保存，错误则返回错误码
	 */
	if(value == WIFI_MEETING_CONF_PC_CMD_SIGNAL)
	{
		printf("id=%d,seat=%d,name=%s,cname=%s,sub_num=%d\n",
				frame_type->con_data.id,frame_type->con_data.seat,
				frame_type->con_data.name,frame_type->con_data.conf_name,
				frame_type->con_data.sub_num);
		/*
		 * 更新链表的内容
		 */

		ret = dmanage_refresh_info(frame_type);
		if(ret)
		{
			return ERROR;
		}
		/*
		 * 下发给会议单元
		 */
		frame_type->evt_data.status = value;
		tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
		ret = tcp_ctrl_module_edit_info(frame_type,msg);
		if(ret)
			return ERROR;
	}else if(value == WIFI_MEETING_CONF_PC_CMD_ALL){


	}else{
		printf("%s-%s-%d-not legal conference data\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,value);
	return SUCCESS;

}


/*
 * tcp_ctrl_pc_write_msg_evt
 * 处理上位机下发的控制类事件消息
 *
 * 事件类消息基本上是进行透传，部分信息需要解析保存在主机
 *
 */
static int tcp_ctrl_pc_write_msg_evt(const unsigned char* msg,Pframe_type frame_type)
{
	int ret = 0;

	frame_type->name_type[0] = msg[0];
	frame_type->code_type[0] = msg[1];

	//话筒管理和音效管理是针对主机而言，所以需要解析
	switch(frame_type->name_type[0])
	{
	case WIFI_MEETING_EVT_MIC:
		frame_type->evt_data.value = msg[2];
		conf_status_set_mic_mode(frame_type->evt_data.value);
		break;
	case WIFI_MEETING_EVT_SEFC:
		frame_type->evt_data.value = msg[2];
		conf_status_set_snd_effect(frame_type->evt_data.value);
		break;
	case WIFI_MEETING_EVT_SPK_NUM:
		frame_type->evt_data.value = msg[2];
		conf_status_set_spk_num(frame_type->evt_data.value);
		break;
	case WIFI_MEETING_EVT_SSID:
		frame_type->evt_data.status = WIFI_MEETING_EVENT_PC_CMD_ALL;

		ret = tcp_ctrl_module_edit_info(frame_type,msg);
		if(ret)
			return ERROR;

		break;
	default:
//		frame_type->evt_data.status = WIFI_MEETING_EVENT_PC_CMD_SIGNAL;
//		tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
//		tcp_ctrl_module_edit_info(frame_type,msg);
		printf("%s-%s-%d-not legal name type\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}
/*
 * tcp_ctrl_pc_write_msg
 * 处理上位机下发的控制类消息
 *
 * 分为事件类和会议类的消息
 *
 */
static int tcp_ctrl_pc_write_msg(const unsigned char* msg,Pframe_type frame_type)
{
	int pos = 0;

	switch(frame_type->data_type)
	{
	case EVENT_DATA:
		tcp_ctrl_pc_write_msg_evt(msg,frame_type);
		break;
	/*
	 * 会议型数据
	 * 需要将数据内容取出，保存至链表中
	 */
	case CONFERENCE_DATA:
		/*
		 * 找到上位机需要下发消息的目标地址的
		 * 在上位机下发指令的时候，目标地址为单元机的fd
		 * 通过目标地址找出是否存在此单元机,并保存fd
		 */
		if(frame_type->name_type[0] == WIFI_MEETING_CON_ID)
		{
			pos = conf_status_find_did_sockfd_sock(frame_type);
			if(pos)
			{
				tcp_ctrl_pc_write_msg_con(msg,frame_type);
			}else{
				printf("%s-%s-%d-not find the unit device\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}

		}else if(frame_type->name_type[0] == WIFI_MEETING_CON_SUBJ){
			tcp_ctrl_pc_write_msg_con(msg,frame_type);
		}

		break;
	default:
		printf("%s-%s-%d-not legal data type\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_pc_read_msg_evt_rep
 * 主机上报上位机查询指令
 *
 * 主要是单元机的属性信息，套接字号，id号，ip地址
 *
 */
static int tcp_ctrl_pc_read_msg_evt_rep(Pframe_type frame_type)
{
	int ret = 0;

	frame_type->name_type[0] = WIFI_MEETING_EVT_RP_TO_PC;
	frame_type->code_type[0] = WIFI_MEETING_STRING;
	frame_type->msg_type = R_REPLY_MSG;
	frame_type->dev_type = HOST_CTRL;

	tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);

	ret =tcp_ctrl_module_edit_info(frame_type,NULL);
	if(ret)
		return ERROR;

	return SUCCESS;
}

/*
 * tcp_ctrl_pc_read_msg
 * 上位机查询类消息处理
 *
 * 分为事件类和会议类的消息
 *
 */
static int tcp_ctrl_pc_read_msg(const unsigned char* msg,Pframe_type frame_type)
{

	switch(frame_type->data_type)
	{
	case EVENT_DATA:
		frame_type->name_type[0] = msg[0];
		frame_type->code_type[0] = msg[1];
		frame_type->evt_data.value = msg[2];

		switch(frame_type->name_type[0])
		{
		//上位机查询连接信息,主机进行应答
		case WIFI_MEETING_EVT_PC_GET_INFO:
			if(frame_type->evt_data.value == 0x01)
			{
				tcp_ctrl_pc_read_msg_evt_rep(frame_type);
			}else{
				printf("%s-%s-%d not legal value\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}
			break;
		}
		break;
	case CONFERENCE_DATA:
		break;
	default:
		printf("%s-%s-%d not legal data type\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_pc_request_msg_evt
 * 上位机下发请求类消息
 */
static int tcp_ctrl_pc_request_msg_evt(const unsigned char* msg,Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pclient_info tmp_info;
	int pos = 0;
	int value = 0;
	int ret = 0;

	frame_type->name_type[0] = msg[0];
	frame_type->code_type[0] = msg[1];
	frame_type->evt_data.value = msg[2];

	//判断事件中的数据是否需要下发给所有单元
	switch(frame_type->name_type[0])
	{
	case WIFI_MEETING_EVT_PWR:
		if(frame_type->evt_data.value == WIFI_MEETING_EVT_PWR_OFF_ALL)
		{
			value = WIFI_MEETING_EVENT_PC_CMD_ALL;
		}
		break;
	case WIFI_MEETING_EVT_CHECKIN:
	case WIFI_MEETING_EVT_VOT:
	case WIFI_MEETING_EVT_SUB:
	case WIFI_MEETING_EVT_ELECTION:
	case WIFI_MEETING_EVT_SCORE:
	case WIFI_MEETING_EVT_CON_MAG:
		value = WIFI_MEETING_EVENT_PC_CMD_ALL;
		break;
	}

	if(value == WIFI_MEETING_EVENT_PC_CMD_ALL)
	{
		frame_type->msg_type = WRITE_MSG;
		frame_type->evt_data.status = WIFI_MEETING_EVENT_PC_CMD_ALL;
		ret = tcp_ctrl_module_edit_info(frame_type,msg);
		if(ret)
			return ERROR;
	}else{
		/*
		 * 找到上位机需要下发消息的目标地址的
		 * 在上位机下发指令的时候，目标地址为单元机的fd
		 * 通过木匾地址找出是否存在此单元机
		 */
		tmp = node_queue->sys_list[CONNECT_LIST]->next;
		while(tmp!=NULL)
		{
			tmp_info = tmp->data;
			if(frame_type->d_id == tmp_info->client_fd)
			{
				frame_type->fd = tmp_info->client_fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}
		if(pos > 0)
		{
			//变换为控制类消息下个给单元机
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = WIFI_MEETING_EVENT_PC_CMD_SIGNAL;
//	todo fix		tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
			frame_type->s_id = 0;
			ret = tcp_ctrl_module_edit_info(frame_type,msg);
			if(ret)
				return ERROR;
		}else{
			printf("%s-%s-%d not find unit info\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_pc_request_msg_con
 * 上位机下发请求类会议消息
 *
 * 请求类会议消息主要是投票结果、选举结果和计分结果
 * 目前是将结果下发给主席单元，在主席单元授权给普通单元后，普通单元才能接收到会议结果
 */
static int tcp_ctrl_pc_request_msg_con(const unsigned char* msg,Pframe_type frame_type)
{

	int ret = 0;
	int pos = 0;
	/*
	 * 找到主席单元
	 */
	pos = conf_status_find_chariman_sockfd(frame_type);
	if(pos > 0)
	{
		frame_type->name_type[0] = msg[0];
		switch(frame_type->name_type[0])
		{
		case WIFI_MEETING_CON_VOTE:
		case WIFI_MEETING_CON_ELEC:
		case WIFI_MEETING_CON_SCORE:
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = WIFI_MEETING_CONF_PC_CMD_SIGNAL;
			ret = tcp_ctrl_module_edit_info(frame_type,msg);
			if(ret)
				return ERROR;
			break;
		default:
			printf("%s-%s-%d not legal command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}else{
		printf("%s-%s-%d not find the chairman\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}
/*
 * tcp_ctrl_pc_request_msg
 * 上位机下发请求类消息
 */
static int tcp_ctrl_pc_request_msg(const unsigned char* msg,Pframe_type frame_type)
{
	int ret = 0;

	switch(frame_type->data_type)
	{
	//事件类比较简单，直接进行透传
	case EVENT_DATA:
		ret = tcp_ctrl_pc_request_msg_evt(msg,frame_type);
		break;
	case CONFERENCE_DATA:
		ret = tcp_ctrl_pc_request_msg_con(msg,frame_type);
		break;
	}

	return ret;
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
	int ret = 0;
	int tmp_fd = frame_type->fd;
	int tmp_type = frame_type->msg_type;
	int name_type = handlbuf[0];

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	frame_type->name_type[0] = handlbuf[0];

	switch(tmp_type)
	{
		case WRITE_MSG:
			ret = tcp_ctrl_pc_write_msg(handlbuf,frame_type);
			break;
		case READ_MSG:
			ret = tcp_ctrl_pc_read_msg(handlbuf,frame_type);
			break;
		case REQUEST_MSG:
			ret = tcp_ctrl_pc_request_msg(handlbuf,frame_type);
			break;
		case ONLINE_REQ:
			pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
			ret = dmanage_add_info(handlbuf,frame_type);
			pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
			break;
	}

	if(tmp_type != READ_MSG){

		printf("%s-%s-%d report to pc\n",__FILE__,__func__,__LINE__);
		frame_type->fd = tmp_fd;
		frame_type->data_type = EVENT_DATA;
		frame_type->msg_type = tmp_type;

//		frame_type->msg_type = W_REPLY_MSG;
//
//		if(tmp_type == ONLINE_REQ)
//			frame_type->msg_type = ONLINE_REQ;

		frame_type->dev_type = HOST_CTRL;
		frame_type->name_type[0] = name_type;
		frame_type->code_type[0] = WIFI_MEETING_ERROR;

		/*
		 * 收到正确消息，回复上位机
		 * 返回成功就开始转圈，返回失败就提示失败
		 *
		 */
		if(ret == SUCCESS)
		{
			frame_type->evt_data.value = TCP_C_ERR_SUCCESS;
			tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
			tcp_ctrl_module_edit_info(frame_type,NULL);
		}else{

			frame_type->evt_data.value = TCP_C_ERR_UNKNOW;
			tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
			tcp_ctrl_module_edit_info(frame_type,NULL);
		}
	}

	return SUCCESS;

}

//end
