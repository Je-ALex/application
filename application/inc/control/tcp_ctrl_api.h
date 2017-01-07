/*
 * tcp_ctrl_api.h
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_API_H_
#define INC_TCP_CTRL_API_H_

#include <arpa/inet.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 主机TCP控制模块保存的终端信息参数
 * TODO 提供参考的结构体
 * 不区分上位机还是单元机，统一使用这个结构体，保存
 *
 */
typedef enum{

	//宣告上线
	WIFI_MEETING_EVENT_ONLINE_REQ = 1,
	//宣告离线
	WIFI_MEETING_EVENT_OFFLINE_REQ,
	//设备心跳
	WIFI_MEETING_EVENT_DEVICE_HEART,
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
	WIFI_MEETING_EVENT_SPK_CLOSE_MIC,
	WIFI_MEETING_EVENT_SPK_CLOSE_REQ,
	WIFI_MEETING_EVENT_SPK_CLOSE_SND,
	WIFI_MEETING_EVENT_SPK_CHAIRMAN_ONLY,
	//投票管理
	WIFI_MEETING_EVENT_VOTE_START,
	WIFI_MEETING_EVENT_VOTE_END,
	WIFI_MEETING_EVENT_VOTE_RESULT,
	WIFI_MEETING_EVENT_VOTE_ASSENT,
	WIFI_MEETING_EVENT_VOTE_NAY,
	WIFI_MEETING_EVENT_VOTE_WAIVER,
	WIFI_MEETING_EVENT_VOTE_TIMEOUT,

	//音效管理
	WIFI_MEETING_EVENT_SOUND,
	//会议服务
	WIFI_MEETING_EVENT_SERVICE_WATER,
	WIFI_MEETING_EVENT_SERVICE_PEN,
	WIFI_MEETING_EVENT_SERVICE_PAPER,
	WIFI_MEETING_EVENT_SERVICE_OTHERS,

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

	//选举管理，开始选举，结束选举，正在进行
	WIFI_MEETING_CONF_ELECTION_START,
	WIFI_MEETING_CONF_ELECTION_END,
	WIFI_MEETING_CONF_ELECTION_RESULT,
	WIFI_MEETING_CONF_ELECTION_UNDERWAY,

	//计分管理，开始计分，结束计分，正在进行
	WIFI_MEETING_CONF_SCORE_START,
	WIFI_MEETING_CONF_SCORE_END,
	WIFI_MEETING_CONF_SCORE_RESULT,
	WIFI_MEETING_CONF_SCORE_UNDERWAY,

	//会议管理，开始会议，结束会议
	WIFI_MEETING_EVENT_CON_MAG_START,
	WIFI_MEETING_EVENT_CON_MAG_END,

	//单元电量状态
	WIFI_MEETING_EVENT_UNIT_ELECTRICITY,

	//上位机下发的指令
	//事件类数据
	WIFI_MEETING_EVENT_PC_CMD_SIGNAL,
	WIFI_MEETING_EVENT_PC_CMD_ALL,
	//会议类数据
	WIFI_MEETING_CONF_PC_CMD_SIGNAL,
	WIFI_MEETING_CONF_PC_CMD_ALL,
	//上位机查询上报
	WIFI_MEETING_HOST_REP_TO_PC,

	//会议参数设置成功、失败，查询应答
	WIFI_MEETING_CONF_WREP_SUCCESS,
	WIFI_MEETING_CONF_WREP_ERR,
	WIFI_MEETING_CONF_RREP,

	//网络异常
	WIFI_MEETING_EVENT_NET_ERROR,

	//todo 议题管理,议题号偏移量，需将value减去偏移量,此定义必须放在最后
	WIFI_MEETING_EVENT_SUBJECT_OFFSET,

}ALL_STATUS;



typedef struct{

	//终端sock_fd属性
	int client_fd;
	//qt获取设备参数信息
	char ip[32];
	char mac[32];
	//终端ip属性信息
	struct sockaddr_in cli_addr;
	int clilen;

	//终端id属性,默认为0，若是pc则为1，主机机则为2，单元机则为3
	unsigned char client_name;
	//单元机电量信息
	unsigned char client_power;
	//qt获取会议参数信息参数
	int id;
	unsigned char seat;


}client_info,*Pclient_info;


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
	char factory_info[64];

	//主机MAC地址
	char mac_addr[32];
	//主机IP
	char ip_addr[32];
	//子网掩码
	char netmask[32];
	//广播地址
	char broad_addr[32];

}host_info,*Phost_info;

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



/********************************
 *
 * TODO 主机系统属性模块
 *
 * 系统属性函数
 * 系统设置:系统初始化，恢复出厂设置、网络状态、本机信息、关机(关闭本机、重启本机、关闭所有单元机)
 * 系统扩展
 *
 *******************************/
/* wifi_conference_sys_init
 * 主机设备控制模块的初始化函数
 *
 * 此函数需要在网络连接正常后进行调用
 *
 * 返回值
 * @SUCCESS
 * @ERROR
 *
 */
int wifi_conference_sys_init();

/*
 * host_info_reset_factory_mode
 * 恢复出厂设置
 * 目前是清除连接信息和清除会议信息相关内容
 *
 * 返回值：
 * @SUCCESS(0)
 * @ERROR
 */
int host_info_reset_factory_mode();

/*
 * host_info_get_network_info
 * 获取主机的网路状态
 *
 * int/out:
 * @Phost_info
 *
 * 返回值:
 * @SUCCESS(0)
 * @ERROR
 */
int host_info_get_network_info(Phost_info ninfo);

