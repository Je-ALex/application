/*
 * client_mic_status_info.c
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 *
 *
 * 发言管理模块处理文件
 *
 * 通过解析事件类型进行发言管理的处理，细分消息类型
 * 主要是单元请求、主席的应答以及模式相应的处理
 *
 */


#include "wifi_sys_init.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "audio_ring_buf.h"
#include "sys_uart_init.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_api.h"
#include "client_mic_status_manage.h"


extern Pglobal_info node_queue;

unsigned int spk_ts = 0;



//TODO 链表操作相关
/*
 * cmsm_delete_spk_node
 * 关闭链表中的发言单元
 *
 * 通过fd进行筛选，关闭对应 单元
 *
 * 输入
 * fd
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_delete_spk_node(int fd)
{
	pclient_node tmp_node = NULL,del = NULL;
	Pas_port sinfo = NULL;

	int pos = 0,status = 0;

	int num = 0;
	/*
	 * 查找连接信息中，是否有该设备存在
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd == fd)
		{
			if(sinfo->asport > 0)
			{
				num = (sinfo->asport-AUDIO_RECV_PORT)/2 + 1;
				conf_status_set_spk_buf_offset(num,0);
				conf_status_set_spk_timestamp(num,0);
			}
			status++;
			break;
		}
		tmp_node = tmp_node->next;
		pos++;
	}

	if(status)
	{
		sys_list_delete(node_queue->sys_list[CONFERENCE_SPK],pos,&del);
		sinfo = del->data;

		printf("%s-%s-%d,remove %d in spk list\n",__FILE__,__func__,__LINE__,
				sinfo->asport);

		free(sinfo);
		free(del);
		sinfo = NULL;
		del = NULL;
	}else{
		printf("%s-%s-%d,not have spk client\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}

/*
 * cmsm_add_spk_node
 * 发言链表中增加发言单元结点
 *
 * 将单元话筒状态存入链表中
 * 主要是相关的fd/席别/发言端口/时间戳/当前状态
 *
 * 输入
 * Pframe_type
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_add_spk_node(Pframe_type type)
{
	Pas_port sinfo = NULL;

	sinfo = (Pas_port)malloc(sizeof(as_port));
	memset(sinfo,0,sizeof(as_port));

	spk_ts++;
	sinfo->oldfd = type->oldfd;
	sinfo->sockfd = type->fd;
	sinfo->seat = type->ucinfo.seat;
	sinfo->asport = type->spk_port;
	sinfo->ts = spk_ts;
	sinfo->status = type->evt_data.status;

	printf("%s-%s-%d fd:%d,asport:%d,ts:%d,status:%d\n",__FILE__,__func__,__LINE__,
			sinfo->sockfd,sinfo->asport,sinfo->ts,sinfo->status);
	/*
	 * 发言信息存入
	 */
	sys_list_add(node_queue->sys_list[CONFERENCE_SPK],sinfo);

	return SUCCESS;

}

/*
 * cmsm_search_spk_node
 * 查找链表中的保存的单元
 *
 * 通过fd判断当前链表中是否有此设备了
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_search_spk_node(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;
	int pos = 0;

	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd == type->fd)
		{
			pos++;
			break;
		}
		tmp_node=tmp_node->next;
	}

	return pos;
}

/*
 * cmsm_update_spk_node
 * 更新链表信息
 *
 * 通过fd判断当前链表中的设备，对此设备进行信息更新操作
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_update_spk_node(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;

//	pclient_node del = NULL;
//	Pas_port ninfo = NULL;

	int pos,status;
//	int tmp_fd = 0;
//	int tmp_ts = 0;
//	int tmp_seat = 0;
//	int tmp_port = 0;

	pos = status =0;

	/*
	 * 查找连接信息中，是否有该设备存在
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd == type->fd)
		{
//			status++;
//			tmp_fd = sinfo->sockfd;
//			tmp_ts = sinfo->ts;
//			tmp_seat = sinfo->seat;
//			tmp_port = sinfo->asport;
			sinfo->status = CMSM_OPEN_SUCCESS;
			sinfo->oldfd = -1;
			printf("%s-%s-%d fd:%d,asport:%d,ts:%d,status:%d\n",__FILE__,__func__,__LINE__,
					sinfo->sockfd,sinfo->asport,sinfo->ts,sinfo->status);
			break;
		}
		tmp_node=tmp_node->next;
		pos++;
	}

//	if(status)
//	{
//
//		list_delete(node_queue->sys_list[CONFERENCE_SPK],pos,&del);
//		sinfo = del->data;
//
//		printf("%s-%s-%d,remove %d in spk list\n",__FILE__,__func__,__LINE__,
//				sinfo->asport);
//
//		free(sinfo);
//		free(del);
//		sinfo = NULL;
//		del = NULL;
//
//		ninfo = (Pas_port)malloc(sizeof(as_port));
//		memset(ninfo,0,sizeof(as_port));
//		ninfo->sockfd = tmp_fd;
//		ninfo->seat = tmp_seat;
//		ninfo->asport = tmp_port;
//		ninfo->ts = tmp_ts;
//		ninfo->status = CMSM_OPEN_SUCCESS;
//
//		printf("%s-%s-%d fd:%d,asport:%d,ts:%d,status:%d\n",__FILE__,__func__,__LINE__,
//				ninfo->sockfd,ninfo->asport,ninfo->ts,ninfo->status);
//		/*
//		 * 发言信息存入
//		 */
//		list_add(node_queue->sys_list[CONFERENCE_SPK],ninfo);
//
//	}else{
//		printf("%s-%s-%d,not have spk client\n",__FILE__,__func__,__LINE__);
//		return ERROR;
//	}


	return SUCCESS;
}

