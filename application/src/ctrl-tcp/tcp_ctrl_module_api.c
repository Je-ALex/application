/*
 * tcp_ctrl_module_api.c
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */



#include "../../header/tcp_ctrl_list.h"
#include "../../header/tcp_ctrl_data_process.h"


extern pclient_node list_head;
extern pthread_mutex_t mutex;


/*
 * 会议类数据
 * 分成控制类，查询类
 */
static void tcp_ctrl_edit_conference_info(Pframe_type type,unsigned char* buf)
{
	int i;
	int num = 0;

	/*
	 * 下发的控制消息
	 */
	switch (type->msg_type)
	{

		case WRITE_MSG:
		{
			/*
			 * 拷贝座位号信息
			 * name-0x01
			 * code-0x05
			 * 进行移位操作，高位保存第字节
			 */
			if(type->con_data.id > 0)
			{
				unsigned char data = 0;
				buf[num++]=WIFI_MEETING_CON_ID;
				buf[num++]=WIFI_MEETING_INT;
				data =(unsigned char) (( type->con_data.id >> 24 ) & 0xff);
				buf[num++] = data;
				data =(unsigned char) (( type->con_data.id >> 16 ) & 0xff);
				buf[num++] = data;
				data =(unsigned char) (( type->con_data.id >> 8 ) & 0xff);
				buf[num++] = data;
				data =(unsigned char) (( type->con_data.id >> 0 ) & 0xff);
				buf[num++] = data;
	//			for(i=0;i<num;i++)
	//			{
	//				printf("%x ",buf[i]);
	//
	//			}
			}
			/*
			 * 席别信息
			 * name-0x02
			 * code-0x01
			 */
			if(type->con_data.seat > 0)
			{
				buf[num++]=WIFI_MEETING_CON_SEAT;
				buf[num++]=WIFI_MEETING_CHAR;
				buf[num++] = type->con_data.seat;
			}
			/*
			 * 姓名信息
			 * name-0x03
			 * code-0x0a
			 * 姓名编码+数据格式编码+内容长度+内容
			 */
			if(strlen(type->con_data.name) > 0)
			{
				buf[num++] = WIFI_MEETING_CON_NAME;
				buf[num++] = WIFI_MEETING_STRING;
				buf[num++] = strlen(type->con_data.name);
				memcpy(&buf[num],type->con_data.name,strlen(type->con_data.name));
				num = num+strlen(type->con_data.name);
			}
			/*
			 * 议题信息
			 * name-0x03
			 * code-0x0a
			 * 姓名编码+数据格式编码+内容长度+内容
			 */
			if(strlen(type->con_data.subj) > 0)
			{
				buf[num++] = WIFI_MEETING_CON_SUBJ;
				buf[num++] = WIFI_MEETING_STRING;
				buf[num++] = strlen(type->con_data.subj);
				memcpy(&buf[num],type->con_data.subj,strlen(type->con_data.subj));
				num = num+strlen(type->con_data.subj);

			}
			type->data_len = num;

			break;
		}
		case READ_MSG:
		{
			if(type->name_type > 0)
			{
				buf[num++] = type->name_type[0];

				buf[num++] = WIFI_MEETING_STRING;
				buf[num++] = strlen(type->con_data.subj);

			}

		}
			break;
		default:
			printf("%s,message type is not legal\n",__func__);
	}
	for(i=0;i<num;i++)
	{
		printf("%x ",buf[i]);

	}
	printf("\n");

}


/*
 * 会议类类信息(0x02)设置
 *
 * 此API是事件类数据统一接口
 *
 * 编辑终端属性，设置id，seats等
 *
 * 输入：
 * @fd--设置终端的fd
 * @id--分配的id
 * @seats--分配的席位,主席机，客席机，旁听机
 * @name--对应终端的姓名，单主机情况是不需要设置姓名
 * @msg_ty结构体--下发的消息类型(控制消息，请求消息，应答消息，查询消息)
 */
int tcp_ctrl_module_edit_info(Pframe_type type)
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;

	char find_fd = -1;
	unsigned char buf[256] = {0};
	unsigned char s_buf[256] = {0};

	/**************************************
	 * 将数据信息进行组包
	 * 参照通讯协议中会议类参数信息进行编码
	 *
	 * 名称		编码格式	数据格式	值
	 * seat		0x01	char	1,2,3
	 * id		0x02	char 	1,2,3...
	 * name		0x03	char	"hushan"
	 ************************************/
	int i;
	/*
	 * 判断是否有此客户端
	 */
	tmp = list_head->next;
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
	 * 会议类数据再次处理
	 * 根据type参数，确定数据的内容和格式
	 */
	switch(type->data_type)
	{
		case CONFERENCE_DATA:
			tcp_ctrl_edit_conference_info(type,buf);
			break;

		case EVENT_DATA:

			break;

	}
	printf("name %s\n",type->con_data.name);
	printf("subj %s\n",type->con_data.subj);
	/*
	 * 发送消息组包
	 * 对数据内容进行封装，增数据头等信息
	 */
	tc_frame_compose(type,buf,s_buf);

	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == type->fd)
		{
			pthread_mutex_lock(&mutex);
			write(pinfo->client_fd, s_buf, type->frame_len);
			pthread_mutex_unlock(&mutex);
			break;
		}
		tmp = tmp->next;

	}

	return SUCCESS;
}


/*
 * 设置会议参数
 *
 * 此API只支持单个议题
 *
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj)
{
	frame_type data_info;

	/*
	 * 测试 发送
	 */
	data_info.con_data.id = 1;
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
 *
 */
int get_the_conference_parameters(int fd_value,int* client_id,char* client_seat,
		char* client_name,char* subj)
{

	frame_type data_info;

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

