/*
 * host_info_get_factory_info
 * 获取主机信息
 * 主机信息是保存在文本文件中，为只读属性
 *
 * out:
 * @Phost_info
 *
 * 返回值:
 * @SUCCESS(0)
 * @ERROR
 */
int host_info_get_factory_info(Phost_info pinfo);



/****************************
 * TODO 单元机状态管理模块
 ***************************/
/*
 * unit_info_get_connected_info
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
 * 返回值:
 * @size 写入的连接客户端结构体个数
 */
int unit_info_get_connected_info(char* name);

/*
 * unit_info_get_running_status
 * 单元机实时状态检测函数,用户只需检测设备号(ID)和具体返回信息
 * 该函数平时为阻塞状态，等有状态改变时才有状态返回，所以应用需要一直获取
 *
 * in/out:
 *
 * @Prun_status详见头文件定义的结构体，返回参数
 *
 * 参考此结构体，应用获取运行状态
	typedef struct{

		int socket_fd;
		int id;
		unsigned char seat;
		unsigned short value;

	}run_status,*Prun_status;

 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int unit_info_get_running_status(Prun_status data);

/*
 * 设备电源设置
 * 关闭所有单元机
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int unit_info_set_device_power_off();

/*
 * unit_info_get_device_power
 * 获取单元机电源状态
 *
 * in:
 * @socket_fd 单元机fd，文本文件获取
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int unit_info_get_device_power(int fd);

/*********************
 * TODO 会议状态管理模块
 * 分为主机部分和单元机部分
 *********************/
/*
 * conf_info_set_mic_mode
 * 话筒管理中的模式设置
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 * 在设置成功后，会收到DSP传递的返回值，通过返回值判断，
 * 再将结果上报
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_mic_mode(int mode);


/*
 * conf_info_get_mic_mode
 * 获取话筒管理中的模式
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 * 返回值：
 * @value
 */
int conf_info_get_mic_mode();

/*
 * conf_info_set_spk_num
 * 话筒管理中的发言管理
 * 设置会议中最大发言人数
 *
 * @num 1/2/4/6/8
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_spk_num(int num);


/*
 * conf_info_get_spk_num
 * 话筒管理中的发言管理
 * 设置会议中最大发言人数
 *
 * 返回值：
 * @num 1/2/4/6/8
 */
int conf_info_get_spk_num();

/*
 * conf_info_set_snd_effect
 * DSP音效设置
 * 采用为管理分别从bit[0-3]表示状态
 * bit  0    1     2     3
 *      AFC  ANC0  ANC1  ANC2
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_snd_effect(int mode);

/*
 * conf_info_get_snd_effect
 * DSP音效获取
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_get_snd_effect();


/*
 * conf_info_set_conference_params
 * 设置会议参数,单主机的情况，主机只需进行ID和席位的编码，
 * 其他设置为NULL即可，现在保留后续参数
 *
 * in:
 * @fd 单元机的fd号，应用可以从文本文件获取
 * @id 设置的ID
 * @seat 设置的席别
 * @pname（保留）
 * @cname（保留）
 *
 * out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_conference_params(int fd,unsigned short id,unsigned char seat,
		char* pname,char* cname);

/*
 * conf_info_get_the_conference_params
 * 获取单元机会议类参数
 * 获取某一个单元机的会议参数信息，
 * 如果为0，则默认为获取全部的参会单元会议信息
 * in:
 * @socket_fd 单元机fd，从文本文件获取
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_get_the_conference_params(int fd);


/*
 * conf_info_get_checkin_total
 * 获取会议中签到应到人数
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @会议中应到人数
 *
 */
int conf_info_get_checkin_total();


/*
 * conf_info_send_vote_result
 * 下发投票结果，单主机的情况下，此接口基本用不上，
 * 投票结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_send_vote_result();

/*
 * conf_info_send_elec_result
 * 下发选举结果，单主机的情况下，此接口基本用不上，
 * 选举结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机,当结束一个议题以后，
 * 自动下发结果，单元机选择性显示
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_send_elec_result();

/*
 * conf_info_send_score_result
 * 下发计分结果，单主机的情况下，此接口基本用不上，
 * 计分结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机,当结束一个议题以后，
 * 自动下发结果，单元机选择性显示
 *
 * in/out:
 * @NULL
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_send_score_result();

























































/***************************************
 *
 * FIXME 保留
 *
 **************************************/

/*
 * conf_info_set_cspk_num
 * 会议中当前发言的人数
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int conf_info_set_cspk_num(int num);

/*
 * conf_info_get_cspk_num
 * 话筒管理中的发言管理
 * 设置会议中最大发言人数
 *
 * 返回值：
 * @current_spk
 */
int conf_info_get_cspk_num();

/*
 * 保留
 * set_the_event_parameter_ssid_pwd
 * 设置单元机ssid和密码
 *
 * in:
 * @socket_fd 单元机fd，文本文件获取
 * @ssid 网络名称
 * @pwd 连接密钥
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int set_the_event_parameter_ssid_pwd(int socket_fd,char* ssid,char* pwd);


/*
 * 设置签到模式
 * 此功能主要主席单元或上位机
 *
 * in:
 * @value 开始签到(0x01),结束签到(0x02)
 * out:
 * @NULL
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 */
int wifi_meeting_set_checkin_mode(int value);

/*
 * conf_info_set_conference_sub_params
 * 设置会议议题内容
 *
 * 返回值：
 * @ERROR
 * @SUCCESS(0)
 *
 */
int conf_info_set_conference_sub_params();



#ifdef __cplusplus
}
#endif

#endif /* INC_TCP_CTRL_API_H_ */






