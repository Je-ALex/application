/*
 * client_heart_manage.c
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#include "wifi_sys_init.h"

#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"

#include "audio_ring_buf.h"

#include "sys_uart_init.h"
#include "tcp_ctrl_server.h"

#include "tcp_ctrl_api.h"

extern Pglobal_info node_queue;


/*
 * TODO
 * 设备心跳管理
 */

/*
 * dmanage_delete_heart_list
 * 删除心跳链表中的设备信息
 */
int chm_delete_heart_list(int fd)
{
	pclient_node tmp_node;
	pclient_node del;
	Pconnect_heart hinfo;

	int pos,status;

	pos = status =0;
	/*
	 * 查找连接信息中，是否有该设备存在
	 */

	tmp_node = node_queue->sys_list[CONNECT_HEART]->next;
	while(tmp_node!=NULL)
	{
		hinfo = tmp_node->data;
		if(hinfo->sockfd == fd)
		{
			status++;
			break;
		}
		tmp_node=tmp_node->next;
		pos++;
	}

	if(status)
	{

		list_delete(node_queue->sys_list[CONNECT_HEART],pos,&del);
		hinfo = del->data;

		printf("%s-%s-%d,remove %d in heart list\n",__FILE__,__func__,__LINE__,hinfo->sockfd);
		free(hinfo);
		free(del);
	}else{
		printf("%s-%s-%d,not have heart client\n",__FILE__,__func__,__LINE__);
	}

	return SUCCESS;
}

/*
 * dmanage_set_communication_heart
 * 为连接设备增加心跳属性
 * 设置设备心跳阈值
 */
int chm_set_communication_heart(Pframe_type type)
{
	Pconnect_heart hinfo;

	hinfo = malloc(sizeof(connect_heart));

	hinfo->sockfd = type->fd;
	hinfo->mac_addr = type->evt_data.unet_info.mac;
	hinfo->status = DEVICE_HEART;
//	printf("%s-%s-%d fd:%d,status：%d\n",__FILE__,__func__,__LINE__,
//			hinfo->sockfd,hinfo->status);
	/*
	 * 连接信息存入链表
	 */
	list_add(node_queue->sys_list[CONNECT_HEART],hinfo);

	return SUCCESS;
}


/*
 * dmanage_get_communication_heart
 * 获取设备心跳属性
 *
 * 获取一次，将心跳属性减一，心跳为0则需要返回此设备sockfd
 */
int chm_get_communication_heart(Pframe_type type)
{
	pclient_node tmp_node;
	Pconnect_heart hinfo;
	int ret = 0;

	tmp_node = node_queue->sys_list[CONNECT_HEART]->next;
	while(tmp_node != NULL)
	{
		hinfo = tmp_node->data;
		if(hinfo != NULL)
		{
			if(hinfo->sockfd)
			{
				hinfo->status--;
				if(!hinfo->status)
				{
					type->fd = hinfo->sockfd;
					type->evt_data.unet_info.mac = hinfo->mac_addr;
					ret = conf_status_check_chairman_legal(type);
					if(ret)
						type->con_data.seat = WIFI_MEETING_CON_SE_CHAIRMAN;
					break;
				}
			}
		}

		tmp_node=tmp_node->next;

	}

	return SUCCESS;
}


/*
 * chm_process_communication_heart
 * 处理设备心跳请求
 * 1、在接收到终端消息就表示心跳正常
 * 2、收到消息类型为心跳，则需要对终端进行回复
 */
int chm_process_communication_heart(const unsigned char* msg,Pframe_type type)
{
	pclient_node tmp_node;
	pclient_node del;
	Pconnect_heart hinfo;

	int ret,pos,status;

	pos = status = 0;

	switch(type->msg_type)
	{
	case REQUEST_MSG:
	case W_REPLY_MSG:
	case R_REPLY_MSG:
	case ONLINE_HEART:
		/*
		 * 查找连接信息中，是否有该设备存在
		 */
		ret = conf_status_check_client_connect_legal(type);
		if(ret)
		{
			tmp_node = node_queue->sys_list[CONNECT_HEART]->next;
			while(tmp_node!=NULL)
			{
				hinfo = tmp_node->data;
				if(hinfo->sockfd == type->fd)
				{
					status++;
					break;
				}
				tmp_node=tmp_node->next;
				pos++;
			}

		}else{
			printf("%s-%s-%d,not have heart client\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
		break;
	}

	if(status)
	{
		list_delete(node_queue->sys_list[CONNECT_HEART],pos,&del);
		hinfo = del->data;

		free(hinfo);
		free(del);

		chm_set_communication_heart(type);

		if(type->msg_type == ONLINE_HEART)
		{
			type->dev_type = HOST_CTRL;
			type->d_id = type->s_id;
			type->s_id = 0;
			type->evt_data.status = WIFI_MEETING_EVENT_DEVICE_HEART;
			tcp_ctrl_module_edit_info(type,msg);
		}

	}

	return SUCCESS;
}

