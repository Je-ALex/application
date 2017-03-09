/*
 * client_mic_status_info.c
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#include "audio_ring_buf.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"
#include "sys_uart_init.h"

//#include "client_mic_status_manage.h"


extern Pglobal_info node_queue;
extern Paudio_queue* rqueue;

unsigned int spk_ts = 0;

/*
 * dmanage_send_mic_status_to_pc
 * 话筒开关状态发送给上位机
 *
 */
int dmanage_send_mic_status_to_pc(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info pinfo;
	unsigned char msg[3] = {0};

	int status = type->evt_data.status;

	msg[0] = WIFI_MEETING_EVT_SPK;
	msg[1] = WIFI_MEETING_CHAR;

	type->msg_type = REQUEST_MSG;

	switch(type->evt_data.value)
	{
	case WIFI_MEETING_EVT_SPK_VETO:
	case WIFI_MEETING_EVT_SPK_CLOSE_MIC:
		msg[2] = WIFI_MEETING_EVT_SPK_CLOSE_MIC;
		break;
	case WIFI_MEETING_EVT_SPK_REQ_SPK:
		msg[2] = WIFI_MEETING_EVT_SPK_REQ_SPK;
		break;
	case WIFI_MEETING_EVT_SPK_OPEN_MIC:
		msg[2] = WIFI_MEETING_EVT_SPK_OPEN_MIC;
		break;
	}
	type->data_len = 3;

	if(conf_status_get_pc_staus() > 0)
	{
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
	}
	type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
	type->code_type[0] = WIFI_MEETING_CHAR;
	type->evt_data.status = status;
	type->msg_type = WRITE_MSG;
	type->dev_type = HOST_CTRL;

	return SUCCESS;

}




/*
 * conf_status_search_spk_node
 * 查找发言链表中的设备信息，时间戳最小的，将会返回fd，用于发送关闭
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 *
 */
int dmanage_search_first_spk_node(Pframe_type type)
{
	pclient_node tmp_node;
	Pas_port sinfo;

	unsigned int ts = 0xFFFFFFFF;

	/*
	 * 找出信息中的最小时间戳
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
			if((sinfo->ts < ts) &&
					(sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN))
			{
				ts = sinfo->ts;
			}
		}

		tmp_node=tmp_node->next;
	}
	if(ts < 0xffffffff)
	{
		/*
		 * 找到最小时间戳的配置信息
		 */
		tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
		while(tmp_node!=NULL)
		{
			sinfo = tmp_node->data;
			if(sinfo->sockfd)
			{
				if(sinfo->ts == ts)
				{
					type->fd = sinfo->sockfd;
					type->spk_port = sinfo->asport;
					break;
				}
			}

			tmp_node=tmp_node->next;
		}
	}

	return SUCCESS;
}

/*
 * 查找发言中，最后上线的设备
 *
 */
int dmanage_search_last_spk_node(Pframe_type type)
{
	pclient_node tmp_node;
	Pas_port sinfo;

	unsigned int ts = 0;

	/*
	 * 找出信息中的最大时间戳
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
			if((sinfo->ts > ts) &&
					(sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN))
			{
				ts = sinfo->ts;
			}
		}

		tmp_node=tmp_node->next;
	}
	if(ts > 0)
	{
		/*
		 * 找到最大设备的配置信息
		 */
		tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
		while(tmp_node!=NULL)
		{
			sinfo = tmp_node->data;
			if(sinfo->sockfd)
			{
				if(sinfo->ts == ts)
				{
					type->fd = sinfo->sockfd;
					type->spk_port = sinfo->asport;
					break;
				}
			}

			tmp_node=tmp_node->next;
		}
	}


	return SUCCESS;
}
/*
 * conf_status_search_not_use_spk_port
 * 查找音频没有使用的端口
 * 轮询查找发言管理链表中的信息，筛选出没有使用的端口号
 * 返回没有使用的端口号
 *
 */
int dmanage_search_not_use_spk_port(Pframe_type type)
{
	pclient_node tmp_node;
	Pas_port sinfo;

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

	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
			if(sinfo->seat != WIFI_MEETING_CON_SE_CHAIRMAN)
			{
				tmp_port = sinfo->asport - AUDIO_RECV_PORT;
				for(i=0;i<7;i++)
				{
					if(tmp_port == port[i][0])
					{
						printf("%s-%s-%d port=%d\n",__FILE__,__func__,
								__LINE__,port[i][0]);
						port[i][1] = tmp_port;
					}
				}
			}
		}
		tmp_node=tmp_node->next;
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
 * conf_status_close_last_spk_client
 * 关闭最后发言的端口号
 * 查找发言管理链表中，最后上线的设备
 * 将关闭消息下发给即将关闭的单元
 *
 */
