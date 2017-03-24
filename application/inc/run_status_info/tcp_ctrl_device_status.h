/*
 * tcp_ctrl_device_status.h
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DEVICE_STATUS_H_
#define INC_TCP_CTRL_DEVICE_STATUS_H_


#include "wifi_sys_init.h"
#include "tcp_ctrl_api.h"

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



int sys_net_status_set(int value);
int sys_net_status_get();


int sys_debug_set_switch(unsigned char value);


int sys_debug_get_switch();


int tcp_ctrl_report_enqueue(Pframe_type frame_type,int value);
int tcp_ctrl_report_dequeue(Prun_status event_tmp);

int tcp_ctrl_tpsend_enqueue(Pframe_type frame_type,unsigned char* msg);
int tcp_ctrl_tpsend_dequeue(Pctrl_tcp_rsqueue event_tmp);

int tcp_ctrl_tprecv_enqueue(int* fd,unsigned char* msg,int* len);

int tcp_ctrl_tprecv_dequeue(Pctrl_tcp_rsqueue msg_tmp);



int conf_status_get_connected_len();
int conf_status_get_client_connect_len();

int conf_status_get_conference_len();

int conf_status_check_client_conf_legal(Pframe_type frame_type);

int conf_status_check_chairman_legal(Pframe_type frame_type);
int conf_status_check_pc_legal(Pframe_type frame_type);

int conf_status_check_client_connect_legal(Pframe_type frame_type);

int conf_status_check_chariman_staus();

int conf_status_find_did_sockfd_id(Pframe_type frame_type);

int conf_status_find_did_sockfd_sock(Pframe_type frame_type);

int conf_status_find_chairman_sockfd(Pframe_type frame_type);

int conf_status_find_max_id();

int conf_status_compare_id(int value);

int conf_status_set_cmspk(int value);

int conf_status_get_cmspk();

int conf_status_set_current_subject(unsigned char num);
int conf_status_get_current_subject();

int conf_status_set_total_subject(unsigned char num);
int conf_status_get_total_subject();

int conf_status_set_subject_property(unsigned char num,unsigned char prop);

int conf_status_get_subject_property(unsigned char num);

int conf_status_save_pc_conf_result(int len,const unsigned char* msg);

int conf_status_save_vote_result(int value);

int conf_status_get_vote_result(unsigned char num,unsigned short* value);

int conf_status_send_hvote_result();

int conf_status_reset_vote_status();

int conf_status_calc_vote_result();

int conf_status_save_elec_result(unsigned short value);

int conf_status_get_elec_result(unsigned char num,unsigned short value);

int conf_status_set_elec_totalp(unsigned char num,unsigned char pep);

int conf_status_get_elec_totalp(unsigned char num);

int conf_status_save_score_result(unsigned char value);

int conf_status_reset_score_status();

int conf_status_calc_score_result();

int conf_status_get_score_result(Pscore_result result);

int conf_status_get_score_totalp();

int conf_status_send_hscore_result();

/*
 * TODO
 * 1/发言管理
 * 2/话筒管理
 */

int conf_status_set_mic_mode(int value);


int conf_status_get_mic_mode();


int conf_status_set_spk_num(int value);

int conf_status_get_spk_num();

int conf_status_set_spk_offset(int value);
int conf_status_get_spk_offset();

int conf_status_set_spk_buf_offset(int num,int value);
int conf_status_get_spk_buf_offset(int num);

int conf_status_set_spk_timestamp(int num,int value);
int conf_status_get_spk_timestamp(int num);

int conf_status_set_cspk_num(int value);
int conf_status_get_cspk_num();

int conf_status_set_snd_brd(int value);
int conf_status_get_snd_brd();


int conf_status_set_snd_effect(int value);

int conf_status_get_snd_effect();


int conf_status_camera_track_postion(int id,int value);
int conf_status_set_camera_track(int value);
int conf_status_get_camera_track();


int conf_status_set_conf_staus(int value);
int conf_status_get_conf_staus();

int conf_status_set_sys_time(Pframe_type frame_type,const unsigned char* msg);
int conf_status_get_sys_time(unsigned char* msg);

int conf_status_set_sys_timestamp(unsigned int value);
int conf_status_get_sys_timestamp();

int conf_status_set_subjet_staus(int value);
int conf_status_get_subjet_staus();

int conf_status_set_pc_staus(int value);
int conf_status_get_pc_staus();


int conf_status_send_vote_result();


int conf_status_send_elec_result();

int conf_status_send_score_result();





#endif /* INC_TCP_CTRL_DEVICE_STATUS_H_ */