/*
 * cmsm_refresh_spk_node
 * 单元话筒开关状态链表信息管理接口函数
 *
 * 主要通过状态对信息进行分类处理
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
int cmsm_refresh_spk_node(Pframe_type type)
{
	int ret = -1;

	switch(type->evt_data.status)
	{
	case CMSM_OPEN_SUCCESS:
	case CMSM_WAIT_REPLY:
		ret = cmsm_add_spk_node(type);
		break;
	case CMSM_UPDATE_REPLY:
		ret = cmsm_update_spk_node(type);
		break;
	case CMSM_MIC_CLOSE:
		ret = cmsm_delete_spk_node(type->fd);
		break;
	}

	return ret;
}


//TODO 逻辑处理相关

/*
 * cmsm_port_status_sendto_pc
 * 单元话筒开关状态上报函数
 *
 * 此函数主要是将单元的话筒开关状态上报给PC,用于进行服务器的同步
 *
 * 输入
 * Pframe_type
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_port_status_sendto_pc(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info pinfo = NULL;

	unsigned char msg[3] = {0};
	int t_index = 0;
	int tmp_fd = type->fd;

	type->msg_type = REQUEST_MSG;
	type->dev_type = HOST_CTRL;

	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->code_type[0] = WIFI_MEETING_CHAR;


	if(conf_status_get_pc_staus() > 0)
	{
		msg[t_index++] = WIFI_MEETING_EVT_SPK;
		msg[t_index++] = WIFI_MEETING_CHAR;
		switch(type->evt_data.value)
		{
		case WIFI_MEETING_EVT_SPK_VETO:
		case WIFI_MEETING_EVT_SPK_CLOSE_MIC:
			msg[t_index++] = WIFI_MEETING_EVT_SPK_CLOSE_MIC;
			break;
		case WIFI_MEETING_EVT_SPK_REQ_SPK:
			msg[t_index++] = WIFI_MEETING_EVT_SPK_REQ_SPK;
			break;
		case WIFI_MEETING_EVT_SPK_CLOSE_REQ:
			msg[t_index++] = WIFI_MEETING_EVT_SPK_CLOSE_REQ;
			break;
		case WIFI_MEETING_EVT_SPK_OPEN_MIC:
			msg[t_index++] = WIFI_MEETING_EVT_SPK_OPEN_MIC;
			break;
		}

		type->data_len = t_index;

		tmp=node_queue->sys_list[CONNECT_LIST]->next;;
		while(tmp != NULL)
		{
			pinfo = tmp->data;

			if(pinfo->client_fd == type->fd)
			{
				type->evt_data.unet_info.mac = pinfo->mac_addr;
				break;
			}
			tmp = tmp->next;
		}

		tmp=node_queue->sys_list[CONNECT_LIST]->next;;
		while(tmp != NULL)
		{
			pinfo = tmp->data;

			if((pinfo->client_name == PC_CTRL) &&
					(conf_status_get_pc_staus() == pinfo->client_fd))
			{
				type->evt_data.status = WIFI_MEETING_HOST_REP_TO_PC;
				//上报上位机的源地址统一为sockfd
				type->d_id = PC_ID;
				type->s_id = type->evt_data.unet_info.mac;
				type->fd = pinfo->client_fd;
				tcp_ctrl_module_edit_info(type,msg);
				break;
			}
			tmp = tmp->next;
		}

		type->fd = tmp_fd;
	}

	return SUCCESS;

}


/*
 * cmsm_search_not_use_spk_port
 * 查找音频链表中没有使用端口号
 *
 * 通过端口链表查找没有使用的端口信息，将端口信息返回
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 *
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_search_not_use_spk_port(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;

	int i;
	int tmp_port = 0;
	int port[8][2] = {
			{2,0},
			{4,0},
			{6,0},
			{8,0},
			{10,0},
			{12,0},
			{14,0},
			{16,0}
	};

	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node != NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
			if(sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN)
			{
				tmp_port = sinfo->asport - AUDIO_RECV_PORT;
				for(i=0;i<8;i++)
				{
					if(tmp_port == port[i][0])
					{
						printf("%s-%s-%d used port=%d\n",__FILE__,__func__,
								__LINE__,port[i][0]);
						port[i][1] = tmp_port;
					}
				}
			}
		}
		tmp_node = tmp_node->next;
	}
	for(i=0;i<conf_status_get_spk_num();i++)
	{
		if(!port[i][1])
		{
			type->spk_port = port[i][0] + AUDIO_RECV_PORT;
			break;
		}
	}

	return SUCCESS;
}


/*
 * cmsm_search_first_spk_node
 * 查找发言链表中的设备信息，当前状态下最先打开话筒的单元，得到此单元的fd和发言端口号
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_search_first_spk_node(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;

	int status = 0;
	unsigned int ts = 0xFFFFFFFF;

	/*
	 * 找出信息中的最小时间戳
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node != NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
			if((sinfo->ts < ts) && (sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN)
					&& (sinfo->status == CMSM_OPEN_SUCCESS))
			{
				ts = sinfo->ts;
				status++;
			}
		}

		tmp_node = tmp_node->next;
	}
	if(ts < 0xffffffff)
	{
		/*
		 * 找到最小时间戳的配置信息
		 */
		tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
		while(tmp_node != NULL)
		{
			sinfo = tmp_node->data;
			if(sinfo->sockfd)
			{
				if(sinfo->ts == ts)
				{
					type->fd = sinfo->sockfd;
					type->spk_port = sinfo->asport;
					sinfo->status = CMSM_PREPARE_CLOSE;
					break;
				}
			}

			tmp_node = tmp_node->next;
		}
	}

	return status;
}

