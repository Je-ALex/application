/*
 * tcp_ctrl_device_status.c
 *
 *  Created on: 2016年10月20日
 *      Author: leon
 */

#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_data_compose.h"
#include "client_heart_manage.h"
#include "tcp_ctrl_data_process.h"
#include "sys_uart_init.h"

extern sys_info sys_in;
extern Pglobal_info node_queue;

static volatile int spk_offset[10] = {0};
static volatile int spk_ts[10] = {0};
/*
 * 网络状态
 */
int sys_net_status_set(int value)
{
	node_queue->con_status->lan_stat = value;
	return SUCCESS;
}
int sys_net_status_get()
{
	return node_queue->con_status->lan_stat;
}
/*
 * 系统调试开关状态获取
 */
int sys_debug_get_switch()
{
//	int log_file = -1;
//	char* path = "/hushan/log.dat";
//	char value = 0;
//
//	log_file = open(path,O_RDONLY);
//
//	if(log_file < 0)
//    {
//		printf("%s-%s-%d log_file open failed \n",__FILE__,__func__,__LINE__);
//
//		return ERROR;
//	}else{
//		read(log_file,&value,sizeof(value));
//		if(value == '0')
//		{
//			return 0;
//		}else if(value=='1'){
//			return 1;
//		}
//	}
//	close(log_file);

	return 0;

}


/*********************************
 *TODO 互斥锁/信号量/
 *********************************/
inline void sys_mutex_lock(int value)
{
	pthread_mutex_lock(&sys_in.sys_mutex[value]);
}
inline void sys_mutex_unlock(int value)
{
	pthread_mutex_unlock(&sys_in.sys_mutex[value]);
}

inline void sys_semaphore_wait(int value)
{
	sem_wait(&sys_in.sys_sem[value]);
}

inline void sys_semaphore_post(int value)
{
	sem_post(&sys_in.sys_sem[value]);
}


/*********************************
 *TODO 队列处理接口函数部分
 *********************************/
/*
 * tcp_ctrl_tprecv_enqueue
 * tcp控制模块的数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tprecv_enqueue(int* fd,unsigned char* msg,int* len)
{

	Pctrl_tcp_rsqueue tmp = NULL;


	tmp = (Pctrl_tcp_rsqueue)malloc(sizeof(ctrl_tcp_rsqueue));
	memset(tmp,0,sizeof(ctrl_tcp_rsqueue));

	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = *fd;
	tmp->len = *len;

	if(tmp->len > 0)
	{
		tmp->msg = malloc(tmp->len);
		memset(tmp->msg,0,tmp->len);

		memcpy(tmp->msg,msg,tmp->len);
	}

	enter_queue(node_queue->sys_queue[CTRL_TCP_RECV_QUEUE],tmp);

	sys_semaphore_post(CTRL_TCP_RECV_SEM);

	return SUCCESS;

}

/*
 * tcp_ctrl_tprecv_dequeue
 * tcp控制模块的接收的数据处队列
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 *
 * in/out:
 * @Pctrl_tcp_rsqueue
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tprecv_dequeue(Pctrl_tcp_rsqueue msg_tmp)
{
	int ret;
	Plinknode node;
	Pctrl_tcp_rsqueue tmp;

	sem_wait(&sys_in.sys_sem[CTRL_TCP_RECV_SEM]);

//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	ret = out_queue(node_queue->sys_queue[CTRL_TCP_RECV_QUEUE],&node);


	if(ret == 0)
	{
		tmp = node->data;

		msg_tmp->socket_fd = tmp->socket_fd;
		msg_tmp->len = tmp->len;
		memcpy(msg_tmp->msg,tmp->msg,tmp->len);

		free(tmp->msg);
		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_tcp_send_enqueue
 * tcp控制模块的数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @msg 组包后的数据信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tpsend_enqueue(Pframe_type type,unsigned char* msg)
{

	Pctrl_tcp_rsqueue tmp = NULL;

	tmp = (Pctrl_tcp_rsqueue)malloc(sizeof(ctrl_tcp_rsqueue));
	memset(tmp,0,sizeof(ctrl_tcp_rsqueue));

	tmp->msg = (unsigned char*)malloc(type->frame_len);
	memset(tmp->msg,0,type->frame_len);

//	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = type->fd;
	tmp->len = type->frame_len;
	memcpy(tmp->msg,msg,type->frame_len);

	enter_queue(node_queue->sys_queue[CTRL_TCP_SEND_QUEUE],tmp);

//	sem_post(&sys_in.sys_sem[CTRL_TCP_SEND_SEM]);

	sys_semaphore_post(CTRL_TCP_SEND_SEM);

	return SUCCESS;
}


/*
 * tcp_ctrl_tpsend_dequeue
 * tcp控制模块的数据出队列
 * 将消息数据送入发送队列等待发送
 *
 * in/out:
 * @Ptcp_send 数据信息结构体
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_tpsend_dequeue(Pctrl_tcp_rsqueue event_tmp)
{

	int ret;
	Plinknode node;
	Pctrl_tcp_rsqueue tmp = NULL;

//	sem_wait(&sys_in.sys_sem[CTRL_TCP_SEND_SEM]);

	sys_semaphore_wait(CTRL_TCP_SEND_SEM);

#if TCP_DBG
	printf("get the value from tcp_ctrl_tpsend_outqueue queue\n");
#endif


	ret = out_queue(node_queue->sys_queue[CTRL_TCP_SEND_QUEUE],&node);
	if(ret == 0)
	{
		tmp = node->data;

		event_tmp->socket_fd = tmp->socket_fd;
		event_tmp->len = tmp->len;
		memcpy(event_tmp->msg,tmp->msg,tmp->len);

		free(tmp->msg);
		free(tmp);
		free(node);

		tmp->msg = NULL;
		tmp = NULL;
		node = NULL;

	}else{
		return ERROR;
	}


	return SUCCESS;
}

/*
 * tcp_ctrl_report_enqueue
 * 上报数据数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @value 事件信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
int tcp_ctrl_report_enqueue(Pframe_type type,int value)
{

	Prun_status tmp;
	tmp = (Prun_status)malloc(sizeof(run_status));
	memset(tmp,0,sizeof(run_status));

//	printf("%s-%s-%d,value:%d\n",__FILE__,__func__,
//			__LINE__,value);

	/*
	 * 单元机固有属性
	 */
	tmp->socket_fd = type->fd;
	tmp->id = type->ucinfo.id;
	tmp->seat = type->ucinfo.seat;
	tmp->value = value;
