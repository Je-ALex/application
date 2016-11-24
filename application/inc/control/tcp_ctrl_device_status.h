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
#include "tcp_ctrl_list.h"

typedef struct{

	pclient_node* sys_list;
	Plinkqueue* sys_queue;
	/*
	 * 会议实时状态
	 */
	Pconference_status con_status;

}global_info,*Pglobal_info;

/*
 * tcp控制模块收发数据信息
 * @socketfd 套接字号
 * @len 数据包长度
 * @msg 详细数据内容
 */
typedef struct{

	int socket_fd;
	int len;
	unsigned char* msg;

}ctrl_tcp_rsqueue,*Pctrl_tcp_rsqueue;



int tcp_ctrl_refresh_conference_list(Pconference_list data_info);


int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value);
int tcp_ctrl_report_dequeue(Prun_status event_tmp);

int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg);
int tcp_ctrl_tpsend_dequeue(Pctrl_tcp_rsqueue event_tmp);

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
int tcp_ctrl_tprecv_enqueue(int* fd,unsigned char* msg,int* len);


int tcp_ctrl_pc_enqueue(Pframe_type frame_type,int value);


#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
