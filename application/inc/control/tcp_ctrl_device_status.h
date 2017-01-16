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
#include "sys_uart_init.h"


#define DBG_ON 1
#define DBG_OFF 0


/*
 * 主机音效设置
 * 采用为管理分别从bit[0-2]表示状态
 * bit
 *  0   AFC
 *  1   ANC0
 *  2  	ANC1
 *
 * bit[0-2] 共有8个状态
 */
typedef enum{

	HOST_SND_EF_OFF = 0,
	HOST_SND_EF_AFC,
	HOST_SND_EF_ANC0,
	HOST_SND_EF_AFC_ANC0,
	HOST_SND_EF_ANC1,
	HOST_SND_EF_AFC_ANC1,
	HOST_SND_EF_ANC1_ANC2,
	HOST_SND_EF_AFC_ANC1_ANC2,
}snd_effect;

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

/*
 * 系统调试开关设置
 */
int sys_debug_set_switch(unsigned char value);

/*
 * 系统调试开关状态获取
 */
int sys_debug_get_switch();


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



int conf_status_get_connected_len();

int conf_status_get_conference_len();

/*
 * conf_status_check_client_conf_legal
 * 检查数据包来源合法性，会议表中是否有此设备
 * 并将此设备的席位赋值给临时变量
 *
 * @Pframe_type
 *
 */
int conf_status_check_client_conf_legal(Pframe_type frame_type);

/*
 * conf_status_check_chariman_legal
 * 检查数据包来源合法性，是否是主席单元发送的数据
 *
 * @Pframe_type
 *
 */
int conf_status_check_chairman_legal(Pframe_type frame_type);

/*
 * conf_status_check_client_connect_legal
 * 检查连接信息中，设备的合法性
 *
 * @Pframe_type
 *
 */
int conf_status_check_client_connect_legal(Pframe_type frame_type);

/*
 * conf_status_check_chariman_staus
 * 检查会议中是否有主席单元存在
 *
 * 返回值
 * @主席状态0无1有
 *
 */
int conf_status_check_chariman_staus();

/*
 * conf_status_find_did_sockfd
 * 检测临时变量中源地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_did_sockfd_id(Pframe_type frame_type);

int conf_status_find_did_sockfd_sock(Pframe_type frame_type);
/*
 * conf_status_find_chariman_sockfd
 * 检测链表中，主席单元的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_chairman_sockfd(Pframe_type frame_type);

/*
 * conf_status_find_max_id
 * 检测链表中，ID最大值
 *
 * @
 *
 */
int conf_status_find_max_id();

/*
 * conf_status_compare_id
 * 比较链表中id号
 *
 */
int conf_status_compare_id(int value);

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

int conf_status_set_subject_property(unsigned char num,unsigned char prop);

/*
 * conf_status_get_subject_property
 * 会议中投票议题的状态
 *
 */
int conf_status_get_subject_property(unsigned char num);

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
int conf_status_get_vote_result(unsigned char num,unsigned short* value);

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
int conf_status_get_elec_result(unsigned char num,unsigned short value);


int conf_status_set_elec_totalp(unsigned char num,unsigned char pep);
/*
 * conf_status_get_elec_totalp
 * 议题中被选举总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_elec_totalp(unsigned char num);

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


/*
 * TODO
 * 1/发言管理
 * 2/话筒管理
 */

/*
 * conf_status_set_mic_mode
 * 会议话筒模式管理
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */
int conf_status_set_mic_mode(int value);

/*
 * conf_status_get_mic_mode
 * 会议最大发言人数
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */
int conf_status_get_mic_mode();

/*
 * conf_status_set_spk_num
 * 会议最大发言人数设置
 *
 * @value投票结果
 *
 */
int conf_status_set_spk_num(int value);

/*
 * conf_status_get_spk_num
 * 会议最大发言人数获取
 *
 * @value投票结果
 *
 */
int conf_status_get_spk_num();


/*
 * conf_status_set_cspk_num
 * 会议当前发言人数设置
 *
 * @value投票结果
 *
 */
int conf_status_set_cspk_num(int value);

/*
 * conf_status_get_cspk_num
 * 会议当前发言人数获取
 *
 *
 */
int conf_status_get_cspk_num();

int conf_status_set_snd_brd(int value);
int conf_status_get_snd_brd();

/*
 * conf_status_set_snd_effect
 * DSP音效设置
 * 采用为管理分别从bit[0-3]表示状态
 * bit  0    1     2     3
 *      AFC  ANC0  ANC1  ANC2
 *
 *  * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_set_snd_effect(int value);

/*
 * conf_status_get_snd_effect
 * DSP音效获取
 *
 * @value AFC(0/1)，ANC(0/1/2/3)
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_get_snd_effect();

int conf_status_set_camera_track(int value);

int conf_status_get_camera_track();

/*
 * conf_status_set_conf_staus
 * 设置会议进程状态
 *
 * @value
 *
 */
int conf_status_set_conf_staus(int value);

/*
 * conf_status_get_conf_staus
 * 获取会议进程状态
 *
 * @value
 *
 */
int conf_status_get_conf_staus();

/*
 * conf_status_set_pc_staus
 * 设置会议中上位机的连接状态
 *
 * @value
 *
 */
int conf_status_set_pc_staus(int value);


/*
 * conf_status_get_pc_staus
 * 获取会议进程中上位机的连接状态
 *
 * @value
 *
 */
int conf_status_get_pc_staus();


int conf_status_send_vote_result();


int conf_status_send_elec_result();

int conf_status_send_score_result();







































































#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