#if TCP_DBG
	printf("enter the tcp_ctrl_report_enqueue..\n");
#endif

	pthread_mutex_lock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);
	enter_queue(node_queue->sys_queue[LOCAL_REP_QUEUE],tmp);
	pthread_mutex_unlock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);

	sem_post(&sys_in.sys_sem[LOCAL_REP_SEM]);

	return SUCCESS;
}

int tcp_ctrl_report_dequeue(Prun_status event_tmp)
{

	int ret;
	Plinknode node;
	Prun_status tmp;

	sem_wait(&sys_in.sys_sem[LOCAL_REP_SEM]);

	pthread_mutex_lock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);
	ret = out_queue(node_queue->sys_queue[LOCAL_REP_QUEUE],&node);
	pthread_mutex_unlock(&sys_in.sys_mutex[LOCAL_REP_MUTEX]);

	if(ret == 0)
	{
		tmp = node->data;
		memcpy(event_tmp,tmp,sizeof(run_status));

		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}

/*********************************
 *TODO 设备连接状态相关
 *********************************/
/*
 * conf_status_get_connected_len
 * 获取会议中连接设备的数量
 */
int conf_status_get_connected_len()
{
	return node_queue->sys_list[CONNECT_LIST]->size;
}

/*
 * conf_status_get_connected_len
 * 获取会议中连接设备的数量
 */
int conf_status_get_client_connect_len()
{
	int ret = 0;
	if(conf_status_get_pc_staus() > 0)
	{
		ret = node_queue->sys_list[CONNECT_LIST]->size - 1;
	}else{
		ret = node_queue->sys_list[CONNECT_LIST]->size;
	}
	return ret;
}

/*
 * conf_status_get_conference_len
 * 获取会议中参会设备数量
 */
int conf_status_get_conference_len()
{
	return node_queue->sys_list[CONFERENCE_LIST]->size;
}

/*
 * conf_status_check_client_legal
 * 检查数据包来源合法性，会议表中是否有此设备
 * 并将此设备的席位赋值给临时变量
 */
int conf_status_check_client_conf_legal(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pconference_list conf_info = NULL;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->fd == type->fd
				&& type->s_id == conf_info->ucinfo.id)
		{
			type->ucinfo.seat = conf_info->ucinfo.seat;
			type->ucinfo.id = conf_info->ucinfo.id;
			pos++;
			break;
		}

		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_check_connect_legal
 * 检查连接信息中，设备的合法性
 */
int conf_status_check_client_connect_legal(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->client_fd == type->fd)
		{
			type->ucinfo.seat = cnet_list->seat;
			pos++;
			break;
		}

		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_check_chariman_legal
 * 检查数据包来源合法性，是否是主席单元发送的数据
 */
int conf_status_check_chairman_legal(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pconference_list conf_info = NULL;
	int pos = 0;

	/*
	 * 判断消息是否是主席单元发送
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->fd == type->fd)
		{
			if(conf_info->ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
			{
				pos++;
				break;
			}
		}
		tmp=tmp->next;
	}

	return pos;
}

int conf_status_check_pc_legal(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->client_fd == type->fd)
		{
			pos++;
			break;
		}

		tmp=tmp->next;
	}
	return pos;
}

/*
 * conf_status_check_chariman_staus
 * 检查会议中是否有主席单元存在
 *
 * 返回值
 * 主席状态
 * 0无
 * 1有
 *
 */
int conf_status_check_chariman_staus()
{
	pclient_node tmp = NULL;
	Pconference_list conf_info = NULL;
	int pos = 0;

	/*
	 * 判断消息是否是主席单元发送
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_find_did_sockfd
 * 检测临时变量中目标地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_did_sockfd_id(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list = NULL;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->id == type->d_id)
		{
			type->fd = cnet_list->client_fd;
			type->ucinfo.seat = cnet_list->seat;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	if(!pos)
		printf("%s-%s-%d，did=%d,pos=%d\n",__FILE__,__func__,__LINE__,type->d_id,pos);

	return pos;
}

/*
 * conf_status_find_pc_did_sockfd
 * 检测临时变量中目标地址对应的客户端的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_pc_did_sockfd(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list;
	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->mac_addr == type->d_id)
		{
			type->fd = cnet_list->client_fd;
			type->d_id = cnet_list->id;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_pc_find_did_id
 * 通过上位机的目标地址找寻MAC相匹配的单元
 */
int conf_status_pc_find_did_id(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cnet_list = NULL;

	int pos = 0;

	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cnet_list = tmp->data;
		if(cnet_list->mac_addr == type->d_id)
		{
			type->d_id = cnet_list->id;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}


/*
 * conf_status_find_chariman_sockfd
 * 检测链表中，主席单元的sockfd
 *
 * @Pframe_type
 *
 */
int conf_status_find_chairman_sockfd(Pframe_type type)
{
	pclient_node tmp = NULL;
	Pconference_list conf_info = NULL;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->ucinfo.seat == WIFI_MEETING_CON_SE_CHAIRMAN)
		{
			printf("find the chairman\n");
			type->fd=conf_info->fd;
			pos++;
			break;
		}
		tmp=tmp->next;
	}

	return pos;
}

/*
 * conf_status_find_max_id
 * 检测链表中，ID最大值
 *
 */
int conf_status_find_max_id()
{
	pclient_node tmp = NULL;
	Pconference_list conf_info = NULL;
	int max_id = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->ucinfo.id)
		{
			if(conf_info->ucinfo.id > max_id)
			{
				max_id = conf_info->ucinfo.id;
			}
		}
		tmp=tmp->next;
	}

	return max_id;
}

