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
frame_type spk_tmp;


/*
 * TODO 发言管理端口链表信息管理
 */

/*
 * dmanage_send_mic_status_to_pc
 * 单元话筒发言状态上报函数
 *
 *
 */
int dmanage_send_mic_status_to_pc(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info pinfo;
	unsigned char msg[3] = {0};
	int tmp_fd = type->fd;
	int status = type->evt_data.status;

	type->msg_type = REQUEST_MSG;

	msg[0] = WIFI_MEETING_EVT_SPK;
	msg[1] = WIFI_MEETING_CHAR;

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
	type->fd = tmp_fd;

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
	type->msg_type = WRITE_MSG;
	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);

	conf_status_camera_track_postion(type->d_id,0);
	tcp_ctrl_module_edit_info(type,NULL);
//	dmanage_delete_spk_node(type->fd);
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

	type->msg_type = WRITE_MSG;
	type->name_type[0] = WIFI_MEETING_EVT_SPK;
	type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	conf_status_camera_track_postion(type->d_id,0);
//	dmanage_delete_spk_node(type->fd);
	tcp_ctrl_module_edit_info(type,NULL);
	//关闭消息发送给上位机
	dmanage_send_mic_status_to_pc(type);


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
			if(tmp>0)
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
 * cmsm_search_spk_node
 * 查找链表中的保存的单元
 */
