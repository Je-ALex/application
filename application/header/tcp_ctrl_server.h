
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


typedef enum{

	SOCKERRO = -3,
	BINDERRO,
	LISNERRO,
	SUCCESS = 0,
	ERROR = -1,

}error;


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
	REPLY_MSG,
	R_REPLY_MSG,

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
 * 会议类名称编码
 *
 */
typedef enum {

	WIFI_MEETING_CON_ID  = 1,
	WIFI_MEETING_CON_SEAT,
	WIFI_MEETING_CON_NAME ,
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
	WIFI_MEETING_EVT_STAT,
	WIFI_MEETING_EVT_SSID,
	WIFI_MEETING_EVT_KEY,
	WIFI_MEETING_EVT_MAC,

}event_name_type;


/*
 * 主机TCP控制模块保存的终端信息参数
 *
 * 不区分上位机还是单元机，统一使用这个结构体，保存
 * @client_from 描述终端设备类型，单元机（0x02）还是上位机(0x01)
 */
typedef struct{

	/*终端的源，区分上位机还是单元机*/
	char	client_from;
	/*终端sock_fd属性*/
	int 	client_fd;
	/*终端id属性,若是pc则为空*/
	int 	client_id;
	/*终端mac属性*/
	char 	client_mac[24];
	/*终端ip属性信息*/
	char 	ip_addr[20];
	struct sockaddr_in cli_addr;
	int 	clilen;
	/*终端席别属性，若是主机则为默认值0*/
	char 	client_seats;
	/*终端的姓名属性,主机则默认为host_pc*/
	char	client_name[32];

} client_info,*Pclient_info;

/*
 * event data
 */
typedef struct {

	unsigned char unit_power;
	unsigned char unit_mic;
	unsigned char unit_speak;
	unsigned char unit_vote;

}event_data;
/*
 * conference data
 */
typedef struct {

	int id;
	unsigned char seat;
	unsigned char name[128];
	unsigned char subj[128];


}conference_data;

typedef struct {

	unsigned char msg_type;
	unsigned char data_type;
	unsigned char dev_type;

	//查询是需要给此赋值
	unsigned char name_type[128];
	unsigned char code_type[128];

	int fd;
	int data_len;
	int frame_len;

	event_data evt_data;
	conference_data con_data;

}frame_type,*Pframe_type;


#endif /* HEADER_TCP_CTRL_SERVER_H_ */