/*
 * conf_status_compare_id
 * 比较链表中id号
 */
int conf_status_compare_id(int value)
{
	pclient_node tmp = NULL;
	Pconference_list conf_info = NULL;
	int pos = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		conf_info = tmp->data;
		if(conf_info->ucinfo.id == value)
		{
			pos++;
			break;
		}
		tmp=tmp->next;
	}
	return pos;
}


/*********************************
 *TODO 会议参数相关接口
 *********************************/
/*
 * conf_status_set_conference_name
 * 会议名的设置
 */
int conf_status_set_conference_name(const char* name,int len)
{
	memset(node_queue->con_status->ccontent.conf_name,0,
			sizeof(node_queue->con_status->ccontent.conf_name));

	memcpy(node_queue->con_status->ccontent.conf_name,name,len);

	return SUCCESS;
}

/*
 * conf_status_set_subject_content
 * 会议的议题内容设置
 */
int conf_status_set_subject_content(int num,const unsigned char* msg,int len)
{
	memset(node_queue->con_status->ccontent.scontent[num].sub,0,
			sizeof(node_queue->con_status->ccontent.scontent[num].sub));

	memcpy(node_queue->con_status->ccontent.scontent[num].sub,msg,len);

	if(conf_status_get_total_subject() == num)
	{
		//需要将内容保存到文件

	}

	return SUCCESS;
}

/*
 * conf_status_set_total_subject
 * 会议议题数量设置
 */
int conf_status_set_total_subject(unsigned char num)
{
	node_queue->con_status->ccontent.total_sub = num;

	return SUCCESS;
}

/*
 * conf_status_get_total_subject
 * 会议议题数量获取
 */
int conf_status_get_total_subject()
{
	return node_queue->con_status->ccontent.total_sub;
}

/*
 * conf_status_set_conf_staus
 * 设置会议进程状态
 */
int conf_status_set_conf_staus(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->cstatus.confer_status = value;

	/*
	 * 会议结束需要将会议信息相关的参数情况
	 * 如投票 选举等结果参数
	 */
	if(value == WIFI_MEETING_EVENT_CON_MAG_END)
	{
		memset(&node_queue->con_status->ccontent,0,sizeof(conf_content));
	}

	return SUCCESS;
}

/*
 * conf_status_get_conf_staus
 * 获取会议进程状态
 */
int conf_status_get_conf_staus()
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->cstatus.confer_status);

	return node_queue->con_status->cstatus.confer_status;
}

/*
 * conf_status_set_subjet_staus
 * 设置会议议题状态
 * 1、开始
 * 2、结束
 * 3、公布结果
 */
int conf_status_set_subjet_staus(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->cstatus.subject_status = value;

	return SUCCESS;
}

/*
 * conf_status_get_subjet_staus
 * 获取会议议题进程状态
 */
int conf_status_get_subjet_staus()
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->cstatus.subject_status);

	return node_queue->con_status->cstatus.subject_status;
}

/*
 * conf_status_set_current_subject
 * 设置当前进行的议题号
 */
int conf_status_set_current_subject(unsigned char num)
{

	node_queue->con_status->cstatus.current_sub = num;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->cstatus.current_sub);

	return SUCCESS;
}

/*
 * conf_status_get_current_subject
 * 会议中投票议题的状态
 */
int conf_status_get_current_subject()
{
	return node_queue->con_status->cstatus.current_sub;
}

/*
 * conf_status_set_subject_property
 * 设置议题属性
 */
int conf_status_set_subject_property(unsigned char num,unsigned char prop)
{
	printf("%s-%s-%d,num=%d,prop=%d\n",__FILE__,__func__,__LINE__,num,prop);

	node_queue->con_status->ccontent.scontent[num].sprop = prop;

	return SUCCESS;
}

/*
 * conf_status_get_subject_property
 * 会议中议题的属性
 */
int conf_status_get_subject_property(unsigned char num)
{
	unsigned char sub_num;
	unsigned char prop = 0;

	if(num == 0)
	{
		sub_num = conf_status_get_current_subject();
		printf("%s-%s-%d,prop=%d\n",__FILE__,__func__,__LINE__,
				node_queue->con_status->ccontent.scontent[sub_num].sprop);
		prop = node_queue->con_status->ccontent.scontent[sub_num].sprop;
	}
	else
	{
		printf("%s-%s-%d,prop=%d\n",__FILE__,__func__,__LINE__,
				node_queue->con_status->ccontent.scontent[num].sprop);
		prop = node_queue->con_status->ccontent.scontent[num].sprop;
	}

	return prop;
}

