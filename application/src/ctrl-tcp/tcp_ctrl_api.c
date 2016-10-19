/*
 * tcp_ctrl_api.c
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#include "../../header/tcp_ctrl_data_compose.h"

#include "../../header/tcp_ctrl_list.h"


extern pclient_node confer_head;
extern pclient_node list_head;


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
int get_the_client_connect_info()
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
 * set_the_conference_parameters.c
 * 设置会议参数,单主机的情况，主机只需进行ID和席位的编码
 *
 *
 *
 *
 * in:@socket_fd,@client_id,@client_seat,@client_name,@subj
 *
 * return:success or error
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj)
{

	Pframe_type data_info;

	Pclient_info pinfo;

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
	memcpy(data_info->con_data.subj,sub,strlen(sub));


	/*
	 * 增加到链表中
	 */
	list_add(confer_head,data_info);

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

	tcp_ctrl_module_edit_info(data_info);

	return SUCCESS;

}
/*
 * 查询会议参数
 * in:
 * @fd_value套接字号
 *
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

	tcp_ctrl_module_edit_info(&data_info);


	return SUCCESS;

}
/* set_the_conference_vote_result.c
 * 下发投票结果函数
 *
 * 将全局变量中的投票结果下发给全部终端
 *
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

	data_info.msg_type = WRITE_MSG;
	data_info.data_type = CONFERENCE_DATA;
	data_info.dev_type = HOST_CTRL;

	tcp_ctrl_module_edit_info(&data_info);


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
 * 电源开关
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

	tcp_ctrl_module_edit_info(&data_info);

	return SUCCESS;

}

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

	tcp_ctrl_module_edit_info(&data_info);

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

	tcp_ctrl_module_edit_info(&data_info);

	return SUCCESS;

}

