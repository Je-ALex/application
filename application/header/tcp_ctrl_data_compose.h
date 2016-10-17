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

#endif /* HEADER_TCP_CTRL_DATA_COMPOSE_H_ */
