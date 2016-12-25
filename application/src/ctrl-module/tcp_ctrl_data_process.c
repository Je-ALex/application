/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */


#include "tcp_ctrl_device_status.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_device_manage.h"

/*
 * 定义两个结构体
 * 存取前状态，存储当前状态
 *
 * 创建查询线程，轮训比对，有差异，就返回新状态
 */

extern Pglobal_info node_queue;

/*
 * tcp_ctrl_source_dest_setting
 * tcp控制模块数据源地址和目标地址设置
 *
 * in：
 * @s_fd 源套接字号
 * @d_fd 目标套接字号
 * out：
 * @Pframe_type
 *
 * return：
 * @error
 * @success
 */
int tcp_ctrl_source_dest_setting(int s_fd,int d_fd,Pframe_type type)
{
	pclient_node tmp = NULL;
	Pclient_info cinfo;
	Pconference_list info;

	int i,j;
	i = j = 0;

	type->d_id = type->s_id = 0;

	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		info = tmp->data;
		if(info->fd == d_fd)
		{
			i++;
			type->d_id = info->con_data.id;
		}
		if(info->fd == s_fd)
		{
			j++;
			type->s_id = info->con_data.id;
		}
		if(i>0 && j>0)
			break;
		tmp = tmp->next;
	}

	/*
	 * 查找上位机
	 * 如果有，将地址信息赋值
	 */
	tmp = node_queue->sys_list[CONNECT_LIST]->next;
	while(tmp!=NULL)
	{
		cinfo = tmp->data;
		if(cinfo->client_fd == d_fd
				&& cinfo->client_name == PC_CTRL)
		{
			i++;
			type->d_id = PC_ID;
		}
		if(cinfo->client_fd == s_fd
				&& cinfo->client_name == PC_CTRL)
		{
			j++;
			type->s_id = PC_ID;
		}
		if(i>0 || j>0)
			break;
		tmp = tmp->next;
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_data_char_to_int
 * tcp控制模块数据源地址和目标地址设置
 *
 * in：
 * @s_fd 源套接字号
 * @d_fd 目标套接字号
 * out：
 * @Pframe_type
 *
 * return：
 * @error
 * @success
 */
int tcp_ctrl_data_char2short(unsigned short* value,unsigned char* buf)
{

	unsigned short tmp = 0;
	tmp = (buf[0] & 0xff) << 8 ;
	tmp = tmp + (buf[1] & 0xff);

	*value = tmp;

	return SUCCESS;
}


/*
 * tcp_ctrl_frame_analysis
 * 数据包解析函数，将有效数据提出到handlbuf中
 */
int tcp_ctrl_frame_analysis(int* fd,unsigned char* buf,int* len,Pframe_type frame_type,
		unsigned char** ret_msg)
{

	int i;
	int tc_index = 0;
	unsigned short package_len = 0;
	unsigned char sum = 0;
	int length = *len;
	unsigned char* buffer = NULL;
#if TCP_DBG
	printf("length: %d\n",length);
	/*
	 * print the receive data
	 */
	printf("receive from %d：",*fd);
	for(i=0;i<length;i++)
	{
		printf("%x ",buf[i]);
	}
	printf("\n");
#endif

	printf("receive from %d：",*fd);
	for(i=0;i<length;i++)
	{
		printf("%x ",buf[i]);
	}
	printf("\n");
	/*
	 * 验证数据头是否正确buf[0]~buf[3]
	 */
	while (buf[tc_index++] != 'D' || buf[tc_index++] != 'S'
			|| buf[tc_index++] != 'D' || buf[tc_index++] != 'S') {

		printf( "%s not legal headers\n", __func__);
		return BUF_HEAD_ERR;
	}
	/*
	 * buf[4]
	 * 判断数据类型
	 * 消息类型(0xf0)4.5-4.7：控制消息(0x01)，请求消息(0x02)，应答消息(0x03)
	 * 数据类型(0x0c)4.2-4.4：事件型数据(0x01)，会议型单元参数(0x02)，会议型会议参数(0x03)
	 * 设备类型数据(0x03)4.0-4.1：上位机发送(0x01)，主机发送(0x02)，单元机发送(0x03)
	 */
	frame_type->msg_type = (buf[tc_index] & MSG_TYPE) >> 4;
	frame_type->data_type = (buf[tc_index] & DATA_TYPE) >> 2;
	frame_type->dev_type = buf[tc_index] & MACHINE_TYPE;
	tc_index++;

	/*
	 * 计算帧总长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[tc_index++] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + (buf[tc_index++] & 0xff);


	/* fixme
	 * 长度最小不小于16(帧信息字节)+1(data字节，最小为一个名称码) = 13
	 */
	if(length < 0x10 || (length != package_len))
	{
		printf( "%s not legal length\n", __FUNCTION__);

		return BUF_LEN_ERR;
	}
	unsigned char id_msg[2] = {0};

	memcpy(id_msg,&buf[tc_index],sizeof(short));
	tcp_ctrl_data_char2short(&frame_type->s_id,id_msg);
	tc_index = tc_index+sizeof(short);

	memcpy(id_msg,&buf[tc_index],sizeof(short));
	tcp_ctrl_data_char2short(&frame_type->d_id,id_msg);
	tc_index = tc_index+sizeof(short);

	/*
	 * fixme 保留四个字节
	 */
	tc_index = tc_index+sizeof(int);

	/*
	 * 校验和的验证
	 */
	for(i = 0;i<package_len-1 ;i++)
	{
		sum += buf[i];
	}
	if(sum != buf[package_len-1])
	{
		printf( "%s not legal checksum\n", __FUNCTION__);

		return BUF_CHECKS_ERR;
	}

	frame_type->fd = *fd;

	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节源地址，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 15 - 1
	 */
	int data_len = package_len - 0x10;
	frame_type->data_len = data_len;

	/*
	 * 保存有效数据
	 * 将有数据另存为一个内存区域
	 */
	buffer = (unsigned char*)calloc(data_len,sizeof(char));

	memcpy(buffer,buf+tc_index,data_len);
#if TCP_DBG
	printf("buffer: ");
	for(i=0;i<data_len;i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");
#endif
	*ret_msg = buffer;

	return SUCCESS;
}


/*
 * tcp_ctrl_msg_send_to
 * 处理后的消息进行何种处理，是发送给上位机还是在主机进行显示
 * 通过判断是否有上位机进行分别处理，有上位机的情况，主机就不进行显示提示
 *
 */
int tcp_ctrl_msg_send_to(Pframe_type type,const unsigned char* msg,int value)
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	int status = 0;

	int tmp_fd = type->fd;

	//本地状态上报qt
	tcp_ctrl_report_enqueue(type,value);

	/*
	 * 查询是否有上位机连接，若有，将信息上报给上位机
	 * 单元机上线后，需要上报上位机，上报内容为fd和ip地址
	 * msg重新组包
	 */
	if(type->dev_type == UNIT_CTRL && conf_status_get_pc_staus() > 0
			&& msg != NULL)
	{
		/*
		 * 单元机上线和下线再有上位机的情况下
		 * 需要上报fd、id、ip、席别、电量
		 */
		if(type->msg_type == ONLINE_REQ ||
				type->msg_type == OFFLINE_REQ)
		{
			getpeername(type->fd,(struct sockaddr*)&cli_addr,
					(socklen_t*)&clilen);

			type->name_type[0] = WIFI_MEETING_EVT_UNIT_ONOFF_LINE;
			type->code_type[0] = WIFI_MEETING_STRING;

			type->evt_data.unet_info.sockfd = type->fd;
			type->evt_data.unet_info.ip = cli_addr.sin_addr.s_addr;

			status++;
		}

		tmp=node_queue->sys_list[CONNECT_LIST]->next;;
		while(tmp != NULL)
		{
			pinfo = tmp->data;

			if((pinfo->client_name == PC_CTRL) &&
					(node_queue->con_status->pc_status == pinfo->client_fd))
			{
				type->evt_data.status = WIFI_MEETING_HOST_REP_TO_PC;
				//上报上位机的源地址统一为sockfd
				type->d_id = PC_ID;
				type->fd = pinfo->client_fd;
				if(status > 0)
				{
					tcp_ctrl_module_edit_info(type,NULL);
				}else{
					type->s_id = tmp_fd;
					tcp_ctrl_module_edit_info(type,msg);
				}
				break;
			}
			tmp = tmp->next;
		}
	}


	return SUCCESS;
}





/*
 * tcp_ctrl_uevent_request_pwr
 * 电源管理，主席单元下发一键关闭单元机功能
 *
 * 主机将请求消息发送给主机， 主机下发关机指令
 *
 * 主机界面提示关闭电源消息
 *
 */
int tcp_ctrl_uevent_request_pwr(Pframe_type frame_type,const unsigned char* msg)
{
	int tmp_fd = 0;
	char tmp_msgt = 0;

	int pos = 0;
	int value = 0;


	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	switch(frame_type->evt_data.value)
	{

	case WIFI_MEETING_EVT_PWR_OFF_ALL:
		/*
		 * 检查是否是主席单元下发的指令
		 */
		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0)
		{

			value = WIFI_MEETING_EVENT_POWER_OFF_ALL;
			//变换为控制类消息下个给单元机
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = value;
			tcp_ctrl_module_edit_info(frame_type,msg);

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

		break;
	default:
		printf("there is not legal value\n");
		return ERROR;

	}
	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_spk_port
 * 音频端口管理
 * 主席单元固定为第一号端口，列席单元为后续的端口号
 *
 * @Pframe_type
 * @value
 *
 */
int tcp_ctrl_uevent_spk_port(Pframe_type frame_type)
{
	int tmp = 0;
	int tmp_num,tmp_port = 0;

	int tmp_fd = frame_type->fd;

	frame_type->msg_type = WRITE_MSG;
	frame_type->dev_type = HOST_CTRL;
	frame_type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;
	frame_type->code_type[0] = WIFI_MEETING_CHAR;

	switch(frame_type->evt_data.status)
	{
	case WIFI_MEETING_EVENT_SPK_ALLOW:
	case WIFI_MEETING_EVENT_SPK_REQ_SPK:
	{
		if(frame_type->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
		{
			tmp_port = AUDIO_RECV_PORT;
			tmp = conf_status_get_cspk_num();
			tmp++;
			conf_status_set_cspk_num(tmp);
			conf_status_set_cmspk(WIFI_MEETING_CON_SE_CHARIMAN);

		}else if(frame_type->con_data.seat == WIFI_MEETING_CON_SE_GUEST
				|| frame_type->con_data.seat == WIFI_MEETING_CON_SE_ATTEND)
		{

			/*
			 * 判断当前发言人数是否饱和，如果饱和，则需要进行处理
			 * 已达到上限，则需要关闭第一个单元
			 * 轮询发言链表中的设备，比较时间戳， 时间戳最小的，表示最早加入设备，需要关闭
			 */
			if(conf_status_get_cspk_num() == conf_status_get_spk_num())
			{
				/*
				 * 查询会议中排位，关闭第一位单元
				 * 将端口下发给新申请的单元
				 *
				 */
				conf_status_search_spk_node(frame_type);
				conf_status_delete_spk_node(frame_type->fd);

				frame_type->name_type[0] = WIFI_MEETING_EVT_SPK;
				frame_type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;

				tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
				tcp_ctrl_module_edit_info(frame_type,NULL);

				/*
				 * 更新发言链表，下发端口信息
				 */
				tmp_port = frame_type->spk_port;
				frame_type->fd = tmp_fd;
				frame_type->name_type[0] = WIFI_MEETING_EVT_AD_PORT;

//				conf_status_refresh_spk_node(frame_type);
//				tcp_ctrl_module_edit_info(frame_type,NULL);

			}else{

				//列席单元是从9002端口开始
				tmp_port = AUDIO_RECV_PORT+2;
				tmp_num = conf_status_get_cspk_num();

				if(conf_status_get_cmspk() == WIFI_MEETING_CON_SE_CHARIMAN)
				{
					tmp_num--;
				}

				switch(tmp_num)
				{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					tmp_port = tmp_port+tmp_num*2;
					//当前发言人数需要告知全局变量
					if((tmp=conf_status_get_cspk_num()) < conf_status_get_spk_num())
					{
						tmp++;
						conf_status_set_cspk_num(tmp);
					}
					break;
				default:
					printf("%s-%s-%d error\n",__FILE__,__func__,__LINE__);
					return ERROR;
				}
			}
		}
		/*
		 * 更新发言管理链表
		 */
		frame_type->spk_port = tmp_port;

		conf_status_refresh_spk_node(frame_type);

		tcp_ctrl_module_edit_info(frame_type,NULL);
		break;
	}

	case WIFI_MEETING_EVENT_SPK_ALOW_SND:
	{
		frame_type->brd_port = AUDIO_SEND_PORT;
		tcp_ctrl_module_edit_info(frame_type,NULL);
		break;
	}
	case WIFI_MEETING_EVENT_SPK_REQ_CLOSE:
		tmp=conf_status_get_cspk_num();
		if(tmp)
			tmp--;
		conf_status_set_cspk_num(tmp);
		if(frame_type->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
		{
			conf_status_set_cmspk(WIFI_MEETING_CON_SE_GUEST);
		}
		conf_status_refresh_spk_node(frame_type);
		break;
	}


	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_spk_manage
 * 发言管理
 * 1、处理单元机的请求情况，判断当前发言人数是否达到上限，达到则告知主席，否则直接下发端口号
 * 2、主席反馈的结果后，处理允许消息，并下发音频端口号
 *
 * @Pframe_type
 * @value
 *
 */
int tcp_ctrl_uevent_spk_manage(Pframe_type frame_type,const unsigned char* msg)
{
	int pos = 0;
	int tmp_fd = frame_type->fd;

	/*
	 * 判断当前请求单元机的席别属性
	 * 主席就直接分配固定端口号
	 * 列席就需要判断当前发言人数和设置的最大发言人数
	 * 旁听则不允许申请发言
	 */
	switch(frame_type->evt_data.status)
	{
	case WIFI_MEETING_EVENT_SPK_REQ_SPK:
	{
		/*
		 * 判断申请发言单元的席别，如果是主席，则直接进行处理
		 * 如果是单元，则需要判断发言人数限制和话筒模式
		 */
		if(frame_type->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN){
			tcp_ctrl_uevent_spk_port(frame_type);
		}else{
			/*
			 * 判断发言人数，如果发言人数为1，则表示主席优先模式
			 * 发言人数为1 ，则直接拒绝单元申请
			 * 发言人数大于1，则需要判断话筒模式和发言人数
			 * FIFO模式，则直接关闭最先发言单元，新单元加入
			 * 其他模式，先判断人，饱和则需要通知主席
			 */
			if(conf_status_get_spk_num() == WIFI_MEETING_CON_SE_CHARIMAN)
			{
				//告知单元，拒绝其发言
				printf("%s-%s-%d only chairman mode\n",__FILE__,__func__,__LINE__);

				frame_type->msg_type = WRITE_MSG;
				frame_type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
				tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);

				tcp_ctrl_module_edit_info(frame_type,NULL);

			}else{
				/*
				 * 判断发言话筒模式，如果是FIFO模式，则直接对进行处理，不需要转发主席
				 */
				if(conf_status_get_mic_mode() == WIFI_MEETING_EVT_MIC_FIFO)
				{
					tcp_ctrl_uevent_spk_port(frame_type);
				}else{
					/*
					 * 判断发言人数是否饱和，如果达到上限，则需要请求主席授权
					 */
					if(conf_status_get_cspk_num() < conf_status_get_spk_num())
					{
						tcp_ctrl_uevent_spk_port(frame_type);
					}else{
						pos = conf_status_find_chariman_sockfd(frame_type);
						if(pos > 0){
							//源地址为请求单元机id，目标地址修改为主机单元id
							tcp_ctrl_source_dest_setting(tmp_fd,frame_type->fd,frame_type);
							tcp_ctrl_module_edit_info(frame_type,msg);
						}else{
							//没有主席，告知单元，拒绝其发言
							printf("%s-%s-%d not find chairman\n",__FILE__,__func__,__LINE__);

							frame_type->msg_type = WRITE_MSG;
							frame_type->evt_data.value = WIFI_MEETING_EVT_SPK_VETO;
							tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);

							tcp_ctrl_module_edit_info(frame_type,NULL);

						}
					}
				}
			}


		}
		break;
	}
	case WIFI_MEETING_EVENT_SPK_ALLOW:
	case WIFI_MEETING_EVENT_SPK_VETO:
	{
		pos = conf_status_check_chariman_legal(frame_type);

		if(pos > 0){
			pos = 0;
			pos = conf_status_find_did_sockfd(frame_type);
			if(pos > 0)
			{
				printf("reply request\n");
				//变换为控制类消息下个给单元机
				frame_type->msg_type = WRITE_MSG;
				tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
				tcp_ctrl_module_edit_info(frame_type,msg);
				//设置单元音频端口信息
				if(frame_type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALLOW)
					tcp_ctrl_uevent_spk_port(frame_type);
			}else{
				printf("%s-%s-%d not the find the unit device\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

		break;
	}
	case WIFI_MEETING_EVENT_SPK_ALOW_SND:
	case WIFI_MEETING_EVENT_SPK_VETO_SND:
	{
		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0){
			pos = 0;
			pos = conf_status_find_did_sockfd(frame_type);
			if(pos > 0)
			{
				printf("reply sound request\n");
				//变换为控制类消息下个给单元机
				frame_type->msg_type = WRITE_MSG;
				tcp_ctrl_source_dest_setting(-1,frame_type->fd,frame_type);
				tcp_ctrl_module_edit_info(frame_type,msg);
				//设置单元音频端口信息
				if(frame_type->evt_data.status == WIFI_MEETING_EVENT_SPK_ALOW_SND)
					tcp_ctrl_uevent_spk_port(frame_type);
			}else{
				printf("%s-%s-%d not the find the unit device\n",__FILE__,__func__,__LINE__);
				return ERROR;
			}

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
		break;
	}
	case WIFI_MEETING_EVENT_SPK_REQ_CLOSE:
	{
		tcp_ctrl_uevent_spk_port(frame_type);
		break;
	}
	}


	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_spk
 * 发言管理函数
 *
 * 主机将请求消息发送给主席单元，主席单元显示几号单元机请求发言
 *
 * 主机界面显示请求发言，如果主席单元处理请求后，显示关闭
 *
 */
int tcp_ctrl_uevent_request_spk(Pframe_type frame_type,const unsigned char* msg)
{

	int value = 0;
	char tmp_msgt = 0;
	int tmp_fd = 0;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	//保存初始源地址信息
	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	switch(frame_type->evt_data.value)
	{
	/*
	 * 主机收到主席单元的允许指令，目标地址为具体允许的单元机ID，源地址为主席单元
	 * 主机下发允许指令，修改源地址为主机，目标地址不变
	 */
	case WIFI_MEETING_EVT_SPK_ALLOW:
		value = WIFI_MEETING_EVENT_SPK_ALLOW;
		break;
	case WIFI_MEETING_EVT_SPK_VETO:
		value = WIFI_MEETING_EVENT_SPK_VETO;
		break;
	case WIFI_MEETING_EVT_SPK_ALOW_SND:
		value = WIFI_MEETING_EVENT_SPK_ALOW_SND;
		break;
	case WIFI_MEETING_EVT_SPK_VETO_SND:
		value = WIFI_MEETING_EVENT_SPK_VETO_SND;
		break;
	case WIFI_MEETING_EVT_SPK_REQ_SND:
		value = WIFI_MEETING_EVENT_SPK_REQ_SND;
		break;
	case WIFI_MEETING_EVT_SPK_REQ_SPK:
		value = WIFI_MEETING_EVENT_SPK_REQ_SPK;
		break;
	case WIFI_MEETING_EVT_SPK_REQ_CLOSE:
		value = WIFI_MEETING_EVENT_SPK_REQ_CLOSE;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;
	}
	frame_type->evt_data.status = value;
	tcp_ctrl_uevent_spk_manage(frame_type,msg);
	/*
	 * 将事件信息发送至消息队列
	 * 告知应用
	 */
	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	tcp_ctrl_msg_send_to(frame_type,msg,value);

	return SUCCESS;

}



/*
 * tcp_ctrl_uevent_request_subject
 * 会议议题控制管理
 *
 * 会议中主席单元会选择进行第几个议题，主机接收到具体的议题号，下发给单元机和上报给上位机
 * 主机单元通过全局变量保存当前议题号
 *
 */
int tcp_ctrl_uevent_request_subject(Pframe_type frame_type,const unsigned char* msg)
{
	int value = 0;
	int pos = 0;
	char tmp_msgt = 0;
	int tmp_fd = 0;

	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	/*
	 * 判断消息是否是主席单元发送
	 */
	pos = conf_status_check_chariman_legal(frame_type);
	if(pos > 0)
	{
		frame_type->msg_type = WRITE_MSG;
		value = frame_type->evt_data.value;
		//议题号保存到全局变量
		conf_status_set_current_subject(value);

		frame_type->evt_data.status = WIFI_MEETING_EVENT_SUBJECT_OFFSET;
		tcp_ctrl_module_edit_info(frame_type,msg);

	}else{
		printf("the subject command is not chariman send\n");
		return ERROR;
	}

	/*
	 * 判断议题号
	 */
	value += WIFI_MEETING_EVENT_SUBJECT_OFFSET;

	/*
	 * 请求消息发送给上位机和主机显示
	 */
	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);
	}

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_service
 * 服务请求
 * 主机接收端服务请求，将请求在主机界面显示或上传上位机
 *
 */
int tcp_ctrl_uevent_request_service(Pframe_type frame_type,const unsigned char* msg)
{

	int value = 0;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);


	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_SER_WATER:
		value = WIFI_MEETING_EVENT_SERVICE_WATER;
		break;
	case WIFI_MEETING_EVT_SER_PEN:
		value = WIFI_MEETING_EVENT_SERVICE_PEN;
		break;
	case WIFI_MEETING_EVT_SER_PAPER:
		value = WIFI_MEETING_EVENT_SERVICE_PAPER;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;

	}
	tcp_ctrl_msg_send_to(frame_type,msg,value);

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_checkin
 * 处理单元机签到功能
 * 单元机请求已经签到的信息
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_checkin(Pframe_type frame_type,const unsigned char* msg)
{
//	pclient_node tmp = NULL;
//	Pconference_list con_list;
	int value = 0;
	int pos = 0;

	int tmp_fd = 0;
	char tmp_msgt = 0;


	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	/*
	 * 开始签到和结束签到的指令需要下发给所有单元
	 */
	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_CHECKIN_START:
		value = WIFI_MEETING_EVENT_CHECKIN_START;
		break;
	case WIFI_MEETING_EVT_CHECKIN_END:
		value = WIFI_MEETING_EVENT_CHECKIN_END;
		break;
	case WIFI_MEETING_EVT_CHECKIN_SELECT:
		//保存签到信息
		value = WIFI_MEETING_EVENT_CHECKIN_SELECT;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;
	}

	//开始和结束的时候需要告知列席
	if(value == WIFI_MEETING_EVENT_CHECKIN_START ||
			value == WIFI_MEETING_EVENT_CHECKIN_END)
	{
		/*
		 * 验证指令是否是主席发送
		 */
		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0)
		{
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = value;
			tcp_ctrl_module_edit_info(frame_type,msg);
		}else{
			printf("the checkin command is not chariman send\n");
			return ERROR;
		}

	}


	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}


	return SUCCESS;
}


/*
 * tcp_ctrl_uevent_request_vote
 * 处理单元机投票实时信息
 * 开始投票，结束投票
 * 赞成，反对，弃权，超时
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_vote(Pframe_type frame_type,const unsigned char* msg)
{

	int value = 0;
	int pos = 0;
	int tmp_fd = 0;
	int tmp_msgt = 0;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	/*
	 * 开始投票和结束投票的指令需要下发给所有单元
	 */
	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_VOT_START:
		value = WIFI_MEETING_EVENT_VOTE_START;
		break;
	case WIFI_MEETING_EVT_VOT_END:
		value = WIFI_MEETING_EVENT_VOTE_END;
		break;
	case WIFI_MEETING_EVT_VOT_RESULT:
		value = WIFI_MEETING_EVENT_VOTE_RESULT;
		break;
	case WIFI_MEETING_EVT_VOT_ASSENT:
 		value = WIFI_MEETING_EVENT_VOTE_ASSENT;
		break;
	case WIFI_MEETING_EVT_VOT_NAY:
 		value = WIFI_MEETING_EVENT_VOTE_NAY;
		break;
	case WIFI_MEETING_EVT_VOT_WAIVER:
 		value = WIFI_MEETING_EVENT_VOTE_WAIVER;
		break;
	case WIFI_MEETING_EVT_VOT_TOUT:
		value = WIFI_MEETING_EVENT_VOTE_TIMEOUT;
		break;
	default:
		printf("there is not legal value\n");
		return ERROR;
	}

	//开始和结束的时候需要告知列席
	if(value == WIFI_MEETING_EVENT_VOTE_START || value == WIFI_MEETING_EVENT_VOTE_END
			|| value == WIFI_MEETING_EVENT_VOTE_RESULT)
	{
		/*
		 * 检查是否是主席单元下发的指令
		 */

		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0)
		{
			//变换为控制类消息下个给单元机
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = value;
			tcp_ctrl_module_edit_info(frame_type,msg);
			//收到结束指令后，延时1ms下发投票结果
			if(value == WIFI_MEETING_EVENT_VOTE_END)
			{
				usleep(1000);
				conf_info_send_vote_result();
			}

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}else if(value > WIFI_MEETING_EVENT_VOTE_RESULT){
		//进行投票结果处理
		conf_status_save_vote_result(value);
	}

	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}

	return SUCCESS;
}

