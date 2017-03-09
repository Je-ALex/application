

#include "sys_uart_init.h"
#include "client_connect_manage.h"

#include "audio_ring_buf.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "client_mac_manage.h"
#include "client_heart_manage.h"
#include "client_mic_status_manage.h"

extern Pglobal_info node_queue;


/*
 * ccm_refresh_connected_list
 * 更新设备连接信息链表
 *
 *
 */
static int ccm_refresh_connected_list(Pconference_list data_info)
{
	pclient_node tmp = NULL;
	pclient_node del = NULL;
	Pclient_info cinfo;
	Pclient_info tcinfo;
	Pclient_info newinfo;

	int pos = 0;
	int status = 0;
	FILE* cfile;
	int ret = 0;

	/*
	 * 首先在连接信息中搜寻socke_fd
	 * 判断是否有此设备连接，删除存在结点，重新插入结点
	 * 更新
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == data_info->fd)
		{
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}

	if(status > 0)
	{
		tcinfo = (Pclient_info)malloc(sizeof(client_info));
		memset(tcinfo,0,sizeof(client_info));

		list_delete(node_queue->sys_list[CONNECT_LIST],pos,&del);
		cinfo = del->data;
		memcpy(tcinfo,cinfo,sizeof(client_info));

		free(cinfo);
		free(del);
//		printf("%s-%s-%d,delete data in the connection list,then add it\n",
//				__FILE__,__func__,__LINE__);

		tcinfo->id = data_info->con_data.id;
		tcinfo->seat = data_info->con_data.seat;
		list_add(node_queue->sys_list[CONNECT_LIST],tcinfo);

	}else{
		printf("%s-%s-%d,there is no client in the connection list\n",
				__FILE__,__func__,__LINE__);
		return ERROR;
	}

	if(status > 0)
	{
		cfile = fopen(CONNECT_FILE,"w+");
		/*
		 * 更新文本文件
		 */
		tmp = node_queue->sys_list[CONNECT_LIST]->next;
		while(tmp != NULL)
		{
			newinfo = tmp->data;
			if(newinfo->client_fd > 0)
			{
//				printf("%s-%s-%d,fd=%d,id=%d,seat=%d\n",__FILE__,__func__,
//						__LINE__,newinfo->client_fd, newinfo->id,newinfo->seat);

				ret = fwrite(newinfo,sizeof(client_info),1,cfile);
//				perror("fwrite");
				if(ret != 1)
				{
					printf("%s-%s-%d,fd=%d,id=%d,seat=%d\n",__FILE__,__func__,
							__LINE__,newinfo->client_fd, newinfo->id,newinfo->seat);
					return ERROR;
				}

			}
			tmp = tmp->next;
			usleep(100);
		}
		fclose(cfile);
	}
	//end

	return SUCCESS;
}


/*
 * ccm_refresh_conference_list
 * 更新会议信息链表
 *
 */