/*
 * cmsm_search_last_spk_node
 * 查找发言链表中，当前状态下，端口号最大的设备
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 *
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_search_last_spk_node(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;

	unsigned int ts = 0;

	/*
	 * 找出信息中的最大时间戳
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node != NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
//			if((sinfo->ts > ts) && (sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN)
//					&& (sinfo->status == CMSM_OPEN_SUCCESS))
//			{
//				ts = sinfo->ts;
//			}

			if((sinfo->asport > ts) && (sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN)
					&& (sinfo->status == CMSM_OPEN_SUCCESS))
			{
				ts = sinfo->asport;
			}

		}

		tmp_node = tmp_node->next;
	}
	if(ts > 0)
	{
		/*
		 * 找到最大设备的配置信息
		 */
		tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
		while(tmp_node != NULL)
		{
			sinfo = tmp_node->data;
			if(sinfo->sockfd)
			{
//				if(sinfo->ts == ts)
//				{
//					type->fd = sinfo->sockfd;
//					type->spk_port = sinfo->asport;
//					break;
//				}
				if(sinfo->asport == ts)
				{
					type->fd = sinfo->sockfd;
					type->spk_port = sinfo->asport;
					break;
				}
			}

			tmp_node = tmp_node->next;
		}
	}


	return SUCCESS;
}

/*
 * cmsm_close_first_spk_client
 * 关闭发言管理链表中最先开启的设备
 *
 * 查找设备中最先开启的单元，将关闭消息发送给此单元
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 *
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_close_first_spk_client(Pframe_type type)
{
	/*
	 * 查询会议中排位，关闭时间戳最小的单元
	 * 将端口下发给新申请的单元
	 */
	cmsm_search_first_spk_node(type);

	type->msg_type = WRITE_MSG;
	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	conf_status_camera_track_postion(type->d_id,0);
