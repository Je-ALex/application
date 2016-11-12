/*
 * tcp_ctrl_api.h
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_API_H_
#define INC_TCP_CTRL_API_H_



#ifdef __cplusplus
extern "C"
{
#endif


typedef enum{

	//宣告上线
	WIFI_MEETING_EVENT_ONLINE_REQ = 1,
	//宣告离线
	WIFI_MEETING_EVENT_OFFLINE_REQ,
	//电源开、关
	WIFI_MEETING_EVENT_POWER_ON,
	WIFI_MEETING_EVENT_POWER_OFF,
	WIFI_MEETING_EVENT_POWER_OFF_ALL,
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

	//选举管理，开始选举，结束选举
	WIFI_MEETING_CONF_REQ_ELECTION_START,
	WIFI_MEETING_CONF_REQ_ELECTION_END,

	//投票管理，开始投票，结束投票
	WIFI_MEETING_CONF_REQ_SCORE_START,
	WIFI_MEETING_CONF_REQ_SCORE_END,

}ALL_STATUS;


/*
 * 主机信息结构体
 * 主要是描述主机的固有属性
 */
typedef struct{

	//主机型号
	char host_model[32];
	//软件版本,硬件版本后续增加
	char version[32];
	//出厂信息
	char factory_information[64];

	//主机MAC地址
	char mac[32];
	//主机IP
	char local_ip[32];
	//子网掩码
	char netmask[32];
}host_info,*Phost_info;

/*
 * 主机TCP控制模块保存的终端信息参数
 * TODO 提供参考的结构体
 * 不区分上位机还是单元机，统一使用这个结构体，保存
 *
 */
typedef struct{

	/*终端sock_fd属性*/
	int 	client_fd;
	/*终端id属性,默认为0，若是pc则为1，主机机则为2，单元机则为3*/
	unsigned char 	client_name;

	/*终端ip属性信息*/
	struct sockaddr_in cli_addr;
	int 	clilen;

	//网关，掩码等等

	//qt获取设备参数信息
	char ip[32];
	char mac[32];

	//qt获取会议参数信息参数
	int id;
	unsigned char seat;


}client_info,*Pclient_info;

/*
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

}run_status,*Prun_status;

/*********************************
 *
 * 系统初始化函数
 *
 ********************************/

/* wifi_conference_sys_init
 * 主机设备控制模块的初始化函数
 *
 * 此函数需要在网络连接正常后进行调用
 *
 */
int wifi_conference_sys_init();



/********************************
 *
 * TODO 基本信息接口
 * 系统设置:恢复出厂设置、网络状态、本机信息、关机(关闭本机、重启本机、关闭所有单元机)
 * 系统扩展
 *
 *******************************/

/*
 * reset_factory_mode
 * 恢复出厂设置
 * 目前是清除连接信息和清除会议信息相关内容
 *
 * return：
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int reset_the_host_factory_mode();

/*
 * get_the_host_network_info
 * 获取主机的网路状态
 *
 * int/out:
 * @Phost_info
 *
 * return:
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int get_the_host_network_info(Phost_info ninfo);

/*
 * get_the_host_factory_infomation
 * 获取主机信息
 * 主机信息是保存在文本文件中，为只读属性
 *
 * out:
 * @Phost_info
 *
 * return:
 * @ERROR(-1)
 * @SUCCESS(0)
 */
int get_the_host_factory_infomation(Phost_info pinfo);

/*
 * get_client_connected_info
 * 扫描接口函数
 *
 * 调用此接口函数后，主机自动进行扫描功能，并将生成信息文件@connection.info
 * 文本文件的存储是直接存取的结构体 @Pclient_info
 *
 * 应用读取文本文件，按照@Pclient_info进行解析，获取终端信息
 *
 * 扫描主要是开机上电有终端连接后，将终端信息的ip返回给应用
 *
 * in/out:
 * @name 文本文件的名字
 *
 * return:
 * 写入的连接客户端个数
 */
int get_client_connected_info(char* name);

/*
 * 设备电源设置
 * 关闭本机(1)、重启本机(2)、关闭所有单元机(3)
 *
 * in:
 * @modle(1,2,3)
 * out：
 * NULL
 *
 * return:
 * @ERROR
 * @SUCCESS
 */
int set_device_power_off(int mode);


