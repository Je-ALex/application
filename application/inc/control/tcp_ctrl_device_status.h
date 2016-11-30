/*
 * tcp_ctrl_device_status.h
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DEVICE_STATUS_H_
#define INC_TCP_CTRL_DEVICE_STATUS_H_

#include "tcp_ctrl_queue.h"
#include "tcp_ctrl_api.h"
#include "tcp_ctrl_list.h"

typedef struct{

	pclient_node* sys_list;
	Plinkqueue* sys_queue;
	/*
	 * 会议实时状态
	 */
	Pconference_status con_status;

}global_info,*Pglobal_info;

/*
 * tcp控制模块收发数据信息
 * @socketfd 套接字号
 * @len 数据包长度
 * @msg 详细数据内容
 */
typedef struct{

	int socket_fd;
	int len;
	unsigned char* msg;

}ctrl_tcp_rsqueue,*Pctrl_tcp_rsqueue;



int tcp_ctrl_refresh_conference_list(Pconference_list data_info);


int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value);
int tcp_ctrl_report_dequeue(Prun_status event_tmp);

int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg);
int tcp_ctrl_tpsend_dequeue(Pctrl_tcp_rsqueue event_tmp);

/*
 * tcp_ctrl_tprecv_enqueue
 * tcp控制模块的数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tprecv_enqueue(int* fd,unsigned char* msg,int* len);

/*
 * tcp_ctrl_tprecv_dequeue
 * tcp控制模块的接收的数据处队列
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 *
 * in/out:
 * @Pctrl_tcp_rsqueue
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tprecv_dequeue(Pctrl_tcp_rsqueue msg_tmp);


/*
 * conf_status_check_client_legal
 * 检查数据包来源合法性，会议表中是否有此设备
 * 并将此设备的席位赋值给临时变量
 *
 * @Pframe_type
 *
 */
int conf_status_check_client_legal(Pframe_type frame_type);

/*
 * conf_status_check_chariman_legal
 * 检查数据包来源合法性，是否是主席单元发送的数据
 *
 * @Pframe_type
 *
 */
int conf_status_check_chariman_legal(Pframe_type frame_type);

/*
 * conf_status_find_did_sockfd
 * 检测临时变量中源地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_did_sockfd(Pframe_type frame_type);

/*
 * conf_status_find_chariman_sockfd
 * 检测链表中，主席单元的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_chariman_sockfd(Pframe_type frame_type);

/*
 * conf_status_set_cmspk
 * 会议中主席的发言状态
 *
 * @value
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_set_cmspk(int value);


/*
 * conf_status_get_cmspk
 * 会议中主席的发言状态
 *
 * 返回值：
 * @status
 */
int conf_status_get_cmspk();

/*
 * conf_status_set_current_subject
 * 会议中投票议题的状态
 *
 */
int conf_status_set_current_subject(unsigned char num);

/*
 * conf_status_get_subject_property
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_current_subject();

/*
 * conf_status_get_subject_property
 * 会议中投票议题的状态
 *
 */
int conf_status_get_subject_property(unsigned char* num);

/*
 * cof_info_vote_result_status
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_vote_result(int value);


/*
 * conf_status_get_vote_result
 * 会议中投票议题的状态
 *
 *
 */
int conf_status_get_vote_result(unsigned char* num,unsigned short* value);

/*
 * conf_status_save_vote_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_elec_result(unsigned short value);

/*
 * conf_status_save_vote_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_get_elec_result(unsigned short value);

/*
 * conf_status_get_elec_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_elec_totalp();

/*
 * conf_status_save_score_result
 * 会议中投票议题的状态
 *
 * @value投票结果
 *
 */
int conf_status_save_score_result(unsigned char value);

/*
 * conf_status_calc_score_result
 * 计算积分结果
 *
 * @value投票结果
 *
 */
int conf_status_calc_score_result();

/*
 * conf_status_get_score_result
 * 获取积分结果
 *
 * @value投票结果
 *
 */
int conf_status_get_score_result(Pscore_result result);

/*
 * conf_status_get_score_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_score_totalp();






























#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
