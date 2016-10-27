/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "../../inc/tcp_ctrl_device_status.h"
#include "../../inc/tcp_ctrl_list.h"
#include "../../inc/tcp_ctrl_api.h"

extern pclient_node list_head;
extern Plinkqueue report_queue;
extern Plinkqueue tcp_send_queue;

extern sys_info sys_in;

extern Pmodule_info node_queue;

int tcp_ctrl_refresh_conference_list(Pconference_info data_info)
{
	pclient_node tmp = NULL;
	pclient_node del = NULL;
	Pclient_info cinfo;
	Pconference_info info;


	int pos = 0;
	int status = 0;


	tmp = node_queue->list_head->next;
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
	tmp = node_queue->conference_head->next;
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

	printf("pos--%d\n",pos);
	if(status > 0)
	{
		list_delete(node_queue->conference_head,pos,&del);
		info = del->data;
		status = 0;
		free(info);
		free(del);
	}else{

		printf("there is no data in the conference list,add it\n");
	}

	list_add(node_queue->conference_head,data_info);

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

	pclient_node node_tmp = NULL;
	Pclient_info pinfo;
	int status = 0;
	Ptcp_send tmp;

	tmp = (Ptcp_send)malloc(sizeof(tcp_send));
	memset(tmp,0,sizeof(tcp_send));
	printf("%s,%d\n",__func__,__LINE__);
#if TCP_DBG
	printf("tcp_ctrl_tpsend_enqueue the queue..\n");
#endif

	node_tmp = node_queue->list_head->next;
	while(node_tmp != NULL)
	{
		pinfo = node_tmp->data;
		if(pinfo->client_fd == frame_type->fd)
		{
			status++;
			break;
		}
		node_tmp = node_tmp->next;
	}
	if(!status)
		return ERROR;
	pthread_mutex_lock(&sys_in.tpsend_queue_mutex);
	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = frame_type->fd;
	tmp->len = frame_type->frame_len;
	tmp->msg = msg;

	enter_queue(node_queue->tcp_send_queue,tmp);
	pthread_mutex_unlock(&sys_in.tpsend_queue_mutex);

	sem_post(&sys_in.tpsend_queue_sem);

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
int tcp_ctrl_tpsend_dequeue(Ptcp_send* event_tmp)
{

	int ret;
	int value;
	Plinknode node;
	Ptcp_send tmp;

	sem_wait(&sys_in.tpsend_queue_sem);

#if TCP_DBG
	printf("get the value from tcp_ctrl_tpsend_outqueue queue\n");
#endif
	pthread_mutex_lock(&sys_in.tpsend_queue_mutex);
	ret = out_queue(node_queue->tcp_send_queue,&node);
	pthread_mutex_unlock(&sys_in.tpsend_queue_mutex);

	if(ret == 0)
	{
		tmp = node->data;
		memcpy(*event_tmp,tmp,sizeof(tcp_send));

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
int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value)
{

	Prun_status event_tmp;

	event_tmp = (Prun_status)malloc(sizeof(run_status));
	memset(event_tmp,0,sizeof(run_status));

	/*
	 * 单元机固有属性
	 */
	event_tmp->socket_fd = frame_type->fd;
	event_tmp->id = frame_type->con_data.id;
	event_tmp->seat = frame_type->con_data.seat;
	event_tmp->value = value;
#if TCP_DBG
	printf("enter the tcp_ctrl_report_enqueue..\n");
#endif
	pthread_mutex_lock(&sys_in.tcp_mutex);
	enter_queue(node_queue->report_queue,event_tmp);
	pthread_mutex_unlock(&sys_in.tcp_mutex);

	sem_post(&sys_in.local_report_sem);

	return SUCCESS;
}

int tcp_ctrl_report_dequeue(Prun_status* event_tmp)
{

	int ret;
	int value;
	Plinknode node;
	Prun_status tmp;

	printf("%s,%d\n",__func__,__LINE__);
	sem_wait(&sys_in.local_report_sem);
	printf("%s,%d\n",__func__,__LINE__);
#if TCP_DBG
	printf("get the value from tcp_ctrl_report_dequeue\n");
#endif
	pthread_mutex_lock(&sys_in.tcp_mutex);
	ret = out_queue(node_queue->report_queue,&node);
	pthread_mutex_unlock(&sys_in.tcp_mutex);

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




int tcp_ctrl_enter_pc_queue(Pframe_type frame_type,int value)
{

	Pclient_info tmp_info;

	tmp_info = (Pclient_info)malloc(sizeof(client_info));
	memset(tmp_info,0,sizeof(client_info));

	/*
	 * 单元机固有属性
	 */
	tmp_info->client_fd = frame_type->fd;

	printf("enter the queue..\n");

	pthread_mutex_lock(&sys_in.tcp_mutex);
	enter_queue(node_queue->report_queue,tmp_info);
	pthread_mutex_unlock(&sys_in.tcp_mutex);

	sem_post(&sys_in.local_report_sem);

	return SUCCESS;
}





















