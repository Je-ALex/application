/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_data_compose.h"

extern sys_info sys_in;
extern Pglobal_info node_queue;




/*
 * 系统调试开关设置
 */
int sys_debug_set_switch(unsigned char value)
{
	node_queue->con_status->debug_sw = value;
	return SUCCESS;
}

/*
 * 系统调试开关状态获取
 */
int sys_debug_get_switch()
{
	return node_queue->con_status->debug_sw;
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
//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

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

//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

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

//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

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

	Prun_status tmp;
	tmp = (Prun_status)malloc(sizeof(run_status));
	memset(tmp,0,sizeof(run_status));

//	printf("%s-%s-%d,value:%d\n",__FILE__,__func__,
//			__LINE__,value);

	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = type->fd;
	tmp->id = type->con_data.id;
	tmp->seat = type->con_data.seat;
	tmp->value = value;
#if TCP_DBG
	printf("enter the tcp_ctrl_report_enqueue..\n");
#endif

	pthread_mutex_lock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);
	enter_queue(node_queue->sys_queue[LOCAL_REP_QUEUE],tmp);
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
 * conf_status_get_connected_len
 * 获取会议中连接设备的数量
 *
 */
int conf_status_get_connected_len()
{
	return node_queue->sys_list[CONNECT_LIST]->size;
}


/*
 * conf_status_get_conference_len
 * 获取会议中参会设备数量
 *
 */
int conf_status_get_conference_len()
{
	return node_queue->sys_list[CONFERENCE_LIST]->size;
}


/*
 * conf_status_check_client_legal
 * 检查数据包来源合法性，会议表中是否有此设备
 * 并将此设备的席位赋值给临时变量
 *
 * @Pframe_type
 *
 */
int conf_status_check_client_conf_legal(Pframe_type frame_type)
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
 * conf_status_check_connect_legal
 * 检查连接信息中，设备的合法性
 *
 * @Pframe_type
 *
 */
int conf_status_check_client_connect_legal(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->client_fd == frame_type->fd)
		{
			frame_type->con_data.seat = cnet_list->seat;
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
int conf_status_check_chairman_legal(Pframe_type frame_type)
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
			if(con_list->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
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
 * conf_status_check_chariman_staus
 * 检查会议中是否有主席单元存在
 *
 * 返回值
 * @主席状态0无1有
 *
 */
int conf_status_check_chariman_staus()
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
		if(con_list->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_find_did_sockfd
 * 检测临时变量中目标地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_did_sockfd_id(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->id == frame_type->d_id)
		{
			frame_type->fd=cnet_list->client_fd;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	printf("%s-%s-%d，did=%d,pos=%d\n",__FILE__,__func__,__LINE__,frame_type->d_id,pos);
	return pos;
}

/*
 * conf_status_get_did_sockfd
 * 检测临时变量中目标地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_did_sockfd_sock(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->client_fd == frame_type->d_id)
		{
			frame_type->fd=cnet_list->client_fd;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	printf("%s-%s-%d，did=%d,pos=%d\n",__FILE__,__func__,__LINE__,frame_type->d_id,pos);
	return pos;
}


/*
 * conf_status_find_chariman_sockfd
 * 检测链表中，主席单元的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_chairman_sockfd(Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->con_data.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			printf("find the chairman\n");
			frame_type->fd=con_list->fd;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_find_max_id
 * 检测链表中，ID最大值
 *
 */
int conf_status_find_max_id()
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int max_id = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->con_data.id)
		{
			if(con_list->con_data.id > max_id)
			{
				max_id = con_list->con_data.id;
			}
		}
		tmp=tmp->next;
	}

	return max_id+1;
}

/*
 * conf_status_compare_id
 * 比较链表中id号
 *
 */
int conf_status_compare_id(int value)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->con_data.id == value)
		{
			pos++;
			break;
		}
		tmp=tmp->next;
	}
	return pos;
}



/*
 * conf_status_set_current_subject
 * 会议中投票议题的状态
 *
 */
int conf_status_set_current_subject(unsigned char num)
{

	node_queue->con_status->sub_num = num;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->sub_num);

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
 * conf_status_set_subject_property
 * 会议中投票议题的状态
 *
 */
int conf_status_set_subject_property(unsigned char num,unsigned char prop)
{
	printf("%s-%s-%d,num=%d,prop=%d\n",__FILE__,__func__,__LINE__,num,prop);
	node_queue->con_status->sub_list[num].subj_prop = prop;

	return SUCCESS;
}