/*
 * get_uint_running_status
 * 单元机实时状态检测函数,用户只需检测设备号(ID)和具体返回信息
 * 该函数平时为阻塞状态，等有状态改变时才有状态返回，所以应用需要一直获取
 *
 * in/out:
 *
 * @Pqueue_event详见头文件定义的结构体，返回参数
 *
 * 参考此结构体，应用获取运行状态
 * typedef struct{
	int socket_fd;
	int id;
	unsigned char seat;
    //具体的改变状态
	unsigned short value;

	}queue_event,*Pqueue_event;

 * return：
 * @ERROR
 * @SUCCESS
 */
int get_unit_running_status(void* event_tmp);




/********************************
 * TODO 会议类参数设置
 * 会议类接口函数
 * 1、单主机情况下，会议参数设置，只有ID号设置和席别设置
 * 2、会议类查询接口，查询某个单元机会议信息全状态
 *
 *******************************/


/*
 * set_the_conference_parameters
 * 设置会议参数,单主机的情况，主机只需进行ID和席位的编码，其他设置为NULL即可，现在保留后续参数
 *

 * in:
 * @socket_fd 单元机的fd号，应用可以从文本文件获取
 * @client_id 设置的ID
 * @client_seat 设置的席别
 * @client_name（保留）
 * @subj（保留）
 *
 * out:
 * @NULL
 *
 * return:
 * @ERROR
 * @SUCCESS
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj);

/*
 * get_the_conference_parameters
 * 获取单元机会议类参数
 * 获取某一个单元机的会议参数信息，如果为空（NULL），则默认为获取全部的参会单元会议信息
 * in:
 * @socket_fd 单元机fd，从文本文件获取
 *
 * return:
 * @ERROR
 * @SUCCESS
 */
int get_the_conference_parameters(int fd_value);

/*
 * 保留
 * set_the_conference_vote_result
 * 下发投票结果，单主机的情况下，此接口基本用不上，投票结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机
 *
 * in/out:
 * @NULL
 *
 * return:
 * @ERROR
 * @SUCCESS
 */
int set_the_conference_vote_result();


/********************************
 *
 * TODO 事件接口函数
 * 1、设置类，主要是对单元机的运行状态进行设置，如电源、发言等等
 * 2、查询类，主要是对单元机的运行状态以及主机状态进行查询，如电源情况、话筒管理等等
 *
 *******************************/

/*
 * set_the_event_parameter_power
 * 设置单元机电源开关
 * 设置某个单元机的电源开关
 *
 * in:
 * @socket_fd单元机fd，文本文件获取
 * @value 0x01为开(保留)，0x02关(只能设置为此数值)
 *
 * out:
 * @NULL
 *
 * return:
 * @ERROR
 * @SUCCESS
 */
int set_the_event_parameter_power(int socket_fd,unsigned char value);

/*
 * get_the_event_parameter_power
 * 获取单元机电源状态
 *
 * in:
 * @socket_fd 单元机fd，文本文件获取
 *
 * return:
 * @ERROR
 * @SUCCESS
 */
int get_the_event_parameter_power(int socket_fd);

/*
 * 保留
 * set_the_event_parameter_ssid_pwd
 * 设置单元机ssid和密码
 *
 * in:
 * @socket_fd 单元机fd，文本文件获取
 * @ssid 网络名称
 * @pwd 连接密钥
 * return:
 * @ERROR
 * @SUCCESS
 */
int set_the_event_parameter_ssid_pwd(int socket_fd,char* ssid,char* pwd);




/***************************************
 *
 * TODO 主机工作模式设置
 *
 **************************************/

/*
 * wifi_meeting_set_mic_mode
 * 话筒管理中的模式设置
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int wifi_meeting_set_mic_mode(int mode);



/*
 * wifi_meeting_set_spk_num
 * 话筒管理中的发言管理
 * 设置会议中最大发言人数
 *
 * @num 1/2/4/6/8
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int wifi_meeting_set_spk_num(int num);

/*
 * 设置签到模式
 * 此功能主要主席单元或上位机
 *
 * in:
 * @value 开始签到(0x01),结束签到(0x02)
 * out:
 * @NULL

 * return:
 * @ERROR
 * @SUCCESS
 */
int wifi_meeting_set_checkin_mode(int value);




/********************************
 *
 * TODO 系统扩展
 * 保留
 *
 ********************************/



#ifdef __cplusplus
}
#endif

#endif /* INC_TCP_CTRL_API_H_ */
