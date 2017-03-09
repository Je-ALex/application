/*
 * tcp_ctrl_api.c
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_device_status.h"
#include "client_connect_manage.h"

extern Pglobal_info node_queue;
extern sys_info sys_in;

/* config_conference_frame_info
 * 配置info信息
 */
static int config_conference_frame_info(Pframe_type type,char value){

	type->msg_type = value;
	type->data_type = CONFERENCE_DATA;
	type->dev_type = HOST_CTRL;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	return SUCCESS;
}

/*
 * 配置info信息
 */
static int config_event_frame_info(Pframe_type type,unsigned char value){


	type->msg_type = value;
	type->data_type = EVENT_DATA;
	type->dev_type = HOST_CTRL;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);

	return SUCCESS;
}

/******************
 * TODO 系统属性模块
 ******************/

/*
 * host_info_reset_factory_mode
 * 恢复出厂设置
 * 目前是清除连接信息和清除会议信息相关内容
 *
 * return：
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int host_info_reset_factory_mode()
{
//	pclient_node tmp = NULL;
//	Pclient_info pinfo;

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	char command[128];
	FILE *fp=NULL;
	snprintf(command,sizeof(command),"reboot");
	fp=popen(command,"r");
	if(fp==NULL)
    {
	    printf("reboot error\n");
	    return -1;
	}
	fclose(fp);

//	tmp = node_queue->sys_list[CONNECT_LIST]->next;
//	while(tmp != NULL)
//	{
//		pinfo = tmp->data;
//
//		if(pinfo!=NULL)
//		{
//			printf("clean fd:%d,ip:%s\n",pinfo->client_fd,inet_ntoa(pinfo->cli_addr.sin_addr));
//			pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
//			dmanage_delete_info(pinfo->client_fd);
//			pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
//
//		}
//		tmp = tmp->next;
//		msleep(10);
//	}

	return SUCCESS;
}

/*
 * host_info_get_network_info
 * 获取主机的网路状态
 *
 * int/out:
 * @Phost_info
 *
 * return:
 * @ERROR
 * @SUCCESS(0)
 */
int host_info_get_network_info(Phost_info ninfo)
{
	struct ifreq ifr;
	unsigned char mac[6];
	unsigned long nIP, nNetmask, nBroadIP;

	char* DevName = "eth0";

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		fprintf(stderr, "Create socket failed!errno=%d", errno);
		return ERROR;
	}

	strcpy(ifr.ifr_name, DevName);

	//获取本机mac地址
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
	{
         return ERROR;
	}
	memcpy(mac, ifr.ifr_hwaddr.sa_data, sizeof(mac));

	sprintf(ninfo->mac_addr,"%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	//获取本机IP地址
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		nIP = 0;
	}
	else
	{
		nIP = *(unsigned long*)&ifr.ifr_broadaddr.sa_data[2];
	}
    strcpy(ninfo->ip_addr,inet_ntoa(*(struct in_addr*)&nIP));
	//获取子网掩码
	if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0)
    {
         nNetmask = 0;
    }
	else
	{
		nNetmask = *(unsigned long*)&ifr.ifr_netmask.sa_data[2];
	}
    strcpy(ninfo->netmask,inet_ntoa(*(struct in_addr*)&nNetmask));

	//获取广播地址
	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) < 0)
	{
		nBroadIP = 0;
	}
	else
	{
		nBroadIP = *(unsigned long*)&ifr.ifr_broadaddr.sa_data[2];
	}
	strcpy(ninfo->broad_addr,inet_ntoa(*(struct in_addr*)&nBroadIP));

//	printf("\tMAC: %02x-%02x-%02x-%02x-%02x-%02x\n",
//	mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//	printf("\tIP: %s\n", inet_ntoa(*(struct in_addr*)&nIP));
//	printf("\tBroadIP: %s\n", inet_ntoa(*(struct in_addr*)&nBroadIP));
//  printf("\tNetmask: %s\n", inet_ntoa(*(struct in_addr*)&nNetmask));


    close(sock);

    if(nIP == 0)
    	return ERROR;

	return SUCCESS;
}

/*
 * host_info_get_factory_info
 * 获取主机信息
 * 主机信息是保存在文本文件中，为只读属性
 *
 * out:
 * @Phost_info
 *
 * return:
 * @ERROR
 * @SUCCESS(0)
 */
int host_info_get_factory_info(Phost_info pinfo)
{
    strcpy(pinfo->version,VERSION);
    strcpy(pinfo->host_model,MODEL);
    strcpy(pinfo->factory_info,PRODUCT);

	return SUCCESS;

}

/*
 * host_info_set_system_power
 * 设置主机电源
 *
 * in: 1是重启  2是关机
 *
 * return:
 * @ERROR
 * @SUCCESS(0)
 */
