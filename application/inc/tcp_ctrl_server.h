
#ifndef INC_TCP_CTRL_SERVER_H_
#define INC_TCP_CTRL_SERVER_H_


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define TCP_DBG 0

#define CTRL_PORT 8080

#define MSG_TYPE 		0xF0 //消息类型
#define DATA_TYPE 		0x0C //数据类型
#define MACHINE_TYPE 	0x03 //设备类型

/*
 * 系统调度所用到的互斥锁和信号量
 */
typedef struct{

	//TCP收发互斥锁
	pthread_mutex_t tcp_mutex;
	//本地状态上队列报互斥锁
	pthread_mutex_t report_queue_mutex;
	//TCP发送队列互斥锁
	pthread_mutex_t tpsend_queue_mutex;
	//上报上位机状态互斥锁
	pthread_mutex_t pc_queue_mutex;

	//本地状态上报信号量
	sem_t local_report_sem;
	//上报上位机数据队列信号量
	sem_t pc_queue_sem;
	//tcp数据发送队列信号量
	sem_t tpsend_queue_sem;

}sys_info;

typedef enum{

	SOCKERRO = -3,
	BINDERRO,
	LISNERRO,

	SUCCESS = 0,
	ERROR = -1,

}error;

typedef struct{


}msg_queue;

/*
 * 应答类消息的错误码
 * 0X01 成功
 * 0X02 头错误
 * 0X03 长度错误
 * 0X04 校验和错误
 */
typedef enum{

	TCP_C_ERR_SUCCESS = 1,
	TCP_C_ERR_HEAD,
	TCP_C_ERR_LENGTH,
	TCP_C_ERR_CHECKSUM,

}tcp_ctrl_err;

/*
 * 0x01	控制内容信息
 * 0x02	查询内容信息
 * 0x03	请求内容信息
 * 0x04 控制应答响应
 * 0x05 查询应答响应
 */
typedef enum{


	WRITE_MSG = 1,
	READ_MSG,
	REQUEST_MSG,
	W_REPLY_MSG,
	R_REPLY_MSG,
	ONLINE_REQ,
	OFFLINE_REQ,

}Message_Type;
/*
 * 0x01	上位机
 * 0x02	主机数据
 * 0x03	单元机数据
 */
typedef enum{

	PC_CTRL = 1,
	HOST_CTRL,
	UNIT_CTRL,

}Machine_Type;

/*
 * 0x01	事件型数据
 * 0x03	会议型参数
 */
typedef enum{

	EVENT_DATA = 1,
	CONFERENCE_DATA,

}Data_Type;


/*
 * 数据格式编码
 *
 */
typedef enum {

	WIFI_MEETING_CHAR = 1,
	WIFI_MEETING_UCHAR,
	WIFI_MEETING_SHORT,
	WIFI_MEETING_USHORT,
	WIFI_MEETING_INT,
	WIFI_MEETING_UINT,
	WIFI_MEETING_LONG,
	WIFI_MEETING_ULONG,
	WIFI_MEETING_LONGLONG,
	WIFI_MEETING_STRING,

	WIFI_MEETING_ERROR = 0xe3,

}client_code_type;


/*
 * 投票结果
 *
 */
typedef enum {

	WIFI_MEETING_CON_V_ASSENT  = 1,
	WIFI_MEETING_CON_V_NAY,
	WIFI_MEETING_CON_V_WAIVER ,
	WIFI_MEETING_CON_V_TOUT ,

}vote_name;
/*
 * 会议类名称编码
 *
 */
typedef enum {

	WIFI_MEETING_CON_ID  = 1,
	WIFI_MEETING_CON_SEAT,
	WIFI_MEETING_CON_NAME ,
	WIFI_MEETING_CON_CNAME ,
	WIFI_MEETING_CON_SUBJ ,
	WIFI_MEETING_CON_VOTE ,

}conference_name_type;


/*
 * 事件类名称编码
 *
 */
