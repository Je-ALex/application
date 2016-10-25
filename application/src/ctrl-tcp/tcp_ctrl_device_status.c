/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "../../inc/tcp_ctrl_device_status.h"

#include "../../inc/tcp_ctrl_list.h"

extern pclient_node list_head;
extern Plinkqueue report_queue;
extern Plinkqueue tcp_send_queue;

extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t tpsend_queue_mutex;


extern sem_t queue_sem;
extern sem_t tpsend_queue_sem;

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

#if TCP_DBG
	printf("tcp_ctrl_tpsend_enqueue the queue..\n");
#endif

	node_tmp = list_head->next;
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
	pthread_mutex_lock(&tpsend_queue_mutex);
	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = frame_type->fd;
	tmp->len = frame_type->frame_len;
	tmp->msg = msg;

	enter_queue(tcp_send_queue,tmp);
	pthread_mutex_unlock(&tpsend_queue_mutex);

	sem_post(&tpsend_queue_sem);

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

	sem_wait(&tpsend_queue_sem);

#if TCP_DBG
	printf("get the value from tcp_ctrl_tpsend_outqueue queue\n");
#endif
	pthread_mutex_lock(&tpsend_queue_mutex);
	ret = out_queue(tcp_send_queue,&node);
	pthread_mutex_unlock(&tpsend_queue_mutex);

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
#if TCP_DBG
	printf("enter the tcp_ctrl_report_enqueue..\n");
#endif
	pthread_mutex_lock(&queue_mutex);
	enter_queue(report_queue,event_tmp);
	pthread_mutex_unlock(&queue_mutex);

	sem_post(&queue_sem);

	return SUCCESS;
}

int tcp_ctrl_report_dequeue(Pqueue_event* event_tmp)
{

	int ret;
	int value;
	Plinknode node;
	Pqueue_event tmp;

	sem_wait(&queue_sem);
#if TCP_DBG
	printf("get the value from tcp_ctrl_report_dequeue\n");
#endif
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





















