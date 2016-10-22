/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "../../header/tcp_ctrl_device_status.h"

extern Plinkqueue report_queue;
extern pthread_mutex_t queue_mutex;


int tcp_ctrl_enter_queue(Pframe_type frame_type,int value)
{

	Pqueue_event event_tmp;

	event_tmp = (Pqueue_event)malloc(sizeof(queue_event));
	memset(event_tmp,0,sizeof(queue_event));

	/*
	 * 单元机固有属性
	 */
	event_tmp->socket_fd = frame_type->fd;
	event_tmp->id = frame_type->con_data.id;
	event_tmp->seat = frame_type->con_data.seat;
	event_tmp->value = value;

	printf("enter the queue..\n");

	pthread_mutex_lock(&queue_mutex);
	enter_queue(report_queue,event_tmp);
	pthread_mutex_unlock(&queue_mutex);

	sem_post(&queue_sem);

	return SUCCESS;
}

int tcp_ctrl_out_queue(Pqueue_event* event_tmp)
{

	int ret;
	int value;
	Plinknode node;
	Pqueue_event tmp;

	sem_wait(&queue_sem);
	printf("get the value from queue\n");

	pthread_mutex_lock(&queue_mutex);
	ret = out_queue(report_queue,&node);
	pthread_mutex_unlock(&queue_mutex);

	if(ret == 0)
	{
		tmp = node->data;
		memcpy(*event_tmp,tmp,sizeof(queue_event));

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

	pthread_mutex_lock(&queue_mutex);
	enter_queue(report_queue,tmp_info);
	pthread_mutex_unlock(&queue_mutex);

	sem_post(&queue_sem);

	return SUCCESS;
}





















