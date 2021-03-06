
#ifndef INC_TCP_CTRL_SERVER_H_
#define INC_TCP_CTRL_SERVER_H_


#define TCP_DBG 0




#define DEVICE_HEART 	5




/*
 * 应答类消息的错误码
 * 0X01 成功
 * 0X02 头错误
 * 0X03 长度错误
 * 0X04 校验和错误
 */
typedef enum{

	TCP_C_ERR_SUCCESS = 1,
	TCP_C_ERR_HEAD,
	TCP_C_ERR_LENGTH,
	TCP_C_ERR_CHECKSUM,
	TCP_C_ERR_UNKNOW,
}tcp_ctrl_err;

/*
 * 0x01	控制内容信息
 * 0x02	查询内容信息
 * 0x03	请求内容信息
 * 0x04 控制应答响应
 * 0x05 查询应答响应
 */
typedef enum{
	WRITE_MSG = 1,
	READ_MSG,
	REQUEST_MSG,
	W_REPLY_MSG,
	R_REPLY_MSG,
	ONLINE_REQ,
	OFFLINE_REQ,
	ONLINE_HEART,

}Message_Type;

/*
 * 0x01	上位机
 * 0x02	主机数据
 * 0x03	单元机数据
 */
typedef enum{

	PC_CTRL = 1,
	HOST_CTRL,
	UNIT_CTRL,

}Machine_Type;

/*
 * 0x01	事件型数据
 * 0x03	会议型参数
 */
typedef enum{

	EVENT_DATA = 1,
	CONFERENCE_DATA,

}Data_Type;


/*
 * 数据格式编码
 *
 */
typedef enum {

	WIFI_MEETING_CHAR = 1,
	WIFI_MEETING_UCHAR,
	WIFI_MEETING_SHORT,
	WIFI_MEETING_USHORT,
	WIFI_MEETING_INT,
	WIFI_MEETING_UINT,
	WIFI_MEETING_LONG,
	WIFI_MEETING_ULONG,
	WIFI_MEETING_LONGLONG,
	WIFI_MEETING_STRING,

	WIFI_MEETING_ERROR = 0xe3,

}client_code_type;


/*
 * 投票结果
 */
typedef enum {

	WIFI_MEETING_CON_V_ASSENT  = 1,
	WIFI_MEETING_CON_V_NAY,
	WIFI_MEETING_CON_V_WAIVER ,
	WIFI_MEETING_CON_V_TOUT ,

}vote_name;

/*
 * 会议类名称编码
 *
 */
typedef enum {

	WIFI_MEETING_CON_ID  = 1,
	WIFI_MEETING_CON_SEAT,
	WIFI_MEETING_CON_NAME ,
	WIFI_MEETING_CON_CNAME ,
	WIFI_MEETING_CON_SUBJ ,
	WIFI_MEETING_CON_VOTE ,
	WIFI_MEETING_CON_ELEC ,
	WIFI_MEETING_CON_SCORE ,

}conference_name_type;

/*
 * 议题属性编码
 */
typedef enum {

	WIFI_MEETING_CON_SUBJ_NORMAL = 0,
	WIFI_MEETING_CON_SUBJ_ELE,
	WIFI_MEETING_CON_SUBJ_VOTE,
	WIFI_MEETING_CON_SUBJ_SCORE,

}subject_attribute;

/*
 * 事件类名称编码
 *
 */
typedef enum {

	WIFI_MEETING_EVT_PWR = 1,
	WIFI_MEETING_EVT_MIC,
	WIFI_MEETING_EVT_CHECKIN,
	WIFI_MEETING_EVT_SPK,
	WIFI_MEETING_EVT_VOT,
	WIFI_MEETING_EVT_SUB,
	WIFI_MEETING_EVT_SEFC,
	WIFI_MEETING_EVT_SER,
	WIFI_MEETING_EVT_ALL_STATUS,
	WIFI_MEETING_EVT_SSID,
	WIFI_MEETING_EVT_KEY,
	WIFI_MEETING_EVT_MAC,

	WIFI_MEETING_EVT_PC_GET_INFO,
	WIFI_MEETING_EVT_RP_TO_PC,
	WIFI_MEETING_EVT_UNIT_ONOFF_LINE,
	WIFI_MEETING_EVT_ELECTION,
	WIFI_MEETING_EVT_SCORE,
	WIFI_MEETING_EVT_CON_MAG,
	WIFI_MEETING_EVT_SPK_NUM,
	WIFI_MEETING_EVT_AD_PORT,
	WIFI_MEETING_EVT_UNIT_ELECTRICITY,
	WIFI_MEETING_EVT_SYS_TIME,
	WIFI_MEETING_EVT_HOST_STATUS,
	WIFI_MEETING_EVT_CAMERA_TRACK,

}event_name_type;

typedef enum {
	WIFI_MEETING_EVT_PWR_ON = 1,
	WIFI_MEETING_EVT_PWR_OFF,
	WIFI_MEETING_EVT_PWR_OFF_ALL,
}event_power;

