/*
 * tcp_ctrl_api.c
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#include "../../inc/tcp_ctrl_data_compose.h"
#include "../../inc/tcp_ctrl_data_process.h"
#include "../../inc/tcp_ctrl_device_status.h"
#include "../../inc/tcp_ctrl_list.h"
#include "../../inc/tcp_ctrl_api.h"



extern pclient_node conference_head;
extern pclient_node list_head;
extern Pconference_status con_status;




/*
 * reset_factory_mode
 * 恢复出厂设置
 * 目前是清除连接信息和清除会议信息相关内容
 *
 * return：
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int reset_the_host_factory_mode()
{
	pclient_node tmp = NULL;
	Pclient_info pinfo;


	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd > 0)
		{
			printf("fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));
			tcp_ctrl_delete_client(pinfo->client_fd);

		}
		tmp = tmp->next;
		usleep(10000);
	}

	return SUCCESS;
}

/*
 * get_the_host_network_info
 * 获取主机的网路状态
 *
 * int/out:
 * @Plocal_status
 *
 * return:
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int get_the_host_network_info(Phost_info list)
{

	struct ifreq ifr;
	unsigned char mac[6];
	unsigned long nIP, nNetmask, nBroadIP;

	char* DevName = "eth0";
	char* version = "v0.0.1";
	char* model = "DS-WF620M";

	strcpy(list->version,version);

	int s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s < 0)
	{
		fprintf(stderr, "Create socket failed!errno=%d", errno);
		return ERROR;
	}
	printf("%s:\n", DevName);

	strcpy(ifr.ifr_name, DevName);

	//获取本机mac地址
	if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0)
	{
         return ERROR;
	}
	memcpy(mac, ifr.ifr_hwaddr.sa_data, sizeof(mac));


	sprintf(list->mac,"%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	//获取本机IP地址

	if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
	{
		nIP = 0;
	}
	else
	{
		nIP = *(unsigned long*)&ifr.ifr_broadaddr.sa_data[2];
	}

    strcpy(list->local_ip,inet_ntoa(*(struct in_addr*)&nIP));
	//获取广播地址

	if (ioctl(s, SIOCGIFBRDADDR, &ifr) < 0)
	{
		nBroadIP = 0;
	}
	else
	{
		nBroadIP = *(unsigned long*)&ifr.ifr_broadaddr.sa_data[2];
	}

	//获取子网掩码
	if (ioctl(s, SIOCGIFNETMASK, &ifr) < 0)
    {
         nNetmask = 0;
    }
	else
	{
		nNetmask = *(unsigned long*)&ifr.ifr_netmask.sa_data[2];
	}

//	printf("\tMAC: %02x-%02x-%02x-%02x-%02x-%02x\n",
//	 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//	printf("\tIP: %s\n", inet_ntoa(*(struct in_addr*)&nIP));
//	printf("\tBroadIP: %s\n", inet_ntoa(*(struct in_addr*)&nBroadIP));
//    printf("\tNetmask: %s\n", inet_ntoa(*(struct in_addr*)&nNetmask));

    strcpy(list->netmask,inet_ntoa(*(struct in_addr*)&nNetmask));

    close(s);

	return SUCCESS;
}


/*
 * get_the_host_factory_infomation
 * 获取主机信息
 * 主机信息是保存在文本文件中，为只读属性
 *
 * out:
 * @Plocal_status
 *
 * return:
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int get_the_host_factory_infomation(Phost_info info)
{
	char hmodel[32];
	char version[32];
	char finformation[32];

	char* ver = "vesrsion=";
	char* model = "model=";
	char* finfp = "finfo=";
	char* str;

	char* buffer;

	int size;
	int result;
	FILE* file;

	file = fopen("host.info","rt");

	if(file == NULL)
	{
		perror("fopen");
		return ERROR;
	}
    /* 获取文件大小 */
    fseek (file , 0 , SEEK_END);
    size = ftell (file);
    rewind (file);

    /* 分配内存存储整个文件 */
    buffer = (char*) malloc (sizeof(char)*size);
    if (buffer == NULL)
    {
    	return ERROR;
    }

    /* 将文件拷贝到buffer中 */
    result = fread (buffer,1,size,file);
    if (result != size)
    {
    	return ERROR;
    }
    /* 现在整个文件已经在buffer中，可由标准输出打印内容 */
