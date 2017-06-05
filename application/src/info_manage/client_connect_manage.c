
#include "wifi_sys_init.h"
#include "sys_uart_init.h"
#include "client_connect_manage.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "audio_ring_buf.h"
#include "client_mac_manage.h"
#include "client_heart_manage.h"
#include "client_mic_status_manage.h"

extern Pglobal_info node_queue;


/*
 * ccm_refresh_connected_list
 * 更新设备连接信息链表
 *
 * 查找连接信息链表，是否有此设备，需要将原结点移除，增加新的结点，并且更新文件内容
 *
 * 输入
 * Pconference_list
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
static int ccm_refresh_connected_list(Pconference_list conf_info)
{
	pclient_node tmp = NULL,del = NULL;
	Pclient_info cinfo = NULL,tcinfo = NULL;

	int pos = 0,status = 0,ret = 0;

	FILE* cfile;

	/*
	 * 首先在连接信息中搜寻fd
	 * 判断是否有此设备连接，删除存在结点，重新插入结点
	 * 更新
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == conf_info->fd)
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

		sys_list_delete(node_queue->sys_list[CONNECT_LIST],pos,&del);
		cinfo = del->data;
		memcpy(tcinfo,cinfo,sizeof(client_info));

		free(cinfo);
		free(del);
		cinfo = NULL;
		del = NULL;

		tcinfo->id = conf_info->ucinfo.id;
		tcinfo->seat = conf_info->ucinfo.seat;
		sys_list_add(node_queue->sys_list[CONNECT_LIST],tcinfo);

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
			tcinfo = tmp->data;
			if(tcinfo->client_fd > 0)
			{
				ret = fwrite(tcinfo,sizeof(client_info),1,cfile);
				if(ret != 1)
				{
					printf("%s-%s-%d,ERROR fd=%d,id=%d,seat=%d\n",__FILE__,__func__,
							__LINE__,tcinfo->client_fd, tcinfo->id,tcinfo->seat);
					return ERROR;
				}

			}
			tmp = tmp->next;
			usleep(100);
		}
		fclose(cfile);
	}

	return SUCCESS;
}


/*
 * ccm_refresh_conference_list
 * 更新会议信息链表
 *
 * 查找会议信息链表中的设备，移除原设备结点，更新新的会议信息
 *
 * 输入
 * Pconference_list
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
int ccm_refresh_conference_list(Pconference_list conf_info)
{
	pclient_node tmp = NULL,del = NULL;
	Pclient_info cinfo = NULL;
	Pconference_list finfo = NULL;

	int pos = 0,status = 0;

	/*
	 * 设置会议参数，需要判断是否当前有设备在连接信息链表中
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == conf_info->fd)
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
	 * 判断会议信息中的设备参数fd
	 * 判断是否有此设备连接，删除存在结点，重新插入结点
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;;
	while(tmp != NULL)
	{
		finfo = tmp->data;
		if(finfo->fd == conf_info->fd)
		{
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}
	if(status > 0)
	{
		sys_list_delete(node_queue->sys_list[CONFERENCE_LIST],pos,&del);
		finfo = del->data;

		free(finfo);
		free(del);
		finfo = NULL;
		del = NULL;

	}else{

		printf("%s-%s-%d,there is no data in the conference list,add it\n",
				__FILE__,__func__,__LINE__);
	}

	sys_list_add(node_queue->sys_list[CONFERENCE_LIST],conf_info);


	return SUCCESS;
}


/*
 * ccm_refresh_info
 * 更新设备信息
 *
 * 在主机或者上位机对单元进行了信息编码的时候，需要对主机相关管理链表进行更新操作
 * 对设备连接链表的信息进行更新，此链表主要是主机本地用于显示
 * 对会议信息链表进行更新操作，
 *
 */
