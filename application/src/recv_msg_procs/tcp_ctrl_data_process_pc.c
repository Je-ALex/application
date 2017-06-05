/*
 * tcp_ctrl_data_process_pc.c
 *
 *  Created on: 2016年11月16日
 *      Author: leon
 */

#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "client_connect_manage.h"
#include "client_mic_status_manage.h"
#include "tcp_ctrl_server.h"



/*
 * tcp_ctrl_pc_write_msg_con
 * 处理上位机下发的控制类会议类消息
 *
 * 会议类中，上位机下发的属于控制类的有ID,席别,姓名,会议名称和议题
 *
 * 主机需要解析出iD等信息保存于会议链表
 * 议题不做解析直接下发
 *
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_write_msg_con(const unsigned char* msg,Pframe_type type)
{
	int tc_index = 0;
	int ret = -1;
	int value = 0;
	int num = 0;

	/*
	 * 上位机下发会议参数
	 * ID,席别,单元姓名,会议名称，议题数量
	 */
	if(msg[tc_index] == WIFI_MEETING_CON_ID){

		type->name_type[0] = msg[tc_index++];
		type->code_type[0] = msg[tc_index++];

		type->ucinfo.id = (msg[tc_index++] << 8);
		type->ucinfo.id = type->ucinfo.id+(msg[tc_index++] << 0);

		if(msg[tc_index] == WIFI_MEETING_CON_SEAT){
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			type->ucinfo.seat = msg[tc_index++];

		}

		if(msg[tc_index] == WIFI_MEETING_CON_NAME){
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			type->ucinfo.name_len = msg[tc_index++];
			memcpy(type->ucinfo.name,&msg[tc_index],type->ucinfo.name_len);

			tc_index = tc_index+type->ucinfo.name_len;

		}
		if(msg[tc_index] == WIFI_MEETING_CON_CNAME){
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			type->ccontent.conf_nlen = msg[tc_index++];
			memcpy(type->ccontent.conf_name,&msg[tc_index],type->ccontent.conf_nlen);

			tc_index = tc_index+type->ccontent.conf_nlen;
		}
		if(msg[tc_index] == WIFI_MEETING_CON_SUBJ){

			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			type->ccontent.total_sub = msg[tc_index++];

			value = WIFI_MEETING_CONF_PC_CMD_SIGNAL;
		}

	 }else if(msg[tc_index] == WIFI_MEETING_CON_SUBJ)
	 {

		num = msg[2];//议题号
		value = msg[3];//议题属性

		conf_status_set_subject_property(num,value);

		switch(value)
		{
			case WIFI_MEETING_CON_SUBJ_NORMAL:
				type->ccontent.conf_nlen = msg[4];
				break;
			case WIFI_MEETING_CON_SUBJ_ELE:
				value = msg[4];
				conf_status_set_elec_totalp(num,value);
				type->ccontent.conf_nlen = msg[11];
				break;
			case WIFI_MEETING_CON_SUBJ_VOTE:
				type->ccontent.conf_nlen = msg[6];
				break;
			case WIFI_MEETING_CON_SUBJ_SCORE:
				type->ccontent.conf_nlen = msg[7];
				break;
		}
		conf_status_set_subject_content(num,msg,type->ccontent.conf_nlen);

		/*
		 * 下发给会议单元
		 */
		value = WIFI_MEETING_CONF_PC_CMD_ALL;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
	 }

	/*
	 * 解析数据格式，主机只保存id等信息到链表
	 * 如果正确则保存，错误则返回错误码
	 */
	if(value == WIFI_MEETING_CONF_PC_CMD_SIGNAL)
	{
		printf("id=%d,seat=%d,name=%s,cname=%s,sub_num=%d\n",
				type->ucinfo.id,type->ucinfo.seat,
				type->ucinfo.name,type->ccontent.conf_name,
				type->ccontent.total_sub);

		conf_status_set_total_subject(type->ccontent.total_sub);

		conf_status_set_conference_name(type->ccontent.conf_name,
				type->ccontent.conf_nlen);

		/*
		 * 更新链表的内容
		 */
		ret = ccm_refresh_info(type);
		if(ret)
			return ERROR;

		/*
		 * 下发给会议单元
		 */
		type->evt_data.status = value;
		tcp_ctrl_source_dest_setting(-1,type->fd,type);
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;

	}else if(value == WIFI_MEETING_CONF_PC_CMD_ALL)
	{

	}else
	{
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
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_write_msg_evt(const unsigned char* msg,Pframe_type type)
{
	int ret = 0;

	type->name_type[0] = msg[0];
	type->code_type[0] = msg[1];

	//话筒管理和音效管理是针对主机而言，所以需要解析
	switch(type->name_type[0])
	{
		case WIFI_MEETING_EVT_MIC:
			type->evt_data.value = msg[2];
			conf_status_set_mic_mode(type->evt_data.value);
			break;
		case WIFI_MEETING_EVT_SEFC:
			type->evt_data.value = msg[2];
			conf_status_set_snd_effect(type->evt_data.value);
			break;
		case WIFI_MEETING_EVT_SPK_NUM:
			type->evt_data.value = msg[2];
			conf_status_set_spk_num(type->evt_data.value);
			break;
		case WIFI_MEETING_EVT_CAMERA_TRACK:
			type->evt_data.value = msg[2];
			//上位机的2代表关闭
			if(type->evt_data.value == 2)
			{
				type->evt_data.value = 0;
			}
			conf_status_set_camera_track(type->evt_data.value);
			break;
		case WIFI_MEETING_EVT_SSID:
			type->evt_data.status = WIFI_MEETING_EVENT_PC_CMD_ALL;

			ret = tcp_ctrl_module_edit_info(type,msg);
			if(ret)
				return ERROR;
			break;
		default:
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
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_write_msg(const unsigned char* msg,Pframe_type type)
{
	int pos = 0;

	switch(type->data_type)
	{
		case EVENT_DATA:
			tcp_ctrl_pc_write_msg_evt(msg,type);
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
			if(type->name_type[0] == WIFI_MEETING_CON_ID)
			{
				pos = conf_status_find_pc_did_sockfd(type);
				if(pos)
				{
					tcp_ctrl_pc_write_msg_con(msg,type);
				}else{
					printf("%s-%s-%d-not find the unit device\n",__FILE__,__func__,__LINE__);
					return ERROR;
				}

			}else if(type->name_type[0] == WIFI_MEETING_CON_SUBJ){
				tcp_ctrl_pc_write_msg_con(msg,type);
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
 * 1、单元相关的查询，主要是单元机的属性信息，套接字号/id号/ip地址等等
 * 2、主机基本信息相关查询,话筒模式/发言人数/音效状态
 *
 * 输入
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 *
 */
static int tcp_ctrl_pc_read_msg_evt_rep(Pframe_type type)
{
	int ret = 0;

	switch(type->name_type[0])
	{
	case WIFI_MEETING_EVT_PC_GET_INFO:
		type->name_type[0] = WIFI_MEETING_EVT_RP_TO_PC;
		break;
	case WIFI_MEETING_EVT_HOST_STATUS:
		type->name_type[0] = WIFI_MEETING_EVT_HOST_STATUS;
		break;
	}
	type->code_type[0] = WIFI_MEETING_STRING;
	type->msg_type = R_REPLY_MSG;
	type->dev_type = HOST_CTRL;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);

	ret =tcp_ctrl_module_edit_info(type,NULL);
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
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 *
 */
static int tcp_ctrl_pc_read_msg(const unsigned char* msg,Pframe_type type)
{

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	switch(type->data_type)
	{
	case EVENT_DATA:
	{
		type->name_type[0] = msg[0];
		type->code_type[0] = msg[1];
		type->evt_data.value = msg[2];

		switch(type->name_type[0])
		{
		//上位机查询连接信息,主机进行应答
		case WIFI_MEETING_EVT_PC_GET_INFO:
		case WIFI_MEETING_EVT_HOST_STATUS:
			if(type->evt_data.value == 0x01)
			{
				tcp_ctrl_pc_read_msg_evt_rep(type);
			}else{
				printf("%s-%s-%d not legal value\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}
		}
		break;
	}

	case CONFERENCE_DATA:
		break;
	default:
		printf("%s-%s-%d not legal data type\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}



/*
 * tcp_ctrl_pc_request_subject
 * 议题请求
 *
 * 将议题号下发给单元
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 返回值
 * 成功
 */
static int tcp_ctrl_pc_request_subject(Pframe_type type,const unsigned char* msg)
{
	int ret = -1;
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			type->evt_data.value);

	//议题号保存到全局变量
	conf_status_set_current_subject(type->evt_data.value);

	type->msg_type = WRITE_MSG;
	type->evt_data.status = WIFI_MEETING_EVENT_SUBJECT_OFFSET;
	ret = tcp_ctrl_module_edit_info(type,msg);
	if(ret)
		return ERROR;

	return SUCCESS;

}


/*
 * tcp_ctrl_pcevent_request_checkin
 * 签到管理
 *
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pcevent_request_checkin(Pframe_type type,const unsigned char* msg)
{

	int value = 0,ret = -1;
	/*
	 * 开始签到和结束签到的指令需要下发给所有单元
	 */
	switch(type->evt_data.value)
	{
	case WIFI_MEETING_EVT_CHECKIN_START:
		value = WIFI_MEETING_EVENT_CHECKIN_START;
		break;
	case WIFI_MEETING_EVT_CHECKIN_END:
		value = WIFI_MEETING_EVENT_CHECKIN_END;
		break;
	case WIFI_MEETING_EVT_CHECKIN_SELECT:
		value = WIFI_MEETING_EVENT_CHECKIN_SELECT;
		break;
	default:
		printf("%s-%s-%d not legal value\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}
	if(value == WIFI_MEETING_EVENT_CHECKIN_START ||
			value == WIFI_MEETING_EVENT_CHECKIN_END)
	{
		conf_status_set_conf_staus(value);
		type->msg_type = WRITE_MSG;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
	}

	return SUCCESS;
}


/*
 * tcp_ctrl_pcevent_request_vote
 * 表决管理
 *
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pcevent_request_vote(Pframe_type type,const unsigned char* msg)
{
	int value = 0,ret = -1;

	switch(type->evt_data.value)
	{
	case WIFI_MEETING_EVT_VOT_START:
		value = WIFI_MEETING_EVENT_VOTE_START;
		break;
	case WIFI_MEETING_EVT_VOT_END:
		value = WIFI_MEETING_EVENT_VOTE_END;
		break;
	case WIFI_MEETING_EVT_VOT_RESULT:
		value = WIFI_MEETING_EVENT_VOTE_RESULT;
		break;
	}
	if(value == WIFI_MEETING_EVENT_VOTE_START ||
			value == WIFI_MEETING_EVENT_VOTE_END)
	{
		conf_status_set_subjet_staus(value);
		type->msg_type = WRITE_MSG;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
	}

	return SUCCESS;
}


/*
 * tcp_ctrl_pcevent_request_election
 * 选举管理
 *
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pcevent_request_election(Pframe_type type,const unsigned char* msg)
{
	int value = 0,ret = -1;

	switch(type->evt_data.value)
	{

	case WIFI_MEETING_EVT_ELECTION_START:
		value = WIFI_MEETING_CONF_ELECTION_START;
		break;
	case WIFI_MEETING_EVT_ELECTION_END:
		value = WIFI_MEETING_CONF_ELECTION_END;
		break;
	case WIFI_MEETING_EVT_ELECTION_RESULT:
		value = WIFI_MEETING_CONF_ELECTION_RESULT;
		break;
	default:
		printf("%s-%s-%d not legal value\n",__FILE__,__func__,__LINE__);
		break;
	}

	if(value == WIFI_MEETING_CONF_ELECTION_START ||
			value == WIFI_MEETING_CONF_ELECTION_END)
	{
		conf_status_set_subjet_staus(value);
		type->msg_type = WRITE_MSG;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
	}
	return SUCCESS;
}


/*
 * tcp_ctrl_pcevent_request_score
 * 评分管理
 *
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pcevent_request_score(Pframe_type type,const unsigned char* msg)
{
	int value = 0,ret = -1;

	switch(type->evt_data.value)
	{

	case WIFI_MEETING_EVT_SCORE_START:
		value = WIFI_MEETING_CONF_SCORE_START;
		break;
	case WIFI_MEETING_EVT_SCORE_END:
		value = WIFI_MEETING_CONF_SCORE_END;
		break;
	case WIFI_MEETING_EVT_SCORE_RESULT:
		value = WIFI_MEETING_CONF_SCORE_RESULT;
		break;
	default:
		printf("%s-%s-%d not legal value\n",__FILE__,__func__,__LINE__);
		break;
	}

	if(value == WIFI_MEETING_CONF_SCORE_START ||
			value == WIFI_MEETING_CONF_SCORE_END)
	{
		conf_status_set_subjet_staus(value);
		type->msg_type = WRITE_MSG;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
	}
	return SUCCESS;
}


/*
 * tcp_ctrl_pc_request_conf_manage
 * 会议管理
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_request_conf_manage(Pframe_type type,const unsigned char* msg)
{
	int value = 0,ret = -1;

	switch(type->evt_data.value)
	{
	case WIFI_MEETING_EVT_CON_MAG_START:
		value = WIFI_MEETING_EVENT_CON_MAG_START;
		break;
	case WIFI_MEETING_EVT_CON_MAG_END:
		value = WIFI_MEETING_EVENT_CON_MAG_END;
		break;

	}

	if(value == WIFI_MEETING_EVENT_CON_MAG_END ||
			value == WIFI_MEETING_EVENT_CON_MAG_START)
	{
		//变换为控制类消息下个给单元机
		type->msg_type = WRITE_MSG;
		type->dev_type = HOST_CTRL;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
		conf_status_set_conf_staus(value);
	}

	return SUCCESS;
}



/*
 * tcp_ctrl_pc_request_sys_time
 * 系统时间管理
 *
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 返回值
 * 成功
 * 失败
 *
 */
static int tcp_ctrl_pc_request_sys_time(Pframe_type type,const unsigned char* msg)
{
	int ret = -1;
	unsigned char tmp_msg[4] = {0};

	conf_status_set_sys_time(type,msg);

	tmp_msg[0] = msg[0];
	tmp_msg[1] = msg[1];
	tmp_msg[2] = msg[3];
	tmp_msg[3] = msg[5];

	type->msg_type = WRITE_MSG;
	type->dev_type = HOST_CTRL;

	type->data_len = 4;

	type->evt_data.status = WIFI_MEETING_EVENT_PC_CMD_ALL;
	ret = tcp_ctrl_module_edit_info(type,tmp_msg);
	if(ret)
		return ERROR;

	return SUCCESS;
}


/*
 * tcp_ctrl_pc_request_spk
 * 发言管理
 *
 * 输入
 * Pframe_type
 * msg
 * 输出
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_request_spk(Pframe_type type,const unsigned char* msg)
{
	int pos = 0;

	switch(type->evt_data.value)
	{
		case WIFI_MEETING_EVT_SPK_ALLOW:
			type->evt_data.status = WIFI_MEETING_EVENT_SPK_ALLOW;
			break;
		case WIFI_MEETING_EVT_SPK_VETO:
			type->evt_data.status = WIFI_MEETING_EVENT_SPK_VETO;
			break;
		case WIFI_MEETING_EVT_SPK_ALOW_SND:
			break;
		case WIFI_MEETING_EVT_SPK_VETO_SND:
			break;
		case WIFI_MEETING_EVT_SPK_REQ_SND:
			break;
		case WIFI_MEETING_EVT_SPK_REQ_SPK:
			break;
		case WIFI_MEETING_EVT_SPK_CLOSE_MIC:
			break;
		default:
			printf("there is not legal value\n");
			return ERROR;
	}

	pos = conf_status_pc_find_did_id(type);
	if(pos)
	{
		cmsm_msg_classify_handle(type,msg);
	}

	return SUCCESS;

}


/*
 * tcp_ctrl_pc_request_msg_evt
 * 上位机下发请求类消息
 *
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_request_msg_evt(const unsigned char* msg,Pframe_type type)
{

	int value = 0,ret = -1;

	type->name_type[0] = msg[0];
	type->code_type[0] = msg[1];
	type->evt_data.value = msg[2];

	printf("%s-%s-%d-value=%d\n",__FILE__,__func__,__LINE__,type->evt_data.value);

	//判断事件中的数据是否需要下发给所有单元
	switch(type->name_type[0])
	{
	case WIFI_MEETING_EVT_PWR:
		if(type->evt_data.value == WIFI_MEETING_EVT_PWR_OFF_ALL)
		{
			value = WIFI_MEETING_EVENT_PC_CMD_ALL;
		}
		break;
	case WIFI_MEETING_EVT_SUB:
		tcp_ctrl_pc_request_subject(type,msg);
		break;
	case WIFI_MEETING_EVT_CHECKIN:
		tcp_ctrl_pcevent_request_checkin(type,msg);
		break;
	case WIFI_MEETING_EVT_VOT:
		tcp_ctrl_pcevent_request_vote(type,msg);
		break;
	case WIFI_MEETING_EVT_ELECTION:
		tcp_ctrl_pcevent_request_election(type,msg);
		break;
	case WIFI_MEETING_EVT_SCORE:
		tcp_ctrl_pcevent_request_score(type,msg);
		break;
	case WIFI_MEETING_EVT_CON_MAG:
		tcp_ctrl_pc_request_conf_manage(type,msg);
		break;
	case WIFI_MEETING_EVT_SPK:
		tcp_ctrl_pc_request_spk(type,msg);
		break;
	case WIFI_MEETING_EVT_SYS_TIME:
		tcp_ctrl_pc_request_sys_time(type,msg);
		break;
	}

	if(value == WIFI_MEETING_EVENT_PC_CMD_ALL)
	{
		type->msg_type = WRITE_MSG;
		type->evt_data.status = value;
		ret = tcp_ctrl_module_edit_info(type,msg);
		if(ret)
			return ERROR;
	}

	return SUCCESS;
}


/*
 * tcp_ctrl_pc_request_msg_con
 * 上位机下发请求类会议消息
 *
 * 请求类会议消息主要是投票结果、选举结果和计分结果
 * 目前是将结果下发给主席单元，在主席单元授权给普通单元后，普通单元才能接收到会议结果
 *
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_request_msg_con(const unsigned char* msg,Pframe_type type)
{

	int ret = -1,pos = 0;

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 找到主席单元
	 */
	pos = conf_status_find_chairman_sockfd(type);
	if(pos > 0)
	{
		type->name_type[0] = msg[0];
		switch(type->name_type[0])
		{
		case WIFI_MEETING_CON_VOTE:
		case WIFI_MEETING_CON_ELEC:
		case WIFI_MEETING_CON_SCORE:

			/*
			 * 保存结果信息至主机
			 * 保存议题状态到主机
			 */
			conf_status_set_pc_conf_result(type->data_len,msg);

			type->msg_type = WRITE_MSG;
			type->evt_data.status = WIFI_MEETING_CONF_PC_CMD_SIGNAL;
			tcp_ctrl_source_dest_setting(-1,type->fd,type);
			ret = tcp_ctrl_module_edit_info(type,msg);
			if(ret)
			{
				printf("%s-%s-%d not legal command\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}
			break;
		default:
			printf("%s-%s-%d not legal command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}else
	{
		printf("%s-%s-%d not find the chairman\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}


/*
 * tcp_ctrl_pc_request_msg
 * 上位机下发请求类消息
 *
 * 分为事件类和会议类
 *
 * 输入
 * msg
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int tcp_ctrl_pc_request_msg(const unsigned char* msg,Pframe_type type)
{
	int ret = -1;

	switch(type->data_type)
	{
	case EVENT_DATA:
		ret = tcp_ctrl_pc_request_msg_evt(msg,type);
		break;
	case CONFERENCE_DATA:
		ret = tcp_ctrl_pc_request_msg_con(msg,type);
		break;
	}

	return ret;
}


/*
 * tcp_ctrl_from_pc
 * 上位机消息处理函数接口
 *
 * 1、上位机上线后先发送宣告在线消息，主机保存上位机连接信息至链表
 * 2、宣告上线后，上位机会发查询(单元机扫描功能)信息(全状态)，获取主机和单元机的状态@client_info
 *
 * 输入
 * handlbuf
 * Pframe_type
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
int tcp_ctrl_from_pc(const unsigned char* handlbuf,Pframe_type type)
{
	int ret = 0;
	int tmp_fd = type->fd;
	int tmp_type = type->msg_type;

	int name_type = handlbuf[0];

	type->name_type[0] = handlbuf[0];

	switch(tmp_type)
	{
		case WRITE_MSG:
			ret = tcp_ctrl_pc_write_msg(handlbuf,type);
			break;
		case READ_MSG:
			ret = tcp_ctrl_pc_read_msg(handlbuf,type);
			break;
		case REQUEST_MSG:
			ret = tcp_ctrl_pc_request_msg(handlbuf,type);
			break;
		case ONLINE_REQ:
			sys_mutex_lock(LIST_MUTEX);
			ret = ccm_add_info(handlbuf,type);
			sys_mutex_unlock(LIST_MUTEX);
			break;
	}
	//接收消息后的上报
	if(tmp_type != READ_MSG)
	{
		type->fd = tmp_fd;
		type->data_type = EVENT_DATA;
		type->msg_type = tmp_type;

		type->dev_type = HOST_CTRL;
		type->name_type[0] = name_type;
		type->code_type[0] = WIFI_MEETING_ERROR;

		/*
		 * 收到正确消息，回复上位机
		 * 返回成功就开始转圈，返回失败就提示失败
		 */
		if(ret == SUCCESS)
		{
			type->evt_data.value = TCP_C_ERR_SUCCESS;
			tcp_ctrl_source_dest_setting(-1,type->fd,type);
			tcp_ctrl_module_edit_info(type,NULL);
		}else{

			type->evt_data.value = TCP_C_ERR_UNKNOW;
			tcp_ctrl_source_dest_setting(-1,type->fd,type);
			tcp_ctrl_module_edit_info(type,NULL);
		}
	}

	return SUCCESS;

}


