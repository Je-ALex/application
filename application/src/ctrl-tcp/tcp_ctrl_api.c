/*
 * tcp_ctrl_api.c
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#include "../../header/tcp_ctrl_data_compose.h"

#include "../../header/tcp_ctrl_list.h"


extern pclient_node list_head;



/*
 * 单元机扫描功能
 * 扫描结果通过结构体返回给应用
 *
 * 扫描主要是开机上电有终端连接后，将终端信息的ip返回给应用
 *
 *
 */
int scanf_client()
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;
	FILE* file;
	int ret;
	file = fopen("info.txt","w+");



	tmp = list_head->next;

	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			printf("fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));

			ret = fwrite(pinfo,sizeof(client_info),1,file);
			perror("fwrite");
		}
		tmp = tmp->next;

	}
	fclose(file);
	return 0;

}



/********************************************
 *
 * 会议型数据设置和查询
 *
 *
 *******************************************/


/*
 * 设置会议参数
 * in:@socket_fd,@client_id,@client_seat,@client_name,@subj
 *
 * return:success or error
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj)
{
	frame_type data_info;

	memset(&data_info,0,sizeof(frame_type));

	/*
	 * 测试 发送
	 */
	data_info.con_data.id = client_id;
	data_info.con_data.seat = 2;
	unsigned char* name = "湖山电器有限责任公司-电声分公司\0";
	unsigned char* sub = "WIFI无线会议系统，项目进展情况,shenm\0";
	memcpy(data_info.con_data.name,name,strlen(name));
	memcpy(data_info.con_data.subj,sub,strlen(sub));

	/*
	 * 将参数保存
	 */
	data_info.fd = fd_value;
//	data_type.con_data.id = client_id;
//	data_type.con_data.seat = client_seat;
//	memcpy(data_type.con_data.name,client_name,strlen(client_name));
//	memcpy(data_type.con_data.subj,subj,strlen(subj));

	data_info.msg_type = WRITE_MSG;
	data_info.data_type = CONFERENCE_DATA;
	data_info.dev_type = HOST_CTRL;

	tcp_ctrl_module_edit_info(&data_info);

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

	data_info.msg_type = READ_MSG;
	data_info.data_type = CONFERENCE_DATA;
	data_info.dev_type = HOST_CTRL;

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
static int config_event_frame_info(Pframe_type type){

	type->msg_type = WRITE_MSG;
	type->data_type = EVENT_DATA;
	type->dev_type = HOST_CTRL;

	return 0;
}
/*
 * 电源开关
 */
int set_the_event_parameter_power(int fd_value,int client_id,int value)
{
	frame_type data_info;

	/*
	 * 测试 发送
	 */
	data_info.con_data.id = 1;

	/*
	 * 将参数保存
	 */
	data_info.fd = fd_value;

	config_event_frame_info(&data_info);

	tcp_ctrl_module_edit_info(&data_info);

	return SUCCESS;

}




