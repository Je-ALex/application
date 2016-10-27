/*
 * tcp_ctrl_device_status.h
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DEVICE_STATUS_H_
#define INC_TCP_CTRL_DEVICE_STATUS_H_

#include "tcp_ctrl_list.h"
#include "tcp_ctrl_queue.h"
#include "tcp_ctrl_api.h"


typedef struct{

	/*
	 * 1、连接信息链表，这个链表主要是生成相关的文件后，QT层可以读取该文件，进行相关操作
	 * 2、终端会议状态链表，这个链表主要是主机自己用，不对外(QT)共享
	 */
	pclient_node list_head;
	pclient_node conference_head;
	/*
	 * 实时状态上报队列
	 */
	Plinkqueue report_queue;
	/*
	 * 实时状态上报pc队列
	 */
	Plinkqueue report_pc_queue;
	/*
	 * tcp发送队列
	 */
	Plinkqueue tcp_send_queue;
	/*
	 * 会议实时状态
	 */
	Pconference_status con_status;


}module_info,*Pmodule_info;

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






int tcp_ctrl_refresh_conference_list(Pconference_info data_info);




int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value);

int tcp_ctrl_report_dequeue(Prun_status* event_tmp);

int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg);
int tcp_ctrl_tpsend_dequeue(Ptcp_send* event_tmp);


#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