/*
 * conf_status_save_pc_conf_result
 * 会议议题对应的结果设置
 */
int conf_status_set_pc_conf_result(int len,const unsigned char* msg)
{
	memset(&node_queue->con_status->cstatus.cresult,0,
			sizeof(node_queue->con_status->cstatus.cresult));

	printf("%s-%s-%d-%d\n",__FILE__,__func__,__LINE__,len);

	node_queue->con_status->cstatus.cresult.len = len;
	memcpy(node_queue->con_status->cstatus.cresult.conf_result,msg,len);

	return SUCCESS;
}

/*
 * conf_status_get_pc_conf_result
 * 获取议题对应
 */
int conf_status_get_pc_conf_result(unsigned char* msg)
{
	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	memcpy(msg,node_queue->con_status->cstatus.cresult.conf_result,
			node_queue->con_status->cstatus.cresult.len);

	return node_queue->con_status->cstatus.cresult.len;
}

/*
 * conf_status_get_vote_result
 * 表决议题结果获取
 */
int conf_status_get_vote_result(unsigned char num,unsigned short* value)
{
	int c_num = 0;

	if(num == 0)
	{
		c_num = conf_status_get_current_subject();
		value[0] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.assent;
		value[1] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.nay;
		value[2] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.waiver;
		value[3] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.timeout;

	}else{
		value[0] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.assent;
		value[1] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.nay;
		value[2] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.waiver;
		value[3] = node_queue->con_status->ccontent.scontent[c_num].sresult.v_result.timeout;
	}

	return SUCCESS;
}

/*
 * conf_status_set_vote_result
 * 表决议题结果设置
 */
int conf_status_set_vote_result(int value)
{
	unsigned char sub_num;
//	int sub_prop = 0;

	sub_num = conf_status_get_current_subject();

	printf("%s-%s-%d,sub_num=%d\n",__FILE__,__func__,__LINE__,sub_num);

//	if(conf_status_get_pc_staus() > 0)
//	{
//		//判断当前议题的属性
//		sub_prop = conf_status_get_subject_property(sub_num);
////		if(sub_prop == WIFI_MEETING_CON_SUBJ_VOTE)
////		{
//			switch(value)
//			{
//			case WIFI_MEETING_EVENT_VOTE_ASSENT:
//				node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.assent++;
//				break;
//			case WIFI_MEETING_EVENT_VOTE_NAY:
//				node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.nay++;
//				break;
//			case WIFI_MEETING_EVENT_VOTE_WAIVER:
//				node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.waiver++;
//				break;
//			case WIFI_MEETING_EVENT_VOTE_TIMEOUT:
//				node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.timeout++;
//				break;
//			}
////		}else{
////			printf("%s-%s-%d the subject is not vote subject\n",__FILE__,__func__,__LINE__);
////			return ERROR;
////		}
//	}else{
		node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.timeout--;
		switch(value)
		{
		case WIFI_MEETING_EVENT_VOTE_ASSENT:
			node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.assent++;
			break;
		case WIFI_MEETING_EVENT_VOTE_NAY:
			node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.nay++;
			break;
		case WIFI_MEETING_EVENT_VOTE_WAIVER:
			node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.waiver++;
			break;
		case WIFI_MEETING_EVENT_VOTE_TIMEOUT:
			node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.timeout++;
			break;
		}
//	}

	return SUCCESS;
}

/*
 * conf_status_reset_vote_status
 * 表决议题结果复位
 */
int conf_status_reset_vote_status()
{
	unsigned char sub_num;

	sub_num = conf_status_get_current_subject();
	printf("%s-%s-%d,sub_num=%d\n",__FILE__,__func__,__LINE__,sub_num);

	node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.assent = 0;
	node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.nay = 0;
	node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.waiver = 0;
	node_queue->con_status->ccontent.scontent[sub_num].sresult.v_result.timeout = conf_status_get_conference_len();

	return SUCCESS;
}

/*
 * conf_status_calc_vote_result
 * 统计表决结果
 */
int conf_status_calc_vote_result()
{
	unsigned short result[4] = {0};
	unsigned char msg[64] = {0};
	int cindex = 0;

	if(conf_status_get_pc_staus() > 0)
	{
		;
	}else{
		conf_status_get_vote_result(0,result);

		msg[cindex++] = WIFI_MEETING_CON_VOTE;
		msg[cindex++] = WIFI_MEETING_STRING;

		msg[cindex++] = WIFI_MEETING_CON_V_ASSENT;
		msg[cindex++] = (result[0]>>8) & 0xff;
		msg[cindex++] = result[0] & 0xff;

		msg[cindex++] = WIFI_MEETING_CON_V_NAY;
		msg[cindex++] = (result[1]>>8) & 0xff;
		msg[cindex++] = result[1] & 0xff;

		msg[cindex++] = WIFI_MEETING_CON_V_WAIVER;
		msg[cindex++] = (result[2]>>8) & 0xff;
		msg[cindex++] = result[2] & 0xff;

		msg[cindex++] = WIFI_MEETING_CON_V_TOUT;
		msg[cindex++] = (result[3]>>8) & 0xff;
		msg[cindex++] = result[3] & 0xff;

		conf_status_set_pc_conf_result(cindex,msg);
	}

	return SUCCESS;

}

