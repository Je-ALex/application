/*
 * tcp_ctrl_data_process.h
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DATA_PROCESS_H_
#define INC_TCP_CTRL_DATA_PROCESS_H_

#include "tcp_ctrl_list.h"
#include "tcp_ctrl_server.h"




int tcp_ctrl_data_char_to_int(int* value,char* buf);

int tcp_ctrl_frame_analysis(int* fd,unsigned char* buf,int* len,Pframe_type frame_type,
		unsigned char** ret_msg);

int tcp_ctrl_from_unit(const unsigned char* handlbuf,Pframe_type frame_type);

int tcp_ctrl_from_pc(const unsigned char* handlbuf,Pframe_type frame_type);


int tcp_ctrl_source_dest_setting(int s_fd,int d_fd,Pframe_type type);

int tcp_ctrl_delete_client(int fd);


/*
 * tcp_ctrl_msg_send_to
 * 处理后的消息进行何种处理，是发送给上位机还是在主机进行显示
 * 通过判断是否有上位机进行分别处理，有上位机的情况，主机就不进行显示提示
 *
 */
int tcp_ctrl_msg_send_to(Pframe_type type,const unsigned char* msg,int value);

/*
 * tcp_ctrl_refresh_connect_list
 * 控制模块客户端连接管理
 *
 * 通过msg_type来判断是否是宣告在线消息
 * 再通过fd判断连接存储情况，如果是新的客户端则存入
 */
int tcp_ctrl_refresh_connect_list(const unsigned char* msg,Pframe_type type);














#endif /* INC_TCP_CTRL_DATA_PROCESS_H_ */
