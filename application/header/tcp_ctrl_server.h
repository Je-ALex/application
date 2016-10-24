
#ifndef HEADER_TCP_CTRL_SERVER_H_
#define HEADER_TCP_CTRL_SERVER_H_


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


#define CTRL_PORT 8080

#define MSG_TYPE 0xF0
#define DATA_TYPE 0x0C
#define MACHINE_TYPE 0x03

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
	WIFI_MEETING_EVT_SPK,
	WIFI_MEETING_EVT_VOT,
	WIFI_MEETING_EVT_SUB,
	WIFI_MEETING_EVT_SEFC,
	WIFI_MEETING_EVT_SER,
	WIFI_MEETING_EVT_ALL_STATUS,
	WIFI_MEETING_EVT_SSID,
	WIFI_MEETING_EVT_KEY,
	WIFI_MEETING_EVT_MAC,
	WIFI_MEETING_EVT_CHECKIN,

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
 * 主机TCP控制模块保存的终端信息参数
 *
 * 不区分上位机还是单元机，统一使用这个结构体，保存
 *
 */
typedef struct{

	/*终端sock_fd属性*/
	int 	client_fd;
	/*终端id属性,默认为0，若是pc则为1，主机机则为2，单元机则为3*/
	unsigned char 	client_name;
	/*终端mac属性*/
	char 	client_mac[24];
	/*终端ip属性信息*/
	struct sockaddr_in cli_addr;
	int 	clilen;

	//网关，掩码等等

	//参数
	int id;
	unsigned char seat;


} client_info,*Pclient_info;



/*
 * 会议动态全局变量
 */
typedef struct {

	int fd;
	char pc_status;
	unsigned char ssid[32];
	unsigned char password[64];
	unsigned char subj[10][128];
	vote_result v_result;

}conference_status,*Pconference_status;

int tcp_ctrl_refresh_conference_list(Pconference_info data_info);









#endif /* HEADER_TCP_CTRL_SERVER_H_ */
