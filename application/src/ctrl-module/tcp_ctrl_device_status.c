/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_api.h"

extern sys_info sys_in;
extern Pglobal_info node_queue;


/*
 * 会议信息存储函数
 *
 */
int tcp_ctrl_refresh_conference_list(Pconference_list data_info)
{
	pclient_node tmp = NULL;
	pclient_node del = NULL;
	Pclient_info cinfo;
	Pconference_list info;


	int pos = 0;
	int status = 0;

	/*
	 * 首先在连接信息中搜寻socke_fd
	 * 判断是否有此设备连接
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
		printf("there is no client in the connection list\n");
		return ERROR;
	}

	/*
	 * 删除会议信息链表中的数据
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;;
	while(tmp != NULL)
	{
		info = tmp->data;
		if(info->fd == data_info->fd)
		{
			printf("find the fd\n");
			status++;
			break;
		}
		pos++;
		tmp = tmp->next;
	}

//	printf("pos--%d\n",pos);
	if(status > 0)
	{
		list_delete(node_queue->sys_list[CONFERENCE_LIST],pos,&del);
		info = del->data;
		status = 0;
		free(info);
		free(del);
		printf("delete data in the conference list,then add it\n");
	}else{

		printf("there is no data in the conference list,add it\n");
	}

	list_add(node_queue->sys_list[CONFERENCE_LIST],data_info);

	return SUCCESS;
}


/*
 * tcp_ctrl_tprecv_enqueue
 * tcp控制模块的数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tprecv_enqueue(int* fd,unsigned char* msg,int* len)
{

	Pctrl_tcp_rsqueue tmp;
	printf("%s %s,%d\n",__FILE__,__func__,__LINE__);

	tmp = (Pctrl_tcp_rsqueue)malloc(sizeof(ctrl_tcp_rsqueue));
	memset(tmp,0,sizeof(ctrl_tcp_rsqueue));

	tmp->msg = malloc(*len);
	memset(tmp->msg,0,*len);
	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = *fd;
	tmp->len = *len;
	memcpy(tmp->msg,msg,tmp->len);

	enter_queue(node_queue->sys_queue[CTRL_TCP_RECV_QUEUE],tmp);

	sem_post(&sys_in.sys_sem[CTRL_TCP_RECV_SEM]);

	return SUCCESS;

}

/*
 * tcp_ctrl_tprecv_dequeue
 * tcp控制模块的接收的数据处队列
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 *
 * in/out:
 * @Pctrl_tcp_rsqueue
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tprecv_dequeue(Pctrl_tcp_rsqueue msg_tmp)
{
	int ret;
	Plinknode node;
	Pctrl_tcp_rsqueue tmp;

	sem_wait(&sys_in.sys_sem[CTRL_TCP_RECV_SEM]);

	printf("%s %s,%d\n",__FILE__,__func__,__LINE__);

	ret = out_queue(node_queue->sys_queue[CTRL_TCP_RECV_QUEUE],&node);


	if(ret == 0)
	{
		tmp = node->data;

		msg_tmp->socket_fd = tmp->socket_fd;
		msg_tmp->len = tmp->len;
		memcpy(msg_tmp->msg,tmp->msg,tmp->len);

		free(tmp->msg);
		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_tcp_send_enqueue
 * tcp控制模块的数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg)
{

	Pctrl_tcp_rsqueue tmp;

	tmp = (Pctrl_tcp_rsqueue)malloc(sizeof(ctrl_tcp_rsqueue));
	memset(tmp,0,sizeof(ctrl_tcp_rsqueue));

	tmp->msg = malloc(frame_type->frame_len);
	memset(tmp->msg,0,frame_type->frame_len);

	printf("%s %s,%d\n",__FILE__,__func__,__LINE__);
#if TCP_DBG
	printf("tcp_ctrl_tpsend_enqueue the queue..\n");
#endif

	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = frame_type->fd;
	tmp->len = frame_type->frame_len;
	memcpy(tmp->msg,msg,frame_type->frame_len);

	enter_queue(node_queue->sys_queue[CTRL_TCP_SEND_QUEUE],tmp);

	sem_post(&sys_in.sys_sem[CTRL_TCP_SEND_SEM]);

	return SUCCESS;
}


/*
 * tcp_ctrl_tpsend_dequeue
 * tcp控制模块的数据出队列
 * 将消息数据送入发送队列等待发送
 *
 * in/out:
 * @Ptcp_send 数据信息结构体
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tpsend_dequeue(Pctrl_tcp_rsqueue event_tmp)
{

	int ret;
	Plinknode node;
	Pctrl_tcp_rsqueue tmp;

	sem_wait(&sys_in.sys_sem[CTRL_TCP_SEND_SEM]);

#if TCP_DBG
	printf("get the value from tcp_ctrl_tpsend_outqueue queue\n");
#endif

	ret = out_queue(node_queue->sys_queue[CTRL_TCP_SEND_QUEUE],&node);


	if(ret == 0)
	{
		tmp = node->data;
		event_tmp->socket_fd = tmp->socket_fd;
		event_tmp->len = tmp->len;
		memcpy(event_tmp->msg,tmp->msg,tmp->len);

		free(tmp->msg);
		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_report_enqueue
 * 上报数据数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @value 事件信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_report_enqueue(Pframe_type type,int value)
{

	Prun_status event_tmp;
	event_tmp = (Prun_status)malloc(sizeof(run_status));
	memset(event_tmp,0,sizeof(run_status));

	printf("%s-%s-%d,value:%d\n",__FILE__,__func__,
			__LINE__,value);

	/*
	 * 单元机固有属性
	 */
	event_tmp->socket_fd = type->fd;
	event_tmp->id = type->con_data.id;
	event_tmp->seat = type->con_data.seat;
	event_tmp->value = value;
#if TCP_DBG
	printf("enter the tcp_ctrl_report_enqueue..\n");
#endif

	pthread_mutex_lock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);
	enter_queue(node_queue->sys_queue[LOCAL_REP_QUEUE],event_tmp);
	pthread_mutex_unlock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);

	sem_post(&sys_in.sys_sem[LOCAL_REP_SEM]);

	return SUCCESS;
}