/*
 * tcp_ctrl_uevent_request_election
 * 选举管理
 *
 * 单元机请求消息选举
 * 将数据信息上报给上位机
 * 通过选举人编号，
 *
 */
int tcp_ctrl_uevent_request_election(Pframe_type frame_type,const unsigned char* msg)
{


	int tmp_fd = 0;
	int tmp_msgt = 0;

	int pos = 0;
	int value = 0;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	switch(frame_type->evt_data.value)
	{

	case WIFI_MEETING_EVT_ELECTION_START:
		value = WIFI_MEETING_CONF_ELECTION_START;
		break;
	case WIFI_MEETING_EVT_ELECTION_END:
		value = WIFI_MEETING_CONF_ELECTION_END;
		break;
	case WIFI_MEETING_EVT_ELECTION_RESULT:
		value = WIFI_MEETING_CONF_ELECTION_RESULT;
		break;
	default:
		//fixme 选举人编号，设置对应编号值累加
		if(frame_type->evt_data.value < conf_status_get_elec_totalp(NULL))
		{
			value = frame_type->evt_data.value - WIFI_MEETING_EVT_ELECTION_UNDERWAY;
			conf_status_save_elec_result(value);
 			value = WIFI_MEETING_CONF_ELECTION_UNDERWAY;
		}
		break;
	}

	//开始和结束的时候需要告知列席单元
	if(value == WIFI_MEETING_CONF_ELECTION_START || value == WIFI_MEETING_CONF_ELECTION_END
			|| value == WIFI_MEETING_CONF_ELECTION_RESULT)
	{
		/*
		 * 检查是否是主席单元下发的指令
		 */

		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0)
		{
			//变换为控制类消息下个给单元机
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = value;
			tcp_ctrl_module_edit_info(frame_type,msg);
			//收到结束指令后，延时1ms下发选举结果
			if(value == WIFI_MEETING_CONF_ELECTION_END)
			{
				usleep(1000);
				conf_info_send_elec_result();
			}

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}

	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}

	return SUCCESS;
}