//    printf("%s\n", buffer);

    /* 结束演示，关闭文件并释放内存 */
    fclose (file);
    free (buffer);

    strcpy(info->version,"v0.0.1");
    strcpy(info->host_model,"DS-WF620M");
    strcpy(info->factory_information,"四川湖山电器有限责任公司");


	return SUCCESS;

}

/*
 * 单元机扫描功能
 * 扫描结果通过结构体返回给应用
 *
 * 扫描主要是开机上电有终端连接后，将终端信息的ip返回给应用
 *
 * in/out:
 * @name
 *
 * return:
 * 写入的连接客户端个数
 */
int get_client_connected_info(char* name)
{

//	pclient_node tmp = NULL;
//	Pclient_info pinfo;
//	FILE *connect_info,*conference_info;
//	int ret;
//
	char* connection_name = "connection.info";
//	/*
//	 * 终端连接信息
//	 */
//	connect_info = fopen("connection.info","w+");
//
//	tmp = list_head->next;
//	while(tmp != NULL)
//	{
//		pinfo = tmp->data;
//		if(pinfo->client_fd > 0)
//		{
//			printf("fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));
//
//			ret = fwrite(pinfo,sizeof(client_info),1,connect_info);
//			perror("fwrite");
//			if(ret != 1)
//				return ERROR;
//
//		}
//		tmp = tmp->next;
//		usleep(10000);
//	}
//	fclose(connect_info);
//
//	return list_head->size;
	strcpy(name,connection_name);

	return list_head->size;

}

/*
 * 设备电源设置
 * 关闭本机(1)、重启本机(2)、关闭单元机(3)
 *
 * in/out:
 * @modle(1,2,3)
 *
 * return:
 * 写入的连接客户端个数
 */
int set_device_power_off(int mode)
{


	return SUCCESS;
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
int get_unit_running_status(Pqueue_event* event_tmp)
{

	int ret;
	ret = tcp_ctrl_report_dequeue(event_tmp);


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

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
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

	frame_type data_info;
	Pconference_info confer_info;

	int ret = 0;
	unsigned char name[128] = "湖山电器有限责任公司-电声分公司";
	unsigned char sub[128] = "WIFI无线会议系统，项目进展情况,shenm";

	memset(&data_info,0,sizeof(frame_type));


	/*
	 * 测试 发送
	 */
	data_info.fd = fd_value;
	data_info.con_data.id = client_id;
	data_info.con_data.seat = client_seat;
	memcpy(data_info.con_data.name,name,strlen(name));
	memcpy(data_info.con_data.subj[0],sub,strlen(sub));


	/*
	 * 会议信息链表
	 */
	confer_info = (Pconference_info)malloc(sizeof(conference_info));
	memset(confer_info,0,sizeof(conference_info));
	confer_info->fd = fd_value;
	confer_info->con_data.id = client_id;
	confer_info->con_data.seat = client_seat;
	memcpy(confer_info->con_data.name,name,strlen(name));
	memcpy(confer_info->con_data.subj[0],sub,strlen(sub));
	/*
	 * 增加到会议信息链表中
	 */
	ret = tcp_ctrl_refresh_conference_list(confer_info);
	if(ret)
		return ERROR;
	/*
	 * 组包下发
	 */
	config_conference_frame_info(&data_info);
	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}
/*
 * get_the_conference_parameters
 * 查询单元机会议类参数
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
	pclient_node tmp = NULL;
	Pconference_info tmp_type;

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


	/*
	 * 请求消息发给所有单元
	 */
	tmp=conference_head->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->con_data.id)
		{
			data_info.fd = tmp_type->fd;

			config_conference_frame_info(&data_info);

			tcp_ctrl_module_edit_info(&data_info,NULL);
		}
		tmp=tmp->next;
		usleep(10000);
	}

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

	tcp_ctrl_source_dest_setting(-1,type->fd,type);

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


/*
 * 设置主机话筒模式
 * FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */

int set_mic_mode()
{



	return SUCCESS;
}


/*
 * 设置发言模式
 *
 *
 */

int set_speak_mode()
{



	return SUCCESS;
}
/*
 * 设置签到模式
 *
 *
 */

int set_checkin_mode()
{



	return SUCCESS;
}

























