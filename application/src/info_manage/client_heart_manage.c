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
#include "client_heart_manage.h"
#include "client_connect_manage.h"

extern Pglobal_info node_queue;


/*
 * chm_delete_heart_list
 * 删除心跳链表中的单元信息
 *
 *
 */
int chm_delete_heart_list(int fd)
{
	pclient_node tmp_node = NULL,del = NULL;

	Pconnect_heart hinfo = NULL;

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
		sys_list_delete(node_queue->sys_list[CONNECT_HEART],pos,&del);
		hinfo = del->data;

		printf("%s-%s-%d,remove %d in heart list\n",__FILE__,__func__,__LINE__,hinfo->sockfd);
		free(hinfo);
		free(del);
		hinfo = NULL;
		del = NULL;
	}else{
		printf("%s-%s-%d,not have heart client\n",__FILE__,__func__,__LINE__);
	}

	return SUCCESS;
}

/*
 * chm_set_communication_heart
 * 为连接设备增加心跳监测信息
 *
 * 设置心跳参数，保存到心跳链表信息中
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 * 返回值
 * 成功
 * 失败
 */
int chm_set_communication_heart(Pframe_type type)
{
	Pconnect_heart hinfo = NULL;

	hinfo = (Pconnect_heart)malloc(sizeof(connect_heart));

	hinfo->sockfd = type->fd;
	hinfo->mac_addr = type->evt_data.unet_info.mac;
	hinfo->status = DEVICE_HEART;
//	printf("%s-%s-%d fd:%d,status：%d\n",__FILE__,__func__,__LINE__,
//			hinfo->sockfd,hinfo->status);
	/*
	 * 连接信息存入链表
	 */
	sys_list_add(node_queue->sys_list[CONNECT_HEART],hinfo);

	return SUCCESS;
}


/*
 * chm_get_communication_heart
 * 获取单元心跳状态信息
 *
 * 获取一次，将心跳属性减一，心跳为0则需要返回此设备sockfd
 *
 * 输入
 * Pframe_type
 * 输出
 * Pframe_type
 * 返回值
 * 成功
 * 失败
 */
int chm_get_communication_heart(Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pconnect_heart hinfo = NULL;
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
						type->ucinfo.seat = WIFI_MEETING_CON_SE_CHAIRMAN;
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
 *
 * 1、在接收到终端消息就表示心跳正常
 * 2、收到消息类型为心跳，则需要对终端进行回复
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
int chm_process_communication_heart(const unsigned char* msg,Pframe_type type)
{
	pclient_node tmp_node = NULL;
	Pconnect_heart hinfo = NULL;

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
					hinfo->status = DEVICE_HEART;
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




/*
 * wifi_sys_ctrl_tcp_heart_sta
 * 设备管理，心跳检测
 * 1、轮询查找当前设备连接信息中的心跳状态
 * 2.判断连接信息，如果状态为0，则表示端口异常，需要关闭
 */
void* wifi_sys_ctrl_tcp_heart_state(void* p)
{
	frame_type type;
	memset(&type,0,sizeof(frame_type));

	pthread_detach(pthread_self());

	while(1)
	{
		if(!conf_status_get_client_connect_len())
		{
			sleep(1);
			conf_status_set_sys_timestamp(1);
			continue;
		}else{
			conf_status_set_sys_timestamp(10);
			sleep(10);
			chm_get_communication_heart(&type);
			if(type.fd)
			{
				printf("%s-%s-%d,%d client connect err,close it\n",__FILE__,__func__,__LINE__
						,type.fd);

				sys_mutex_lock(LIST_MUTEX);
				ccm_delete_info(type.fd);
				sys_mutex_unlock(LIST_MUTEX);

				close(type.fd);
				type.fd = 0;
			}
		}
	}

}