/*
 * tcp_ctrl_uevent_request_score
 * 评分管理
 * 单元机请求消息计分
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_score(Pframe_type frame_type,const unsigned char* msg)
{


	int tmp_fd = 0;
	int tmp_msgt = 0;

	int pos = 0;
	int value = 0;

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	switch(frame_type->evt_data.value)
	{

	case WIFI_MEETING_EVT_SCORE_START:
		value = WIFI_MEETING_CONF_SCORE_START;
		break;
	case WIFI_MEETING_EVT_SCORE_END:

		value = WIFI_MEETING_CONF_SCORE_END;
		break;
	default:
		//fixme 计分议题,不能超过编号人数

		if(conf_status_get_score_totalp() < node_queue->sys_list[CONFERENCE_LIST]->size)
		{
			conf_status_save_score_result(frame_type->evt_data.value);
			value = WIFI_MEETING_CONF_SCORE_UNDERWAY;
		}
		break;
	}

	if(value == WIFI_MEETING_CONF_SCORE_START || value == WIFI_MEETING_CONF_SCORE_END
			|| value == WIFI_MEETING_CONF_SCORE_RESULT)
	{
		/*
		 * 检查是否是主席单元下发的指令
		 */

		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0)
		{
			//变换为控制类消息下个给单元机
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = value;
			tcp_ctrl_module_edit_info(frame_type,msg);
			//收到结束指令后，延时1ms下发计分结果
			if(value == WIFI_MEETING_CONF_SCORE_END)
			{
				usleep(1000);
				conf_info_send_score_result();
			}

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

	}

	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}

	return SUCCESS;

}


