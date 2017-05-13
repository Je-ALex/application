/*
 * client_heart_manage.h
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#ifndef INC_INFO_MANAGE_CLIENT_HEART_MANAGE_H_
#define INC_INFO_MANAGE_CLIENT_HEART_MANAGE_H_


/*
 * 心跳状态链表信息
 * 保存套接字号
 * 心跳状态
 */
typedef struct {

	int sockfd;
	unsigned int mac_addr;
	volatile char status;

}connect_heart,*Pconnect_heart;

int chm_delete_heart_list(int fd);

int chm_set_communication_heart(Pframe_type type);

int chm_get_communication_heart(Pframe_type type);

int chm_process_communication_heart(const unsigned char* msg,Pframe_type type);

void* wifi_sys_ctrl_tcp_heart_state(void* p);


#endif /* INC_INFO_MANAGE_CLIENT_HEART_MANAGE_H_ */
