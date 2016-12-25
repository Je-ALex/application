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

extern Pglobal_info node_queue;

unsigned int spk_ts = 0;




int conf_status_delete_spk_node(int fd)
{
	pclient_node tmp_node;
	pclient_node del;
	Pas_port sinfo;

	int pos,status;

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
				sinfo->sockfd);

		free(sinfo);
		free(del);
	}else{
		printf("%s-%s-%d,not have spk client\n",__FILE__,__func__,__LINE__);
	}


}

int conf_status_add_spk_node(Pframe_type type)
{
	Pas_port sinfo;

	sinfo = malloc(sizeof(as_port));

	spk_ts++;

	sinfo->sockfd = type->fd;
	sinfo->asport = type->spk_port;
	sinfo->ts = spk_ts;
	printf("%s-%s-%d fd:%d,asport：%d,ts:%d\n",__FILE__,__func__,__LINE__,
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
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 *
 */
int conf_status_search_spk_node(Pframe_type type)
{
	pclient_node tmp_node;
	Pas_port sinfo;

	unsigned int ts = 0;

	tmp_node = node_queue->sys_list[CONFERENCE_SPK]->next;
	while(tmp_node!=NULL)
	{
		sinfo = tmp_node->data;
		if(sinfo->sockfd)
		{
			if(sinfo->ts > ts)
			{
				type->spk_port = sinfo->asport;
				type->fd = sinfo->sockfd;

				ts = sinfo->ts;
			}
			break;
		}
		tmp_node=tmp_node->next;

	}

	return SUCCESS;
}


/*
 * conf_status_refresh_spk_node
 * 更新会议链表中的发言管理设备信息
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 *
 */
int conf_status_refresh_spk_node(Pframe_type type)
{
	switch(type->evt_data.status)
	{
	case WIFI_MEETING_EVENT_SPK_ALLOW:
	case WIFI_MEETING_EVENT_SPK_REQ_SPK:
		conf_status_add_spk_node(type);
		break;
	case WIFI_MEETING_EVENT_SPK_REQ_CLOSE:
		conf_status_delete_spk_node(type->fd);
		break;
	}


	return SUCCESS;
}



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
				ret = conf_status_check_chariman_legal(type);
				if(ret)
					type->con_data.seat = WIFI_MEETING_CON_SE_CHARIMAN;
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

	pos = status =0;

	switch(type->msg_type)
	{
	case REQUEST_MSG:
	case W_REPLY_MSG:
	case R_REPLY_MSG:
	case ONLINE_HEART:
		/*
		 * 查找连接信息中，是否有该设备存在
		 */
		ret = conf_status_check_connect_legal(type);
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

			type->d_id = type->s_id;
			type->s_id = 0;
			type->evt_data.status = WIFI_MEETING_EVENT_DEVICE_HEART;
			tcp_ctrl_module_edit_info(type,msg);
		}

	}

	return SUCCESS;
}


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
			printf("%s-%s-%d,add %s in txt\nn",__FILE__,__func__,__LINE__,
					inet_ntoa(cinfo->cli_addr.sin_addr));
			ret = fwrite(cinfo,sizeof(client_info),1,file);
			perror("fwrite");
			if(ret != 1)
				return ERROR;

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

	dmanage_delete_heart_list(fd);

	dmanage_delete_connected_list(fd);

	dmanage_delete_conference_list(fd);

	/*
	 * 下线消息告知主机或上位机
	 */
	//上报上位机部分
	frame_type tmp_type;
	memset(&tmp_type,0,sizeof(frame_type));
	tmp_type.msg_type = OFFLINE_REQ;
	tmp_type.dev_type = UNIT_CTRL;

	tmp_type.fd = fd;
	tcp_ctrl_msg_send_to(&tmp_type,NULL,WIFI_MEETING_EVENT_OFFLINE_REQ);

	return SUCCESS;
}


