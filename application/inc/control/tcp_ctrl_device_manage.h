/*
 * tcp_ctrl_device_manage.h
 *
 *  Created on: 2016年12月21日
 *      Author: leon
 */

#ifndef INC_CONTROL_TCP_CTRL_DEVICE_MANAGE_H_
#define INC_CONTROL_TCP_CTRL_DEVICE_MANAGE_H_











int conf_status_delete_spk_node(int fd);

int conf_status_add_spk_node(Pframe_type type);

int conf_status_search_spk_node(Pframe_type type);

int conf_status_refresh_spk_node(Pframe_type type);


int dmanage_set_communication_heart(Pframe_type type);

int dmanage_get_communication_heart(Pframe_type type);

int dmanage_process_communication_heart(const unsigned char* msg,Pframe_type type);


int dmanage_delete_conference_list(int fd);

int dmanage_delete_connected_list(int fd);

int dmanage_delete_info(int fd);


int dmanage_add_info(const unsigned char* msg,Pframe_type type);

int dmanage_refresh_connected_list(Pconference_list data_info);

int dmanage_refresh_conference_list(Pconference_list data_info);

int dmanage_refresh_info(const Pframe_type data_info);


#endif /* INC_CONTROL_TCP_CTRL_DEVICE_MANAGE_H_ */