int host_info_set_system_power(int mode)
{
	char command[64];
	FILE *fp=NULL;

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	if(mode == 1)
	{
		snprintf(command,sizeof(command),"reboot");
	}else if(mode == 2){
		snprintf(command,sizeof(command),"shoutdown");
	}

	fp=popen(command,"r");
	if(fp==NULL)
    {
	    printf("reboot error\n");
	    return -1;
	}
	fclose(fp);

	return SUCCESS;

}

/*
 * host_info_get_system_time
 * 获取系统时间
 *
 * return:
 * @ERROR
 * @SUCCESS(0)
 */
int host_info_get_system_time(unsigned char* value)
{
	int ret = 0;

	ret = conf_status_get_sys_timestamp();

	value[0] = ret/3600;
	value[1] = (ret%3600)/60;
	value[2] = (ret%3600)%60;

	return SUCCESS;
}
/******************
 * TODO 单元机管理模块
 ******************/
/*
 * unit_info_get_connected_info
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
int unit_info_get_connected_info(char* name)
{
	int ret = 0;

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	strcpy(name,CONNECT_FILE);

	ret = conf_status_get_connected_len();

	printf("%s-%s-%d-connect size=%d\n",__FILE__,__func__,__LINE__,ret);

	return ret;

}


/*
 * unit_info_get_running_status
 * 单元机实时状态检测函数,用户只需检测设备号和具体返回信息
 *
 * in/out:
 * @Pqueue_event详见头文件定义的结构体，返回参数
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int unit_info_get_running_status(Prun_status data)
{
	int ret;

	ret = tcp_ctrl_report_dequeue(data);

	if(ret)
		return ERROR;

	return SUCCESS;
}

/*
 * unit_info_set_device_power_off
 * 关闭所有单元机
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int unit_info_set_device_power_off()
{

	frame_type data_info;
	unsigned char buf[3] = {0};
	int ret = 0;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 将参数保存
	 */
	data_info.name_type[0] = WIFI_MEETING_EVT_PWR;
	data_info.code_type[0] = WIFI_MEETING_CHAR;
	data_info.evt_data.value = WIFI_MEETING_EVT_PWR_OFF_ALL;
	data_info.evt_data.status = WIFI_MEETING_EVENT_POWER_OFF_ALL;

	tcp_ctrl_edit_event_content(&data_info,buf);

	config_event_frame_info(&data_info,WRITE_MSG);

	ret = tcp_ctrl_module_edit_info(&data_info,buf);

	if(ret)
		return ERROR;

	return SUCCESS;

}

/*
 * unit_info_get_device_power
 * 获取单元机电源状态
 *
 * in:
 * @fd_value设备号
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int unit_info_get_device_power(int fd)
{

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	/*
	 * 将参数保存
	 */
	data_info.fd = fd;
	data_info.name_type[0] = WIFI_MEETING_EVT_PWR;
	data_info.code_type[0] = WIFI_MEETING_CHAR;

	config_event_frame_info(&data_info,READ_MSG);

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}




/*********************
 * TODO 会议状态管理模块
 * 分为主机部分和单元机部分
 *********************/

/*
 * conf_info_set_mic_mode
 * 话筒管理中的模式设置
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 * 在设置成功后，会收到DSP传递的返回值，通过返回值判断，
 * 再将结果上报
 * FIFO模式和标准模式下，可以对发言人数进行设置，自由模式下，默认最大发言人数为最大值
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_mic_mode(int mode)
{
	int ret;
	ret=conf_status_set_mic_mode(mode);
	return ret;
}

/*
 * conf_info_get_mic_mode
 * 获取话筒管理中的模式
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 * 返回值：
 * @value
 */
int conf_info_get_mic_mode()
{
	int mode = 0;

	mode = conf_status_get_mic_mode() - WIFI_MEETING_EVT_MIC_CHAIRMAN;


	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			mode);

	return mode;
}

/*
 * conf_info_set_spk_num
 * 话筒管理中的发言管理
 * 设置会议中最大发言人数
 *
 * @num 1/2/4/6/8
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_set_spk_num(int num)
{
	int ret;
	ret=conf_status_set_spk_num(num);

	printf("%s-%s-%d，%d\n",__FILE__,__func__,__LINE__,ret);

	return ret;
}

/*
 * conf_info_get_spk_num
 * 话筒管理中的发言管理
 * 获取设置当前设置的最大发言人数
 *
 * 返回值：
 * @num 1/2/4/6/8
 */
int conf_info_get_spk_num()
{
	int ret = 0;

	ret = conf_status_get_spk_num();

	printf("%s-%s-%d，%d\n",__FILE__,__func__,__LINE__,ret);
	return ret;
}

