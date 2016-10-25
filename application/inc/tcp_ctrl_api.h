/*
 * tcp_ctrl_api.h
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_API_H_
#define INC_TCP_CTRL_API_H_

/*
 * get_unit_connected_info
 * 单元机扫描接口函数
 * 调用此接口函数后，主机自动进行扫描功能，并将生成信息文件@connect_info.conf
 *
 * in/out:
 * NULL
 *
 * return：
 * @error
 * @success
 */
int get_unit_connected_info();

/*
 * get_uint_running_status
 * 单元机实时状态检测函数,用户只需检测设备号和具体返回信息
 *
 * in/out:
 *
 * @Pqueue_event详见头文件定义的结构体，返回参数
 *
 * typedef struct{
	int socket_fd;
	int id;
	unsigned char seat;
	unsigned short value;

	}queue_event,*Pqueue_event;

 * return：
 * @error
 * @success
 */
int get_unit_running_status(void** event_tmp);




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
 * @socket_fd
 * @client_id
 * @client_seat
 * @client_name（保留）
 * @subj（保留）
 *
 * out:
 * @NULL
 *
 * return:
 * @error
 * @success
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj);

/*
 * get_the_conference_parameters
 * 获取单元机会议类参数
 *

 * in:
 * @socket_fd 获取某一个单元机的会议参数信息，如果为空（NULL），则默认为获取全部的参会单元会议信息
 *
 * return:
 * @error
 * @success
 */
int get_the_conference_parameters(int fd_value);

/*
 * set_the_conference_vote_result
 * 下发投票结果，单主机的情况下，此接口基本用不上，投票结果由上位机统计，通过主机下发给单元机
 * 默认下发给所有参会单元机
 *
 * in/out:
 * @NULL
 *
 * return:
 * @error
 * @success
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
 * @fd_value设备号
 * @value值
 *
 * out:
 * @NULL
 *
 * return:
 * @error
 * @success
 */
int set_the_event_parameter_power(int fd_value,unsigned char value);

/*
 * get_the_event_parameter_power
 * 获取单元机电源状态
 *
 * in:
 * @fd_value设备号
 *
 * return:
 * @error
 * @success
 */
int get_the_event_parameter_power(int fd_value);


int set_the_event_parameter_ssid_pwd(int fd_value,char* ssid,char* pwd);




#endif /* INC_TCP_CTRL_API_H_ */
