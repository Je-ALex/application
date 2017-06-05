/*
 * wifi_sys_init.h
 *
 *  Created on: 2017年3月9日
 *      Author: leon
 */

#ifndef INC_WIFI_SYS_INIT_H_
#define INC_WIFI_SYS_INIT_H_

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
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "wifi_sys_queue.h"
#include "wifi_sys_list.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef char 	int8;
typedef short 	int16;
typedef int 	int32;
typedef unsigned char  	uint8;
typedef unsigned short 	uint16;
typedef unsigned int   	uint32;
typedef long long 		off64;


/*
 * socket相关端口信息
 */
#define CTRL_BROADCAST_PORT 50001	//控制广播端口
#define CTRL_TCP_PORT 		8080	//控制交互端口
#define	AUDIO_RECV_PORT 	14336	//音频流接收端口
#define	AUDIO_SEND_PORT 	15000	//音频流广播端口

/*
 * 设备系统参数
 */
#define LOG_FILE		"LOG.txt"
#define VERSION			"V 1.0.0-r61-603"
#define	MODEL			"DS-WF620M"
#define PRODUCT			"四川湖山电器有限责任公司"
#define CONNECT_FILE 	"connection.info"
#define PID_FILE 		"sys_init.pid"


#define DEBUG_SW  0

/*
 * 会议类相关宏定义
 */
#define PC_ID 			0xFFFF	//上位机默认ID
#define HOST_ID 		0x0		//主机默认ID

#define WIFI_SYS_ELE_NUM 			32		//被选举人中最大编号
#define WIFI_SYS_PEP_NAME_LEN		64		//名称长度
#define WIFI_SYS_CONF_NAME_LEN		128		//会议名称长度
#define WIFI_SYS_SUBJECT_NUM		50		//当前会议议题数目
#define WIFI_SYS_SUB_TITLE_LEN		128		//议题标题长度
#define WIFI_SYS_VAR_NUM			10		//系统信号数目

#define		AUDP_RECV_NUM	7 //网络音频接收数(+1偏移量)
#define 	MAX_SPK_NUM 	8 //音频数据总通道数(网络+本地+1偏移量)
#define 	DEF_SPK_NUM 	4 //默认发言人数
#define 	DEF_MIC_MODE 	2 //默认话筒发言模式



#define msleep(x) usleep(x*1000)


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

/*
 * TODO 系统基本信息，定义系统基础变量信息
 * 1、链表信息
 * 2、队列信息
 * 3、互斥锁新
 * 4、信号量信息
 */
