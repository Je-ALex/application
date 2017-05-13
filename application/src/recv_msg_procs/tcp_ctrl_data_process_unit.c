/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */

#include "tcp_ctrl_server.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_device_status.h"
#include "client_connect_manage.h"
#include "client_heart_manage.h"
#include "client_mic_status_manage.h"
#include "sys_uart_init.h"
#include "tcp_ctrl_data_process.h"

/*
 * 定义两个结构体
 * 存取前状态，存储当前状态
 *
 * 创建查询线程，轮训比对，有差异，就返回新状态
 */

extern Pglobal_info node_queue;
extern sys_info sys_in;


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
	Pconference_list conf_info;

	int i,j;
	i = j = 0;

	type->d_id = type->s_id = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->fd == d_fd)
		{
			i++;
			type->d_id = conf_info->ucinfo.id;
		}
		if(conf_info->fd == s_fd)
		{
			j++;
			type->s_id = conf_info->ucinfo.id;
		}
		if(i>0 && j>0)
			break;
		tmp = tmp->next;
	}

	/*
	 * 查找上位机
	 * 如果有，将地址信息赋值
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == d_fd
				&& cinfo->client_name == PC_CTRL)
		{
			i++;
			type->d_id = PC_ID;
		}
		if(cinfo->client_fd == s_fd
				&& cinfo->client_name == PC_CTRL)
		{
			j++;
			type->s_id = PC_ID;
		}
		if(i>0 || j>0)
			break;
		tmp = tmp->next;
	}

	return SUCCESS;
}


/*
 * tcp_ctrl_uevent_request_pwr
 * 电源管理，主席单元下发一键关闭单元机功能
 *
 * 主机将请求消息发送给主机， 主机下发关机指令
 *
 * 主机界面提示关闭电源消息
 *
 */
