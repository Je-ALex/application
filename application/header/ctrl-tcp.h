/*
 * ctrl-tcp.h
 *
 *  Created on: 2016年10月9日
 *      Author: leon
 */

#ifndef HEADER_CTRL_TCP_H_
#define HEADER_CTRL_TCP_H_


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
	/*终端id属性,若是主机则为空*/
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


enum{

	SOCKERRO = -3,
	BINDERRO,
	LISNERRO,
	SUCCESS = 0,
	ERROR = -1,

};
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
 * 0x01	设置信息
 * 0x02	查询信息
 * 0x03	请求信息
 * 0x04 应答消息
 */
typedef enum{

	//控制
	Write_msg = 1,
	//读取
	Read_msg,
	//以上是发送端的类型

	//请求类型可能是发送类，可能是接收类
	Request_msg,
	//以下是接收端的类型
	//应答
	Reply_msg,

}Message_Type;

/*
 * event data
 */
typedef struct {

	unsigned char unit_power;


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

	char msg_type;
	char data_type;
	char dev_type;

	//查询是需要给此赋值
	unsigned char name_type[128];
	unsigned char code_type[128];

	int fd;
	int data_len;
	int frame_len;

	event_data evt_data;
	conference_data con_data;

}frame_type,*Pframe_type;

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

static void tc_process_ctrl_msg_from_reply(int* client_fd,char* msg,Pframe_type frame_type);




#endif /* HEADER_CTRL_TCP_H_ */