/*
 * conf_status_get_subject_property
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_subject_property(unsigned char num)
{
	unsigned char sub_num;
	unsigned char prop = 0;

	if(num == 0)
	{
		sub_num = conf_status_get_current_subject();
		printf("%s-%s-%d,prop=%d\n",__FILE__,__func__,__LINE__,
				node_queue->con_status->sub_list[sub_num].subj_prop);
		prop = node_queue->con_status->sub_list[sub_num].subj_prop;
	}
	else
	{
		printf("%s-%s-%d,prop=%d\n",__FILE__,__func__,__LINE__,
				node_queue->con_status->sub_list[num].subj_prop);
		prop = node_queue->con_status->sub_list[num].subj_prop;
	}

	return prop;
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
	unsigned char sub_num;
	int sub_prop = 0;

	sub_num = conf_status_get_current_subject();
	printf("%s-%s-%d,sub_num=%d\n",__FILE__,__func__,__LINE__,sub_num);
	//判断当前议题的属性
	sub_prop = conf_status_get_subject_property(sub_num);
	if(sub_prop == WIFI_MEETING_CON_SUBJ_VOTE)
	{
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
	}else{
		printf("%s-%s-%d the subject is not vote subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}
/*
 * conf_status_get_vote_result
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_vote_result(unsigned char num,unsigned short* value)
{
	int c_num = 0;

	if(num == 0)
	{
		c_num = conf_status_get_current_subject();
		value[0] = node_queue->con_status->sub_list[c_num].v_result.assent;
		value[1] = node_queue->con_status->sub_list[c_num].v_result.nay;
		value[2] = node_queue->con_status->sub_list[c_num].v_result.waiver;
		value[3] = node_queue->con_status->sub_list[c_num].v_result.timeout;

	}else{
		value[0] = node_queue->con_status->sub_list[num].v_result.assent;
		value[1] = node_queue->con_status->sub_list[num].v_result.nay;
		value[2] = node_queue->con_status->sub_list[num].v_result.waiver;
		value[3] = node_queue->con_status->sub_list[num].v_result.timeout;

	}

	return SUCCESS;
}


/*
 * conf_status_save_elec_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_elec_result(unsigned short value)
{
	unsigned char sub_num = conf_status_get_current_subject();
	int sub_prop = 0;

	//判断当前议题的属性
	sub_prop = conf_status_get_subject_property(sub_num);
	if(sub_prop == WIFI_MEETING_CON_SUBJ_ELE)
	{

		node_queue->con_status->sub_list[sub_num].ele_result.ele_id[value]++;
	}else{
		printf("%s-%s-%d the subject is not ele subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
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
int conf_status_get_elec_result(unsigned char num,unsigned short value)
{
	int sub_num ;

	if(num == 0){
		sub_num = conf_status_get_current_subject();
	}else{
		sub_num = num;
	}
	return node_queue->con_status->sub_list[sub_num].ele_result.ele_id[value];
}


/*
 * conf_status_set_elec_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_set_elec_totalp(unsigned char num,unsigned char pep)
{

	node_queue->con_status->sub_list[num].ele_result.ele_total = pep;

	return SUCCESS;
}

/*
 * conf_status_get_elec_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_elec_totalp(unsigned char num)
{
	unsigned char sub_num ;
	int sub_prop = 0;

	if(num == 0){
		sub_num = conf_status_get_current_subject();
	}else{
		sub_num = num;
	}

	//判断当前议题的属性
	sub_prop = conf_status_get_subject_property(sub_num);
	if(sub_prop == WIFI_MEETING_CON_SUBJ_ELE)
	{
		return node_queue->con_status->sub_list[sub_num].ele_result.ele_total;
	}else
	{
		printf("%s-%s-%d the subject is not elec subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

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
	unsigned char sub_num = conf_status_get_current_subject();

	int sub_prop = 0;

	//判断当前议题的属性
	sub_prop = conf_status_get_subject_property(sub_num);
	if(sub_prop == WIFI_MEETING_CON_SUBJ_SCORE)
	{
		node_queue->con_status->sub_list[sub_num].scr_result.score += value;
		node_queue->con_status->sub_list[sub_num].scr_result.num_peop++;
	}else{
		printf("%s-%s-%d the subject is not score subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

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
 * 获取计分结果
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
 * 进行计分总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_score_totalp()
{
	int sub_num = conf_status_get_current_subject();
	return node_queue->con_status->sub_list[sub_num].scr_result.num_peop;
}


/*
 * TODO
 * 1/发言管理
 * 2/话筒管理
 * 3/音效管理
 * 4/会议管理
 *
 */


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
 * conf_status_set_mic_mode
 * 设置会议话筒模式
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */
int conf_status_set_mic_mode(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	value = value+WIFI_MEETING_EVT_MIC_CHAIRMAN;

	node_queue->con_status->mic_mode = value;

	return SUCCESS;
}

/*
 * conf_status_get_mic_mode
 * 获取话筒模式
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */
int conf_status_get_mic_mode()
{
	int mode;

	mode = node_queue->con_status->mic_mode;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			mode);
	return mode;
}

/*
 * conf_status_set_spk_num
 * 会议最大发言人数设置
 *
 * @value投票结果
 *
 */
int conf_status_set_spk_num(int value)
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->spk_number = value;

	return SUCCESS;
}

/*
 * conf_status_get_spk_num
 * 会议最大发言人数获取
 *
 * @value投票结果
 *
 */
int conf_status_get_spk_num()
{

	return node_queue->con_status->spk_number;
}

/*
 * conf_status_set_cspk_num
 * 会议当前发言人数设置
 *
 * @value投票结果
 *
 */