static int tcp_ctrl_uevent_request_pwr(Pframe_type type,const unsigned char* msg)
{
	int tmp_fd = 0;
	char tmp_msg = 0;

	int pos = 0;
	int value = 0;


	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	tmp_fd = type->fd;
	tmp_msg = type->msg_type;

	switch(type->evt_data.value)
	{

	case WIFI_MEETING_EVT_PWR_OFF_ALL:
		/*
		 * 检查是否是主席单元下发的指令
		 */
		pos = conf_status_check_chairman_legal(type);
		if(pos > 0)
		{
			value = WIFI_MEETING_EVENT_POWER_OFF_ALL;
			//变换为控制类消息下个给单元机
			type->msg_type = WRITE_MSG;
			type->evt_data.status = value;
			tcp_ctrl_module_edit_info(type,msg);

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

		break;
	default:
		printf("%s-%s-%d there is not legal value\n",__FILE__,__func__,__LINE__);
		return ERROR;

	}
	type->fd = tmp_fd;
	type->msg_type = tmp_msg;
	if(value)
	{
		tcp_ctrl_msg_send_to(type,msg,value);

	}

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_spk
 * 请求发言处理函数
 *
 * 将单元的请求发言类型进行分类处理，在调用详细消息的处理函数
 *
 * 输入
 * frame_type信息保存结构体
 * msg消息数据包
 *
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 */
int tcp_ctrl_uevent_request_spk(Pframe_type type,const unsigned char* msg)
{
	int value = 0;
//	char tmp_msg = 0;
//	int tmp_fd = 0;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	//保存初始源地址信息
//	tmp_fd = frame_type->fd;
//	tmp_msg = frame_type->msg_type;

	switch(type->evt_data.value)
	{
	/*
	 * 主机收到主席单元的允许指令，目标地址为具体允许的单元机ID，源地址为主席单元
	 * 主机下发允许指令，修改源地址为主机，目标地址不变
	 */
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
	case WIFI_MEETING_EVT_SPK_REQ_SND:
		value = WIFI_MEETING_EVENT_SPK_REQ_SND;
		break;
	case WIFI_MEETING_EVT_SPK_REQ_SPK:
		value = WIFI_MEETING_EVENT_SPK_REQ_SPK;
		break;
	case WIFI_MEETING_EVT_SPK_CLOSE_MIC:
		value = WIFI_MEETING_EVENT_SPK_CLOSE_MIC;
		break;
	case WIFI_MEETING_EVT_SPK_CLOSE_REQ:
		value = WIFI_MEETING_EVENT_SPK_CLOSE_REQ;
		break;
	case WIFI_MEETING_EVT_SPK_CLOSE_SND:
		value = WIFI_MEETING_EVENT_SPK_CLOSE_SND;
		break;
	case WIFI_MEETING_EVT_SPK_CHAIRMAN_ONLY_ON:
		value = WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON;
		break;
	case WIFI_MEETING_EVT_SPK_CHAIRMAN_ONLY_OFF:
		value = WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_OFF;
		break;

	default:
		printf("%s-%s-%d there is not legal value\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	type->evt_data.status = value;
	cmsm_msg_classify_handle(type,msg);

	/*
	 * 将事件信息发送至消息队列
	 * 告知应用
	 */
//	if(value)
//	{
//		frame_type->fd = tmp_fd;
//		frame_type->msg_type = tmp_msg;
//		tcp_ctrl_msg_send_to(frame_type,msg,value);
//	}

	return SUCCESS;
}



/*
 * tcp_ctrl_uevent_request_subject
 * 会议议题控制管理
 *
 * 会议中主席单元会选择进行第几个议题，主机接收到具体的议题号，下发给单元机和上报给上位机
 * 主机单元通过全局变量保存当前议题号
 *
 */
int tcp_ctrl_uevent_request_subject(Pframe_type type,const unsigned char* msg)
{
	int value = 0;
	int pos = 0;
	char tmp_msgt = 0;
	int tmp_fd = 0;

	tmp_fd = type->fd;
	tmp_msgt = type->msg_type;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	/*
	 * 判断消息是否是主席单元发送
	 */
	pos = conf_status_check_chairman_legal(type);
	if(pos > 0)
	{
		type->msg_type = WRITE_MSG;
		value = type->evt_data.value;
		//议题号保存到全局变量
		conf_status_set_current_subject(value);

		type->evt_data.status = WIFI_MEETING_EVENT_SUBJECT_OFFSET;
		tcp_ctrl_module_edit_info(type,msg);

	}else{
		printf("the subject command is not chairman send\n");
		return ERROR;
	}

	/*
	 * 判断议题号
	 */
	value += WIFI_MEETING_EVENT_SUBJECT_OFFSET;

	/*
	 * 请求消息发送给上位机和主机显示
	 */
	type->fd = tmp_fd;
	type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_service
 * 服务请求
 * 主机接收端服务请求，将请求在主机界面显示或上传上位机
 *
 */
int tcp_ctrl_uevent_request_service(Pframe_type type,const unsigned char* msg)
{

	int value = 0;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}


	switch(type->evt_data.value)
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
	case WIFI_MEETING_EVT_SER_TEA:
		value = WIFI_MEETING_EVENT_SERVICE_TEA;
		break;
	case WIFI_MEETING_EVT_SER_TEC:
		value = WIFI_MEETING_EVENT_SERVICE_OTHERS;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;

	}
	tcp_ctrl_msg_send_to(type,msg,value);

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_checkin
 * 处理单元机签到功能
 * 单元机请求已经签到的信息
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_checkin(Pframe_type type,const unsigned char* msg)
{

	int value = 0;
	int pos = 0;

	int tmp_fd = 0;
	char tmp_msgt = 0;


	tmp_fd = type->fd;
	tmp_msgt = type->msg_type;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

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
		//保存签到信息
		value = WIFI_MEETING_EVENT_CHECKIN_SELECT;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;
	}

	//开始和结束的时候需要告知列席
	if(value == WIFI_MEETING_EVENT_CHECKIN_START ||
			value == WIFI_MEETING_EVENT_CHECKIN_END)
	{
		/*
		 * 验证指令是否是主席发送
		 */
		pos = conf_status_check_chairman_legal(type);
		if(pos > 0)
		{
			type->msg_type = WRITE_MSG;
			type->dev_type = HOST_CTRL;
			type->evt_data.status = value;
			conf_status_set_conf_staus(value);

			tcp_ctrl_module_edit_info(type,msg);
		}else{
			printf("the checkin command is not chariman send\n");
			return ERROR;
		}

	}

	type->fd = tmp_fd;
	type->dev_type = UNIT_CTRL;
	type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);

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
int tcp_ctrl_uevent_request_vote(Pframe_type type,const unsigned char* msg)
{

	int value = 0;
	int pos = 0;
	int tmp_fd = 0;
	int tmp_msgt = 0;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	tmp_fd = type->fd;
	tmp_msgt = type->msg_type;

	/*
	 * 开始投票和结束投票的指令需要下发给所有单元
	 */
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
	case WIFI_MEETING_EVT_VOT_ASSENT:
 		value = WIFI_MEETING_EVENT_VOTE_ASSENT;
		break;
	case WIFI_MEETING_EVT_VOT_NAY:
 		value = WIFI_MEETING_EVENT_VOTE_NAY;
		break;
	case WIFI_MEETING_EVT_VOT_WAIVER:
 		value = WIFI_MEETING_EVENT_VOTE_WAIVER;
		break;
	case WIFI_MEETING_EVT_VOT_TOUT:
		value = WIFI_MEETING_EVENT_VOTE_TIMEOUT;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;
	}

	//开始和结束的时候需要告知列席
	if(value == WIFI_MEETING_EVENT_VOTE_START ||
			value == WIFI_MEETING_EVENT_VOTE_END)
	{
		/*
		 * 检查是否是主席单元下发的指令
		 */
		pos = conf_status_check_chairman_legal(type);
		if(pos > 0)
		{
			conf_status_set_subjet_staus(value);
			//变换为控制类消息下个给单元机
			type->msg_type = WRITE_MSG;
			type->evt_data.status = value;
			tcp_ctrl_module_edit_info(type,msg);

			if(value == WIFI_MEETING_EVENT_VOTE_START)
			{
				conf_status_reset_vote_status();
			}

			if(value == WIFI_MEETING_EVENT_VOTE_END)
			{
				conf_status_calc_vote_result();
				conf_status_send_hvote_result();
			}

		}else{
			printf("%s-%s-%d not the chairman command\n",__FILE__,
					__func__,__LINE__);
			return ERROR;
		}
	}else if(value == WIFI_MEETING_EVENT_VOTE_RESULT){
		conf_status_set_subjet_staus(value);
		conf_status_send_vote_result();

	}else if(value > WIFI_MEETING_EVENT_VOTE_RESULT){
		//进行投票结果处理
		conf_status_set_vote_result(value);
	}

	type->fd = tmp_fd;
	type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);

	}

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_election
 * 选举管理
 *
 * 单元机请求消息选举
 * 将数据信息上报给上位机
 * 通过选举人编号，
 *
 */
int tcp_ctrl_uevent_request_election(Pframe_type type,const unsigned char* msg)
{
	int tmp_fd = 0;
	int tmp_msgt = 0;

	int pos = 0;
	int value = 0;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	tmp_fd = type->fd;
	tmp_msgt = type->msg_type;

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
		//fixme 选举人编号，设置对应编号值累加
//		if((type->evt_data.value - WIFI_MEETING_EVT_ELECTION_RESULT)
//				<= conf_status_get_elec_totalp(0))
//		{
			value = type->evt_data.value - WIFI_MEETING_EVT_ELECTION_RESULT;
			conf_status_set_elec_result(&msg[WIFI_MEETING_EVT_ELECTION_RESULT]);
 			value = WIFI_MEETING_CONF_ELECTION_UNDERWAY;
//		}
		break;
	}

	//开始和结束的时候需要告知列席单元
	if(value == WIFI_MEETING_CONF_ELECTION_START ||
			value == WIFI_MEETING_CONF_ELECTION_END)
	{
		/*
		 * 检查是否是主席单元下发的指令
		 */
		pos = conf_status_check_chairman_legal(type);
		if(pos > 0)
		{
			conf_status_set_subjet_staus(value);
			//变换为控制类消息下个给单元机
			type->msg_type = WRITE_MSG;
			type->evt_data.status = value;
			tcp_ctrl_module_edit_info(type,msg);

		}else{
			printf("%s-%s-%d not the chairman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}else if( value == WIFI_MEETING_CONF_ELECTION_RESULT)
	{
		conf_status_set_subjet_staus(value);
		//收到结束指令后，延时1ms下发选举结果
		conf_status_send_elec_result();
	}

	type->fd = tmp_fd;
	type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);
	}

	return SUCCESS;
}


/*
 * tcp_ctrl_uevent_request_score
 * 评分管理
 * 单元机请求消息计分
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_score(Pframe_type type,const unsigned char* msg)
{
	int tmp_fd = 0;
	int tmp_msgt = 0;

	int pos = 0;
	int value = 0;

	int soffset = 0x04;

	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	tmp_fd = type->fd;
	tmp_msgt = type->msg_type;

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
		//fixme 计分议题,不能超过编号人数
//		if(conf_status_get_score_totalp() < node_queue->sys_list[CONFERENCE_LIST]->size)
//		{
		type->evt_data.value = type->evt_data.value - soffset;
		conf_status_set_score_result(type->evt_data.value);
		value = WIFI_MEETING_CONF_SCORE_UNDERWAY;
//		}
		break;
	}

	if(value == WIFI_MEETING_CONF_SCORE_START ||
			value == WIFI_MEETING_CONF_SCORE_END)
	{
		/*
		 * 检查是否是主席单元下发的指令
		 */
		pos = conf_status_check_chairman_legal(type);
		if(pos > 0)
		{
			conf_status_set_subjet_staus(value);
			if(value == WIFI_MEETING_CONF_SCORE_START)
			{
				conf_status_reset_score_status();
			}
			//变换为控制类消息下个给单元机
			type->msg_type = WRITE_MSG;
			type->evt_data.status = value;
			tcp_ctrl_module_edit_info(type,msg);
			if(value == WIFI_MEETING_CONF_SCORE_END)
			{
				conf_status_calc_score_result();
				conf_status_send_hscore_result();
			}
		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,
					__func__,__LINE__);
			return ERROR;
		}

	}else if(value == WIFI_MEETING_CONF_SCORE_RESULT)
	{
		conf_status_set_subjet_staus(value);
		conf_status_send_score_result();
	}

	type->fd = tmp_fd;
	type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);

	}

	return SUCCESS;

}


