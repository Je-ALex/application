/*
 * tcp_ctrl_data_process.h
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_DATA_PROCESS_H_
#define HEADER_TCP_CTRL_DATA_PROCESS_H_

#include "ctrl-tcp.h"



char* tc_frame_analysis(int* fd,const char* buf,int* len,Pframe_type frame_type);
int tc_frame_compose(Pframe_type type,char* params,unsigned char* result_buf);
int tc_unit_reply(const char* msg,Pframe_type frame_type);




























#endif /* HEADER_TCP_CTRL_DATA_PROCESS_H_ */
