/*
 * client_connect_manage.h
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#ifndef INC_INFO_MANAGE_CLIENT_CONNECT_MANAGE_H_
#define INC_INFO_MANAGE_CLIENT_CONNECT_MANAGE_H_






int ccm_delete_info(int fd);


int ccm_add_info(const unsigned char* msg,Pframe_type type);


int ccm_refresh_conference_list(Pconference_list data_info);

int ccm_refresh_info(const Pframe_type data_info);



#endif /* INC_INFO_MANAGE_CLIENT_CONNECT_MANAGE_H_ */
