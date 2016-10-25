/*
 * tcp_ctrl_module_api.h
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_DATA_COMPOSE_H_
#define INC_TCP_CTRL_DATA_COMPOSE_H_

#include "../inc/tcp_ctrl_server.h"

int tcp_ctrl_frame_compose(Pframe_type type,const unsigned char* params,unsigned char* result_buf);

int tcp_ctrl_module_edit_info(Pframe_type type,const unsigned char* msg);

void tcp_ctrl_edit_event_content(Pframe_type type,unsigned char* buf);

#endif /* INC_TCP_CTRL_DATA_COMPOSE_H_ */
