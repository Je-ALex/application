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


/*
 * socket相关端口信息
 */
#define CTRL_BROADCAST_PORT 50001

#define CTRL_TCP_PORT 		8080
#define	AUDIO_RECV_PORT 	9000
#define	AUDIO_SEND_PORT 	9090

/*
 * 设备系统参数
 */
#define VERSION		"V 1.0.5-R1"
#define	MODEL		"DS-WF620M"
#define PRODUCT		"四川湖山电器有限责任公司"

#define CONNECT_FILE "connection.info"
/*
 * 会议类相关宏定义
 */

#define PC_ID 			0xFFFF
#define HOST_ID 		0x0


#define ELE_NUM 		32

#define NAME_LEN		64

#define CONF_NAME_LEN		128

#define SUBJECT_NUM			50
#define SUBJECT_NAME_LEN	128

#define SYS_VAL 10

#define 	MAX_SPK_NUM 	8
#define 	DEF_SPK_NUM 	4
#define 	DEF_MIC_MODE 	2


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
 * system list/mutex/queue
 */
typedef enum{

	//设备连接链表
	CONNECT_LIST = 0,
	//设备连接心跳链表
	CONNECT_HEART,
	//MAC映射表
	CONNECT_MAC_TABLE,
	//会议发言管理链表
	CONFERENCE_SPK,
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

	pthread_mutex_t sys_mutex[SYS_VAL];
	sem_t sys_sem[SYS_VAL];

}sys_info;

/*
 * 网络信息参数
 */
typedef struct {

	 char ssid[32];
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
	unsigned short ele_id[ELE_NUM];
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
 * 议题内容
 * 议题名称
 * 议题的类型，投票表决等属性
 */
typedef struct{

	//议题名称和议题属性
//	unsigned char subj[SUBJECT_NUM][SUBJECT_NAME_LEN];

	unsigned char subj[SUBJECT_NAME_LEN];
	unsigned char subj_prop;

	vote_result v_result;
	election_result ele_result;
	score_result scr_result;

}subject_info,*Psubject_info;

/*
 * 提议公布结果内容
 */
typedef struct {

	//当前议题结果
	unsigned char conf_result[CONF_NAME_LEN];
	unsigned char len;

}conf_pc_result;

/*
 * 会议动态全局变量
 * 主要是会议的信息，不涉及参会人员信息
 * 会议信息是单次会议中不会更改的状态
 * 投票，表决，计分是会涉及到下发单元机，所以需要
 */
typedef struct {

	//上位机标记
	volatile int pc_status;
	//会议状态
	volatile unsigned char confer_status;
	//议题状态
	volatile unsigned char suject_status;
	//会议名称
	unsigned char conf_name[CONF_NAME_LEN];
	//议题表
	subject_info sub_list[SUBJECT_NUM];
	//议题总数
	volatile unsigned char total_sub;
	//当前议题
	volatile unsigned char sub_num;
	conf_pc_result cresult;

	//音频状态信息
	volatile unsigned char mic_mode;
	volatile unsigned char snd_effect;
	volatile unsigned char snd_brdcast;
	//设置的发言人数
	volatile unsigned char spk_number;

	//当前发言的人数
	volatile unsigned char current_spk;
	volatile unsigned char spk_offset[8];
	volatile unsigned int	spk_ts[8];
	//摄像跟踪状态开关
	volatile unsigned char camera_track;
	//主席发言状态
	volatile int chirman_t;

	//DEBUG
	unsigned char debug_sw;

	//系统时间
	unsigned char sys_time[4];
	//系统时间秒
	unsigned int sys_stime;

	net_info network;

}conference_status,*Pconference_status;

typedef struct{

	pclient_node* sys_list;
	Plinkqueue* sys_queue;
	/*
	 * 会议实时状态
	 */
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
 * 单元会议参数
 * 主要是描述每个单元机的属性状态
 */
typedef struct {

	/*
	 * 主机ID为0x0000，上位机ID为0xFFFF
	 */
	unsigned short id;
	unsigned char seat;
	unsigned char sub_num;
	//会议参数
	char name[NAME_LEN];
	char conf_name[CONF_NAME_LEN];
	char subj[SUBJECT_NUM][SUBJECT_NAME_LEN];

	/*
	 * 保存单个单元的议题结果
	 */
	vote_result v_result;
	election_result elec_rsult;
	score_result src_result;

}signal_unit_data;

/*
 * 收发数据的帧信息
 * 主要是在收发数据时，保存数据的信息
 */
typedef struct {

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
	signal_unit_data con_data;

}frame_type,*Pframe_type;






/*
 * MAC映射表
 */
typedef struct {

	unsigned int mac_number;
	unsigned short mac_table;

}connect_mac_table,*Pconnect_mac_table;

/*
 * 心跳状态链表信息
 * 保存套接字号
 * 心跳状态
 */
typedef struct {

	int sockfd;
	unsigned int mac_addr;
	volatile char status;

}connect_heart,*Pconnect_heart;

/*
 * 发言设备的端口管理
 * 主要是套接字号
 * 席别号 音频端口号 时间戳
 */
typedef struct {

	int sockfd;
	int seat;
	int asport;

	unsigned int ts;

}as_port,*Pas_port;

/*
 * 会议信息链表
 * 保存单元的id等信息
 */
typedef struct {

	int fd;
	signal_unit_data con_data;

}conference_list,*Pconference_list;



#endif /* INC_WIFI_SYS_INIT_H_ */