/*
 * tcp_ctrl_uevent_request_conf_manage
 * 会议管理
 *
 * 开始会议和结束会议
 */
int tcp_ctrl_uevent_request_conf_manage(Pframe_type type,const unsigned char* msg)
{

	int pos = 0;
	int value = 0;

	char tmp_msgt = 0;
	int tmp_fd = 0;


	if(sys_debug_get_switch())
	{
		printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
				type->evt_data.value);
	}

	//保存初始源地址信息
	tmp_fd = type->fd;
	tmp_msgt = type->msg_type;

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

		pos = conf_status_check_chairman_legal(type);
		if(pos > 0)
		{
			//变换为控制类消息下个给单元机
//			if(value == WIFI_MEETING_EVENT_CON_MAG_END)
//			{
			type->msg_type = WRITE_MSG;
			type->dev_type = HOST_CTRL;
			type->evt_data.status = value;
				tcp_ctrl_module_edit_info(type,msg);
//			}
				conf_status_set_current_subject(1);
				conf_status_set_conf_staus(value);

		}else{
			printf("%s-%s-%d not the chairman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}

	type->fd = tmp_fd;
	type->dev_type = UNIT_CTRL;
	type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);

	}

	return SUCCESS;
}


/*
 * tcp_ctrl_uevent_request_conf_manage
 * 电量状态请求，主机需要转发上位机
 *
 * 开始会议和结束会议
 */
