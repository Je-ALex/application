/*
 * tcp_ctrl_data_process.h
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DATA_PROCESS_H_
#define INC_TCP_CTRL_DATA_PROCESS_H_

#include "wifi_sys_init.h"

#define MSG_TYPE 		0xF0 //消息类型
#define DATA_TYPE 		0x0C //数据类型
#define MACHINE_TYPE 	0x03 //设备类型

#define PKG_LEN		0x10//数据包最小固定长度

/*
 * 通用函数接口
 */
int tcp_ctrl_data_char2short(unsigned short* value,unsigned char* buf);

int tcp_ctrl_msg_send_to(Pframe_type type,const unsigned char* msg,int value);

int tcp_ctrl_source_dest_setting(int s_fd,int d_fd,Pframe_type type);



/*
 * 单元相关处理
 */

int tcp_ctrl_from_unit(const unsigned char* handlbuf,Pframe_type frame_type);

/*
 * 上位机相关处理
 */

int tcp_ctrl_from_pc(const unsigned char* handlbuf,Pframe_type frame_type);

/*
 * 统一数据处理
 */
void* wifi_sys_ctrl_tcp_procs_data(void* p);








#endif /* INC_TCP_CTRL_DATA_PROCESS_H_ */
