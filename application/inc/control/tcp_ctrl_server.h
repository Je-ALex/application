
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
#define UDP_PORT 50001


#define MSG_TYPE 		0xF0 //消息类型
#define DATA_TYPE 		0x0C //数据类型
#define MACHINE_TYPE 	0x03 //设备类型

#define PC_ID 0xFFFF
/*
 * 系统调度所用到的互斥锁和信号量
 */
typedef struct{

	//设备控制模块中TCP数据收发互斥锁
	pthread_mutex_t ctrl_tcp_mutex;
	//本地状态上报QT消息队列互斥锁
	pthread_mutex_t lreport_queue_mutex;
	//设备控制模块消息进出队列互斥锁
	pthread_mutex_t ctcp_queue_mutex;
	//上报上位机状态互斥锁
	pthread_mutex_t pc_queue_mutex;

	//本地状态上报信号量
	sem_t local_report_sem;
	//上报上位机数据队列信号量
	sem_t pc_queue_sem;
	//tcp数据发送队列信号量
	sem_t tpsend_queue_sem;



	pthread_mutex_t sys_mutex[10];
	sem_t sys_sem[10];

}sys_info;

/*
 * system list/mutex/queue
 */
typedef enum{

	//设备连接链表
	CONNECT_LIST = 0,
	//会议信息链表
	CONFERENCE_LIST,

	//本地状态上报qt消息队列
	LOCAL_REP_QUEUE = 0,
	//状态上报上位机消息队列
	CTRL_REP_PC_QUEUE,
	//tcp接收数据消息队列
	CTRL_TCP_RECV_QUEUE,
	//tcp发送数据消息队列
	CTRL_TCP_SEND_QUEUE,


	//设备控制模块中TCP数据收发互斥锁
	CTRL_TCP_MUTEX = 0,
	//本地状态上报QT消息队列互斥锁
	LOCAL_REP_MUTEX,
	//上报上位机状态互斥锁
	PC_REP_MUTEX,
	//TCP接收消息进出队列互斥锁
	CTRL_TCP_RQUEUE_MUTEX,
	//TCP发送消息进出队列互斥锁
	CTRL_TCP_SQUEUE_MUTEX,


	//本地状态上报信号量
	LOCAL_REP_SEM = 0,
	//上报上位机数据队列信号量
	PC_REP_SEM,
	//TCP发送消息信号量
	CTRL_TCP_SEND_SEM,
	//TCP接收消息信号量
	CTRL_TCP_RECV_SEM,

}sys_signal;

/*
 * 系统错误码
 */
typedef enum{

	SOCKERRO = -20,
	BINDERRO,
	LISNERRO,
	SETOPTERRO,
	CONECT_NOT_LEGAL,

	BUF_HEAD_ERR,
	BUF_LEN_ERR,
	BUF_CHECKS_ERR,

	INIT_LIST_ERR,
	INIT_QUEUE_ERR,

	ERROR,
	SUCCESS = 0,

}errs;

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
	WIFI_MEETING_EVT_UNIT_ONOFF,
	WIFI_MEETING_EVT_ELECTION,
	WIFI_MEETING_EVT_SCORE,

}event_name_type;

typedef enum {
	WIFI_MEETING_EVT_PWR_ON = 1,
	WIFI_MEETING_EVT_PWR_OFF,
	WIFI_MEETING_EVT_PWR_OFF_ALL,
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

typedef enum {

	WIFI_MEETING_EVT_ELECTION_START = 1,
	WIFI_MEETING_EVT_ELECTION_END,

}event_election;

typedef enum {

	WIFI_MEETING_EVT_SCORE_START = 1,
	WIFI_MEETING_EVT_SCORE_END,

}event_score;







/*
 * 席别
 */
typedef enum {

	WIFI_MEETING_CON_SE_CHARIMAN = 1,
	WIFI_MEETING_CON_SE_GUEST,
	WIFI_MEETING_CON_SE_ATTEND,

}conference_seat;

/*
 * 事件数据
 */
typedef struct {

	unsigned char value;
	unsigned char ssid[32];
	unsigned char password[64];

}event_data;

//投票类型
typedef struct{

	unsigned short assent;
	unsigned short nay;
	unsigned short waiver;
	unsigned short timeout;

}vote_result;

//选举类型
typedef struct{

	unsigned short ele_id[256];

}election_result;

//计分类型
typedef struct{

	unsigned short score[256];

}score_result;

/*
 * 会议类参数
 */
typedef struct {

	/*
	 * 主机ID为0x0000，上位机ID为0xFFFF
	 */
	unsigned short id;
	unsigned char seat;
	unsigned char name[64];
	unsigned char conf_name[128];
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
	unsigned short s_id;
	unsigned short d_id;

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


	int pc_status;
	unsigned char ssid[32];
	unsigned char password[64];
	unsigned char subj[10][128];

	vote_result v_result;
	election_result ele_result;
	score_result scr_result;

	int mic_mode;
	int spk_number;

}conference_status,*Pconference_status;



/*
 * wifi_sys_ctrl_tcp_recv
 * 设备控制模块TCP数据接收模块
 *
 * 采用epoll模式，维护多终端
 * 接收客户端的通讯数据
 *
 */
void* wifi_sys_ctrl_tcp_recv(void* p);

/*
 * wifi_sys_ctrl_tcp_procs_data
 * 设备控制模块TCP接收数据处理线程
 *
 * 将消息从队列中读取，进行处理后发送
 *
 */
void* wifi_sys_ctrl_tcp_procs_data(void* p);

/*
 * wifi_sys_ctrl_tcp_send
 * 设备控制模块TCP数据下发线程
 *
 * 发送消息进入发送队列，进行数据下发
 *
 */
void* wifi_sys_ctrl_tcp_send(void* p);


void* control_tcp_send(void* p);
void* control_tcp_queue(void* p);



#endif /* INC_TCP_CTRL_SERVER_H_ */