int ccm_refresh_conference_list(Pconference_list data_info)
{
	pclient_node tmp = NULL;
	pclient_node del = NULL;
	Pclient_info cinfo;
	Pconference_list finfo;

	int pos = 0;
	int status = 0;

	/*
	 * 设置会议参数，需要判断是否当前有设备在连接信息链表中
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == data_info->fd)
		{
			status++;
			break;
		}
		tmp = tmp->next;
	}

	if(status > 0)
	{
		status = 0;
	}else{
		printf("%s-%s-%d,there is no client in the connected list\n",
				__FILE__,__func__,__LINE__);
		return ERROR;
	}


	/*
	 * 判断会议信息中的设备参数socke_fd
	 * 判断是否有此设备连接，删除存在结点，重新插入结点
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;;
	while(tmp != NULL)
	{
		finfo = tmp->data;
		if(finfo->fd == data_info->fd)
		{
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}
	if(status > 0)
	{
		list_delete(node_queue->sys_list[CONFERENCE_LIST],pos,&del);
		finfo = del->data;
		status = 0;
		free(finfo);
		free(del);
//		printf("%s-%s-%d,delete data in the conference list,then add it\n",
//				__FILE__,__func__,__LINE__);
	}else{

		printf("%s-%s-%d,there is no data in the conference list,add it\n",
				__FILE__,__func__,__LINE__);
	}

	list_add(node_queue->sys_list[CONFERENCE_LIST],data_info);
	//end

	return SUCCESS;
}


/*
 * ccm_refresh_info
 * 更新设备信息
 * 1、连接链表信息，文本文件
 * 2、会议链表信息
 *
 */
int ccm_refresh_info(const Pframe_type data_info)
{
	Pconference_list confer_info = NULL;
	int ret = 0;

	/*
	 * 更新到连接信息链表中
	 */
	ret = cmsm_refresh_spk_node(data_info);
	if(ret)
	{
		printf("%s-%s-%d,dmanage_refresh_spk_node err\n",__FILE__,__func__,__LINE__);
		free(confer_info);
		return ERROR;
	}

	/*
	 * 会议信息链表
	 */
	confer_info = (Pconference_list)malloc(sizeof(conference_list));
	memset(confer_info,0,sizeof(conference_list));
	confer_info->fd = data_info->fd;
	confer_info->con_data.id = data_info->con_data.id;
	confer_info->con_data.seat = data_info->con_data.seat;
	memcpy(confer_info->con_data.name,data_info->con_data.name,strlen(data_info->con_data.name));
	memcpy(confer_info->con_data.conf_name,data_info->con_data.conf_name,strlen(data_info->con_data.conf_name));

	/*
	 * 更新到连接信息链表中
	 */
	ret = ccm_refresh_connected_list(confer_info);
	if(ret)
	{
		printf("%s-%s-%d,dmanage_refresh_connected_list id=%d,err\n",__FILE__,__func__,__LINE__,
				confer_info->con_data.id);
		free(confer_info);
		return ERROR;
	}

	/*
	 * 更新到会议信息链表中
	 */
	ret = ccm_refresh_conference_list(confer_info);
	if(ret)
	{
		printf("%s-%s-%d,dmanage_refresh_conference_list err\n",__FILE__,__func__,__LINE__);
		free(confer_info);
		return ERROR;
	}

	return SUCCESS;
}



/*
 * ccm_delete_connected_list
 * 删除链接设备信息
 *
 */
static int ccm_delete_connected_list(int fd)
{

	pclient_node tmp = NULL;
	pclient_node del = NULL;

	Pclient_info pinfo;
	Pclient_info cinfo;

	FILE* file;

	int ret;
	int status = 0;
	int pos = 0;

//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 删除链接信息链表中的客户端
	 */
	tmp =  node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == fd)
		{
			/*
			 * 上位机下线
			 */
			if(pinfo->client_name == PC_CTRL)
			{
				conf_status_set_pc_staus(ERROR);
			}
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}

	if(status > 0)
	{
		list_delete(node_queue->sys_list[CONNECT_LIST],pos,&del);
		pinfo = del->data;
		printf("%s-%s-%d,remove %s in connected list\n",__FILE__,__func__,__LINE__,
				inet_ntoa(pinfo->cli_addr.sin_addr));

		free(pinfo);
		free(del);
	}else{
		printf("%s-%s-%d,there is no data in connected list\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	/*
	 * 更新文本文件
	 */
	file = fopen(CONNECT_FILE,"w+");
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp != NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd > 0)
		{
//			printf("%s-%s-%d,add %s in txt\n",__FILE__,__func__,__LINE__,
//					inet_ntoa(cinfo->cli_addr.sin_addr));
			ret = fwrite(cinfo,sizeof(client_info),1,file);
//			perror("fwrite");
			if(ret != 1)
				return ERROR;
			msleep(1);
		}
		tmp = tmp->next;
		usleep(1000);
	}
	fclose(file);

	return SUCCESS;
}




/*
 * ccm_delete_conference_list
 * 删除会议链表信息
 *
 */