/*
 * tcp_ctrl_uevent_request_conf_manage
 * 会议管理
 *
 * 开始会议和结束会议
 */
int tcp_ctrl_uevent_request_conf_manage(Pframe_type frame_type,const unsigned char* msg)
{

	int pos = 0;
	int value = 0;

	char tmp_msgt = 0;
	int tmp_fd = 0;


	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);

	//保存初始源地址信息
	tmp_fd = frame_type->fd;
	tmp_msgt = frame_type->msg_type;

	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_CON_MAG_START:
		value = WIFI_MEETING_EVENT_CON_MAG_START;
		break;
	case WIFI_MEETING_EVT_CON_MAG_END:
		value = WIFI_MEETING_EVENT_CON_MAG_END;
		break;

	}

	if(value == WIFI_MEETING_EVENT_CON_MAG_END ||
			value == WIFI_MEETING_EVENT_CON_MAG_START)
	{

		pos = conf_status_check_chariman_legal(frame_type);
		if(pos > 0)
		{
			//变换为控制类消息下个给单元机
			frame_type->msg_type = WRITE_MSG;
			frame_type->evt_data.status = value;
			tcp_ctrl_module_edit_info(frame_type,msg);
			conf_status_set_conf_staus(value);

		}else{
			printf("%s-%s-%d not the chariman command\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}
	}

	frame_type->fd = tmp_fd;
	frame_type->msg_type = tmp_msgt;
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}

	return SUCCESS;
}
/*
 * tcp_ctrl_unit_request_msg
 * 单元机请求消息处理函数
 * 请求消息再细分为事件类和会议类
 *
 * 处理接收到的消息，将消息内容进行解析，存取到全局结构体中@new_uint_data
 *
 * 事件类已增加发言，投票，签到，服务
 *
 * 会议类暂时没有
 * 消息格式
 * char | char | char
 * 	内容	       编码	    数值
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 * 处理请求类消息有两种情况：
 * 1/有上位机的情况，这个时候，系统初始化时单元机上线，不做特殊处理，如果是会议中途加入则需要自动对其编号
 * 2/没有上位机情况，系统初始化时，不做任何处理,会议过程中，则需要自动对新上线设备编号
 *
 */