/*
 * conf_status_calc_vote_result
 * 发送主机统计的表决结果
 */
int conf_status_send_hvote_result()
{
	unsigned char msg[64] = {0};
	frame_type data_info;
	int ret = 0;
	int pos = 0;

	if(conf_status_get_pc_staus() > 0)
	{
		;
	}else{
		memset(&data_info,0,sizeof(frame_type));

		data_info.msg_type = WRITE_MSG;
		data_info.data_type = CONFERENCE_DATA;
		data_info.dev_type = HOST_CTRL;

		data_info.evt_data.status = WIFI_MEETING_EVENT_PC_CMD_SIGNAL;

		pos = conf_status_find_chairman_sockfd(&data_info);
		if(pos > 0)
		{
			ret = conf_status_get_pc_conf_result(msg);
			data_info.data_len = ret;
			tcp_ctrl_source_dest_setting(-1,data_info.fd,&data_info);
			tcp_ctrl_module_edit_info(&data_info,msg);
		}
	}


	return SUCCESS;
}

/*
 * conf_status_send_vote_result
 * 发送表决结果给所有单元
 */
int conf_status_send_vote_result()
{
	unsigned char msg[64] = {0};
	frame_type data_info;
	int ret = 0;
	memset(&data_info,0,sizeof(frame_type));

//	unsigned short result[4] = {0};

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	data_info.msg_type = WRITE_MSG;
	data_info.data_type = CONFERENCE_DATA;
	data_info.dev_type = HOST_CTRL;

	data_info.evt_data.status = WIFI_MEETING_EVENT_PC_CMD_ALL;
	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
//	if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_VOTE)
//	{
//		data_info.name_type[0] = WIFI_MEETING_CON_VOTE;
//		data_info.code_type[0] = WIFI_MEETING_STRING;
//
//		data_info.msg_type = WRITE_MSG;
//		data_info.data_type = CONFERENCE_DATA;
//		data_info.dev_type = HOST_CTRL;
//
//		if(conf_status_get_pc_staus()>0)
//		{
//			data_info.evt_data.status = WIFI_MEETING_CONF_SCORE_RESULT;
//			ret = conf_status_get_pc_conf_result(msg);
//			data_info.data_len = ret;
//			tcp_ctrl_module_edit_info(&data_info,msg);
//
//		}else{
//
//			conf_status_get_vote_result(0,result);
//
//			data_info.con_data.v_result.assent = result[0];
//			data_info.con_data.v_result.nay = result[1];
//			data_info.con_data.v_result.waiver = result[2];
//			data_info.con_data.v_result.timeout = result[3];
//
//			tcp_ctrl_module_edit_info(&data_info,NULL);
//
//		}
//	}else{
//		printf("%s-%s-%d not vote subject\n",__FILE__,__func__,__LINE__);
//		return ERROR;
//	}

	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
	if(conf_status_get_pc_staus()>0)
	{
//		if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_VOTE)
//		{

			ret = conf_status_get_pc_conf_result(msg);
			data_info.data_len = ret;

			tcp_ctrl_module_edit_info(&data_info,msg);

//		}else{
//
//			printf("%s-%s-%d not vote subject\n",__FILE__,__func__,__LINE__);
//			return ERROR;
//		}
	}else{

		ret = conf_status_get_pc_conf_result(msg);
		data_info.data_len = ret;

		tcp_ctrl_module_edit_info(&data_info,msg);

	}

	return SUCCESS;

}

/*
 * conf_status_set_elec_result
 * 设置选举结果
 */
int conf_status_set_elec_result(const unsigned char* msg)
{
	unsigned char sub_num = conf_status_get_current_subject();
//	int sub_prop = 0;
	int totalp = 0;

	//判断当前议题的属性
//	sub_prop = conf_status_get_subject_property(sub_num);
//	if(sub_prop == WIFI_MEETING_CON_SUBJ_ELE)
//	{
	int i,value = 0;

	totalp = conf_status_get_elec_totalp(sub_num);
	for(i=0;i<totalp;i++)
	{
		value = msg[i];
		node_queue->con_status->ccontent.scontent[sub_num].sresult.ele_result.ele_id[value]++;
	}

//	}else{
//		printf("%s-%s-%d the subject is not ele subject\n",__FILE__,__func__,__LINE__);
//		return ERROR;
//	}

	return SUCCESS;
}

/*
 * conf_status_get_elec_result
 * 获取选举结果
 */
int conf_status_get_elec_result(unsigned char num,unsigned short value)
{
	int sub_num ;

	if(num == 0){
		sub_num = conf_status_get_current_subject();
	}else{
		sub_num = num;
	}

	return node_queue->con_status->ccontent.scontent[sub_num].sresult.ele_result.ele_id[value];
}

/*
 * conf_status_set_elec_totalp
 * 设置参选总人数
 */
int conf_status_set_elec_totalp(unsigned char num,unsigned char pep)
{
	node_queue->con_status->ccontent.scontent[num].sresult.ele_result.ele_total = pep;

	printf("%s-%s-%d  ele_total=%d\n",__FILE__,
			__func__,__LINE__,pep);

	return SUCCESS;
}

/*
 * conf_status_get_elec_totalp
 * 获取参选总人数
 */
