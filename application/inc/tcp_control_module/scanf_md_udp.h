/*
 * scanf_md_udp.h
 *
 *  Created on: 2016年11月8日
 *      Author: leon
 */

#ifndef INC_SCANF_MD_UDP_H_
#define INC_SCANF_MD_UDP_H_

#include "tcp_ctrl_server.h"

typedef enum{
	 UDP_BRD = 1,
	 UDP_SIG,

}udp_type;

/*
 * wifi_sys_ctrl_udp_server
 * udp线程，设备发现
 *
 * 主要是服务端的控制，数据包的广播
 *
 */
void* wifi_sys_ctrl_udp_server(void* p);


#endif /* INC_SCANF_MD_UDP_H_ */