//	cmsm_delete_spk_node(type->fd);
	tcp_ctrl_module_edit_info(type,NULL);
	//关闭消息发送给上位机

//	cmsm_port_status_sendto_pc(type);

	return SUCCESS;
}

/*
 * cmsm_close_last_spk_client
 * 关闭发言链表中最后开启的设备
 *
 * 找到发言链表中最后开启的单元，将关闭信息下发给此单元，上报给PC
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_close_last_spk_client(Pframe_type type)
{
	/*
	 * 查询会议中排位，关闭时间戳最大的单元
	 * 将端口下发给新申请的单元
	 */
	cmsm_search_last_spk_node(type);
	type->msg_type = WRITE_MSG;
	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);

	conf_status_camera_track_postion(type->d_id,0);
	tcp_ctrl_module_edit_info(type,NULL);

	//关闭消息发送给上位机
	cmsm_port_status_sendto_pc(type);

//	cmsm_delete_spk_node(type->fd);

	return SUCCESS;

}

/*
 * cmsm_close_guest_spk_client
 * 关闭列席单元的发言端口
 *
 * 查找发言链表中的列席单元，关闭列席单元
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
static int cmsm_close_guest_spk_client(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;
	int tmp = 0;

	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	/*
	 * 查找连接信息中，是否有该设备存在
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			tcp_ctrl_source_dest_setting(-1,sinfo->sockfd,type);
			type->fd = sinfo->sockfd;
			tcp_ctrl_module_edit_info(type,NULL);

//			cmsm_delete_spk_node(sinfo->sockfd);
			cmsm_port_status_sendto_pc(type);
			tmp = conf_status_get_cspk_num();
			if(tmp > 0)
			{
				tmp--;
				conf_status_set_cspk_num(tmp);
			}

			msleep(10);
		}
		tmp_node = tmp_node->next;

	}

	conf_status_set_spk_num(WIFI_MEETING_EVT_MIC_CHAIRMAN);

	return SUCCESS;
}
/*
 * cmsm_through_reply_proc_port
 * 话筒开关状态应答函数处理接口
 *
 * 主要是通过单元返回的412信息，对相应的单元进行链表结点删除和新单元链表信息更新操作
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 * 返回值
 * 成功
 * 失败
 */
int cmsm_through_reply_proc_port(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pas_port sinfo = NULL;

	int status = 0;
	int tmp_fd = 0;
	int tmp_port = 0;


	cmsm_delete_spk_node(type->fd);
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
	cmsm_port_status_sendto_pc(type);

	/*
	 * 查找连接信息中，是否有该设备存在
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->status != CMSM_OPEN_SUCCESS
				&& sinfo->oldfd == type->fd)
		{
			tmp_fd = sinfo->sockfd;
			tmp_port = sinfo->asport;
			status++;
			break;
		}
		tmp_node = tmp_node->next;
	}

	if(status)
	{
		type->fd = tmp_fd;
		type->msg_type = WRITE_MSG;
		type->dev_type = HOST_CTRL;
		type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
		type->code_type[0] = WIFI_MEETING_CHAR;
		type->spk_port = tmp_port;

		type->evt_data.status = CMSM_UPDATE_REPLY;
		cmsm_refresh_spk_node(type);

		tcp_ctrl_source_dest_setting(-1,type->fd,type);
		//摄像跟踪设置
		conf_status_camera_track_postion(type->d_id,1);

		tcp_ctrl_module_edit_info(type,NULL);

		//告知上位机有设备打开话筒
		type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
		cmsm_port_status_sendto_pc(type);
	}

	return SUCCESS;
}


/*
 * cmsm_chairman_port
 * 主席单元发言端口处理函数
 *
 * 主要是对主席话筒开关状态进行处理
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
static int cmsm_chairman_port(Pframe_type type)
{
	int csn = 0;
	int tmp_fd = type->fd;

	/*
	 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
	 * 人数饱满，则需要关闭最后上线的设备
	 */
	if(conf_status_get_cspk_num() == conf_status_get_spk_num())
	{
		cmsm_close_last_spk_client(type);

		cmsm_delete_spk_node(type->fd);

	}else{
		csn = conf_status_get_cspk_num();
		csn++;
		conf_status_set_cspk_num(csn);
	}

	conf_status_set_cmspk(WIFI_MEETING_CON_SE_CHAIRMAN);

	type->spk_port = AUDIO_RECV_PORT;
	type->fd = tmp_fd;
	type->msg_type = WRITE_MSG;
	type->dev_type = HOST_CTRL;
	type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
	type->code_type[0] = WIFI_MEETING_CHAR;

	type->evt_data.status = CMSM_OPEN_SUCCESS;
	cmsm_refresh_spk_node(type);

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	//摄像跟踪设置
	conf_status_camera_track_postion(type->d_id,1);

	tcp_ctrl_module_edit_info(type,NULL);

	//告知上位机有设备打开话筒
	type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
	cmsm_port_status_sendto_pc(type);


	return SUCCESS;
}