int tcp_ctrl_report_dequeue(Prun_status event_tmp)
{

	int ret;
	Plinknode node;
	Prun_status tmp;

	sem_wait(&sys_in.sys_sem[LOCAL_REP_SEM]);

#if TCP_DBG
	printf("get the value from tcp_ctrl_report_dequeue\n");
#endif
	pthread_mutex_lock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);
	ret = out_queue(node_queue->sys_queue[LOCAL_REP_QUEUE],&node);
	pthread_mutex_unlock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);

	if(ret == 0)
	{
		tmp = node->data;
		memcpy(event_tmp,tmp,sizeof(run_status));

		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}


/*
 * conf_status_check_client_legal
 * 检查数据包来源合法性，会议表中是否有此设备
 * 并将此设备的席位赋值给临时变量
 *
 * @Pframe_type
 *
 */
int conf_status_check_client_legal(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->fd == frame_type->fd
				&& frame_type->s_id == con_list->con_data.id)
		{
			frame_type->con_data.seat = con_list->con_data.seat;
			frame_type->con_data.id = con_list->con_data.id;
			pos++;
			break;
		}

		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_check_chariman_legal
 * 检查数据包来源合法性，是否是主席单元发送的数据
 *
 * @Pframe_type
 *
 */
int conf_status_check_chariman_legal(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int pos = 0;

	/*
	 * 判断消息是否是主席单元发送
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->fd == frame_type->fd)
		{
			if(con_list->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
			{
				pos++;
				break;
			}
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_find_did_sockfd
 * 检测临时变量中源地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_did_sockfd(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->con_data.id == frame_type->d_id)
		{
			frame_type->fd=con_list->fd;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_find_chariman_sockfd
 * 检测链表中，主席单元的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_chariman_sockfd(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
		{
			printf("find the chariman\n");
			frame_type->fd=con_list->fd;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}
/*
 * conf_status_set_cmspk
 * 会议中主席的发言状态
 *
 * @value
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_set_cmspk(int value){


	node_queue->con_status->chirman_t = value;

	return SUCCESS;
}


/*
 * conf_status_get_cmspk
 * 会议中主席的发言状态
 *
 * 返回值：
 * @status
 */
int conf_status_get_cmspk(){

	return node_queue->con_status->chirman_t;
}

/*
 * conf_status_set_current_subject
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_set_current_subject(unsigned char num)
{
	node_queue->con_status->sub_num = num;
	return SUCCESS;
}

/*
 * conf_status_get_subject_property
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_current_subject()
{
	return node_queue->con_status->sub_num;
}

/*
 * conf_status_get_subject_property
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_subject_property(unsigned char* num)
{
	if(num == NULL)
		return node_queue->con_status->sub_list[node_queue->con_status->sub_num].subj_prop;
	else
		return node_queue->con_status->sub_list[*num].subj_prop;

}

/*
 * conf_status_save_vote_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_vote_result(int value)
{
	int sub_num = conf_status_get_current_subject();

	switch(value)
	{
	case WIFI_MEETING_EVENT_VOTE_ASSENT:
		node_queue->con_status->sub_list[sub_num].v_result.assent++;
		break;
	case WIFI_MEETING_EVENT_VOTE_NAY:
		node_queue->con_status->sub_list[sub_num].v_result.nay++;
		break;
	case WIFI_MEETING_EVENT_VOTE_WAIVER:
		node_queue->con_status->sub_list[sub_num].v_result.waiver++;
		break;
	case WIFI_MEETING_EVENT_VOTE_TIMEOUT:
		node_queue->con_status->sub_list[sub_num].v_result.timeout++;
		break;
	}

	return SUCCESS;
}
/*
 * conf_status_get_vote_result
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_vote_result(unsigned char* num,unsigned short* value)
{
	int sub_num = *num;
	int c_num = conf_status_get_current_subject();

	if(num == NULL)
	{
		value[0] =
				node_queue->con_status->sub_list[c_num].v_result.assent;
		value[1] =
				node_queue->con_status->sub_list[c_num].v_result.nay;
		value[2] =
				node_queue->con_status->sub_list[c_num].v_result.waiver;
		value[3] =
				node_queue->con_status->sub_list[c_num].v_result.timeout;

	}else{
		value[0] =
				node_queue->con_status->sub_list[sub_num].v_result.assent;
		value[1] =
				node_queue->con_status->sub_list[sub_num].v_result.nay;
		value[2] =
				node_queue->con_status->sub_list[sub_num].v_result.waiver;
		value[3] =
				node_queue->con_status->sub_list[sub_num].v_result.timeout;

	}

	return SUCCESS;
}


/*
 * conf_status_save_vote_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_elec_result(unsigned short value)
{
	int sub_num = conf_status_get_current_subject();

	node_queue->con_status->sub_list[sub_num].ele_result.ele_id[value]++;

	return SUCCESS;
}

/*
 * conf_status_save_vote_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_get_elec_result(unsigned short value)
{
	int sub_num = conf_status_get_current_subject();
	return node_queue->con_status->sub_list[sub_num].ele_result.ele_id[value];
}

/*
 * conf_status_get_elec_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_elec_totalp()
{
	int sub_num = conf_status_get_current_subject();
	return node_queue->con_status->sub_list[sub_num].ele_result.ele_total;
}

/*
 * conf_status_save_score_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_score_result(unsigned char value)
{
	int sub_num = conf_status_get_current_subject();
	node_queue->con_status->sub_list[sub_num].scr_result.score += value;
	node_queue->con_status->sub_list[sub_num].scr_result.num_peop++;
	return SUCCESS;
}

/*
 * conf_status_calc_score_result
 * 计算积分结果
 *
 * @value投票结果
 *
 */
int conf_status_calc_score_result()
{
	unsigned char value = 0;
	int sub_num = conf_status_get_current_subject();

	value = node_queue->con_status->sub_list[sub_num].scr_result.score /
					node_queue->con_status->sub_list[sub_num].scr_result.num_peop;
	node_queue->con_status->sub_list[sub_num].scr_result.score_r = value;

	return SUCCESS;
}

/*
 * conf_status_get_score_result
 * 获取积分结果
 *
 * @value投票结果
 *
 */
int conf_status_get_score_result(Pscore_result result)
{
	int sub_num = conf_status_get_current_subject();

	result->num_peop = conf_status_get_score_totalp();
	result->score = node_queue->con_status->sub_list[sub_num].scr_result.score;
	result->score_r = node_queue->con_status->sub_list[sub_num].scr_result.score_r;

	return SUCCESS;
}

/*
 * conf_status_get_score_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_score_totalp()
{
	int sub_num = conf_status_get_current_subject();
	return node_queue->con_status->sub_list[sub_num].scr_result.num_peop;
}



























