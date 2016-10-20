/*
 * tcp_ctrl_data_process.h
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_DATA_PROCESS_H_
#define HEADER_TCP_CTRL_DATA_PROCESS_H_

#include "tcp_ctrl_server.h"
#include "tcp_ctrl_list.h"




int tcp_ctrl_data_char_to_int(int* value,char* buf);

int tcp_ctrl_frame_analysis(int* fd,unsigned char* buf,int* len,Pframe_type frame_type,
		unsigned char** ret_msg);

int tcp_ctrl_from_unit(const unsigned char* handlbuf,Pframe_type frame_type);

int tcp_ctrl_from_pc(const unsigned char* handlbuf,Pframe_type frame_type);





















#endif /* HEADER_TCP_CTRL_DATA_PROCESS_H_ */