int dmanage_close_last_spk_client(Pframe_type type)
{

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 查询会议中排位，关闭时间戳最大的单元
	 * 将端口下发给新申请的单元
	 */
	dmanage_search_last_spk_node(type);

	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	sys_uart_video_set(type->d_id,0);
	tcp_ctrl_module_edit_info(type,NULL);

	//关闭消息发送给上位机
	dmanage_send_mic_status_to_pc(type);

//	dmanage_delete_spk_node(type->fd);

	return SUCCESS;

}


/*
 * conf_status_close_first_spk_client
 * 关闭发言管理链表中最先上线的设备
 *
 * 查找链表中最先上线的设备
 * 将关闭消息下发给单元
 *
 */
int dmanage_close_first_spk_client(Pframe_type type)
{
	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 查询会议中排位，关闭时间戳最小的单元
	 * 将端口下发给新申请的单元
	 */
	dmanage_search_first_spk_node(type);


	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	sys_uart_video_set(type->d_id,0);

	tcp_ctrl_module_edit_info(type,NULL);
	//关闭消息发送给上位机
	dmanage_send_mic_status_to_pc(type);
//	dmanage_delete_spk_node(type->fd);

	return SUCCESS;
}

/*
 * conf_status_close_guest_spk_client
 * 关闭列席单元的发言端口
 *
 * 查找发言链表中，所有的列席单元
 * 下发关闭消息个单元
 */
int dmanage_close_guest_spk_client(Pframe_type type)
{
	pclient_node tmp_node;
	Pas_port sinfo;
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

			//dmanage_delete_spk_node(sinfo->sockfd);
			dmanage_send_mic_status_to_pc(type);
			tmp=conf_status_get_cspk_num();
			if(tmp)
			{
				tmp--;
				conf_status_set_cspk_num(tmp);
			}

			msleep(10);
		}
		tmp_node=tmp_node->next;

	}

	conf_status_set_spk_num(WIFI_MEETING_EVT_MIC_CHAIRMAN);

	return SUCCESS;
}


/*
 * conf_status_delete_spk_node
 * 关闭链表中的发言结点
 */
int dmanage_delete_spk_node(int fd)
{
	pclient_node tmp_node;
	pclient_node del;
	Pas_port sinfo;

	int pos,status;
	int num = 0;
	pos = status =0;

	/*
	 * 查找连接信息中，是否有该设备存在
	 */
	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd == fd)
		{
			/*
			 * 复位音频接收队列
			 */
			num = ((sinfo->asport -AUDIO_RECV_PORT)/2 + 1);
//			audio_queue_reset(rqueue[num]);
			conf_status_set_spk_buf_offset(num,0);
			conf_status_set_spk_timestamp(num,0);
			status++;
			break;
		}
		tmp_node=tmp_node->next;
		pos++;
	}

	if(status)
	{

		list_delete(node_queue->sys_list[CONFERENCE_SPK],pos,&del);
		sinfo = del->data;

		printf("%s-%s-%d,remove %d in spk list\n",__FILE__,__func__,__LINE__,
				sinfo->asport);

		free(sinfo);
		free(del);
	}else{
		printf("%s-%s-%d,not have spk client\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}

/*
 * conf_status_add_spk_node
 * 发言链表中增加发言结点
 */
int dmanage_add_spk_node(Pframe_type type)
{
	Pas_port sinfo;

	sinfo = malloc(sizeof(as_port));

	spk_ts++;

	sinfo->sockfd = type->fd;
	sinfo->seat = type->con_data.seat;
	sinfo->asport = type->spk_port;
	sinfo->ts = spk_ts;
	printf("%s-%s-%d fd:%d,asport:%d,ts:%d\n",__FILE__,__func__,__LINE__,
			sinfo->sockfd,sinfo->asport,sinfo->ts);
	/*
	 * 发言信息存入
	 */
	list_add(node_queue->sys_list[CONFERENCE_SPK],sinfo);

	return SUCCESS;

}


/*
 * dmanage_refresh_spk_node
 * 更新会议链表中的发言管理设备信息
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 *
 */
int cmsm_refresh_spk_node(Pframe_type type)
{
	int ret = 0;
	switch(type->evt_data.status)
	{
	case WIFI_MEETING_EVENT_SPK_ALLOW:
	case WIFI_MEETING_EVENT_SPK_REQ_SPK:
		ret = dmanage_add_spk_node(type);
		break;
	case WIFI_MEETING_EVENT_SPK_CLOSE_MIC:
		ret = dmanage_delete_spk_node(type->fd);
		break;
	}


	return ret;
}