/*
 * conf_info_set_cspk_num
 * 设置会议中当前发言的人数
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_cspk_num(int num)
{
	int ret;
	ret=conf_status_set_cspk_num(num);

	return ret;
}


/*
 * conf_info_get_cspk_num
 * 获取当前发言人数
 *
 * 返回值：
 * @current_spk
 */
int conf_info_get_cspk_num()
{
	 return conf_status_get_cspk_num();
}

/*
 * conf_info_set_snd_effect
 * DSP音效设置
 * 采用为管理分别从bit[0-2]表示状态
 *
 * bit  0    1     2     3
 *      AFC  ANC0  ANC1  ANC2
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_snd_effect(int mode)
{
	int ret;

	ret=conf_status_set_snd_effect(mode);
	return ret;
}

/*
 * conf_info_get_snd_effect
 * DSP音效获取
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_get_snd_effect()
{

	return conf_status_get_snd_effect();
}


/*
 * conf_info_set_conference_params
 * 设置会议参数,单主机的情况，主机只需进行ID和席位的编码，
 * 其他设置为NULL即可，现在保留后续参数
 *
 * in:
 * @socket_fd
 * @client_id
 * @client_seat
 * @client_name（保留）
 * @subj（保留）
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_conference_params(int fd,unsigned short id,unsigned char seat,
		char* pname,char* cname)
{
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	frame_type data_info;
	int ret = 0;

	memset(&data_info,0,sizeof(frame_type));

	getpeername(fd,(struct sockaddr*)&cli_addr,
			(socklen_t*)&clilen);

	printf("%s-%s-%d ,ip=%s,id=%d\n",__FILE__,__func__,__LINE__,inet_ntoa(cli_addr.sin_addr),id);

	data_info.fd = fd;
	data_info.con_data.id = id;
	data_info.con_data.seat = seat;

	/*
	 * 更新到链表
	 */
	ret = ccm_refresh_info(&data_info);
	if(ret)
		return ERROR;


	/*
	 * 组包下发
	 */
	config_conference_frame_info(&data_info,WRITE_MSG);
	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}


/*
 * conf_info_get_the_conference_params
 * 查询单元机会议类参数
 *

 * in:
 * @socket_fd 获取某一个单元机的会议参数信息，
 * 如果为0，则默认为获取全部的参会单元会议信息
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_get_the_conference_params(int fd)
{

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));
	int ret = 0;

	printf("%s-%s-%d,fd=%d\n",__FILE__,__func__,__LINE__,fd);

	data_info.fd = fd;

	//判断是否需要继续全部单元机查询
	if(data_info.fd)
		//用议题表示需要全部查询
		data_info.name_type[0] = WIFI_MEETING_CON_SUBJ;
	else
		data_info.name_type[0] = WIFI_MEETING_CON_ID;

	config_conference_frame_info(&data_info,READ_MSG);

	ret = tcp_ctrl_module_edit_info(&data_info,NULL);
	if(ret)
		return ERROR;

	return SUCCESS;

}


/*
 * conf_info_get_checkin_total
 * 获取会议中签到应到人数
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @会议中应到人数
 *
 */
int conf_info_get_checkin_total()
{
	int ret = 0;

	ret = conf_status_get_conference_len();

	printf("%s-%s-%d,checkin=%d\n",__FILE__,__func__,__LINE__,ret);

	return ret;
}

/*
 * conf_info_send_vote_result
 * 下发投票结果，单主机的情况下，此接口基本用不上，投票结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机,当结束一个议题以后，自动下发结果，单元机选择性显示
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_send_vote_result()
{
	int ret;

	ret = conf_status_send_vote_result();

	return ret;

}

/*
 * conf_info_send_elec_result
 * 下发选举结果，单主机的情况下，此接口基本用不上，选举结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机,当结束一个议题以后，自动下发结果，单元机选择性显示
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_send_elec_result()
{
	int ret;

	ret = conf_status_send_elec_result();

	return ret;
}

/*
 * conf_info_send_score_result
 * 下发计分结果，单主机的情况下，此接口基本用不上，计分结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机,当结束一个议题以后，自动下发结果，单元机选择性显示
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_send_score_result()
{


	int ret;

	ret = conf_status_send_score_result();

	return ret;
}

/*********************
 * TODO 系统扩展功能
 *********************/

/*
 * sys_exp_set_camera_track
 *
 * in:
 * @value
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int sys_exp_set_camera_track(int value)
{
	int ret;

	ret = conf_status_set_camera_track(value);

	return ret;
}

/*
 * sys_exp_get_camera_track
 *
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int sys_exp_get_camera_track()
{
	int ret;

	ret = conf_status_get_camera_track();

	return ret;
}











/***************************************
 *
 * FIXME 保留
 *
 **************************************/