typedef enum {

	WIFI_MEETING_EVT_PWR = 1,
	WIFI_MEETING_EVT_MIC,
	WIFI_MEETING_EVT_CHECKIN,
	WIFI_MEETING_EVT_SPK,
	WIFI_MEETING_EVT_VOT,
	WIFI_MEETING_EVT_SUB,
	WIFI_MEETING_EVT_SEFC,
	WIFI_MEETING_EVT_SER,
	WIFI_MEETING_EVT_ALL_STATUS,
	WIFI_MEETING_EVT_SSID,
	WIFI_MEETING_EVT_KEY,
	WIFI_MEETING_EVT_MAC,
	WIFI_MEETING_EVT_PC_GET_INFO,
	WIFI_MEETING_EVT_RP_TO_PC,


}event_name_type;

typedef enum {
	WIFI_MEETING_EVT_PWR_ON = 1,
	WIFI_MEETING_EVT_PWR_OFF,
}event_power;

typedef enum {
	WIFI_MEETING_EVT_MIC_FIFO = 1,
	WIFI_MEETING_EVT_MIC_STAD,
	WIFI_MEETING_EVT_MIC_FREE,
}event_mic;

typedef enum {
	WIFI_MEETING_EVT_SPK_ALLOW = 1,
	WIFI_MEETING_EVT_SPK_VETO,
	WIFI_MEETING_EVT_SPK_ALOW_SND,
	WIFI_MEETING_EVT_SPK_VETO_SND,
	WIFI_MEETING_EVT_SPK_REQ_SND,
	WIFI_MEETING_EVT_SPK_REQ_SPK,
}event_speak;

typedef enum {
	WIFI_MEETING_EVT_VOT_START = 1,
	WIFI_MEETING_EVT_VOT_END,
	WIFI_MEETING_EVT_VOT_ASSENT,
	WIFI_MEETING_EVT_VOT_NAY,
	WIFI_MEETING_EVT_VOT_WAIVER,
	WIFI_MEETING_EVT_VOT_TOUT,
}event_vote;


typedef enum {

	WIFI_MEETING_EVT_SER_WATER = 1,
	WIFI_MEETING_EVT_SER_PEN,
	WIFI_MEETING_EVT_SER_PAPER,

}event_service;

typedef enum {

	WIFI_MEETING_EVT_CHECKIN_START = 1,
	WIFI_MEETING_EVT_CHECKIN_END,
	WIFI_MEETING_EVT_CHECKIN_SELECT,

}event_checkin;











/*
 * 席别
 */
typedef enum {

	WIFI_MEETING_CON_SE_CHARIMAN = 1,
	WIFI_MEETING_CON_SE_GUEST,
	WIFI_MEETING_CON_SE_ATTEND,

}conference_seat;
/*
 * event data
 */
typedef struct {

	unsigned char value;
	unsigned char ssid[32];
	unsigned char password[64];

}event_data;

typedef struct{

	unsigned short assent;
	unsigned short nay;
	unsigned short waiver;
	unsigned short timeout;
}vote_result;

/*
 * conference data
 */
typedef struct {

	/*
	 * 主机ID为0x0000，上位机ID为0xFFFF
	 */
	int id;
	unsigned char seat;
	unsigned char name[128];
	unsigned char subj[10][128];
	vote_result v_result;

}conference_data;

typedef struct {

	unsigned char msg_type;
	unsigned char data_type;
	unsigned char dev_type;

	//查询是需要给此赋值,考虑现在有SSID和密码同时进行下发的情况，所以为数组
	unsigned char name_type[2];
	unsigned char code_type[2];

	int fd;
	int data_len;
	int frame_len;
	int s_id;
	int d_id;

	event_data evt_data;
	conference_data con_data;

}frame_type,*Pframe_type;

/*
 * 会议信息链表
 * 主要有fd和事件类、会议类内容
 */
typedef struct {

	int fd;
	event_data evt_data;
	conference_data con_data;

}conference_info,*Pconference_info;



/*
 * 会议动态全局变量
 */
typedef struct {

	int fd;
	int pc_status;
	unsigned char ssid[32];
	unsigned char password[64];
	unsigned char subj[10][128];
	vote_result v_result;

}conference_status,*Pconference_status;




int tcp_ctrl_refresh_conference_list(Pconference_info data_info);







#endif /* INC_TCP_CTRL_SERVER_H_ */