int tcp_ctrl_unit_request_msg(const unsigned char* msg,Pframe_type type)
{

	int pos = 0;

	type->name_type[0] = msg[0];

	/*
	 * 检查设备的合法性
	 * 赋值id和席别,上报qt
	 * 发言管理功能，是特殊情况，任何时间单元机上线后，均有可能请求音频功能
	 */
//	if(type->name_type[0] != WIFI_MEETING_EVT_SPK)
//	{
//		pos = conf_status_check_client_legal(type);
//	}else{
//		pos = conf_status_check_connect_legal(type);
//	}
	pos = conf_status_check_client_legal(type);
	if(pos > 0){
		/*
		 * 判读是事件型还是会议型
		 */
		switch(type->data_type)
		{

		case EVENT_DATA:
		{
			type->name_type[0] = msg[0];
			type->code_type[0] = msg[1];
			type->evt_data.value = msg[2];

			switch(type->name_type[0])
			{
				/*
				 * 电源管理
				 */
				case WIFI_MEETING_EVT_PWR:
					tcp_ctrl_uevent_request_pwr(type,msg);
					break;
				/*
				 * 发言管理
				 */
				case WIFI_MEETING_EVT_SPK:
					tcp_ctrl_uevent_request_spk(type,msg);
					break;
				/*
				 * 投票管理
				 */
				case WIFI_MEETING_EVT_VOT:
					tcp_ctrl_uevent_request_vote(type,msg);
					break;
				/*
				 * 议题管理
				 */
				case WIFI_MEETING_EVT_SUB:
					tcp_ctrl_uevent_request_subject(type,msg);
					break;
				/*
				 * 服务请求
				 */
				case WIFI_MEETING_EVT_SER:
					tcp_ctrl_uevent_request_service(type,msg);
					break;
				/*
				 * 签到管理
				 */
				case WIFI_MEETING_EVT_CHECKIN:
					tcp_ctrl_uevent_request_checkin(type,msg);
					break;
				/*
				 * 选举管理
				 */
				case WIFI_MEETING_EVT_ELECTION:
					tcp_ctrl_uevent_request_election(type,msg);
					break;
				/*
				 * 评分管理
				 */
				case WIFI_MEETING_EVT_SCORE:
					tcp_ctrl_uevent_request_score(type,msg);
					break;
				/*
				 * 会议管理
				 */
				case WIFI_MEETING_EVT_CON_MAG:
					tcp_ctrl_uevent_request_conf_manage(type,msg);
					break;
				default:
					printf("there is legal value\n");
					return ERROR;

			}
		}
			break;

		case CONFERENCE_DATA:
			break;

		default:
			printf("there is legal value\n");
			return ERROR;
		}

	}
	else{
		printf("%s-%s-%d not the legal client\n",__FILE__,__func__,__LINE__);
		return ERROR;

	}

	return SUCCESS;

}