int ccm_refresh_info(const Pframe_type type)
{
	Pconference_list conf_info = NULL;
	int ret = 0;

	/*
	 * 会议信息链表
	 */
	conf_info = (Pconference_list)malloc(sizeof(conference_list));
	memset(conf_info,0,sizeof(conference_list));
	conf_info->fd = type->fd;
	conf_info->ucinfo.id = type->ucinfo.id;
	conf_info->ucinfo.seat = type->ucinfo.seat;
	memcpy(conf_info->ucinfo.name,type->ucinfo.name,type->ucinfo.name_len);

	/*
	 * 更新到连接信息链表中
	 */
	ret = ccm_refresh_connected_list(conf_info);
	if(ret)
	{
		printf("%s-%s-%d,ccm_refresh_connected_list id=%d,err\n",__FILE__,__func__,__LINE__,
				conf_info->ucinfo.id);
		free(conf_info);
		return ERROR;
	}

	/*
	 * 更新到会议信息链表中
	 */
	ret = ccm_refresh_conference_list(conf_info);
	if(ret)
	{
		printf("%s-%s-%d,ccm_refresh_conference_list err\n",__FILE__,__func__,__LINE__);
		free(conf_info);
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
		sys_list_delete(node_queue->sys_list[CONNECT_LIST],pos,&del);
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
		sys_list_delete(node_queue->sys_list[CONFERENCE_LIST],pos,&del);
		cinfo = del->data;

		printf("%s-%s-%d remove %d in conference list\n",__FILE__,__func__,__LINE__,cinfo->fd);
		free(cinfo);
		free(del);
		cinfo = NULL;
		del = NULL;
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
				tmp_type.ucinfo.id = cinfo->id;
				tmp_type.ucinfo.seat = cinfo->seat;
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

		tmp_type.evt_data.status = CMSM_MIC_CLOSE;
		ret = cmsm_refresh_spk_node(&tmp_type);
		if(!ret)
		{
			tmp=conf_status_get_cspk_num();
			if(tmp)
			{
				tmp--;
				conf_status_set_cspk_num(tmp);
			}
			if(tmp_type.ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
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
 * 将临时结构中的信息保存到连接信息结构中，存入链表和文件中
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
	 * 区分上线的设备类型，单元机还是主机
	 */
	info = (Pclient_info)malloc(sizeof(client_info));
	memset(info,0,sizeof(client_info));


	info->client_fd = type->fd;
	getpeername(info->client_fd,(struct sockaddr*)&cli_addr,(socklen_t*)&clilen);
	info->cli_addr = cli_addr;
	info->clilen = clilen;
	sprintf(info->ip,"%s",inet_ntoa(cli_addr.sin_addr));

	if(type->dev_type == PC_CTRL)
	{

		info->client_name = PC_CTRL;
		conf_status_set_pc_staus(type->fd);
		//上位机ID号位0xffff，无席别
		if(type->s_id > 0)
		{
			info->id = type->s_id;
		}

	}else{

		info->client_name = UNIT_CTRL;
		info->seat = type->ucinfo.seat;
		info->id = type->s_id;
		info->client_power = type->evt_data.electricity;
		info->mac_addr = type->evt_data.unet_info.mac;

	}

	/*
	 * 检查该客户端是否已经存在
	 * 链表如果为空，则不需要进行检查，直接存储
	 * 链表不为空，则通过读取再比对，进行存储
	 */
	tmp_node = node_queue->sys_list[CONNECT_LIST]->next;
	do
	{
		if(tmp_node == NULL)
		{
			break;
		}else{
			pinfo = tmp_node->data;
			if(type->dev_type != PC_CTRL)
			{
				if(pinfo->mac_addr == type->evt_data.unet_info.mac)
				{
					state++;
					printf("%s-%s-%d,the client is exist\n",__FILE__,__func__,__LINE__);
					free(info);
					info = NULL;
					return ERROR;
				}
			}else{
					if(pinfo->client_fd == type->fd)
					{
						state++;
						printf("%s-%s-%d,the client is exist\n",__FILE__,__func__,__LINE__);
						free(info);
						info = NULL;
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
		sys_list_add(node_queue->sys_list[CONNECT_LIST],info);

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
 * 单元信息录入会议链表函数
 *
 * 单元上线主要分为两种
 * 一上线单元有ID号，则表示此单元可能有会议相关内容，则将内容更新到会议信息链表中
 *   判断单元ID与链表中的信息进行匹配，如果当前有此ID，则需要对其重新编号为最大ID
 *
 * 二上线单元没有ID号，如果是会议中上线的单元，则需要对此单元进行默认编号，用于会议管理
 *   判断是否是已经开始会议，开始会议则需要对其进行编号为最大ID
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 * 返回值
 * 成功
 * 失败
 *
 */
static int ccm_add_conference_info(Pframe_type type)
{
	Pconference_list conf_info = NULL;

	conf_info = (Pconference_list)malloc(sizeof(conference_list));
	memset(conf_info,0,sizeof(conference_list));

	/*
	 * 保存至会议信息链表
	 * 如果上线单元机有id号则将上线单元机信息进行存储
	 * 会议中上线的单元机，主机会自动给其分配ID号，用于管理
	 */
	conf_info->fd = type->fd;
	conf_info->ucinfo.id = type->ucinfo.id = type->s_id;
	conf_info->ucinfo.seat = type->ucinfo.seat;
	//判断当前是否有会议主席，如果有则上线的单元设置为客席，会议链表中只能存在一个主席
	if(conf_status_check_chariman_staus())
	{
		if(type->ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			conf_info->ucinfo.seat = type->ucinfo.seat = WIFI_MEETING_CON_SE_GUEST;
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
			type->s_id = conf_status_find_max_id() + 1;

			conf_info->ucinfo.id = type->ucinfo.id = type->s_id;
			//将修改后的会议参数下发给单元机
			conf_info_set_conference_params(type->fd,type->ucinfo.id ,
					type->ucinfo.seat,NULL,NULL);

		}else{
			ccm_refresh_conference_list(conf_info);
		}
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
			type->ucinfo.id = conf_status_find_max_id() + 1;
			//将修改后的会议参数下发给单元机
			conf_info_set_conference_params(type->fd,type->ucinfo.id,
					type->ucinfo.seat,NULL,NULL);
		}
		free(conf_info);
	}

	return SUCCESS;
}


/*
 * dmanage_sync_client_sys_time
 * 同步会议单元的系统时间
 *
 */
static int ccm_sync_client_sys_time(Pframe_type type)
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
 * 设备连接管理信息添加函数
 *
 * 设备上线后需要发送上线请求消息，通过此函数将上线消息内容保存到数据结构中
 * 在进行逻辑判断后存入设备连接链表和会议信息链表中
 *
 * 设备连接链表
 * 主要是将连接至主机的所有设备添加到连接信息链表，主要是连接的信息FD/IP/MAC/席别/电量等等
 *
 * 设备心跳链表
 * 将单元增加到心跳链表，主要是FD/HEART
 *
 * 会议信息链表
 * 将单元的信息添加到会议信息链表中FD/ID/会议内容
 *
 * 上线消息进行上报，本地上报和上位机上报，判断进行时间同步
 *
 * 输入
 * Pframe_type
 * 输出
 * 返回值
 * 成功
 * 失败
 */
int ccm_add_info(const unsigned char* msg,Pframe_type type)
{
	int tmp_fd = 0;
	int tmp_did = 0;
	int ret = 0;

	if(type->dev_type == UNIT_CTRL)
	{
		type->ucinfo.seat = msg[0];
		type->evt_data.electricity = msg[1];

//		type->evt_data.unet_info.mac = msg[2]<<24;
//		type->evt_data.unet_info.mac = type->evt_data.unet_info.mac+(msg[3]<<16);
//		type->evt_data.unet_info.mac = type->evt_data.unet_info.mac+(msg[4]<<8);
//		type->evt_data.unet_info.mac = type->evt_data.unet_info.mac+msg[5];
		type->evt_data.unet_info.mac = msg[4]<<8 | msg[5];
	}

	tmp_fd = type->fd;
	tmp_did = type->s_id;

	ret = ccm_add_connected_info(type);
	if(ret)
	{
		return ERROR;
	}

	if(type->dev_type != PC_CTRL)
	{
		chm_set_communication_heart(type);
		ccm_add_conference_info(type);

		tcp_ctrl_msg_send_to(type,msg,WIFI_MEETING_EVENT_ONLINE_REQ);

		type->fd = tmp_fd;
		type->d_id = tmp_did;
		type->s_id = 0;

		if(conf_status_get_pc_staus() > 0)
		{
			ccm_sync_client_sys_time(type);
		}
	}

	return SUCCESS;
}




