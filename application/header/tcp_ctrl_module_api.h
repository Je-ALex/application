/*
 * tcp_ctrl_module_api.h
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_MODULE_API_H_
#define HEADER_TCP_CTRL_MODULE_API_H_

#include "tcp_ctrl_server.h"


int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj);

#endif /* HEADER_TCP_CTRL_MODULE_API_H_ */
