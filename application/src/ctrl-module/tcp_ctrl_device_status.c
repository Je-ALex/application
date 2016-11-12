/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "tcp_ctrl_device_status.h"

#include "tcp_ctrl_api.h"

extern sys_info sys_in;
extern Pmodule_info node_queue;



/*
 * 会议信息存储函数
 *
 */
int tcp_ctrl_refresh_conference_list(Pconference_info data_info)
{
	pclient_node tmp = NULL;
	pclient_node del = NULL;
	Pclient_info cinfo;
	Pconference_info info;


	int pos = 0;
	int status = 0;


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
int tcp_ctrl_tprecv_dequeue(Pctrl_tcp_rsqueue event_tmp)
{

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

	Pclient_info pinfo;
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
//		memcpy(*event_tmp,tmp,sizeof(tcp_send_queue));
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

int tcp_ctrl_report_dequeue(Prun_status* event_tmp)
{

	int ret;
	int value;
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
		memcpy(*event_tmp,tmp,sizeof(run_status));

		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}




int tcp_ctrl_pc_enqueue(Pframe_type frame_type,int value)
{

	Pclient_info tmp_info;

	tmp_info = (Pclient_info)malloc(sizeof(client_info));
	memset(tmp_info,0,sizeof(client_info));

	printf("%s,%d\n",__func__,__LINE__);
	/*
	 * 单元机固有属性
	 */
	tmp_info->client_fd = frame_type->fd;

	pthread_mutex_lock(&sys_in.sys_mutex[PC_REP_MUTEX]);
	enter_queue(node_queue->sys_queue[CTRL_REP_PC_QUEUE],tmp_info);
	pthread_mutex_unlock(&sys_in.sys_mutex[PC_REP_MUTEX]);

	sem_post(&sys_in.sys_sem[PC_REP_SEM]);

	return SUCCESS;
}





















