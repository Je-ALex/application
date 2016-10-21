/*
 * tcp_ctrl_api.c
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#include "../../header/tcp_ctrl_data_compose.h"
#include "../../header/tcp_ctrl_list.h"
#include "../../header/tcp_ctrl_device_status.h"

extern pclient_node confer_head;
extern pclient_node list_head;
extern Pconference_status con_status;

/*
 * 单元机扫描功能
 * 扫描结果通过结构体返回给应用
 *
 * 扫描主要是开机上电有终端连接后，将终端信息的ip返回给应用
 *
 * 返回终端连接个数
 *
 *
 */
int get_unit_connected_info()
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;
	FILE *connect_info,*conference_code;
	int ret;
	/*
	 * 终端连接信息
	 */
	connect_info = fopen("connect_info.conf","w+");
	/*
	 * 会议参数信息
	 */
//	conference_code = fopen("conference_code.conf","w+");

	tmp = list_head->next;

	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			printf("fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));

			ret = fwrite(pinfo,sizeof(client_info),1,connect_info);
			perror("fwrite");
			if(ret != 1)
				return ERROR;

		}
		tmp = tmp->next;
		usleep(10000);
	}
	fclose(connect_info);

	return list_head->size;

}


/*
 * get_uint_running_status
 * 单元机实时状态检测函数,用户只需检测设备号和具体返回信息
 *
 * in/out:
 *
 * @Pqueue_event详见头文件定义的结构体，返回参数
 *
 * return：
 * @error
 * @success
 */
int get_uint_running_status(Pqueue_event event_tmp)
{

	int ret;
	ret = tcp_ctrl_out_queue(&event_tmp);
	printf("fd--%d,id--%d,seat--%d,value--%d\n",
			event_tmp->socket_fd,event_tmp->id,event_tmp->seat,
			event_tmp->value);

	if(ret)
		return ERROR;

	return SUCCESS;
}




/********************************************
 *
 * 会议型数据设置和查询
 *
 *
 *******************************************/


/*
 * 配置info信息
 */
static int config_conference_frame_info(Pframe_type type){

	type->msg_type = WRITE_MSG;
	type->data_type = CONFERENCE_DATA;
	type->dev_type = HOST_CTRL;

	return 0;
}

/*
 * set_the_conference_parameters
 * 设置会议参数,单主机的情况，主机只需进行ID和席位的编码，其他设置为NULL即可，现在保留后续参数
 *

 * in:
 * @socket_fd
 * @client_id
 * @client_seat
 * @client_name（保留）
 * @subj（保留）
 *
 * return:
 * @error
 * @success
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj)
{

	Pframe_type data_info;

	data_info = (Pframe_type)malloc(sizeof(frame_type));

	memset(data_info,0,sizeof(frame_type));


	/*
	 * 测试 发送
	 */
	data_info->fd = fd_value;
	data_info->con_data.id = client_id;
	data_info->con_data.seat = client_seat;
	unsigned char* name = "湖山电器有限责任公司-电声分公司\0";
	unsigned char* sub = "WIFI无线会议系统，项目进展情况,shenm\0";
	memcpy(data_info->con_data.name,name,strlen(name));
	memcpy(data_info->con_data.subj[0],sub,strlen(sub));


	/*
	 * 增加到链表中
	 */

	tcp_ctrl_refresh_data_in_list(data_info);
	/*
	 * 将参数保存
	 */
//	data_info.fd = fd_value;
//	data_type.con_data.id = client_id;
//	data_type.con_data.seat = client_seat;
//	memcpy(data_type.con_data.name,client_name,strlen(client_name));
//	memcpy(data_type.con_data.subj,subj,strlen(subj));

	data_info->msg_type = WRITE_MSG;
	data_info->data_type = CONFERENCE_DATA;
	data_info->dev_type = HOST_CTRL;

	tcp_ctrl_module_edit_info(data_info,NULL);

	return SUCCESS;

}
/*
 * get_the_conference_parameters
 * 获取单元机会议类参数
 *

 * in:
 * @socket_fd 获取某一个单元机的会议参数信息，如果为空（NULL），则默认为获取全部的参会单元会议信息
 *
 * return:
 * @error
 * @success
 */
int get_the_conference_parameters(int fd_value)
{

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));
	/*
	 * 将参数保存
	 */
	data_info.fd = fd_value;

	config_conference_frame_info(&data_info);
	data_info.msg_type = READ_MSG;

	tcp_ctrl_module_edit_info(&data_info,NULL);


	return SUCCESS;

}
/*
 * set_the_conference_vote_result
 * 下发投票结果，单主机的情况下，此接口基本用不上，投票结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机
 *
 * in/out:
 * @NULL
 *
 * return:
 * @error
 * @success
 */
int set_the_conference_vote_result()
{

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	/*
	 * 投票结果
	 */
	data_info.name_type[0] = WIFI_MEETING_CON_VOTE;
	data_info.code_type[0] = WIFI_MEETING_STRING;

	data_info.con_data.v_result.assent = con_status->v_result.assent;
	data_info.con_data.v_result.nay = con_status->v_result.nay;
	data_info.con_data.v_result.waiver = con_status->v_result.waiver;
	data_info.con_data.v_result.timeout = con_status->v_result.timeout;


	config_conference_frame_info(&data_info);

	tcp_ctrl_module_edit_info(&data_info,NULL);


	return SUCCESS;

}

/********************************************
 *
 * 事件型数据设置和查询
 *
 *
 *******************************************/

/*
 * 配置info信息
 */
static int config_event_frame_info(Pframe_type type,unsigned char* value){

	if(value != NULL)
	{
		type->msg_type = WRITE_MSG;
	}else{
		type->msg_type = READ_MSG;
	}

	type->data_type = EVENT_DATA;
	type->dev_type = HOST_CTRL;

	return 0;
}


/*
 * set_the_event_parameter_power
 * 设置单元机电源开关
 * 设置某个单元机的电源开关
 *
 * in:
 * @fd_value设备号
 * @value值
 *
 * out:
 * @NULL
 *
 * return:
 * @error
 * @success
 */
int set_the_event_parameter_power(int fd_value,unsigned char value)
{
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	/*
	 * 将参数保存
	 */
	data_info.fd = fd_value;
	data_info.evt_data.value = value;
	data_info.name_type[0] = WIFI_MEETING_EVT_PWR;
	data_info.code_type[0] = WIFI_MEETING_CHAR;

	config_event_frame_info(&data_info,&value);

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}

/*
 * get_the_event_parameter_power
 * 获取单元机电源状态
 *
 * in:
 * @fd_value设备号
 *
 * return:
 * @error
 * @success
 */
int get_the_event_parameter_power(int fd_value)
{

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	/*
	 * 将参数保存
	 */
	data_info.fd = fd_value;
	data_info.name_type[0] = WIFI_MEETING_EVT_PWR;
	data_info.code_type[0] = WIFI_MEETING_CHAR;

	config_event_frame_info(&data_info,NULL);

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}

int set_the_event_parameter_ssid_pwd(int fd_value,char* ssid,char* pwd)
{
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	data_info.fd = fd_value;
	data_info.name_type[0] = WIFI_MEETING_EVT_SSID;
	data_info.code_type[0] = WIFI_MEETING_STRING;
	data_info.name_type[1] = WIFI_MEETING_EVT_KEY;
	data_info.code_type[1] = WIFI_MEETING_STRING;

	memcpy(&data_info.evt_data.ssid,ssid,strlen(ssid));
	memcpy(&data_info.evt_data.password,pwd,strlen(pwd));

	config_event_frame_info(&data_info,ssid);

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}


































