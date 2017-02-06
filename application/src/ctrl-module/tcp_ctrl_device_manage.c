/*
 * tcp_ctrl_device_manage.c
 *
 *  Created on: 2016年12月21日
 *      Author: leon
 */

#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_device_manage.h"
#include "sys_uart_init.h"
#include "audio_ring_buf.h"

extern Pglobal_info node_queue;
extern Paudio_queue* rqueue;
extern sys_info sys_in;

unsigned int spk_ts = 0;


/*
 * TODO
 * 发言管理的设备音频端口管理
 */
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
	int port[7][2] = {
			{2,0},
			{4,0},
			{6,0},
			{8,0},
			{10,0},
			{12,0},
			{14,0}
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
	tcp_ctrl_module_edit_info(type,NULL);

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
	tcp_ctrl_module_edit_info(type,NULL);

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

			dmanage_delete_spk_node(sinfo->sockfd);

			tmp=conf_status_get_cspk_num();
			if(tmp)
			{
				tmp--;
				conf_status_set_cspk_num(tmp);
			}

			msleep(1);
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
			audio_queue_reset(rqueue[num]);
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
 * dmanage_refresh_spk_node
 * 更新会议链表中的发言管理设备信息
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 *
 */
int dmanage_refresh_spk_node(Pframe_type type)
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
 * TODO
 * 设备心跳管理
 */

/*
 * dmanage_delete_heart_list
 * 删除心跳链表中的设备信息
 */
int dmanage_delete_heart_list(int fd)
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
int dmanage_set_communication_heart(Pframe_type type)
{
	Pconnect_heart hinfo;

	hinfo = malloc(sizeof(connect_heart));

	hinfo->sockfd = type->fd;
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
int dmanage_get_communication_heart(Pframe_type type)
{
	pclient_node tmp_node;
	Pconnect_heart hinfo;
	int ret = 0;

	tmp_node = node_queue->sys_list[CONNECT_HEART]->next;
	while(tmp_node!=NULL)
	{
		hinfo = tmp_node->data;
		if(hinfo->sockfd)
		{
			hinfo->status--;
			if(!hinfo->status)
			{
				type->fd = hinfo->sockfd;
				ret = conf_status_check_chairman_legal(type);
				if(ret)
					type->con_data.seat = WIFI_MEETING_CON_SE_CHAIRMAN;
				break;
			}

		}
		tmp_node=tmp_node->next;

	}

	return SUCCESS;
}


/*
 * dmanage_process_communication_heart
 * 处理设备心跳请求
 * 1、在接收到终端消息就表示心跳正常
 * 2、收到消息类型为心跳，则需要对终端进行回复
 */
int dmanage_process_communication_heart(const unsigned char* msg,Pframe_type type)
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

		dmanage_set_communication_heart(type);

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
 * TODO
 * 设备连接信息管理
 */

/*
 * dmanage_refresh_connected_list
 * 更新设备连接信息链表
 *
 */
int dmanage_refresh_connected_list(Pconference_list data_info)
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
 * dmanage_refresh_conference_list
 * 更新会议信息链表
 *
 */
int dmanage_refresh_conference_list(Pconference_list data_info)
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
 * dmanage_refresh_info
 * 更新设备信息
 * 1、连接链表信息，文本文件
 * 2、会议链表信息
 *
 */
int dmanage_refresh_info(const Pframe_type data_info)
{
	Pconference_list confer_info = NULL;
	int ret = 0;

	/*
	 * 更新到连接信息链表中
	 */
	ret = dmanage_refresh_spk_node(data_info);
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
//	pthread_mutex_lock(&sys_in.sys_mutex[LIST_MUTEX]);
	ret = dmanage_refresh_connected_list(confer_info);
//	pthread_mutex_unlock(&sys_in.sys_mutex[LIST_MUTEX]);
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
	ret = dmanage_refresh_conference_list(confer_info);
	if(ret)
	{
		printf("%s-%s-%d,dmanage_refresh_conference_list err\n",__FILE__,__func__,__LINE__);
		free(confer_info);
		return ERROR;
	}

	return SUCCESS;
}



/*
 *
 */
int dmanage_delete_connected_list(int fd)
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


int dmanage_delete_conference_list(int fd)
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
 * dmanage_delete_info
 * 客户端 删除函数
 * 删除链表中的连接信息
 * 先通过查找链表，定位离线客户端的位置，在调用链表删除函数，删除链表中的设备
 *
 * 更新文本文件
 */
int dmanage_delete_info(int fd)
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
		ret = dmanage_refresh_spk_node(&tmp_type);
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

		dmanage_delete_heart_list(fd);

		dmanage_delete_conference_list(fd);
	}
	dmanage_delete_connected_list(fd);

	return SUCCESS;
}


/*
 * dmanage_add_connected_info
 * 终端信息录入函数
 *
 * 生成设备连接信息文本文件，采用结构体方式进行存储
 * 应用在设置参数时，需要读取文本文件中的fd信息，进行参数设置
 *
 */
int dmanage_add_connected_info(Pframe_type type)
{
	FILE* file;
	pclient_node tmp_node = NULL;
	Pclient_info info;
	Pclient_info pinfo;

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
			if(pinfo->client_fd == type->fd)
			{
				state++;
				printf("the client is exist\n");
				free(info);
				return ERROR;
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
//		if(sys_debug_get_switch())
//		{
//			perror("fwrite");
//		}
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
 * dmanage_add_conference_info
 * 终端信息录入函数
 *
 * 生成设备连接信息文本文件，采用结构体方式进行存储
 * 应用在设置参数时，需要读取文本文件中的fd信息，进行参数设置
 *
 */
int dmanage_add_conference_info(Pframe_type type)
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
		dmanage_refresh_conference_list(confer_info);

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
			dmanage_refresh_conference_list(confer_info);
		}

	}

	return SUCCESS;
}

int dmanage_set_sys_time(Pframe_type type)
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
 * dmanage_add_info
 *
 */
int dmanage_add_info(const unsigned char* msg,Pframe_type type)
{

	int tmp_fd = 0;
	int tmp_did = 0;

	type->con_data.seat = msg[0];
	type->evt_data.electricity = msg[1];


	tmp_fd = type->fd;
	tmp_did = type->s_id;

	dmanage_add_connected_info(type);
	/*
	 * 消息上报
	 *
	 * 添加心跳链表设备
	 */
	if(type->dev_type != PC_CTRL){

		/*
		 * 下发系统时间给上线单元
		 */

		/*
		 * 增加心跳设备
		 */
		dmanage_set_communication_heart(type);

		dmanage_add_conference_info(type);
		/*
		 * 发送至本地上报队列
		 */
		tcp_ctrl_msg_send_to(type,msg,WIFI_MEETING_EVENT_ONLINE_REQ);

		type->fd = tmp_fd;
		type->d_id = tmp_did;
		type->s_id = 0;

		dmanage_set_sys_time(type);

	}

	return SUCCESS;
}