typedef enum {

	WIFI_MEETING_EVT_MIC_CHAIRMAN = 1,//主席优先
	WIFI_MEETING_EVT_MIC_FIFO,
	WIFI_MEETING_EVT_MIC_STAD,
	WIFI_MEETING_EVT_MIC_FREE,
}event_mic;

typedef enum {
	WIFI_MEETING_EVT_SPK_ONE= 1,
	WIFI_MEETING_EVT_SPK_TWO,
	WIFI_MEETING_EVT_SPK_FOURE,
	WIFI_MEETING_EVT_SPK_SIX,
	WIFI_MEETING_EVT_SPK_EIGHT,
}event_spk_num;

typedef enum {
	WIFI_MEETING_EVT_SPK_ALLOW = 1,
	WIFI_MEETING_EVT_SPK_VETO,
	WIFI_MEETING_EVT_SPK_ALOW_SND,
	WIFI_MEETING_EVT_SPK_VETO_SND,
	WIFI_MEETING_EVT_SPK_REQ_SND,
	WIFI_MEETING_EVT_SPK_REQ_SPK,
	WIFI_MEETING_EVT_SPK_CLOSE_MIC,
	WIFI_MEETING_EVT_SPK_CLOSE_REQ,
	WIFI_MEETING_EVT_SPK_CLOSE_SND,
	WIFI_MEETING_EVT_SPK_CHAIRMAN_ONLY_ON,
	WIFI_MEETING_EVT_SPK_CHAIRMAN_ONLY_OFF,
	WIFI_MEETING_EVT_SPK_OPEN_MIC,

}event_speak;


typedef enum {

	WIFI_MEETING_EVT_SER_WATER = 1,
	WIFI_MEETING_EVT_SER_PEN,
	WIFI_MEETING_EVT_SER_PAPER,
	WIFI_MEETING_EVT_SER_TEA,
	WIFI_MEETING_EVT_SER_TEC,

}event_service;

typedef enum {

	WIFI_MEETING_EVT_CHECKIN_START = 1,
	WIFI_MEETING_EVT_CHECKIN_END,
	WIFI_MEETING_EVT_CHECKIN_SELECT,

}event_checkin;

typedef enum {
	WIFI_MEETING_EVT_VOT_START = 1,
	WIFI_MEETING_EVT_VOT_END,
	WIFI_MEETING_EVT_VOT_RESULT,
	WIFI_MEETING_EVT_VOT_ASSENT,
	WIFI_MEETING_EVT_VOT_NAY,
	WIFI_MEETING_EVT_VOT_WAIVER,
	WIFI_MEETING_EVT_VOT_TOUT,
}event_vote;

typedef enum {

	WIFI_MEETING_EVT_ELECTION_START = 1,
	WIFI_MEETING_EVT_ELECTION_END,
	WIFI_MEETING_EVT_ELECTION_RESULT,
	WIFI_MEETING_EVT_ELECTION_UNDERWAY,

}event_election;

typedef enum {

	WIFI_MEETING_EVT_SCORE_START = 1,
	WIFI_MEETING_EVT_SCORE_END,
	WIFI_MEETING_EVT_SCORE_RESULT,
	WIFI_MEETING_EVT_SCORE_UNDERWAY,
}event_score;

typedef enum {

	WIFI_MEETING_EVT_CON_MAG_START = 1,
	WIFI_MEETING_EVT_CON_MAG_END,
}conference_manage;

/*
 * 主机上发送单元参数给上位机编码
 */
typedef enum {

	WIFI_MEETING_EVT_RP_TO_PC_FD = 1,
	WIFI_MEETING_EVT_RP_TO_PC_ID,
	WIFI_MEETING_EVT_RP_TO_PC_IP,
	WIFI_MEETING_EVT_RP_TO_PC_SEAT,
	WIFI_MEETING_EVT_RP_TO_PC_POWER,
}report_uinfo_to_pc;

/*
 * 主机基本信息相关
 */
typedef enum {

	WIFI_MEETING_EVT_HOST_MIC = 1,
	WIFI_MEETING_EVT_HOST_SPK,
	WIFI_MEETING_EVT_HOST_SND,
	WIFI_MEETING_EVT_HOST_CTRACK,

}report_hinfo_to_pc;

typedef enum {

	WIFI_MEETING_EVT_AD_PORT_SPK = 1,
	WIFI_MEETING_EVT_AD_PORT_LSN,
}audio_port;


/*
 * 席别
 */
typedef enum {

	WIFI_MEETING_CON_SE_CHAIRMAN = 1,
	WIFI_MEETING_CON_SE_GUEST,
	WIFI_MEETING_CON_SE_ATTEND,

}conference_seat;



void* wifi_sys_ctrl_tcp_recv(void* p);

void* wifi_sys_ctrl_tcp_send(void* p);



void* control_tcp_send(void* p);
void* control_tcp_queue(void* p);



#endif /* INC_TCP_CTRL_SERVER_H_ */