typedef enum{

	//设备连接链表
	CONNECT_LIST = 0,
	//设备连接心跳链表
	CONNECT_HEART,
	//MAC映射表
	CONNECT_MAC_TABLE,
	//发言端口等信息链表
	CONFERENCE_SPK,
	//会议信息链表
	CONFERENCE_LIST,

	//本地状态上报qt消息队列
	LOCAL_REP_QUEUE = 0,
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
	//list互斥锁
	LIST_MUTEX,
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
 * 系统调度所用到的互斥锁和信号量
 * 互斥锁和信号量的内容由@sys_signal中进行了定义
 *
 */
typedef struct{

	pthread_mutex_t sys_mutex[WIFI_SYS_VAR_NUM];
	sem_t sys_sem[WIFI_SYS_VAR_NUM];

}sys_info;

/*
 * 网络信息参数
 */
typedef struct {

	 int8 ssid[32];
	 char key[64];
	 unsigned int mac;
	 int sockfd;
	 unsigned int ip;
	 unsigned short port;

}net_info,*Pnet_info;

/*
 * 投票型会议，投票管理内容
 * 单个议题的投票状态
 */
typedef struct{

	unsigned short assent;
	unsigned short nay;
	unsigned short waiver;
	unsigned short timeout;

}vote_result,*Pvote_result;


/*
 * 选举型会议
 * 需要标记选举人编号，在下发议题时，会解析出选举人编号
 * 单元机反馈后，更新被选举人的状态，结束选举后，计算被选举人的总的结果
 */
typedef struct{
	//选举编号ID
	unsigned short ele_id[WIFI_SYS_ELE_NUM];
	//被选举人总人数
	unsigned char ele_total;

}election_result,*Pelection_result;


/*
 * 计分型数据，单个议题的计分，所以一个变量就够了
 * 单元机会上报分数，根据不同id号，区分不同单元机的分数
 */
typedef struct{

	unsigned int score;
	unsigned short num_peop;
	//计分平均值
	float score_r;

}score_result,*Pscore_result;


/*
 * 提议公布结果内容
 */
typedef struct {

	//当前议题结果
	unsigned char conf_result[WIFI_SYS_CONF_NAME_LEN];
	unsigned char len;

}conf_pc_result;

typedef struct {

	vote_result v_result;
	election_result ele_result;
	score_result scr_result;

}sub_result,*Psub_result;

typedef struct {

	//议题名称和议题属性
	char sub[WIFI_SYS_SUB_TITLE_LEN];
	unsigned char slen;
	unsigned char sprop;

	sub_result sresult;

}sub_content,*Psub_content;

typedef struct {

	char conf_name[WIFI_SYS_CONF_NAME_LEN];
	unsigned char conf_nlen;
	sub_content scontent[WIFI_SYS_SUBJECT_NUM];

	unsigned char total_sub;

}conf_content,*Pconf_content;

typedef struct {

	//会议状态
	volatile unsigned char confer_status;
	//议题状态
	volatile unsigned char subject_status;
	//当前议题
	volatile unsigned char current_sub;

	conf_pc_result cresult;

}conf_status,*Pconf_status;

typedef struct {
	//话筒模式
	volatile unsigned char mic_mode;
	//音效模式
	volatile unsigned char snd_effect;
	//音频下发状态
	volatile unsigned char snd_brdcast;
	//发言人数
	volatile unsigned char spk_number;
	//当前发言人数
	volatile unsigned char current_spk;
	//主席发言状态
	volatile int chirman_t;
}audio_status,*Paudio_status;

typedef struct {
	//上位机状态
	volatile int pc_status;
	//摄像跟踪状态
	volatile unsigned char camera_track;
}common_status,*Pcommon_status;


/*
 * 全局变量
 * 1、会议内容部分
 * 2、会议状态部分
 * 3、音频状态部分
 * 4、其他参数部分
 */
typedef struct {


//1、会议内容部分
	conf_content ccontent;
//2、会议状态部分
	conf_status cstatus;
//3、音频状态部分
	audio_status astatus;
//4、其他参数部分
	common_status cmstatus;

	int lan_stat;
	//系统时间
	unsigned char sys_time[4];
	//系统时间秒
	unsigned int sys_stime;

	net_info network;

}conference_status,*Pconference_status;




typedef struct{

	pclient_node* sys_list;
	Plinkqueue* sys_queue;

	//会议实时状态
	Pconference_status con_status;

}global_info,*Pglobal_info;


/*
 * 单元相关的事件型数据
 * 主要是单元的网络参数
 * 事件相关的值
 * 单元电量信息
 */
typedef struct {

	net_info unet_info;
	unsigned char value;
	//上报状态信息
	unsigned char status;
	unsigned char electricity;

}event_data;


/*
 * 单元会议相关参数
 * ID/席别/姓名/姓名长度
 */
typedef struct {

	unsigned short id;
	unsigned char seat;
	char name[WIFI_SYS_PEP_NAME_LEN];
	unsigned char name_len;

}unit_cinfo,*Punit_cinfo;

/*
 * 收发数据的帧信息
 * 主要是在收发数据时，保存数据的信息
 */
typedef struct {

	int oldfd;
	int fd;
	unsigned char msg_type;
	unsigned char data_type;
	unsigned char dev_type;

	int data_len;
	int frame_len;
	unsigned short s_id;
	unsigned short d_id;
	//查询是需要给此赋值,考虑现在有SSID和密码同时进行下发的情况，所以为数组
	unsigned char name_type[2];
	unsigned char code_type[2];

	unsigned short spk_port;	//发言端口
	unsigned short brd_port;	//音频广播端口

	event_data evt_data;

	unit_cinfo ucinfo;
	conf_content ccontent;
	sub_result sresult;

}frame_type,*Pframe_type;


/*
 * 会议信息链表
 * 此链表主要是保存单元的会议信息
 * 单元的fd
 * 单元的会议信息
 *
 */
typedef struct {

	int fd;
	unit_cinfo ucinfo;
}conference_list,*Pconference_list;


int wifi_sys_net_thread_init();
int wifi_sys_net_thread_deinit();

#ifdef __cplusplus
}
#endif

#endif /* INC_WIFI_SYS_INIT_H_ */
