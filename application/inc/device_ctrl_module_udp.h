/*
 * device_ctrl_module_udp.h
 *
 *  Created on: 2016年10月21日
 *      Author: leon
 */

#ifndef INC_DEVICE_CTRL_MODULE_UDP_H_
#define INC_DEVICE_CTRL_MODULE_UDP_H_

#define DEF_PORT 50001

int device_ctrl_module_udp(int port);

void* udp_server(void* p);

#endif /* INC_DEVICE_CTRL_MODULE_UDP_H_ */