/*
 * tcp_ctrl_unit_reply_conference
 * 单元机会议类应答消息处理函数
 * 消息分为控制类应答和查询类应答
 * 消息会首先在主机做出处理，如果有上位机，还需通过上位机进行处理
 *
 * 处理接收到的消息，将消息内容进行解析
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 */
int tcp_ctrl_unit_reply_conference(const unsigned char* msg,Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list con_list;

	Pconference_list confer_info;
	int tc_index = 0;

	int ret = 0;
	int str_len = 0;

	int value = 0;

	printf("%s-%s-%d ",__FILE__,__func__,__LINE__);

	int i;
	for(i=0;i<frame_type->data_len;i++)
	{
		printf("0x%02x ",msg[i]);
	}
	printf("\n");

	/*
	 * 赋值id和席别
	 * qt上报
	 */
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		con_list = tmp->data;
		if(con_list->fd == frame_type->fd)
		{
			frame_type->con_data.id = frame_type->s_id;
			frame_type->con_data.seat=con_list->con_data.seat;
			break;
		}
		tmp=tmp->next;
	}

	switch(frame_type->msg_type)
	{
	/*
	 * 控制类应答是设置失败，单元机返回的
	 * 所以，在接收到此消息后，主机要进行显示，还要将信息上报给上位机
	 */
	case W_REPLY_MSG:
	{

		frame_type->name_type[0] = msg[tc_index++];
		/*
		 * 会议类控制应答，单元机只返回失败情况0xe3
		 * 上报消息进入队列
		 */
		if(msg[tc_index++] == WIFI_MEETING_ERROR)
		{
			switch(msg[tc_index++])
			{
			case TCP_C_ERR_SUCCESS:
				value = WIFI_MEETING_CONF_WREP_SUCCESS;
				break;
			case TCP_C_ERR_HEAD:
			case TCP_C_ERR_LENGTH:
			case TCP_C_ERR_CHECKSUM:
				value = WIFI_MEETING_CONF_WREP_ERR;
				break;
			}
		}
		break;
	}
	case R_REPLY_MSG:
	{

		/*
		 * 会议查询类应答，单元机应答全状态
		 * 对于单主机情况，主机只关心ID 和席别
		 * 有上位机情况，将应答信息转发给上位机
		 */
		if(msg[tc_index++] == WIFI_MEETING_CON_ID){
			--tc_index;
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];

			frame_type->con_data.id = (msg[tc_index++] << 8);
			frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);
			value = WIFI_MEETING_CONF_RREP;
		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){
			--tc_index;
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];
			frame_type->con_data.seat = msg[tc_index++];

		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_NAME){
			--tc_index;
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];
			str_len = msg[tc_index++];
			memcpy(frame_type->con_data.name,&msg[tc_index],str_len);

			tc_index = tc_index+str_len;

		 }
		if(msg[tc_index++] == WIFI_MEETING_CON_CNAME){
			--tc_index;
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];
			str_len = msg[tc_index++];
			memcpy(frame_type->con_data.conf_name,&msg[tc_index],str_len);

			tc_index = tc_index+str_len;

		 }

		if(value == WIFI_MEETING_CONF_RREP)
		{
			printf("id=%d,seat=%d,name=%s,cname=%s\n",
					frame_type->con_data.id,frame_type->con_data.seat,
					frame_type->con_data.name,frame_type->con_data.conf_name);
			/*
			 * 更新conference_head链表的内容
			 */
			confer_info = (Pconference_list)malloc(sizeof(conference_list));
			memset(confer_info,0,sizeof(conference_list));
			confer_info->fd = frame_type->fd;
			confer_info->con_data.id =  frame_type->con_data.id;
			confer_info->con_data.seat =  frame_type->con_data.seat;
			if(strlen(frame_type->con_data.name) > 0)
			{
				memcpy(confer_info->con_data.name,frame_type->con_data.name,strlen(frame_type->con_data.name));
				memcpy(confer_info->con_data.conf_name,frame_type->con_data.conf_name,strlen(frame_type->con_data.conf_name));
			}

			/*
			 * 更新到会议信息链表中
			 */
			ret = dmanage_refresh_conference_list(confer_info);
			if(ret)
			{
				free(confer_info);
				return ERROR;
			}
		}
		break;
	}
	}

	/*
	 * 消息进行处理
	 * 如果有PC就直接将数据传递给PC
	 * 单主机的话，就需要上报给应用
	 */
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);
	}

	return SUCCESS;

}