/*
 * cmsm_guest_port
 * 其他单元发言端口处理函数
 *
 * 主要是对其他单元发言端口进行处理
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
static int cmsm_guest_port(Pframe_type type)
{
	int tmp_fd = type->fd;
	int csn = 0;

	/*
	 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
	 * 已达到上限，则需要关闭第一个单元
	 * 轮询发言链表中的设备，比较时间戳， 时间戳最小的，表示最早加入设备，需要关闭
	 */
	if(conf_status_get_cspk_num() == conf_status_get_spk_num())
	{
		cmsm_close_first_spk_client(type);

		type->oldfd = type->fd;
		type->fd = tmp_fd;
		type->evt_data.status = CMSM_WAIT_REPLY;
		cmsm_refresh_spk_node(type);

	}else{

		/*
		 * 人数没有饱和，则查找没有使用的端口
		 */
		cmsm_search_not_use_spk_port(type);

		csn = conf_status_get_cspk_num();
		csn++;
		conf_status_set_cspk_num(csn);

		type->fd = tmp_fd;
		type->msg_type = WRITE_MSG;
		type->dev_type = HOST_CTRL;
		type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
		type->code_type[0] = WIFI_MEETING_CHAR;

		type->evt_data.status = CMSM_OPEN_SUCCESS;
		cmsm_refresh_spk_node(type);

		tcp_ctrl_source_dest_setting(-1,type->fd,type);
		//摄像跟踪设置
		conf_status_camera_track_postion(type->d_id,1);

		tcp_ctrl_module_edit_info(type,NULL);

		//告知上位机有设备打开话筒
		type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
		cmsm_port_status_sendto_pc(type);
	}

	return SUCCESS;
}

/*
 * cmsm_spk_port_status_onoff
 * 发言端口的管理函数
 *
 * 对单元的席别进行分类，分别进行处理
 *
 * 输入
 * tmp_type
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 */
static int cmsm_spk_port_status_onoff(Pframe_type type)
{

	/*
	 * 判断此单元状态是否已在链表中更新
	 */
	if(!cmsm_search_spk_node(type))
	{
		switch(type->ucinfo.seat)
		{
			case WIFI_MEETING_CON_SE_CHAIRMAN:
				cmsm_chairman_port(type);
				break;
			case WIFI_MEETING_CON_SE_GUEST:
			case WIFI_MEETING_CON_SE_ATTEND:
				cmsm_guest_port(type);
				break;
		}

	}else{
		printf("%s-%s-%d the unit %d request already process\n",__FILE__,__func__,
				__LINE__,type->fd);
	}


	return SUCCESS;

}

/*
 * cmsm_spk_other_status_onoff
 * 其他状态的处理函数
 *
 * 单元的话筒关闭/请求发言关闭(保留)/主席优先模式开关
 *
 * 输入
 * type
 * msg
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_spk_other_status_onoff(Pframe_type type,const unsigned char* msg)
{
	int pos = 0;
	int tmp = 0;
	int tmp_fd = type->fd;

	switch(type->evt_data.status)
	{
		case WIFI_MEETING_EVENT_SPK_CLOSE_MIC:
		{
			conf_status_camera_track_postion(type->s_id,0);
			tmp=conf_status_get_cspk_num();
			if(tmp>0)
			{
				tmp--;
				conf_status_set_cspk_num(tmp);
			}

			if(type->ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
			{
				conf_status_set_cmspk(WIFI_MEETING_CON_SE_GUEST);
			}
			type->evt_data.status = CMSM_MIC_CLOSE;
			cmsm_refresh_spk_node(type);
			cmsm_port_status_sendto_pc(type);
			break;
		}
		/*
		 * 单元请求发言关闭
		 * 需要将请求关闭的消息推送给主席，主席清空消息
		 */
		case WIFI_MEETING_EVENT_SPK_CLOSE_REQ:
		{
			pos = conf_status_find_chairman_sockfd(type);
			if(pos > 0){
				//源地址为请求单元机id，目标地址修改为主机单元id
				tcp_ctrl_source_dest_setting(tmp_fd,type->fd,type);
				tcp_ctrl_module_edit_info(type,msg);
			}else{
				printf("%s-%s-%d not have chariman \n",__FILE__,__func__,__LINE__);
			}
			cmsm_port_status_sendto_pc(type);
			break;
		}
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON:
		{
			cmsm_close_guest_spk_client(type);
			break;
		}
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_OFF:
		{
			conf_status_set_spk_num(DEF_SPK_NUM);
			break;
		}

	}
	return SUCCESS;
}