int conf_status_set_cspk_num(int value)
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);
	node_queue->con_status->current_spk = value;

	return SUCCESS;
}

/*
 * conf_status_get_cspk_num
 * 会议当前发言人数获取
 *
 *
 */
int conf_status_get_cspk_num()
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->current_spk);

	return node_queue->con_status->current_spk;
}

/*
 * conf_status_set_snd_brd
 * 会议当前音频下发状态
 *
 * @value投票结果
 *
 */
int conf_status_set_snd_brd(int value)
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->snd_brdcast = value;

	return SUCCESS;
}

/*
 * conf_status_get_snd_brd
 * 会议当前发言人数获取
 *
 */
int conf_status_get_snd_brd()
{

	return node_queue->con_status->snd_brdcast;
}

/*
 * conf_status_set_snd_effect
 * DSP音效设置
 * 采用为管理分别从bit[0-2]表示状态
 * bit   0    1     2
 *      AFC  ANC0  ANC1
 *              ANC2
 * bit[0-2] 共有8个状态
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_set_snd_effect(int value)
{
	printf("%s-%s-%d,value=0x%02x\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->snd_effect = value;

	uart_snd_effect_set(value);

	return SUCCESS;
}

/*
 * conf_info_get_snd_effect
 * DSP音效获取
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_get_snd_effect()
{
	printf("%s-%s-%d,value=0x%02x\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->snd_effect);

	return node_queue->con_status->snd_effect;

}

/*
 * conf_status_set_camera_track
 * 设置摄像跟踪设置
 */
int conf_status_set_camera_track(int value)
{
	node_queue->con_status->camera_track = value;

	return SUCCESS;
}

/*
 * conf_status_get_camera_track
 * 摄像跟踪获取
 */
int conf_status_get_camera_track()
{
	return node_queue->con_status->camera_track;
}

/*
 * conf_status_set_conf_staus
 * 设置会议进程状态
 *
 * @value
 *
 */
int conf_status_set_conf_staus(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->confer_status = value;

	return SUCCESS;
}

/*
 * conf_status_get_conf_staus
 * 获取会议进程状态
 *
 * @value
 *
 */
int conf_status_get_conf_staus()
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->confer_status);

	return node_queue->con_status->confer_status;
}

/*
 * conf_status_set_pc_staus
 * 设置会议中上位机的连接状态
 *
 * @value
 *
 */
int conf_status_set_pc_staus(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->pc_status = value;

	return SUCCESS;
}

/*
 * conf_status_get_pc_staus
 * 获取会议进程中上位机的连接状态
 *
 * @value
 *
 */
int conf_status_get_pc_staus()
{
//	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
//			node_queue->con_status->pc_status);

	return node_queue->con_status->pc_status;
}


int conf_status_send_vote_result()
{
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));
	unsigned short result[4] = {0};

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
	if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_VOTE)
	{
		data_info.name_type[0] = WIFI_MEETING_CON_VOTE;
		data_info.code_type[0] = WIFI_MEETING_STRING;

		conf_status_get_vote_result(0,result);

		data_info.con_data.v_result.assent = result[0];
		data_info.con_data.v_result.nay = result[1];
		data_info.con_data.v_result.waiver = result[2];
		data_info.con_data.v_result.timeout = result[3];

		data_info.msg_type = WRITE_MSG;
		data_info.data_type = CONFERENCE_DATA;
		data_info.dev_type = HOST_CTRL;

		tcp_ctrl_module_edit_info(&data_info,NULL);

	}else{

		printf("%s-%s-%d not vote subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	return SUCCESS;
}

int conf_status_send_elec_result()
{
	int i;
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
	if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_ELE)
	{
		data_info.name_type[0] = WIFI_MEETING_CON_ELEC;
		data_info.code_type[0] = WIFI_MEETING_STRING;
		data_info.msg_type = WRITE_MSG;
		data_info.data_type = CONFERENCE_DATA;
		data_info.dev_type = HOST_CTRL;

		//将全局变量中的数据保存到
		for(i=1;i<=conf_status_get_elec_totalp(0);i++)
		{
			data_info.con_data.elec_rsult.ele_id[i] = conf_status_get_elec_result(0,i);
		}

	}else{
		printf("%s-%s-%d not election subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	tcp_ctrl_module_edit_info(&data_info,NULL);


	return SUCCESS;
}

int conf_status_send_score_result()
{
	score_result result;

	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);
	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
	if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_SCORE)
	{
		conf_status_get_score_result(&result);
		data_info.name_type[0] = WIFI_MEETING_CON_SCORE;
		data_info.code_type[0] = WIFI_MEETING_STRING;

		data_info.msg_type = WRITE_MSG;
		data_info.data_type = CONFERENCE_DATA;
		data_info.dev_type = HOST_CTRL;

		data_info.con_data.src_result.score_r = result.score_r;
	}else{

		printf("%s-%s-%d not score subject\n",__FILE__,__func__,__LINE__);
		return ERROR;
	}

	tcp_ctrl_module_edit_info(&data_info,NULL);

	return SUCCESS;
}