static int ccm_delete_conference_list(int fd)
{
	pclient_node tmp = NULL;
	pclient_node del = NULL;
	Pconference_list cinfo;

	int status = 0;
	int pos = 0;

	/*
	 * 删除会议信息链表中的数据
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp != NULL)
	{
		cinfo = tmp->data;
		if(cinfo->fd == fd)
		{
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}
	if(status > 0)
	{
		list_delete(node_queue->sys_list[CONFERENCE_LIST],pos,&del);
		cinfo = del->data;

		printf("%s-%s-%d remove %d in conference list\n",__FILE__,__func__,__LINE__,cinfo->fd);
		free(cinfo);
		free(del);
	}else{
		printf("%s-%s-%d there is no data in conference list\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}


/*
 * ccm_delete_info
 * 客户端 删除函数
 * 删除链表中的连接信息
 * 先通过查找链表，定位离线客户端的位置，在调用链表删除函数，删除链表中的设备
 *
 * 更新文本文件
 *
 */
int ccm_delete_info(int fd)
{
	int tmp = 0;
	int ret = 0;
	pclient_node ctmp = NULL;
	Pclient_info cinfo;
	//上报上位机部分
	frame_type tmp_type;
	memset(&tmp_type,0,sizeof(frame_type));

	if(conf_status_get_pc_staus() != fd)
	{
		ctmp = node_queue->sys_list[CONNECT_LIST]->next;
		while(ctmp!=NULL)
		{
			cinfo = ctmp->data;
			if(cinfo->client_fd == fd)
			{
				tmp_type.con_data.id = cinfo->id;
				tmp_type.con_data.seat = cinfo->seat;
				break;
			}
			ctmp = ctmp->next;
		}

		/*
		 * 下线消息告知主机或上位机
		 */
		tmp_type.msg_type = OFFLINE_REQ;
		tmp_type.dev_type = UNIT_CTRL;
		tmp_type.data_type = EVENT_DATA;
		tmp_type.fd = fd;

		tmp_type.evt_data.status = WIFI_MEETING_EVENT_SPK_CLOSE_MIC;
		ret = cmsm_refresh_spk_node(&tmp_type);
		if(!ret)
		{
			tmp=conf_status_get_cspk_num();
			if(tmp)
			{
				tmp--;
				conf_status_set_cspk_num(tmp);
			}
			if(conf_status_get_cmspk() == WIFI_MEETING_CON_SE_CHAIRMAN)
			{
				conf_status_set_cmspk(WIFI_MEETING_CON_SE_GUEST);
			}
		}

		tcp_ctrl_msg_send_to(&tmp_type,NULL,WIFI_MEETING_EVENT_OFFLINE_REQ);

		chm_delete_heart_list(fd);

		ccm_delete_conference_list(fd);
	}
	ccm_delete_connected_list(fd);

	return SUCCESS;
}


/*
 * ccm_add_connected_info
 * 终端信息录入函数
 *
 * 生成设备连接信息文本文件，采用结构体方式进行存储
 * 应用在设置参数时，需要读取文本文件中的fd信息，进行参数设置
 *
 */
static int ccm_add_connected_info(Pframe_type type)
{
	FILE* file;
	pclient_node tmp_node = NULL;
	Pclient_info info = NULL;
	Pclient_info pinfo = NULL;

	int ret;
	int state = 0;

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

	/*
	 * 将新连接终端信息录入结构体中
	 * 这里可以将fd和IP信息存入到本地链表和文件中
	 *
	 * 区分上线的设备类型，单元机还是主机
	 *
	 */
	info = (Pclient_info)malloc(sizeof(client_info));
	memset(info,0,sizeof(client_info));


	info->client_fd = type->fd;
	getpeername(info->client_fd,(struct sockaddr*)&cli_addr,(socklen_t*)&clilen);
	info->cli_addr = cli_addr;
	info->clilen = clilen;
	sprintf(info->ip,"%s",inet_ntoa(cli_addr.sin_addr));

	if(type->dev_type == PC_CTRL){

		info->client_name = PC_CTRL;
		conf_status_set_pc_staus(type->fd);
		//上位机ID号位0xffff，无席别
		if(type->s_id > 0)
		{
			info->id = type->s_id;
		}

	}else{

		info->client_name = UNIT_CTRL;
		info->seat = type->con_data.seat;
		info->id = type->s_id;
		info->client_power = type->evt_data.electricity;
		info->mac_addr = type->evt_data.unet_info.mac;

		/*
		 * 查找MAC表信息
		 */
//		ret = cmm_search_client_mac(info);
//
//		if(ret != SUCCESS)
//			return ERROR;

	}

	/*
	 * 检查该客户端是否已经存在
	 *
	 * 链表如果为空，则不需要进行检查，直接存储
	 * 链表不为空，则通过读取再比对，进行存储
	 *
	 */
	tmp_node = node_queue->sys_list[CONNECT_LIST]->next;
	do
	{
		if(tmp_node == NULL)
		{
			break;
		}else{
			pinfo = tmp_node->data;
			if(type->dev_type != PC_CTRL){
	//			if(pinfo->client_fd == type->fd)

				if(pinfo->mac_addr == type->evt_data.unet_info.mac)
				{
					state++;
					printf("%s-%s-%d,the client is exist\n",__FILE__,__func__,__LINE__);
					free(info);
					return ERROR;
				}
			}else{
					if(pinfo->client_fd == type->fd)
					{
						state++;
						printf("%s-%s-%d,the client is exist\n",__FILE__,__func__,__LINE__);
						free(info);
						return ERROR;
					}
			}

		}
		tmp_node = tmp_node->next;

	}while(tmp_node != NULL);

	if(state == 0)
	{
		/*
		 * 连接信息存入链表
		 */
		list_add(node_queue->sys_list[CONNECT_LIST],info);

		/*
		 * 存入到文本文件
		 * 更新文本信息
		 */
		file = fopen(CONNECT_FILE,"a+");

		printf("%s-%s-%d fd:%d,ip:%s,machine:%d\n",__FILE__,__func__,__LINE__,info->client_fd,
				inet_ntoa(info->cli_addr.sin_addr),info->client_name);

		ret = fwrite(info,sizeof(client_info),1,file);
		if(sys_debug_get_switch())
		{
			perror("fwrite");
		}
		if(ret != 1)
			return ERROR;

		fclose(file);
	}

	return SUCCESS;
}


/* FIXME
 * dmanage_sync_conference_status
 * 设备会议状态同步
 * 1、签到状态 是否开始会议
 * 2、开始会议后，议题的状态
 * 		1.议题 开始 结束 公布状态
 *
 */
int dmanage_sync_conference_status(Pframe_type type)
{
	int ret = 0;

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	data_info.data_type = EVENT_DATA;
	data_info.dev_type = HOST_CTRL;
	data_info.msg_type = WRITE_MSG;

	/*
	 * 签到状态，是否开始会议
	 */
	ret = conf_status_get_conf_staus();
	switch(ret)
	{
	case WIFI_MEETING_EVENT_CHECKIN_START:
		data_info.name_type[0] = WIFI_MEETING_EVT_CHECKIN;
		data_info.code_type[0] = WIFI_MEETING_CHAR;

		break;
	case WIFI_MEETING_EVENT_CON_MAG_START:

		break;
	}

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;
}

/*
 * ccm_add_conference_info
 * 终端信息录入函数
 *
 * 生成设备连接信息文本文件，采用结构体方式进行存储
 * 应用在设置参数时，需要读取文本文件中的fd信息，进行参数设置
 *
 */
static int ccm_add_conference_info(Pframe_type type)
{
	Pconference_list confer_info;

	confer_info = (Pconference_list)malloc(sizeof(conference_list));
	memset(confer_info,0,sizeof(conference_list));

	/*
	 * 保存至会议信息链表
	 * 如果上线单元机有id号则将上线单元机信息进行存储
	 *
	 * 会议中上线的单元机，主机会自动给其分配ID号，用于管理
	 */
	confer_info->fd = type->fd;
	confer_info->con_data.id = type->con_data.id = type->s_id;
	confer_info->con_data.seat = type->con_data.seat;
	//判断当前是否有会议主席，如果有则上线的单元设置为客席，会议链表中只能存在一个主席
	if(conf_status_check_chariman_staus())
	{
		if(type->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			confer_info->con_data.seat = type->con_data.seat = WIFI_MEETING_CON_SE_GUEST;
		}

	}
	/*
	 * 1、判断源地址是否存在，存在则判断是否与现有设备冲突，冲突则设置为最大ID并保存到链表
	 * 2、如果源地址为0.且在会议中加入，则需要自动对其进行编号和席别设置
	 */
	if(type->s_id > 0)
	{
		if(conf_status_compare_id(type->s_id))
		{
			type->s_id = conf_status_find_max_id();

			confer_info->con_data.id = type->con_data.id = type->s_id;
			//将修改后的会议参数下发给单元机
			conf_info_set_conference_params(type->fd,type->con_data.id ,
					type->con_data.seat,NULL,NULL);

		}
		ccm_refresh_conference_list(confer_info);

		/*
		 * 判断会议状态，如果是中途加入的id大于0的设备，则须告知当前会议状态
		 */
//		dmanage_sync_conference_status(type);

	}else if(type->s_id == 0)
	{
		if((conf_status_get_conf_staus() ==
				WIFI_MEETING_EVENT_CON_MAG_START))
		{
			//会议过程中上线的设备，需要对其进行编号
			type->s_id = conf_status_find_max_id();
			confer_info->con_data.id = type->con_data.id = type->s_id;
			//将修改后的会议参数下发给单元机
			conf_info_set_conference_params(type->fd,type->con_data.id,
					type->con_data.seat,NULL,NULL);
			ccm_refresh_conference_list(confer_info);
		}

	}

	return SUCCESS;
}


/*
 * dmanage_sync_client_sys_time
 * 同步会议单元的系统时间
 *
 */
static int dmanage_sync_client_sys_time(Pframe_type type)
{
	int ret = 0;
	unsigned char msg[4] = {0};

	conf_status_get_sys_time(msg);

	type->msg_type = WRITE_MSG;
	type->data_type = EVENT_DATA;
	type->dev_type = HOST_CTRL;

	type->data_len = 4;

	type->evt_data.status = WIFI_MEETING_EVENT_UNIT_SYS_TIME;
	ret = tcp_ctrl_module_edit_info(type,msg);
	if(ret)
		return ERROR;

	return SUCCESS;
}

/*
 * ccm_add_info
 *
 */
int ccm_add_info(const unsigned char* msg,Pframe_type type)
{
	int tmp_fd = 0;
	int tmp_did = 0;

	/*
	 * 判断设备类型
	 */
	if(type->dev_type == UNIT_CTRL)
	{
		type->con_data.seat = msg[0];
		type->evt_data.electricity = msg[1];

//		type->evt_data.unet_info.mac = msg[2]<<24;
//		type->evt_data.unet_info.mac = type->evt_data.unet_info.mac+(msg[3]<<16);
		type->evt_data.unet_info.mac = type->evt_data.unet_info.mac+(msg[4]<<8);
		type->evt_data.unet_info.mac = type->evt_data.unet_info.mac+msg[5];
	}

	tmp_fd = type->fd;
	tmp_did = type->s_id;

	ccm_add_connected_info(type);

	/*
	 * 消息上报
	 *
	 * 添加心跳链表设备
	 */
	if(type->dev_type != PC_CTRL){

		/*
		 * 增加心跳设备
		 */
		chm_set_communication_heart(type);

		ccm_add_conference_info(type);
		/*
		 * 发送至本地上报队列
		 */
		tcp_ctrl_msg_send_to(type,msg,WIFI_MEETING_EVENT_ONLINE_REQ);

		type->fd = tmp_fd;
		type->d_id = tmp_did;
		type->s_id = 0;

		/*
		 * 下发系统时间给上线单元
		 */
		if(conf_status_get_pc_staus()>0)
		{
			dmanage_sync_client_sys_time(type);
		}


	}

	return SUCCESS;
}