/*
 * cmsm_handle_spk_req
 * 请求发言接口处理函数
 *
 * 主席的发言优先级是最高的，所以先判断主席状态
 * 其它单元的发言需要根据发言人数模式和话筒模式进行判断
 *
 * 发言人数设为1人，则默认为主席优先模式，只能主席发言
 *
 * 输入
 * frame_type
 * msg
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_handle_spk_request(Pframe_type type,const unsigned char* msg)
{
	int pos = 0;
	int tmp_fd = type->fd;//保存源fd

	/*
	 * 判断申请发言单元的席别，如果是主席，则直接进行处理
	 * 如果是其它单元，则需要判断发言人数和话筒模式
	 */
	if(type->ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN){
		cmsm_spk_port_status_onoff(type);
	}else{
		/*
		 * 判断发言人数，如果发言人数为1，则表示主席优先模式打开
		 * 发言人数为1 ，则直接拒绝单元申请
		 *
		 * 发言人数大于1，则需要判断话筒模式和发言人数
		 * FIFO模式，则直接关闭最先发言单元，新单元加入
		 * 其他模式，先判断发言人数，饱和则需要通知主席
		 */
		if(conf_status_get_spk_num() == WIFI_MEETING_EVT_MIC_CHAIRMAN)
		{
			//直接回复单元，拒绝其发言
			type->msg_type = WRITE_MSG;
			type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
			tcp_ctrl_source_dest_setting(-1,type->fd,type);

			tcp_ctrl_module_edit_info(type,NULL);

		}else if(conf_status_get_cspk_num() == conf_status_get_spk_num()
				&& conf_status_get_mic_mode() != WIFI_MEETING_EVT_MIC_FIFO){

			/*
			 * 将请求消息发送给主席，无主席和上位机，则直接发送拒绝消息给单元
			 */
			pos = conf_status_find_chairman_sockfd(type);
			if(pos > 0){

				//源地址为请求单元机id，目标地址修改为主机单元id
				tcp_ctrl_source_dest_setting(tmp_fd,type->fd,type);
				tcp_ctrl_module_edit_info(type,msg);
			}else{
				//没有主席和上位机，告知单元，拒绝其发言
				printf("%s-%s-%d not find chairman and pc\n",__FILE__,__func__,__LINE__);

				type->msg_type = WRITE_MSG;
				type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
				tcp_ctrl_source_dest_setting(-1,type->fd,type);

				tcp_ctrl_module_edit_info(type,NULL);

			}
			//请求消息发送给上位机
			type->fd = tmp_fd;
			cmsm_port_status_sendto_pc(type);

		}else{
			//当前发言人数小于设置人数
			cmsm_spk_port_status_onoff(type);
		}
	}

	return SUCCESS;
}

/*
 * cmsm_handle_chairm_rreply
 * 主席应答请求消息处理函数
 *
 * 单元的请求消息收到后，有需要主席批准的消息，需要进行处理
 * 判断是否是主席的应答，通过目标地址找到需要进行批复的单元
 * 下发控制应答消息，如果是批准，还需下发端口信息
 *
 * 输入
 * frame_type
 * msg
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_handle_request_reply(Pframe_type type,const unsigned char* msg)
{
	int pos = 0;

	//允许和拒绝消息只能是主席或上位机发送
	pos = conf_status_check_chairman_legal(type) | \
			conf_status_check_pc_legal(type);

	if(pos > 0){
		//找到目标地址对应的单元
		pos = conf_status_find_did_sockfd_id(type);
		if(pos > 0)
		{
			//变换为控制类消息发送给单元
			type->msg_type = WRITE_MSG;
			tcp_ctrl_source_dest_setting(-1,type->fd,type);
			tcp_ctrl_module_edit_info(type,msg);
			//设置单元音频端口信息
			if(type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALLOW)
			{
				cmsm_spk_port_status_onoff(type);
			}

		}else{
			printf("%s-%s-%d not the find the unit device\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

	}else{
		printf("%s-%s-%d not the chairman or PC command\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}

/*
 * cmsm_handle_snd_brd
 * 音频下发处理接口
 *
 * 音频下发前期设计为请求/允许/拒绝三个接口，目前为请求就应答
 *
 * 输入
 * frame_type
 * msg
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 *
 */