/*
 * tcp_ctrl_unit_reply_event
 * 事件类应答消息处理
 *
 * 事件类应答不用区分应答类型，单元机应答的内容为固定格式
 * 消息内容为name+code+ID
 *
 */
int tcp_ctrl_unit_reply_event(const unsigned char* msg,Pframe_type frame_type)
{
	pclient_node tmp = NULL;
	Pconference_list tmp_type;

	int value = 0;

	frame_type->name_type[0] = msg[0];
	frame_type->code_type[0] = msg[1];
	frame_type->evt_data.value = msg[2];

	printf("%s-%s-%d,value=%d\n",__FILE__,__func__,__LINE__,
			frame_type->evt_data.value);
	/*
	 * 赋值id和席别
	 * 在上报qt时需要
	 */
	frame_type->con_data.id = frame_type->s_id;
	//席别
	tmp = node_queue->sys_list[CONFERENCE_LIST]->next;
	while(tmp!=NULL)
	{
		tmp_type = tmp->data;
		if(tmp_type->con_data.id == frame_type->s_id)
		{
			frame_type->con_data.seat=tmp_type->con_data.seat;
			break;
		}
		tmp=tmp->next;
	}

	switch(frame_type->name_type[0])
	{
	/*
	 * 单元机电源
	 */
	case WIFI_MEETING_EVT_PWR:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_PWR_ON:
			value = WIFI_MEETING_EVENT_POWER_ON;
			break;
		case WIFI_MEETING_EVT_PWR_OFF:
		case WIFI_MEETING_EVT_PWR_OFF_ALL:
			value = WIFI_MEETING_EVENT_POWER_OFF;
			break;
		}
		break;
	}
	/*
	 * 话筒应答
	 */
	case WIFI_MEETING_EVT_MIC:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_MIC_FIFO:
			value = WIFI_MEETING_EVENT_MIC_FIFO;
			break;
		case WIFI_MEETING_EVT_MIC_STAD:
			value = WIFI_MEETING_EVENT_MIC_STAD;
			break;
		case WIFI_MEETING_EVT_MIC_FREE:
			value = WIFI_MEETING_EVENT_MIC_FREE;
			break;
		}

		break;
	}
	/*
	 * 签到应答
	 */
	case WIFI_MEETING_EVT_CHECKIN:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_CHECKIN_START:
			value = WIFI_MEETING_EVENT_CHECKIN_START;
			break;
		case WIFI_MEETING_EVT_CHECKIN_END:
			value = WIFI_MEETING_EVENT_CHECKIN_END;
			break;

		}

		break;
	}
	/*
	 * 发言管理应答
	 */
	case WIFI_MEETING_EVT_SPK:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_SPK_ALLOW:
			value = WIFI_MEETING_EVENT_SPK_ALLOW;
			break;
		case WIFI_MEETING_EVT_SPK_VETO:
			value = WIFI_MEETING_EVENT_SPK_VETO;
			break;
		case WIFI_MEETING_EVT_SPK_ALOW_SND:
			value = WIFI_MEETING_EVENT_SPK_ALOW_SND;
			break;
		case WIFI_MEETING_EVT_SPK_VETO_SND:
			value = WIFI_MEETING_EVENT_SPK_VETO_SND;
			break;
		case WIFI_MEETING_EVT_SPK_REQ_SPK:
			value = WIFI_MEETING_EVENT_SPK_REQ_SPK;
			break;
		}

		break;
	}
	/*
	 * 投票管理应答
	 */
	case WIFI_MEETING_EVT_VOT:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_VOT_START:
			value = WIFI_MEETING_EVENT_VOTE_START;
			break;
		case WIFI_MEETING_EVT_VOT_END:
			value = WIFI_MEETING_EVENT_VOTE_END;
			break;
		case WIFI_MEETING_EVT_VOT_ASSENT:
			value = WIFI_MEETING_EVENT_VOTE_ASSENT;
			break;
		case WIFI_MEETING_EVT_VOT_NAY:
			value = WIFI_MEETING_EVENT_VOTE_NAY;
			break;
		case WIFI_MEETING_EVT_VOT_WAIVER:
			value = WIFI_MEETING_EVENT_VOTE_WAIVER;
			break;
		case WIFI_MEETING_EVT_VOT_TOUT:
			value = WIFI_MEETING_EVENT_VOTE_TIMEOUT;
			break;
		}

		break;
	}
	/*
	 * 议题管理应答
	 */
	case WIFI_MEETING_EVT_SUB:
	{
		value = frame_type->evt_data.value + SUBJECT_OFFSET;

		break;
	}

	/*
	 * ssid和key管理应答
	 */
	case WIFI_MEETING_EVT_SSID:
	case WIFI_MEETING_EVT_KEY:
	{
		value = WIFI_MEETING_EVENT_SSID_KEY;

		break;
	}
	/*
	 * 选举管理应答
	 */
	case WIFI_MEETING_EVT_ELECTION:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_ELECTION_START:
			value = WIFI_MEETING_CONF_ELECTION_START;
			break;
		case WIFI_MEETING_EVT_ELECTION_END:
			value = WIFI_MEETING_CONF_ELECTION_END;
			break;
		}
		break;
	}
	/*
	 * 计分管理应答
	 */
	case WIFI_MEETING_EVT_SCORE:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_SCORE_START:
			value = WIFI_MEETING_CONF_SCORE_START;
			break;
		case WIFI_MEETING_EVT_SCORE_END:
			value = WIFI_MEETING_CONF_SCORE_END;
			break;
		}

		break;
	}
	/*
	 * 会议管理应答
	 */
	case WIFI_MEETING_EVT_CON_MAG:
	{
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_CON_MAG_START:
			value = WIFI_MEETING_EVENT_CON_MAG_START;
			break;
		case WIFI_MEETING_EVT_CON_MAG_END:
			value = WIFI_MEETING_EVENT_CON_MAG_END;
			break;
		}
		break;
	}
	}
	if(value > 0)
	{
		tcp_ctrl_msg_send_to(frame_type,msg,value);

	}
	return SUCCESS;
}