int tcp_ctrl_uevent_request_electricity(Pframe_type type,const unsigned char* msg)
{
	int value = 0;
	value = WIFI_MEETING_EVENT_UNIT_ELECTRICITY;

	tcp_ctrl_msg_send_to(type,msg,value);

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
 * 消息格式
 * char | char | char
 * 	内容	       编码	    数值
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 * 处理请求类消息有两种情况：
 * 1/有上位机的情况，这个时候，系统初始化时单元机上线，不做特殊处理，如果是会议中途加入则需要自动对其编号
 * 2/没有上位机情况，系统初始化时，不做任何处理,会议过程中，则需要自动对新上线设备编号
 *
 */
int tcp_ctrl_unit_request_msg(const unsigned char* msg,Pframe_type type)
{
	int pos = 0;

	/*
	 * 检查设备的合法性
	 * 赋值id和席别,上报qt
	 * 发言管理功能，是特殊情况，任何时间单元机上线后，均有可能请求音频功能
	 */
	pos = conf_status_check_client_conf_legal(type);

	if(pos > 0){

		/*
		 * 判读是事件型还是会议型
		 */
		switch(type->data_type)
		{

		case EVENT_DATA:
		{
			type->name_type[0] = msg[0];
			type->code_type[0] = msg[1];
			type->evt_data.value = msg[2];

			switch(type->name_type[0])
			{
				/*
				 * 电源管理
				 */
				case WIFI_MEETING_EVT_PWR:
					tcp_ctrl_uevent_request_pwr(type,msg);
					break;
				/*
				 * 发言管理
				 */
				case WIFI_MEETING_EVT_SPK:
					tcp_ctrl_uevent_request_spk(type,msg);
					break;
				/*
				 * 投票管理
				 */
				case WIFI_MEETING_EVT_VOT:
					tcp_ctrl_uevent_request_vote(type,msg);
					break;
				/*
				 * 议题管理
				 */
				case WIFI_MEETING_EVT_SUB:
					tcp_ctrl_uevent_request_subject(type,msg);
					break;
				/*
				 * 服务请求
				 */
				case WIFI_MEETING_EVT_SER:
					tcp_ctrl_uevent_request_service(type,msg);
					break;
				/*
				 * 签到管理
				 */
				case WIFI_MEETING_EVT_CHECKIN:
					tcp_ctrl_uevent_request_checkin(type,msg);
					break;
				/*
				 * 选举管理
				 */
				case WIFI_MEETING_EVT_ELECTION:
					tcp_ctrl_uevent_request_election(type,msg);
					break;
				/*
				 * 评分管理
				 */
				case WIFI_MEETING_EVT_SCORE:
					tcp_ctrl_uevent_request_score(type,msg);
					break;
				/*
				 * 会议管理
				 */
				case WIFI_MEETING_EVT_CON_MAG:
					tcp_ctrl_uevent_request_conf_manage(type,msg);
					break;
				/*
				 * 单元电量
				 */
				case WIFI_MEETING_EVT_UNIT_ELECTRICITY:
					tcp_ctrl_uevent_request_electricity(type,msg);
					break;
				default:
					printf("%s-%s-%d not legal event\n",__FILE__,__func__,__LINE__);
					return ERROR;

			}
		}
			break;

		case CONFERENCE_DATA:
			break;

		default:
			printf("%s-%s-%d not legal event\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

	}else{
		printf("%s-%s-%d the client not in conference list\n",__FILE__,__func__,__LINE__);
		return ERROR;

	}

	return SUCCESS;

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
int tcp_ctrl_unit_reply_conference(const unsigned char* msg,Pframe_type type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list = NULL,conf_info = NULL;

	int tc_index = 0;

	int ret = 0;
	int str_len = 0;
	int value = 0;

//	printf("%s-%s-%d ",__FILE__,__func__,__LINE__);

	/*
	 * 赋值id和席别
	 * qt上报
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->fd == type->fd)
		{
			type->ucinfo.id = type->s_id;
			type->ucinfo.seat=con_list->ucinfo.seat;
			break;
		}
		tmp=tmp->next;
	}

	switch(type->msg_type)
	{
	/*
	 * 控制类应答是设置失败，单元机返回的
	 * 所以，在接收到此消息后，主机要进行显示，还要将信息上报给上位机
	 */
	case W_REPLY_MSG:
	{

		type->name_type[0] = msg[tc_index++];
		/*
		 * 会议类控制应答，单元机只返回失败情况0xe3
		 * 上报消息进入队列
		 */
		if(msg[tc_index++] == WIFI_MEETING_ERROR)
		{
			switch(msg[tc_index++])
			{
			case TCP_C_ERR_SUCCESS:
				value = WIFI_MEETING_CONF_WREP_SUCCESS;
				break;
			case TCP_C_ERR_HEAD:
			case TCP_C_ERR_LENGTH:
			case TCP_C_ERR_CHECKSUM:
				value = WIFI_MEETING_CONF_WREP_ERR;
				break;
			}
		}
		break;
	}
	case R_REPLY_MSG:
	{

		/*
		 * 会议查询类应答，单元机应答全状态
		 * 对于单主机情况，主机只关心ID 和席别
		 * 有上位机情况，将应答信息转发给上位机
		 */
		if(msg[tc_index++] == WIFI_MEETING_CON_ID){
			--tc_index;
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];

			type->ucinfo.id = (msg[tc_index++] << 8);
			type->ucinfo.id = type->ucinfo.id+(msg[tc_index++] << 0);
			value = WIFI_MEETING_CONF_RREP;
		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){
			--tc_index;
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			type->ucinfo.seat = msg[tc_index++];

		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_NAME){
			--tc_index;
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			str_len = msg[tc_index++];
			memcpy(type->ucinfo.name,&msg[tc_index],str_len);

			tc_index = tc_index+str_len;

		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_CNAME){
			--tc_index;
			type->name_type[0] = msg[tc_index++];
			type->code_type[0] = msg[tc_index++];
			str_len = msg[tc_index++];
			memcpy(type->ccontent.conf_name,&msg[tc_index],str_len);

			tc_index = tc_index+str_len;

		 }

		if(value == WIFI_MEETING_CONF_RREP)
		{
			printf("id=%d,seat=%d,name=%s,cname=%s\n",
					type->ucinfo.id,type->ucinfo.seat,
					type->ucinfo.name,type->ccontent.conf_name);
			/*
			 * 更新conference_head链表的内容
			 */
			conf_info = (Pconference_list)malloc(sizeof(conference_list));
			memset(conf_info,0,sizeof(conference_list));
			conf_info->fd = type->fd;
			conf_info->ucinfo.id =  type->ucinfo.id;
			conf_info->ucinfo.seat =  type->ucinfo.seat;

			if(strlen(type->ucinfo.name) > 0)
			{
				memcpy(conf_info->ucinfo.name,type->ucinfo.name,strlen(type->ucinfo.name));
			}

			/*
			 * 更新到会议信息链表中
			 */
			ret = ccm_refresh_conference_list(conf_info);
			if(ret)
			{
				free(conf_info);
				return ERROR;
			}
		}
		break;
	}
	}

	/*
	 * 消息进行处理
	 * 如果有PC就直接将数据传递给PC
	 * 单主机的话，就需要上报给应用
	 */
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(type,msg,value);
	}

	return SUCCESS;

}


/*
 * tcp_ctrl_unit_reply_event
 * 事件类应答消息处理
 *
 * 事件类应答不用区分应答类型，单元机应答的内容为固定格式
 * 消息内容为name+code+ID
 *
 */
int tcp_ctrl_unit_reply_event(const unsigned char* msg,Pframe_type type)
{
//	pclient_node tmp = NULL;
//	Pconference_list tmp_type = NULL;

	int value = 0;

	type->name_type[0] = msg[0];
	type->code_type[0] = msg[1];
	type->evt_data.value = msg[2];

//	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
//			frame_type->evt_data.value);
	/*
	 * 赋值id和席别
	 * 在上报qt时需要
	 */
//	frame_type->con_data.id = frame_type->s_id;
//	//席别
//	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
//	while(tmp!=NULL)
//	{
//		tmp_type = tmp->data;
//		if(tmp_type->con_data.id == frame_type->s_id)
//		{
//			frame_type->con_data.seat=tmp_type->con_data.seat;
//			break;
//		}
//		tmp=tmp->next;
//	}

	switch(type->name_type[0])
	{
	/*
	 * 单元机电源
	 */
	case WIFI_MEETING_EVT_PWR:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_PWR_ON:
			value = WIFI_MEETING_EVENT_POWER_ON;
			break;
		case WIFI_MEETING_EVT_PWR_OFF:
		case WIFI_MEETING_EVT_PWR_OFF_ALL:
			value = WIFI_MEETING_EVENT_POWER_OFF;
			break;
		}
		break;
	}
	/*
	 * 话筒应答
	 */
	case WIFI_MEETING_EVT_MIC:
	{
		switch(type->evt_data.value)
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
	/*
	 * 签到应答
	 */
	case WIFI_MEETING_EVT_CHECKIN:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_CHECKIN_START:
			value = WIFI_MEETING_EVENT_CHECKIN_START;
			break;
		case WIFI_MEETING_EVT_CHECKIN_END:
			value = WIFI_MEETING_EVENT_CHECKIN_END;
			break;

		}
		break;
	}
	/*
	 * 发言管理应答
	 */
	case WIFI_MEETING_EVT_SPK:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_SPK_ALLOW:
			value = WIFI_MEETING_EVENT_SPK_ALLOW;
			break;
		case WIFI_MEETING_EVT_SPK_VETO:
			value = WIFI_MEETING_EVENT_SPK_VETO;
			cmsm_through_reply_proc_port(type);
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
	/*
	 * 投票管理应答
	 */
	case WIFI_MEETING_EVT_VOT:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_VOT_START:
			value = WIFI_MEETING_EVENT_VOTE_START;
			break;
		case WIFI_MEETING_EVT_VOT_END:
			value = WIFI_MEETING_EVENT_VOTE_END;
			break;
		case WIFI_MEETING_EVT_VOT_ASSENT:
			value = WIFI_MEETING_EVENT_VOTE_ASSENT;
			break;
		case WIFI_MEETING_EVT_VOT_NAY:
			value = WIFI_MEETING_EVENT_VOTE_NAY;
			break;
		case WIFI_MEETING_EVT_VOT_WAIVER:
			value = WIFI_MEETING_EVENT_VOTE_WAIVER;
			break;
		case WIFI_MEETING_EVT_VOT_TOUT:
			value = WIFI_MEETING_EVENT_VOTE_TIMEOUT;
			break;
		}
		break;
	}
	/*
	 * 议题管理应答
	 */
	case WIFI_MEETING_EVT_SUB:
	{
		value = type->evt_data.value + WIFI_MEETING_EVENT_SUBJECT_OFFSET;
		break;
	}

	/*
	 * ssid和key管理应答
	 */
	case WIFI_MEETING_EVT_SSID:
	case WIFI_MEETING_EVT_KEY:
	{
		value = WIFI_MEETING_EVENT_SSID_KEY;
		break;
	}
	/*
	 * 选举管理应答
	 */
	case WIFI_MEETING_EVT_ELECTION:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_ELECTION_START:
			value = WIFI_MEETING_CONF_ELECTION_START;
			break;
		case WIFI_MEETING_EVT_ELECTION_END:
			value = WIFI_MEETING_CONF_ELECTION_END;
			break;
		}
		break;
	}
	/*
	 * 计分管理应答
	 */
	case WIFI_MEETING_EVT_SCORE:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_SCORE_START:
			value = WIFI_MEETING_CONF_SCORE_START;
			break;
		case WIFI_MEETING_EVT_SCORE_END:
			value = WIFI_MEETING_CONF_SCORE_END;
			break;
		}

		break;
	}
	/*
	 * 会议管理应答
	 */
	case WIFI_MEETING_EVT_CON_MAG:
	{
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_CON_MAG_START:
			value = WIFI_MEETING_EVENT_CON_MAG_START;
			break;
		case WIFI_MEETING_EVT_CON_MAG_END:
			value = WIFI_MEETING_EVENT_CON_MAG_END;
			break;
		}
		break;
	}

	}

	if(value > 0)
	{
//		tcp_ctrl_msg_send_to(frame_type,msg,value);
	}

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
int tcp_ctrl_from_unit(const unsigned char* handlbuf,Pframe_type type)
{

#if TCP_DBG
	int i;
	printf("handlbuf: ");
	for(i=0;i<type->data_len;i++)
	{
		printf("0x%02x ",handlbuf[i]);
	}
	printf("\n");

#endif

	chm_process_communication_heart(handlbuf,type);

	/*
	 * 消息类型分类
	 * 具体细分为控制类应答和查询类应答
	 */
	switch(type->msg_type)
	{
		/*
		 * 请求类消息
		 */
		case REQUEST_MSG:
			tcp_ctrl_unit_request_msg(handlbuf,type);
			break;
		/*
		 * 控制类应答
		 * 查询类应答
		 */
		case W_REPLY_MSG:
		case R_REPLY_MSG:
		{
			if(type->data_type == CONFERENCE_DATA)
			{
				tcp_ctrl_unit_reply_conference(handlbuf,type);
			}
			else if(type->data_type == EVENT_DATA){
				tcp_ctrl_unit_reply_event(handlbuf,type);
			}
			break;
		}
		/*
		 * 上线请求消息
		 */
		case ONLINE_REQ:
			pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
			ccm_add_info(handlbuf,type);
			pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
			break;
		/*
		 * 在线心跳
		 */
		case ONLINE_HEART:
			break;
	}

	return SUCCESS;
}