int conf_status_get_elec_totalp(unsigned char num)
{
	unsigned char sub_num ;
//	int sub_prop = 0;

	if(num == 0){
		sub_num = conf_status_get_current_subject();
	}else{
		sub_num = num;
	}

	//判断当前议题的属性
//	sub_prop = conf_status_get_subject_property(sub_num);
//	if(sub_prop == WIFI_MEETING_CON_SUBJ_ELE)
//	{
		return node_queue->con_status->ccontent.scontent[sub_num].sresult.ele_result.ele_total;
//	}else
//	{
//		printf("%s-%s-%d the subject is not elec subject\n",__FILE__,__func__,__LINE__);
//		return ERROR;
//	}

}

/*
 * conf_status_send_elec_result
 * 发送评分结果给所有单元
 */
int conf_status_send_elec_result()
{

	unsigned char msg[64] = {0};
	int ret = 0;
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);


	data_info.msg_type = WRITE_MSG;
	data_info.data_type = CONFERENCE_DATA;
	data_info.dev_type = HOST_CTRL;

	data_info.evt_data.status = WIFI_MEETING_EVENT_PC_CMD_ALL;
//
//	/*
//	 * 判断当前议题的属性，符合就下发，不是则返回错误码
//	 */
//	if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_ELE)
//	{
//
//		data_info.name_type[0] = WIFI_MEETING_CON_ELEC;
//		data_info.code_type[0] = WIFI_MEETING_STRING;
//
//		data_info.msg_type = WRITE_MSG;
//		data_info.data_type = CONFERENCE_DATA;
//		data_info.dev_type = HOST_CTRL;
//
//		if(conf_status_get_pc_staus()>0)
//		{
//			data_info.evt_data.status = WIFI_MEETING_CONF_ELECTION_RESULT;
//			ret = conf_status_get_pc_conf_result(msg);
//			data_info.data_len = ret;
//			tcp_ctrl_module_edit_info(&data_info,msg);
//
//		}else{
//
//			//将全局变量中的数据保存到
//			for(i=1;i<=conf_status_get_elec_totalp(0);i++)
//			{
//				data_info.con_data.elec_rsult.ele_id[i] = conf_status_get_elec_result(0,i);
//			}
//			tcp_ctrl_module_edit_info(&data_info,NULL);
//
//		}
//
//	}else{
//
//		printf("%s-%s-%d not election subject\n",__FILE__,__func__,__LINE__);
//		return ERROR;
//	}

	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
	if(conf_status_get_pc_staus()>0)
	{
//		if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_ELE)
//		{
			ret = conf_status_get_pc_conf_result(msg);
			data_info.data_len = ret;

			tcp_ctrl_module_edit_info(&data_info,msg);

//		}else{
//
//			printf("%s-%s-%d not elec subject\n",__FILE__,__func__,__LINE__);
//			return ERROR;
//		}
	}else{

		ret = conf_status_get_pc_conf_result(msg);
		data_info.data_len = ret;

		tcp_ctrl_module_edit_info(&data_info,msg);

	}


	return SUCCESS;
}

/*
 * conf_status_set_score_result
 * 评分结果的统计
 */
int conf_status_set_score_result(unsigned char value)
{
	unsigned char sub_num = conf_status_get_current_subject();

//	int sub_prop = 0;
	if(conf_status_get_pc_staus() > 0)
	{
		//判断当前议题的属性
//		sub_prop = conf_status_get_subject_property(sub_num);
//		if(sub_prop == WIFI_MEETING_CON_SUBJ_SCORE)
//		{
			node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score += value;
			node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop++;
//		}else{
//			printf("%s-%s-%d the subject is not score subject\n",__FILE__,__func__,__LINE__);
//			return ERROR;
//		}

	}else{
		node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score += value;
		node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop++;

	}

	return SUCCESS;
}

/*
 * conf_status_reset_score_status
 * 评分结果复位
 */
int conf_status_reset_score_status()
{
	unsigned char sub_num = conf_status_get_current_subject();

//	int sub_prop = 0;
	if(conf_status_get_pc_staus() > 0)
	{
		//判断当前议题的属性
//		sub_prop = conf_status_get_subject_property(sub_num);
//		if(sub_prop == WIFI_MEETING_CON_SUBJ_SCORE)
//		{
			node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score = 0;
			node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop = 0;
//		}else{
//			printf("%s-%s-%d the subject is not score subject\n",__FILE__,__func__,__LINE__);
//			return ERROR;
//		}
	}else{
		node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score = 0;
		node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop = 0;

	}

	return SUCCESS;
}

/*
 * conf_status_calc_score_result
 * 计算积分结果
 */
int conf_status_calc_score_result()
{
	float value = 0.0;
	char msg[6] = {0};

	msg[0] = WIFI_MEETING_CON_SCORE;
	msg[1] = WIFI_MEETING_STRING;
	int sub_num = conf_status_get_current_subject();

	value = (double)node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score /
					(double)node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop;
	node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score_r = value;

	sprintf(&msg[2],"%.1f",value);

	conf_status_set_pc_conf_result(sizeof(msg),(unsigned char*)msg);

	return SUCCESS;
}

/*
 * conf_status_get_score_result
 * 获取计分结果
 *
 * @value投票结果
 *
 */
int conf_status_get_score_result(Pscore_result result)
{
	int sub_num = conf_status_get_current_subject();

	result->num_peop = conf_status_get_score_totalp();
	result->score = node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score;
	result->score_r = node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.score_r;

	return SUCCESS;
}

/*
 * conf_status_set_score_totalp
 * 进行计分总人数
 *
 * @value投票结果
 *
 */
int conf_status_set_score_totalp()
{

	int sub_num = conf_status_get_current_subject();

	return node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop;

}

