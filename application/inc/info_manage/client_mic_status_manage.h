/*
 * client_mic_status_manage.h
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#ifndef INC_INFO_MANAGE_CLIENT_MIC_STATUS_MANAGE_H_
#define INC_INFO_MANAGE_CLIENT_MIC_STATUS_MANAGE_H_


/*
 * 发言设备的端口管理
 * 主要是套接字号
 * 席别号 音频端口号 时间戳
 */
typedef struct {

	int sockfd;
	int seat;
	int asport;
	unsigned int ts;

}as_port,*Pas_port;


int dmanage_send_mic_status_to_pc(Pframe_type type);

int dmanage_search_not_use_spk_port(Pframe_type type);

int dmanage_close_last_spk_client(Pframe_type type);

int dmanage_close_first_spk_client(Pframe_type type);

int dmanage_close_guest_spk_client(Pframe_type type);

int dmanage_delete_spk_node(int fd);

int dmanage_add_spk_node(Pframe_type type);

int dmanage_search_last_spk_node(Pframe_type type);

int dmanage_search_first_spk_node(Pframe_type type);

int cmsm_refresh_spk_node(Pframe_type type);



int cmsm_msg_classify_handle(Pframe_type frame_type,const unsigned char* msg);



#endif /* INC_INFO_MANAGE_CLIENT_MIC_STATUS_MANAGE_H_ */
