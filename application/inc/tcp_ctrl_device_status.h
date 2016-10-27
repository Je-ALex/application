/*
 * tcp_ctrl_device_status.h
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DEVICE_STATUS_H_
#define INC_TCP_CTRL_DEVICE_STATUS_H_

#include "tcp_ctrl_queue.h"
#include "tcp_ctrl_api.h"




/*
 * tcp发送队列消息
 * socketfd
 * len
 * conrent
 */
typedef struct{

	int socket_fd;
	int len;
	unsigned char* msg;

}tcp_send,*Ptcp_send;











int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value);

int tcp_ctrl_report_dequeue(Pqueue_event* event_tmp);

int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg);
int tcp_ctrl_tpsend_dequeue(Ptcp_send* event_tmp);


#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
