/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */


#include "../../header/tcp_ctrl_device_status.h"

extern Plinkqueue report_queue;



int tcp_ctrl_enter_queue(Pframe_type frame_type)
{

	Pqueue_event event_tmp;

	event_tmp = (Pqueue_event)malloc(sizeof(queue_event));
	memset(event_tmp,0,sizeof(queue_event));

	event_tmp->socket_fd = frame_type->fd;
	event_tmp->type = frame_type->data_type;
	event_tmp->name = frame_type->name_type[0];

	if(frame_type->data_type == EVENT_DATA)
		event_tmp->value = frame_type->evt_data.value;

	printf("enter the queue..\n");
	enter_queue(report_queue,event_tmp);
	sem_post(&queue_sem);

	return SUCCESS;
}

int tcp_ctrl_out_queue(Pqueue_event event_tmp)
{

	int ret;
	Plinknode node;

	sem_wait(&queue_sem);
	printf("get the value from queue\n");
	ret = out_queue(report_queue,&node);

	if(ret == 0)
	{
		event_tmp = node->data;
		free(node);
//		free(event_tmp);
	}
	process_queue_info(event_tmp);
	return SUCCESS;
}

int process_queue_info(Pqueue_event event_tmp)
{

	int fd = 0;
	int value = 0;

	fd = event_tmp->socket_fd;

	switch(event_tmp->type)
	{
		/*
		 * 事件状态改变
		 */
		case EVENT_DATA:
		{
			switch(event_tmp->name)
			{
			case WIFI_MEETING_EVT_PWR:
			{
				switch(event_tmp->value)
				{
				case WIFI_MEETING_EVT_PWR_ON:
					value = WIFI_MEETING_EVENT_POWER_ON;
					break;
				case WIFI_MEETING_EVT_PWR_OFF:
					value = WIFI_MEETING_EVENT_POWER_OFF;
					break;
				}
				break;
			}
			case WIFI_MEETING_EVT_MIC:
			{
				switch(event_tmp->value)
				{
				case WIFI_MEETING_EVT_MIC_FIFO:
					value = WIFI_MEETING_EVENT_MIC_FIFO;
					break;
				case WIFI_MEETING_EVT_MIC_STAD:
					value = WIFI_MEETING_EVENT_MIC_STAD;
					break;
				case WIFI_MEETING_EVT_MIC_FREE:
					value = WIFI_MEETING_EVENT_MIC_FREE;
					break;
				}

				break;
			}
			case WIFI_MEETING_EVT_SPK:
			{
				switch(event_tmp->value)
				{
				case WIFI_MEETING_EVT_SPK_ALLOW:
					value = WIFI_MEETING_EVENT_SPK_ALLOW;
					break;
				case WIFI_MEETING_EVT_SPK_VETO:
					value = WIFI_MEETING_EVENT_SPK_VETO;
					break;
				case WIFI_MEETING_EVT_SPK_ALOW_SND:
					value = WIFI_MEETING_EVENT_SPK_ALOW_SND;
					break;
				case WIFI_MEETING_EVT_SPK_VETO_SND:
					value = WIFI_MEETING_EVENT_SPK_VETO_SND;
					break;
				case WIFI_MEETING_EVT_SPK_REQ_SPK:
					value = WIFI_MEETING_EVENT_SPK_REQ_SPK;
					break;
				}

				break;
			}
			}
		}

			break;
			/*
			 * 会议状态
			 */
		case CONFERENCE_DATA:
			break;


	}

	set_device_status(fd,value);

	return SUCCESS;
}



























