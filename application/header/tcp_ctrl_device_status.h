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


	WIFI_MEETING_EVENT_CHECKIN,

	WIFI_MEETING_CONF_WREP_SUCCESS,
	WIFI_MEETING_CONF_WREP_ERR,
	WIFI_MEETING_CONF_RREP,

	WIFI_MEETING_CONF_REQ_VOTE_ASSENT,
	WIFI_MEETING_CONF_REQ_VOTE_NAY,
	WIFI_MEETING_CONF_REQ_VOTE_WAIVER,
	WIFI_MEETING_CONF_REQ_VOTE_TIMEOUT,

}ALL_STATUS;



/*
 * 队列消息
 * 在消息中需要告知哪个机器(socket_fd)状态改变
 * status标记为具体的改变状态类，如
 * 哪个类型(data_type),哪个内容(name_type),改变内容(value)
 */
typedef struct{

	/*
	 * 单元机识别
	 */
	int socket_fd;

	int id;
	unsigned char seat;

	/*
	 * 具体改变值
	 */
	unsigned short value;

}queue_event,*Pqueue_event;


















int tcp_ctrl_enter_queue(Pframe_type frame_type,int value);

int tcp_ctrl_out_queue(Pqueue_event* event_tmp);



#endif /* HEADER_TCP_CTRL_DEVICE_STATUS_H_ */