int conf_info_set_conference_params_test(int fd,unsigned short id,unsigned char seat,
		char* pname,char* cname)
{

	frame_type data_info;

	int ret = 0;
	char name[128] = "赵钱孙李";
	char conf_name[128] = "WIFI会议第一次全体会议";

	memset(&data_info,0,sizeof(frame_type));

	printf("%s,%d,fd_value=%d,client_id=%d\n",__func__,
			__LINE__,fd,id);

	/*
	 * 测试 发送
	 */
	data_info.fd = fd;
	data_info.con_data.id = id;
	data_info.con_data.seat = seat;
	memcpy(data_info.con_data.name,name,strlen(name));
	memcpy(data_info.con_data.conf_name,conf_name,strlen(conf_name));
	data_info.con_data.sub_num = 10;

	/*
	 * 增加到会议信息链表中
	 */
	ret = ccm_refresh_info(&data_info);
	if(ret)
		return ERROR;
	/*
	 * 组包下发
	 */
	config_conference_frame_info(&data_info,WRITE_MSG);
	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}


/*
 *  wifi_meeting_get_vote_status
 *  获取投票结果
 *
 *	@num 议题编号
 *  @Pvote_result 投票结果类型
 *
 *	return：
 *	@ERROR
 *	@SUCCESS
 */
int wifi_meeting_get_vote_status(unsigned char num,Pvote_result result)
{
	unsigned short vote_r[4] = {0};

	if(conf_status_get_subject_property(num) == WIFI_MEETING_CON_SUBJ_VOTE)
	{
		conf_status_get_vote_result(num,vote_r);
		result->assent = vote_r[0];//node_queue->con_status->sub_list[num].v_result.assent;
		result->nay = vote_r[1];//node_queue->con_status->sub_list[num].v_result.nay;
		result->waiver = vote_r[2];//node_queue->con_status->sub_list[num].v_result.waiver;
		result->timeout = vote_r[3];//node_queue->con_status->sub_list[num].v_result.timeout;
	}else{
		printf("%s-%s-%d not vote subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}
	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	return SUCCESS;
}

/*
 *  wifi_meeting_get_elec_status
 *  获取表决结果
 *
 *	@num 议题编号
 *  @Pvote_result 投票结果类型
 *
 *	return：
 *	@ERROR
 *	@SUCCESS
 */
int wifi_meeting_get_elec_status(unsigned char num,Pelection_result result)
{
	int i;

	if(conf_status_get_subject_property(num) == WIFI_MEETING_CON_SUBJ_ELE)
	{
		for(i=1;i<=conf_status_get_elec_totalp(num);i++)
		{

			result->ele_id[i] = conf_status_get_elec_result(num,i);
		}

	}else{
		printf("%s-%s-%d not election subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	return SUCCESS;
}

/*
 *  wifi_meeting_get_score_status
 *  获取计分结果
 *
 *	@num 议题编号
 *  @Pvote_result 投票结果类型
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int wifi_meeting_get_score_status(unsigned char num,Pscore_result result)
{

	if(conf_status_get_subject_property(num) == WIFI_MEETING_CON_SUBJ_SCORE)
	{
		conf_status_get_score_result(result);

	}else{
		printf("%s-%s-%d not score subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	return SUCCESS;
}


/*
 * set_the_event_parameter_ssid_pwd
 * 设置单元机ssid和密码
 *
 * in:
 * @socket_fd套接字号
 * @ssid
 * @key
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int set_the_event_parameter_ssid_pwd(int socket_fd,char* ssid,char* pwd)
{
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	data_info.fd = socket_fd;
	data_info.name_type[0] = WIFI_MEETING_EVT_SSID;
	data_info.code_type[0] = WIFI_MEETING_STRING;

	data_info.name_type[1] = WIFI_MEETING_EVT_KEY;
	data_info.code_type[1] = WIFI_MEETING_STRING;

	memcpy(&data_info.evt_data.unet_info.ssid,ssid,strlen(ssid));
	memcpy(&data_info.evt_data.unet_info.key,pwd,strlen(pwd));

	config_event_frame_info(&data_info,WRITE_MSG);

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}
/*
 * conf_info_set_conference_sub_params
 * 设置会议议题内容
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_set_conference_sub_params()
{

	frame_type data_info;
	int i = 0;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	data_info.name_type[0] = WIFI_MEETING_CON_SUBJ;

	unsigned char subject[10][19]=
	{
			{0x01,0x01,0x01,0x05,0x01,0x44,0x53,0x44,0x53,0x44,0x08,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44},
	};
	/*
	 * 测试 发送
	 */

	memcpy(&data_info.con_data.subj[0],subject[i],19);

	/*
	 *
	 * 组包下发
	 */
	config_conference_frame_info(&data_info,WRITE_MSG);
	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;

}