/*
 * tcp_ctrl_add_client
 * 终端信息录入函数
 *
 * 生成设备连接信息文本文件，采用结构体方式进行存储
 * 应用在设置参数时，需要读取文本文件中的fd信息，进行参数设置
 *
 */
int dmanage_add_connected_info(Pclient_info value)
{

	FILE* file;
	int ret;

	/*
	 * 连接信息存入链表
	 */
	list_add(node_queue->sys_list[CONNECT_LIST],value);

	/*
	 * 存入到文本文件
	 * 更新文本信息
	 */
	file = fopen(CONNECT_FILE,"a+");

	printf("%s-%s-%d fd:%d,ip:%s,machine:%d\n",__FILE__,__func__,__LINE__,value->client_fd,
			inet_ntoa(value->cli_addr.sin_addr),value->client_name);

	ret = fwrite(value,sizeof(client_info),1,file);
	perror("fwrite");
	if(ret != 1)
		return ERROR;

	fclose(file);

	return SUCCESS;
}

/*
 * dmanage_add_info
 * 删除
 *
 */
int dmanage_add_info(const unsigned char* msg,Pframe_type type)
{

	/*
	 * 终端信息录入结构体
	 */
	pclient_node tmp_node = NULL;
	Pclient_info info;
	Pclient_info pinfo;
	Pconference_list confer_info;

	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

	int state = 0;

	/*
	 * 宣告上线消息类型
	 */
//	if(type->msg_type == ONLINE_REQ)
//	{
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
		sprintf(info->ip,"%s",inet_ntoa(info->cli_addr.sin_addr));

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
			info->seat = type->con_data.seat = msg[0];
			info->id = type->s_id;

			//增加电量信息
			info->client_power = type->evt_data.electricity = msg[1];
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
				state++;
				dmanage_add_connected_info(info);
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
			dmanage_add_connected_info(info);
		}
		/*
		 * 添加心跳链表设备
		 */
		if(type->dev_type != PC_CTRL){
			dmanage_set_communication_heart(type);
		}

		/*
		 * 保存至会议信息链表
		 * 如果上线单元机有id号则将上线单元机信息进行存储
		 *
		 * 会议中上线的单元机，主机会自动给其分配ID号，用于管理
		 */
		if(type->dev_type != PC_CTRL)
		{
			confer_info = (Pconference_list)malloc(sizeof(conference_list));
			memset(confer_info,0,sizeof(conference_list));

			confer_info->fd = type->fd;
			confer_info->con_data.id = type->con_data.id = type->s_id;
			confer_info->con_data.seat = type->con_data.seat;
			//判断当前是否有会议主席，如果有则上线的单元设置为客席，会议链表中只能存在一个主席
			if(conf_status_check_chariman_staus())
			{
				confer_info->con_data.seat = type->con_data.seat = WIFI_MEETING_CON_SE_GUEST;
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

			/*
			 * 发送至本地上报队列
			 */
			tcp_ctrl_msg_send_to(type,msg,WIFI_MEETING_EVENT_ONLINE_REQ);
		}

//	}



	return SUCCESS;
}



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
		printf("%s-%s-%d,delete data in the connection list,then add it\n",
				__FILE__,__func__,__LINE__);

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
				printf("%s-%s-%d,fd=%d,id=%d,seat=%d\n",__FILE__,__func__,
						__LINE__,newinfo->client_fd, newinfo->id,newinfo->seat);

				ret = fwrite(newinfo,sizeof(client_info),1,cfile);
				perror("fwrite");
				if(ret != 1)
					return ERROR;

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
		printf("%s-%s-%d,delete data in the conference list,then add it\n",
				__FILE__,__func__,__LINE__);
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
	Pconference_list confer_info;
	int ret = 0;

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
	ret = dmanage_refresh_connected_list(confer_info);
	if(ret)
	{
		printf("%s-%s-%d,dmanage_refresh_connected_list err\n",__FILE__,__func__,__LINE__);
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
