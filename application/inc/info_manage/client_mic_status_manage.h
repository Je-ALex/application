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

	int oldfd;
	int sockfd;
	int seat;
	int asport;
	unsigned int ts;
	int status;

}as_port,*Pas_port;

typedef enum{

	CMSM_OPEN_SUCCESS,
	CMSM_PREPARE_CLOSE,
	CMSM_WAIT_REPLY,
	CMSM_UPDATE_REPLY,
	CMSM_MIC_CLOSE,

}MicStatus;


int cmsm_refresh_spk_node(Pframe_type type);

int cmsm_msg_classify_handle(Pframe_type frame_type,const unsigned char* msg);

int cmsm_through_reply_proc_port(Pframe_type type);

#endif /* INC_INFO_MANAGE_CLIENT_MIC_STATUS_MANAGE_H_ */
