/*
 * tcp_ctrl_device_status.h
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_DEVICE_STATUS_H_
#define HEADER_TCP_CTRL_DEVICE_STATUS_H_

#include "tcp_ctrl_queue.h"


extern sem_t queue_sem;

extern sem_t status_sem;


typedef enum{

	WIFI_MEETING_EVENT_POWER_ON = 1,
	WIFI_MEETING_EVENT_POWER_OFF,

	WIFI_MEETING_EVENT_MIC_FIFO,
	WIFI_MEETING_EVENT_MIC_STAD,
	WIFI_MEETING_EVENT_MIC_FREE,

	WIFI_MEETING_EVENT_SPK_ALLOW,
	WIFI_MEETING_EVENT_SPK_VETO,
	WIFI_MEETING_EVENT_SPK_ALOW_SND,
	WIFI_MEETING_EVENT_SPK_VETO_SND,
	WIFI_MEETING_EVENT_SPK_REQ_SPK,


}ALL_STATUS;






















int tcp_ctrl_enter_queue(Pframe_type frame_type);
int tcp_ctrl_out_queue(Pqueue_event event_tmp);
int process_queue_info(Pqueue_event event_tmp);


#endif /* HEADER_TCP_CTRL_DEVICE_STATUS_H_ */