int cmsm_search_spk_node(Pframe_type type)
{
	pclient_node tmp_node;
	Pas_port sinfo;
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




/*
 * TODO 话筒管理逻辑处理
 */

int tcp_ctrl_uevent_spk_port_reply(int fd)
{

	if(spk_tmp.spk_port)
	{
		dmanage_delete_spk_node(fd);

		cmsm_refresh_spk_node(&spk_tmp);

		tcp_ctrl_module_edit_info(&spk_tmp,NULL);
	}

	if(spk_tmp.evt_data.status == WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON)
	{
		dmanage_delete_spk_node(fd);
	}
	memset(&spk_tmp,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	return SUCCESS;
}

int conf_status_set_spk_now_fd(Pframe_type type)
{
	memcpy(&spk_tmp,type,sizeof(frame_type));

	return SUCCESS;
}


/*
 * cmsm_handle_spk_port
 * 发言端口处理函数
 *
 * 此接口是将端口号下发给相应的单元，并且将对应的关闭信息发送给被关闭的单元
 *
 * 此函数处理的就是两件事，端口关闭和端口打开信息的处理
 *
 *
 *
 */
int cmsm_handle_spk_port(Pframe_type type)
{
	int tmp = 0;
	int tmp_port = 0;
	int cnum = 0;
	int tmp_fd = type->fd;
	int tmp2;

	memset(&spk_tmp,0,sizeof(frame_type));

	type->msg_type = WRITE_MSG;
	type->dev_type = HOST_CTRL;

	type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
	type->code_type[0] = WIFI_MEETING_CHAR;

	switch(type->evt_data.status)
	{
	case WIFI_MEETING_EVENT_SPK_ALLOW:
	case WIFI_MEETING_EVENT_SPK_REQ_SPK:
	{
		if(type->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			tmp_port = AUDIO_RECV_PORT;
			/*
			 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
			 * 人数饱满，则需要关闭最后上线的设备
			 */
			if(conf_status_get_cspk_num() == conf_status_get_spk_num())
			{
				dmanage_close_last_spk_client(type);
			}else{

				cnum = 1;
//				tmp = conf_status_get_cspk_num();
//				tmp++;
//				conf_status_set_cspk_num(tmp);
			}
			conf_status_set_cmspk(WIFI_MEETING_CON_SE_CHAIRMAN);
			type->fd = tmp_fd;
			type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;

		}else if(type->con_data.seat == WIFI_MEETING_CON_SE_GUEST
				|| type->con_data.seat == WIFI_MEETING_CON_SE_ATTEND)
		{

			/*
			 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
			 * 已达到上限，则需要关闭第一个单元
			 * 轮询发言链表中的设备，比较时间戳， 时间戳最小的，表示最早加入设备，需要关闭
			 */
			if(conf_status_get_cspk_num() == conf_status_get_spk_num())
			{
				/*
				 * 人数已满，需要进行管理，关闭最早上线单元
				 */
				dmanage_close_first_spk_client(type);
				/*
				 * 更新发言链表，下发端口信息
				 */
				tmp2 = type->fd;
				tmp_port = type->spk_port;
				type->fd = tmp_fd;
				type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;

			}else{

				/*
				 * 判断发言端口状态，是否有单元自己关闭，然后新上线的单元，需要使用下线单元的端口号
				 */
				dmanage_search_not_use_spk_port(type);
				tmp_port = type->spk_port;

				//当前发言人数需要告知全局变量
//				if((tmp=conf_status_get_cspk_num()) < conf_status_get_spk_num())
//				{
//					tmp++;
//					conf_status_set_cspk_num(tmp);
//				}
				cnum = 1;
			}
		}

		tcp_ctrl_source_dest_setting(-1,type->fd,type);
		/*
		 * 下发给摄像
		 */
		conf_status_camera_track_postion(type->d_id,1);
		/*
		 * 更新发言管理链表
		 */
		type->spk_port = tmp_port;

		if(conf_status_get_cspk_num() == conf_status_get_spk_num())
		{
//			conf_status_set_spk_now_fd(type);
			dmanage_delete_spk_node(tmp2);

			cmsm_refresh_spk_node(type);

			tcp_ctrl_module_edit_info(type,NULL);


		}else{
			cmsm_refresh_spk_node(type);

			tcp_ctrl_module_edit_info(type,NULL);
		}

		if(cnum == 1)
		{
			tmp = conf_status_get_cspk_num();
			tmp++;
			conf_status_set_cspk_num(tmp);
		}

		type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
		dmanage_send_mic_status_to_pc(type);
		break;
	}
//	case WIFI_MEETING_EVENT_SPK_VETO:
//	{
//		type->name_type[0] = WIFI_MEETING_EVT_SPK;
//		type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
//
//		tcp_ctrl_source_dest_setting(-1,type->fd,type);
//		tcp_ctrl_module_edit_info(type,NULL);
//		break;
//	}
	case WIFI_MEETING_EVENT_SPK_REQ_SND:
	case WIFI_MEETING_EVENT_SPK_ALOW_SND:
	{
		conf_status_set_snd_brd(WIFI_MEETING_EVENT_SPK_REQ_SND);
		type->brd_port = AUDIO_SEND_PORT;
		tcp_ctrl_module_edit_info(type,NULL);
		break;
	}
	case WIFI_MEETING_EVENT_SPK_CLOSE_SND:
	{
		conf_status_set_snd_brd(WIFI_MEETING_EVENT_SPK_CLOSE_SND);
		break;
	}
	case WIFI_MEETING_EVENT_SPK_CLOSE_MIC:

//		tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);

		conf_status_camera_track_postion(type->s_id,0);
		tmp=conf_status_get_cspk_num();
		if(tmp>0)
		{
			tmp--;
			conf_status_set_cspk_num(tmp);
		}

		if(type->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			conf_status_set_cmspk(WIFI_MEETING_CON_SE_GUEST);
		}
		cmsm_refresh_spk_node(type);
		dmanage_send_mic_status_to_pc(type);
		break;
	case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON:
	{
		dmanage_close_guest_spk_client(type);
		conf_status_set_spk_now_fd(type);
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
		dmanage_close_last_spk_client(type);

		dmanage_delete_spk_node(type->fd);

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

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	//摄像跟踪设置
	conf_status_camera_track_postion(type->d_id,1);

	cmsm_refresh_spk_node(type);

	tcp_ctrl_module_edit_info(type,NULL);

	//告知上位机有设备打开话筒
	type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
	dmanage_send_mic_status_to_pc(type);

	return SUCCESS;
}


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

		dmanage_close_first_spk_client(type);

		dmanage_delete_spk_node(type->fd);

	}else{

		/*
		 * 人数没有饱和，则查找没有使用的端口
		 */
		dmanage_search_not_use_spk_port(type);
		csn = conf_status_get_cspk_num();
		csn++;
		conf_status_set_cspk_num(csn);
	}

	type->fd = tmp_fd;
	type->msg_type = WRITE_MSG;
	type->dev_type = HOST_CTRL;
	type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
	type->code_type[0] = WIFI_MEETING_CHAR;

	tcp_ctrl_source_dest_setting(-1,type->fd,type);
	//摄像跟踪设置
	conf_status_camera_track_postion(type->d_id,1);

	cmsm_refresh_spk_node(type);

	tcp_ctrl_module_edit_info(type,NULL);

	//告知上位机有设备打开话筒
	type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
	dmanage_send_mic_status_to_pc(type);

	return SUCCESS;
}


/*
 * cmsm_spk_port_status_onoff
 * 发言端口的管理函数
 *
 * 主要是处理端口号的打开和关闭，并且更新链表信息
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
//	int tmp_port = 0;
//	int csn = 0;
//	int cs_state = 0;
//	int tmp2 = 0;
//	int tmp_fd = type->fd;
//
//	type->msg_type = WRITE_MSG;
//	type->dev_type = HOST_CTRL;
//	type->code_type[0] = WIFI_MEETING_CHAR;

	/*
	 * 判断此单元状态是否已在链表中更新
	 */
	if(!cmsm_search_spk_node(type))
	{
		switch(type->con_data.seat)
		{
			case WIFI_MEETING_CON_SE_CHAIRMAN:
//				/*
//				 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
//				 * 人数饱满，则需要关闭最后上线的设备
//				 */
//				tmp_port = AUDIO_RECV_PORT;
//
//				if(conf_status_get_cspk_num() == conf_status_get_spk_num())
//				{
//					dmanage_close_last_spk_client(type);
//					tmp2 = type->fd;
//
//				}else{
//					cs_state++;
//				}
//
//				conf_status_set_cmspk(WIFI_MEETING_CON_SE_CHAIRMAN);

				cmsm_chairman_port(type);
				break;
			case WIFI_MEETING_CON_SE_GUEST:
			case WIFI_MEETING_CON_SE_ATTEND:
//				/*
//				 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
//				 * 已达到上限，则需要关闭第一个单元
//				 * 轮询发言链表中的设备，比较时间戳， 时间戳最小的，表示最早加入设备，需要关闭
//				 */
//				if(conf_status_get_cspk_num() == conf_status_get_spk_num())
//				{
//					/*
//					 * 人数已满，需要进行管理，关闭最早上线单元
//					 */
//					dmanage_close_first_spk_client(type);
//					/*
//					 * 更新发言链表，下发端口信息
//					 */
//					tmp2 = type->fd;
//					tmp_port = type->spk_port;
//
//				}else{
//					cs_state++;
//					/*
//					 * 人数没有饱和，则查找没有使用的端口
//					 */
//					dmanage_search_not_use_spk_port(type);
//					tmp_port = type->spk_port;
//				}
				cmsm_guest_port(type);

				break;
		}

//		type->fd = tmp_fd;
//		/*
//		 * 发送打开端口信息给单元
//		 */
//		type->msg_type = WRITE_MSG;
//		type->dev_type = HOST_CTRL;
//
//		type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
//		type->code_type[0] = WIFI_MEETING_CHAR;
//
//		tcp_ctrl_source_dest_setting(-1,type->fd,type);
//		//摄像跟踪设置
//		conf_status_camera_track_postion(type->d_id,1);
//
//		type->spk_port = tmp_port;
//
//		//通过当前人数是否有变化进行是否移除操作
//		if(cs_state)
//		{
//			csn = conf_status_get_cspk_num();
//			csn++;
//			conf_status_set_cspk_num(csn);
//
//			cmsm_refresh_spk_node(type);
//
//			tcp_ctrl_module_edit_info(type,NULL);
//
//		}else{
//			dmanage_delete_spk_node(tmp2);
//
//			cmsm_refresh_spk_node(type);
//
//			tcp_ctrl_module_edit_info(type,NULL);
//
//		}
//		//告知上位机有设备打开话筒
//		type->evt_data.value = WIFI_MEETING_EVT_SPK_OPEN_MIC;
//		dmanage_send_mic_status_to_pc(type);
	}else{
		printf("%s-%s-%d the unit request already process\n",__FILE__,__func__,__LINE__);
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

			if(type->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
			{
				conf_status_set_cmspk(WIFI_MEETING_CON_SE_GUEST);
			}
			cmsm_refresh_spk_node(type);
			dmanage_send_mic_status_to_pc(type);
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
			break;
		}
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON:
		{
			dmanage_close_guest_spk_client(type);
//			conf_status_set_spk_now_fd(tmp_type);
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
	if(type->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN){
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
			if(pos > 0 || conf_status_get_pc_staus() > 0){
				//请求消息发送给主席
				if(pos > 0)
				{
					//源地址为请求单元机id，目标地址修改为主机单元id
					tcp_ctrl_source_dest_setting(tmp_fd,type->fd,type);
					tcp_ctrl_module_edit_info(type,msg);
					type->fd = tmp_fd;
				}
				//请求消息发送给上位机
				dmanage_send_mic_status_to_pc(type);
			}else{
				//没有主席和上位机，告知单元，拒绝其发言
				printf("%s-%s-%d not find chairman and pc\n",__FILE__,__func__,__LINE__);

				type->msg_type = WRITE_MSG;
				type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
				tcp_ctrl_source_dest_setting(-1,type->fd,type);

				tcp_ctrl_module_edit_info(type,NULL);

			}
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
			if(conf_status_check_client_connect_legal(type)
					&& type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALLOW)
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
 * 音频下发处理函数
 *
 * 此接口暂时没用
 * 设计思路和请求发言差不多，但是计划是固定只能打开几个
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
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
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
		/*
		 * 发言话筒
		 */
		case WIFI_MEETING_EVENT_SPK_REQ_SPK:
			cmsm_handle_spk_request(frame_type,msg);
//			/*
//			 * 判断申请发言单元的席别，如果是主席，则直接进行处理
//			 * 如果是其它单元，则需要判断发言人数和话筒模式
//			 */
//			if(frame_type->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN){
//				cmsm_handle_spk_port(frame_type);
//			}else{
//
//				/*
//				 * 判断发言人数，如果发言人数为1，则表示主席优先模式打开
//				 * 发言人数为1 ，则直接拒绝单元申请
//				 *
//				 * 发言人数大于1，则需要判断话筒模式和发言人数
//				 * FIFO模式，则直接关闭最先发言单元，新单元加入
//				 * 其他模式，先判断发言人数，饱和则需要通知主席
//				 */
//				if(conf_status_get_spk_num() == WIFI_MEETING_EVT_MIC_CHAIRMAN)
//				{
//
//	//				printf("%s-%s-%d only chairman mode\n",__FILE__,__func__,__LINE__);
//					//直接回复单元，拒绝其发言
//					frame_type->msg_type = WRITE_MSG;
//					frame_type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
//					tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
//
//					tcp_ctrl_module_edit_info(frame_type,NULL);
//
//				}else{
//					/*
//					 * 判断发言话筒模式，如果是FIFO模式，则直接进行处理，不需要转发主席
//					 */
//					if(conf_status_get_mic_mode() == WIFI_MEETING_EVT_MIC_FIFO)
//					{
//						cmsm_handle_spk_port(frame_type);
//					}else{
//						/*
//						 * 判断发言人数是否饱和，如果达到上限，则需要请求主席授权
//						 */
//						if(conf_status_get_cspk_num() < conf_status_get_spk_num())
//						{
//							cmsm_handle_spk_port(frame_type);
//						}else{
//							/*
//							 * 将请求消息发送给主席，无主席和上位机，则直接发送拒绝消息给单元
//							 */
//							pos = conf_status_find_chairman_sockfd(frame_type);
//							if(pos > 0 || conf_status_get_pc_staus() > 0){
//								//请求消息发送给主席
//								if(pos > 0)
//								{
//									//源地址为请求单元机id，目标地址修改为主机单元id
//									tcp_ctrl_source_dest_setting(tmp_fd,frame_type->fd,frame_type);
//									tcp_ctrl_module_edit_info(frame_type,msg);
//									frame_type->fd = tmp_fd;
//								}
//								//请求消息发送给上位机
//								dmanage_send_mic_status_to_pc(frame_type);
//							}else{
//								//没有主席和上位机，告知单元，拒绝其发言
//								printf("%s-%s-%d not find chairman and pc\n",__FILE__,__func__,__LINE__);
//
//								frame_type->msg_type = WRITE_MSG;
//								frame_type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
//								tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
//
//								tcp_ctrl_module_edit_info(frame_type,NULL);
//
//							}
//						}
//					}
//				}
//			}
			break;

		case WIFI_MEETING_EVENT_SPK_ALLOW:
		case WIFI_MEETING_EVENT_SPK_VETO:
			cmsm_handle_request_reply(frame_type,msg);
//			//允许和拒绝消息只能是主席发送
//			pos = conf_status_check_chairman_legal(frame_type);
//
//			if(pos > 0){
//	//			pos = 0;
//				//找到目标地址对应的单元
//				pos = conf_status_find_did_sockfd_id(frame_type);
//				if(pos > 0)
//				{
//					//变换为控制类消息发送给单元
//					frame_type->msg_type = WRITE_MSG;
//					tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
//					tcp_ctrl_module_edit_info(frame_type,msg);
//					//设置单元音频端口信息
//					if(conf_status_check_client_connect_legal(frame_type)
//							&& frame_type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALLOW)
//					{
//						cmsm_handle_spk_port(frame_type);
//					}
//
//				}else{
//					printf("%s-%s-%d not the find the unit device\n",__FILE__,__func__,__LINE__);
//					return ERROR;
//				}
//
//			}else{
//				printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
//				return ERROR;
//			}
			break;
		/*
		 * 音频下发
		 */
		case WIFI_MEETING_EVENT_SPK_REQ_SND:
		case WIFI_MEETING_EVENT_SPK_ALOW_SND:
		case WIFI_MEETING_EVENT_SPK_VETO_SND:
		case WIFI_MEETING_EVENT_SPK_CLOSE_SND:
			cmsm_handle_snd_brd(frame_type,msg);
			//音频请求接口，暂时不做判断
//			pos = conf_status_check_chairman_legal(frame_type);
//			if(pos > 0){
//				pos = 0;
//				pos = conf_status_find_did_sockfd_id(frame_type);
//				if(pos > 0)
//				{
//					printf("reply sound request\n");
//					//变换为控制类消息下个给单元机
//					frame_type->msg_type = WRITE_MSG;
//					tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
//					tcp_ctrl_module_edit_info(frame_type,msg);
//					//设置单元音频端口信息
//					if(frame_type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALOW_SND)
//						cmsm_handle_spk_port(frame_type);
//				}else{
//					printf("%s-%s-%d not the find the unit device\n",__FILE__,__func__,__LINE__);
//					return ERROR;
//				}
//
//			}else{
//				printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
//				return ERROR;
//			}
			break;
		/*
		 * 其他请求
		 */
		case WIFI_MEETING_EVENT_SPK_CLOSE_MIC:
		case WIFI_MEETING_EVENT_SPK_CLOSE_REQ:
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_ON:
		case WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY_OFF:
			cmsm_spk_other_status_onoff(frame_type,msg);
			break;

	}

	return SUCCESS;
}







