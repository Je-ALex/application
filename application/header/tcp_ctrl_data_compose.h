/*
 * tcp_ctrl_module_api.h
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_DATA_COMPOSE_H_
#define HEADER_TCP_CTRL_DATA_COMPOSE_H_

#include "tcp_ctrl_server.h"

int tcp_ctrl_frame_compose(Pframe_type type,char* params,unsigned char* result_buf);

int tcp_ctrl_module_edit_info(Pframe_type type);

void tcp_ctrl_edit_event_content(Pframe_type type,unsigned char* buf);

#endif /* HEADER_TCP_CTRL_DATA_COMPOSE_H_ */