/*
 * tcp_ctrl_from_unit
 * 设备控制模块单元机数据处理函数
 *
 * in：
 * @handlbuf解析后的数据内容
 * @Pframe_type帧信息内容
 *
 * 函数先进行消息类型判别，分为请求类和控制应答、查询应答类
 * 在应答类消息中有细分为事件型和会议型
 *
 */
int tcp_ctrl_from_unit(const unsigned char* handlbuf,Pframe_type type)
{

#if TCP_DBG
	int i;
	printf("handlbuf: ");
	for(i=0;i<type->data_len;i++)
	{
		printf("0x%02x ",handlbuf[i]);
	}
	printf("\n");

#endif

	dmanage_process_communication_heart(handlbuf,type);

	/*
	 * 消息类型分类
	 * 具体细分为控制类应答和查询类应答
	 */
	switch(type->msg_type)
	{
		/*
		 * 请求类消息
		 */
		case REQUEST_MSG:
		{
			tcp_ctrl_unit_request_msg(handlbuf,type);
			break;
		}
		/*
		 * 控制类应答
		 * 查询类应答
		 */
		case W_REPLY_MSG:
		case R_REPLY_MSG:
		{
			if(type->data_type == CONFERENCE_DATA)
			{
				tcp_ctrl_unit_reply_conference(handlbuf,type);
			}
			else if(type->data_type == EVENT_DATA){
				tcp_ctrl_unit_reply_event(handlbuf,type);
			}
			break;
		}
		/*
		 * 上线请求消息
		 */
		case ONLINE_REQ:
			dmanage_add_info(handlbuf,type);
			break;
		/*
		 * 在线心跳
		 */
		case ONLINE_HEART:
			break;
	}

	return SUCCESS;
}


/***************
 *
 * UNIT PART END
 *
 ***************/