/*
 * conf_status_get_score_totalp
 * 进行计分总人数
 *
 * @value投票结果
 *
 */
int conf_status_get_score_totalp()
{
	int sub_num = conf_status_get_current_subject();
	return node_queue->con_status->ccontent.scontent[sub_num].sresult.scr_result.num_peop;
}

/*
 * conf_status_send_hscore_result
 *
 */
int conf_status_send_hscore_result()
{
	unsigned char msg[64] = {0};
	int ret = 0;
	int pos = 0;
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	if(conf_status_get_pc_staus() > 0)
	{
		;
	}else{
		data_info.name_type[0] = WIFI_MEETING_CON_SCORE;
		data_info.code_type[0] = WIFI_MEETING_STRING;

		data_info.msg_type = WRITE_MSG;
		data_info.data_type = CONFERENCE_DATA;
		data_info.dev_type = HOST_CTRL;

		data_info.evt_data.status = WIFI_MEETING_EVENT_PC_CMD_SIGNAL;

		pos = conf_status_find_chairman_sockfd(&data_info);
		if(pos > 0)
		{
			ret = conf_status_get_pc_conf_result(msg);
			data_info.data_len = ret;

			tcp_ctrl_source_dest_setting(-1,data_info.fd,&data_info);

			tcp_ctrl_module_edit_info(&data_info,msg);
		}
	}


	return SUCCESS;
}

/*
 * conf_status_send_score_result
 * 下发评分结果给所有单元
 */
int conf_status_send_score_result()
{
	unsigned char msg[64] = {0};
	int ret = 0;
	frame_type data_info;
	memset(&data_info,0,sizeof(frame_type));

	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	data_info.name_type[0] = WIFI_MEETING_CON_SCORE;
	data_info.code_type[0] = WIFI_MEETING_STRING;

	data_info.msg_type = WRITE_MSG;
	data_info.data_type = CONFERENCE_DATA;
	data_info.dev_type = HOST_CTRL;

	data_info.evt_data.status = WIFI_MEETING_CONF_SCORE_RESULT;
	/*
	 * 判断当前议题的属性，符合就下发，不是则返回错误码
	 */
	if(conf_status_get_pc_staus()>0)
	{
//		if(conf_status_get_subject_property(0) == WIFI_MEETING_CON_SUBJ_SCORE)
//		{

			ret = conf_status_get_pc_conf_result(msg);
			data_info.data_len = ret;

			tcp_ctrl_module_edit_info(&data_info,msg);

//		}else{
//
//			printf("%s-%s-%d not score subject\n",__FILE__,__func__,__LINE__);
//			return ERROR;
//		}
	}else{

		ret = conf_status_get_pc_conf_result(msg);
		data_info.data_len = ret;

		tcp_ctrl_module_edit_info(&data_info,msg);

	}


	return SUCCESS;
}



/*********************************
 *TODO 发言管理相关接口
 *********************************/
/*
 * conf_status_set_cmspk
 * 会议中主席的发言状态
 *
 * @value
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_set_cmspk(int value)
{

	node_queue->con_status->chirman_t = value;

	return SUCCESS;
}

/*
 * conf_status_get_cmspk
 * 会议中主席的发言状态
 */
int conf_status_get_cmspk()
{
	return node_queue->con_status->chirman_t;
}

/*
 * conf_status_set_cspk_num
 * 会议当前发言人数设置
 */
int conf_status_set_cspk_num(int value)
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->current_spk = value;

	return SUCCESS;
}

/*
 * conf_status_get_cspk_num
 * 会议当前发言人数获取
 */
int conf_status_get_cspk_num()
{
	return node_queue->con_status->current_spk;
}

/*
 * conf_status_set_snd_brd
 * 会议当前音频下发状态
 *
 */
int conf_status_set_snd_brd(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	if(value == WIFI_MEETING_EVENT_SPK_REQ_SND)
	{
		node_queue->con_status->snd_brdcast++;

	}else if(value == WIFI_MEETING_EVENT_SPK_CLOSE_SND)
	{
		node_queue->con_status->snd_brdcast--;
	}

	return SUCCESS;
}

/*
 * conf_status_get_snd_brd
 * 音频下发状态获取
 */
int conf_status_get_snd_brd()
{
	return node_queue->con_status->snd_brdcast;
}


/*********************************
 *TODO 主机相关功能接口
 *********************************/
/*
 * conf_status_set_mic_mode
 * 设置会议话筒模式
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */
int conf_status_set_mic_mode(int value)
{

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	value = value+WIFI_MEETING_EVT_MIC_CHAIRMAN;

	node_queue->con_status->mic_mode = value;

	return SUCCESS;
}


/*
 * conf_status_get_mic_mode
 * 获取话筒模式
 *
 * @value FIFO模式(1)、标准模式(2)、自由模式(3)
 *
 */
int conf_status_get_mic_mode()
{
	return node_queue->con_status->mic_mode;
}


/*
 * conf_status_set_snd_effect
 * DSP音效设置
 *
 * 用bit[0-3]对应的位表示ANC和AFC的状态
 * bit   0   1    2    3
 * ANC      ANC1 ANC2 ANC3
 * AFC  AFC
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_set_snd_effect(int value)
{
	printf("%s-%s-%d,value=0x%02x\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->snd_effect = value;

	uart_snd_effect_set(value);

	return SUCCESS;
}


/*
 * conf_info_get_snd_effect
 * DSP音效获取
 *
 * 返回值：
 * @ERROR
 * @SUCCESS
 */
