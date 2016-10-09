/*
 * ctrl-tcp.h
 *
 *  Created on: 2016年10月9日
 *      Author: leon
 */

#ifndef HEADER_CTRL_TCP_H_
#define HEADER_CTRL_TCP_H_

#include <sys/epoll.h>

/*
 * 连接主机的设备信息
 * 主要有
 * socket fd
 * client addr的信息
 * client mac的信息
 */
typedef struct {

	int fd;
	char clint_mac[24];
	struct sockaddr_in cli_addr;
	int clilen;

}Client_Info;

typedef struct {

	int ID;
	int fd;
	unsigned int IPaddr;
	int Port;
	int Online;

} ControlDeviceT,*PControlDeviceT;

typedef struct {
    void* data;
    struct client_node *next;

} client_node, *pclient_node;

typedef struct {
    int size;
    pclient_node last;
    pclient_node first;
} client_nodeHead, *pclient_nodeHead;

typedef struct {

	int fd;
	char clint_mac[24];

}Server_Info;

enum{

	SOCKERRO = -3,
	BINDERRO,
	LISNERRO,
	SUCCESS = 0,

};

typedef enum{

	PC_CTRL = 1,
	HOST_CTRL,
	UNIT_CTRL,

}Machine_Type;
typedef enum{

	EVT_DATA = 1,
	UNIT_DATA,
	CONFERENCE_DATA,

}Data_Type;
typedef enum{

	CTRL_DATA = 1,
	REQUEST_DATA,
	REPLY_DATA,
}Message_Type;


typedef struct {

	char msg_type;
	char data_type;
	char dev_type;

}Frame_Type;





#endif /* HEADER_CTRL_TCP_H_ */
