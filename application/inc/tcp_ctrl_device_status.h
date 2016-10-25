/*
 * tcp_ctrl_device_status.h
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DEVICE_STATUS_H_
#define INC_TCP_CTRL_DEVICE_STATUS_H_

#include "../inc/tcp_ctrl_queue.h"





typedef enum{

	//宣告上线
	WIFI_MEETING_EVENT_ONLINE_REQ = 1,
	//宣告离线
	WIFI_MEETING_EVENT_OFFLINE_REQ,
	//电源开、关
	WIFI_MEETING_EVENT_POWER_ON,
	WIFI_MEETING_EVENT_POWER_OFF,
	//FIFO、标准、自由
	WIFI_MEETING_EVENT_MIC_FIFO,
	WIFI_MEETING_EVENT_MIC_STAD,
	WIFI_MEETING_EVENT_MIC_FREE,
	//发言允许、拒绝，音频下发、拒绝，请求发言
	WIFI_MEETING_EVENT_SPK_ALLOW,
	WIFI_MEETING_EVENT_SPK_VETO,
	WIFI_MEETING_EVENT_SPK_ALOW_SND,
	WIFI_MEETING_EVENT_SPK_VETO_SND,
	WIFI_MEETING_EVENT_SPK_REQ_SND,
	WIFI_MEETING_EVENT_SPK_REQ_SPK,
	//投票管理
	WIFI_MEETING_EVENT_VOTE_START,
	WIFI_MEETING_EVENT_VOTE_END,
	WIFI_MEETING_EVENT_VOTE_ASSENT,
	WIFI_MEETING_EVENT_VOTE_NAY,
	WIFI_MEETING_EVENT_VOTE_WAIVER,
	WIFI_MEETING_EVENT_VOTE_TOUT,

	//议题管理
	WIFI_MEETING_EVENT_SUBJECT_ONE,
	WIFI_MEETING_EVENT_SUBJECT_TWO,
	WIFI_MEETING_EVENT_SUBJECT_THREE,
	WIFI_MEETING_EVENT_SUBJECT_FOUR,
	WIFI_MEETING_EVENT_SUBJECT_FIVE,
	WIFI_MEETING_EVENT_SUBJECT_SIX,
	WIFI_MEETING_EVENT_SUBJECT_SEVEN,
	WIFI_MEETING_EVENT_SUBJECT_EIGHT,
	WIFI_MEETING_EVENT_SUBJECT_NINE,
	WIFI_MEETING_EVENT_SUBJECT_TEN,
	//音效管理
	WIFI_MEETING_EVENT_SOUND,
	//会议服务
	WIFI_MEETING_EVENT_SERVICE_WATER,
	WIFI_MEETING_EVENT_SERVICE_PEN,
	WIFI_MEETING_EVENT_SERVICE_PAPER,
	//全状态
	WIFI_MEETING_EVENT_ALL_STATUS,
	//ssid和密码
	WIFI_MEETING_EVENT_SSID_KEY,
	//mac地址
	WIFI_MEETING_EVENT_MAC,
	//签到
	WIFI_MEETING_EVENT_CHECKIN_START,
	WIFI_MEETING_EVENT_CHECKIN_END,
	WIFI_MEETING_EVENT_CHECKIN_SELECT,
	//会议参数设置成功、失败，查询应答
	WIFI_MEETING_CONF_WREP_SUCCESS,
	WIFI_MEETING_CONF_WREP_ERR,
	WIFI_MEETING_CONF_RREP,
	//赞成、反对、弃权、超时
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

}test_event,*Ptest_event;















int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value);

int tcp_ctrl_report_dequeue(Pqueue_event* event_tmp);

int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg);
int tcp_ctrl_tpsend_dequeue(Ptcp_send* event_tmp);


#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