int conf_status_get_snd_effect()
{
	printf("%s-%s-%d,value=0x%02x\n",__FILE__,__func__,__LINE__,
			node_queue->con_status->snd_effect);

	return node_queue->con_status->snd_effect;

}


/*
 * conf_status_set_spk_num
 * 会议最大发言人数设置
 *
 * @value投票结果
 *
 */
int conf_status_set_spk_num(int value)
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	node_queue->con_status->spk_number = value;

	return SUCCESS;
}


/*
 * conf_status_get_spk_num
 * 会议最大发言人数获取
 *
 */
int conf_status_get_spk_num()
{
	return node_queue->con_status->spk_number;
}




/*********************************
 *TODO 音频处理相关接口函数
 *********************************/
/*
 * conf_status_get_spk_offset
 * 获取发言人数中的主席状态
 * 音频处理中需要判定的发言偏移量，用于队列数据的处理
 *
 * 如果有主席发言，则最大发言人数的偏移量就是设置的值
 * 如果主席没有发言，则偏移量需要增加一人
 *
 * 返回值
 * 偏移值
 */
int conf_status_get_spk_offset()
{
	int value = 0;

	if(conf_status_get_cmspk() == WIFI_MEETING_CON_SE_CHAIRMAN)
	{
		value = node_queue->con_status->spk_number;
	}else{
		value = node_queue->con_status->spk_number + WIFI_MEETING_CON_SE_CHAIRMAN;
	}

	return value;
}


/*
 * conf_status_set_spk_buf_offset
 * 音频接收缓存池的偏移量设置
 */
int conf_status_set_spk_buf_offset(int num,int value)
{
	spk_offset[num] = value;

	return SUCCESS;
}

/*
 * conf_status_get_spk_buf_offset
 * 音频接收缓存池的偏移量获取
 */
inline int conf_status_get_spk_buf_offset(int num)
{
	return spk_offset[num];
}

/*
 * conf_status_set_spk_timestamp
 * 音频接收数据时间戳设置
 */
int conf_status_set_spk_timestamp(int num,int value)
{
	spk_ts[num] = value;
	return SUCCESS;
}

/*
 * conf_status_get_spk_timestamp
 * 音频接收数据时间戳获取
 */
inline int conf_status_get_spk_timestamp(int num)
{
	return spk_ts[num];
}


/*********************************
 *TODO 摄像跟踪相关接口
 *********************************/
/*
 * conf_status_camera_track_postion
 * 摄像跟踪预置点的设置
 */
int conf_status_camera_track_postion(int id,int value)
{
	if(conf_status_get_camera_track())
	{
		sys_uart_video_set(id,value);
	}

	return SUCCESS;
}

/*
 * conf_status_set_camera_track
 * 设置摄像跟踪开关设置
 */
int conf_status_set_camera_track(int value)
{
	node_queue->con_status->camera_track = value;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);

	return SUCCESS;
}

/*
 * conf_status_get_camera_track
 * 摄像跟踪开关获取
 */
int conf_status_get_camera_track()
{
	return node_queue->con_status->camera_track;
}


/*********************************
 *TODO 系统属性相关接口
 *********************************/
/*
 * conf_status_set_sys_time
 * 设置系统时间
 */
int conf_status_set_sys_time(Pframe_type type,const unsigned char* msg)
{
	node_queue->con_status->sys_time[0] = msg[0];
	node_queue->con_status->sys_time[1] = msg[1];
	node_queue->con_status->sys_time[2] = msg[3];
	node_queue->con_status->sys_time[3] = msg[5];

	node_queue->con_status->sys_stime = node_queue->con_status->sys_time[2]*3600 +
			node_queue->con_status->sys_time[3]*60;

	tcp_ctrl_report_enqueue(type,WIFI_MEETING_EVENT_UNIT_SYS_TIME);

	return SUCCESS;
}

/*
 * conf_status_set_sys_time
 * 获取系统时间
 */
int conf_status_get_sys_time(unsigned char* msg)
{
	int ret = 0;

	node_queue->con_status->sys_time[0] = WIFI_MEETING_EVT_SYS_TIME;
	node_queue->con_status->sys_time[1] = WIFI_MEETING_SHORT;

	ret = conf_status_get_sys_timestamp();

	node_queue->con_status->sys_time[2] = ret/3600;
	node_queue->con_status->sys_time[3] = (ret%3600)/60;

	memcpy(msg,node_queue->con_status->sys_time,4);

	return SUCCESS;
}

/*
 * conf_status_set_sys_timestamp
 * 系统时间计数
 */
int conf_status_set_sys_timestamp(unsigned int value)
{

	node_queue->con_status->sys_stime += value;

	//24小时制，24时置0
	if(node_queue->con_status->sys_stime == 86400)
	{
		node_queue->con_status->sys_stime = 0;
	}

	return SUCCESS;
}

/*
 * conf_status_set_sys_timestamp
 * 获取系统时间计数
 */
int conf_status_get_sys_timestamp()
{
	return node_queue->con_status->sys_stime;
}

/*
 * conf_status_set_pc_staus
 * 设置会议中上位机的连接状态
 */
int conf_status_set_pc_staus(int value)
{
	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			value);
	node_queue->con_status->pc_status = value;

	return SUCCESS;
}

/*
 * conf_status_get_pc_staus
 * 获取会议进程中上位机的连接状态
 */
int conf_status_get_pc_staus()
{
	return node_queue->con_status->pc_status;
}