static int cmsm_handle_snd_brd(Pframe_type type,const unsigned char* msg)
{
	int pos = 0;

	type->msg_type = WRITE_MSG;
	type->dev_type = HOST_CTRL;

	type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
	type->code_type[0] = WIFI_MEETING_CHAR;

	switch(type->evt_data.status)
	{
	case WIFI_MEETING_EVENT_SPK_REQ_SND:
		conf_status_set_snd_brd(WIFI_MEETING_EVENT_SPK_REQ_SND);
		type->brd_port = AUDIO_SEND_PORT;
		tcp_ctrl_module_edit_info(type,NULL);
		break;
	case WIFI_MEETING_EVENT_SPK_ALOW_SND:
	case WIFI_MEETING_EVENT_SPK_VETO_SND:
		//音频请求接口，暂时不做判断
		pos = conf_status_check_chairman_legal(type);
		if(pos > 0){
			pos = 0;
			pos = conf_status_find_did_sockfd_id(type);
			if(pos > 0)
			{
				//变换为控制类消息下个给单元机
				type->msg_type = WRITE_MSG;
				tcp_ctrl_source_dest_setting(-1,type->fd,type);
				tcp_ctrl_module_edit_info(type,msg);
				//设置单元音频端口信息
				if(type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALOW_SND)
					cmsm_spk_other_status_onoff(type,msg);
			}else{
				printf("%s-%s-%d not the find the unit device\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}

		}else{
			printf("%s-%s-%d not the chairman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
		break;
	case WIFI_MEETING_EVENT_SPK_CLOSE_SND:
	{
		conf_status_set_snd_brd(WIFI_MEETING_EVENT_SPK_CLOSE_SND);
		break;
	}
	}

	return SUCCESS;
}

/*
 * cmsm_msg_classify_handle
 * 发言请求消息的分类处理函数
 *
 * 发言模式主要有三种:FIFO/标准模式/自由模式/主席优先
 * 发言人数设置有:1/2/4/6/8
 *
 * 目前单元的请求消息类型有:
 * 普通单元:
 * 请求发言/申请音频下发/话筒关闭/音频关闭/请求关闭/
 * 主席单元:
 * 允许发言/拒绝发言/主席优先打开/主席优先关闭
 *
 * 此函数主要是针对不同消息类型进行处理，对于发言请求类，需要结合当前发言模式进行进一步处理
 * 理论上应该是一个事件对应一个接口函数
 *
 * 输入
 * frame_type数据参数结构体
 * msg消息数据包
 *
 * 输出
 * 无
 *
 * 返回值
 * 成功
 * 失败
 *
 */
int cmsm_msg_classify_handle(Pframe_type frame_type,const unsigned char* msg)
{

	switch(frame_type->evt_data.status)
	{
		case WIFI_MEETING_EVENT_SPK_REQ_SPK:
			cmsm_handle_spk_request(frame_type,msg);
			break;
		case WIFI_MEETING_EVENT_SPK_ALLOW:
		case WIFI_MEETING_EVENT_SPK_VETO:
			cmsm_handle_request_reply(frame_type,msg);
			break;
		case WIFI_MEETING_EVENT_SPK_REQ_SND:
		case WIFI_MEETING_EVENT_SPK_ALOW_SND:
		case WIFI_MEETING_EVENT_SPK_VETO_SND:
		case WIFI_MEETING_EVENT_SPK_CLOSE_SND:
			cmsm_handle_snd_brd(frame_type,msg);
			break;
		case WIFI_MEETING_EVENT_SPK_CLOSE_MIC:
		case WIFI_MEETING_EVENT_SPK_CLOSE_REQ:
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON:
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_OFF:
			cmsm_spk_other_status_onoff(frame_type,msg);
			break;
	}

	return SUCCESS;
}







