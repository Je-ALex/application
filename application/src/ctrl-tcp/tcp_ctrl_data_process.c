/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */


#include "../../header/tcp_ctrl_data_process.h"
#include "../../header/tcp_ctrl_queue.h"
#include "../../header/tcp_ctrl_device_status.h"
/*
 * 定义两个结构体
 * 存取前状态，存储当前状态
 *
 * 创建查询线程，轮训比对，有差异，就返回新状态
 */
extern Pconference_status con_status;
extern pclient_node list_head;
extern pclient_node conference_head;
extern pthread_mutex_t queue_mutex;

int tcp_ctrl_data_char_to_int(int* value,char* buf)
{

	int tmp = 0;
	tmp = buf[0] << 24;
	tmp = tmp + (buf[1] << 16);
	tmp = tmp + (buf[2] << 8);
	tmp = tmp + (buf[3] << 0);

	*value = tmp;

	return SUCCESS;
}


/*
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

	printf("length: %d\n",length);
	/*
	 * print the receive data
	 */
	printf("receive from：");
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

		printf( "%s not legal headers\n", __FUNCTION__);
		return ERROR;
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
	package_len = package_len + buf[tc_index++] & 0xff;

	printf("package_len: %d\n",package_len);

	/* fixme
	 * 长度最小不小于16(帧信息字节)+1(data字节，最小为一个名称码) = 13
	 */
	if(length < 0x10 || (length != package_len))
	{
		printf( "%s not legal length\n", __FUNCTION__);

		return ERROR;
	}
	unsigned char id_msg[4] = {0};

	memcpy(id_msg,&buf[tc_index],sizeof(int));
	tcp_ctrl_data_char_to_int(&frame_type->s_id,id_msg);
	tc_index = tc_index+sizeof(int);

	memcpy(id_msg,&buf[tc_index],sizeof(int));
	tcp_ctrl_data_char_to_int(&frame_type->d_id,id_msg);
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

		return ERROR;
	}

	frame_type->fd = *fd;

	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节源地址，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 15 - 1
	 */
	int data_len = package_len - 15 - 1;
	frame_type->data_len = data_len;

	/*
	 * 保存有效数据
	 * 将有数据另存为一个内存区域
	 */
	buffer = (unsigned char*)calloc(data_len,sizeof(char));

	memcpy(buffer,buf+tc_index,data_len);

	printf("buffer: ");
	for(i=0;i<sizeof(buffer);i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");

	*ret_msg = buffer;

	return SUCCESS;
}


/*
 * tcp_ctrl_msg_send_to
 * 处理后的消息进行何种处理，是发送给上位机还是在主机进行显示
 * 通过判断是否有上位机进行分别处理，有上位机的情况，主机就不进行显示提示
 *
 */
int tcp_ctrl_msg_send_to(Pframe_type frame_type,const unsigned char* msg,int value)
{

	pclient_node tmp = NULL;
	Pclient_info tmp_type;
	/*
	 * 查询是否有上位机连接，若有，将投票信息上报给上位机
	 */
	if(con_status->pc_status == PC_CTRL)
	{
		tmp=list_head->next;
		while(tmp != NULL)
		{
			tmp_type = tmp->data;

			if(tmp_type->client_name == PC_CTRL)
			{
				frame_type->fd = tmp_type->client_fd;
				tcp_ctrl_module_edit_info(frame_type,msg);

				break;
			}
			tmp = tmp->next;
		}
	}else{
		printf("there is no pc\n");

		/*
		 * 将信息发送至消息队列
		 */
		tcp_ctrl_enter_queue(frame_type,value);

	}



	return SUCCESS;
}
/*
 * tcp_ctrl_uevent_request_spk
 * 发言请求函数
 *
 * 主机将请求消息发送给主席单元，主席单元显示几号单元机请求发言
 *
 * 主机界面显示请求发言，如果主席单元处理请求后，显示关闭
 *
 */
int tcp_ctrl_uevent_request_spk(Pframe_type frame_type,const unsigned char* msg)
{

	pclient_node tmp = NULL;
	Pframe_type tmp_type;

	int pos = 0;
	int tmp_fd = 0;

	tmp_fd = frame_type->fd;

	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_SPK_ALLOW:
		tmp=conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id == frame_type->d_id)
			{
				printf("allow the speaking\n");
				frame_type->fd=tmp_type->fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}
		if(pos>0){
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		/*
		 * 将事件信息发送至消息队列
		 */
		frame_type->fd = tmp_fd;
		tcp_ctrl_enter_queue(frame_type,WIFI_MEETING_EVENT_SPK_ALLOW);
		break;
	case WIFI_MEETING_EVT_SPK_VETO:
		tmp=conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.id == frame_type->d_id)
			{
				printf("vote the speaking\n");
				frame_type->fd=tmp_type->fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}
		if(pos>0){
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		/*
		 * 将事件信息发送至消息队列
		 */
		frame_type->fd = tmp_fd;
		tcp_ctrl_enter_queue(frame_type,WIFI_MEETING_EVENT_SPK_VETO);
		break;
	case WIFI_MEETING_EVT_SPK_ALOW_SND:
	case WIFI_MEETING_EVT_SPK_VETO_SND:
		break;
	case WIFI_MEETING_EVT_SPK_REQ_SPK:
		/*
		 * 请求消息发给主席单元
		 */
		tmp=conference_head->next;
		while(tmp!=NULL)
		{
			tmp_type = tmp->data;
			if(tmp_type->con_data.seat == WIFI_MEETING_CON_SE_CHARIMAN)
			{
				printf("find the chariman\n");
				frame_type->fd=tmp_type->fd;
				pos++;
				break;
			}
			tmp=tmp->next;
		}
		if(pos>0){
			tcp_ctrl_module_edit_info(frame_type,msg);
		}
		/*
		 * 请求消息发送给上位机和主机显示
		 */
		frame_type->fd = tmp_fd;
		tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_EVENT_SPK_REQ_SPK);

		break;
	}


	return SUCCESS;

}


/*
 * tcp_ctrl_uevent_request_vote
 * 处理单元机投票实时信息
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_vote(Pframe_type frame_type,const unsigned char* msg)
{

	int value = 0;

	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_VOT_ASSENT:
		con_status->v_result.assent++;
		value = WIFI_MEETING_CONF_REQ_VOTE_ASSENT;
		break;
	case WIFI_MEETING_EVT_VOT_NAY:
		con_status->v_result.nay++;
		value = WIFI_MEETING_CONF_REQ_VOTE_NAY;
		break;
	case WIFI_MEETING_EVT_VOT_WAIVER:
		con_status->v_result.waiver++;
		value = WIFI_MEETING_CONF_REQ_VOTE_WAIVER;
		break;
	case WIFI_MEETING_EVT_VOT_TOUT:
		con_status->v_result.timeout++;
		value = WIFI_MEETING_CONF_REQ_VOTE_TIMEOUT;
		break;
	}

	tcp_ctrl_msg_send_to(frame_type,msg,value);

	return SUCCESS;
}


/*
 * tcp_ctrl_uevent_request_checkin
 * 处理单元机签到功能
 *
 * 将数据信息上报给上位机
 *
 */
int tcp_ctrl_uevent_request_checkin(Pframe_type frame_type,const unsigned char* msg)
{



	tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_EVENT_CHECKIN);


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
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 */
int tcp_ctrl_unit_request_msg(const unsigned char* msg,Pframe_type frame_type)
{
	int tc_index = 0;
	int num = 0;
	int i;

	/*
	 * 解析msg，共5个字节，席别和ID
	 * 在上线请求时，会携带单元机的ID信息，没有就默认为0
	 * s_id源ID，d_id目标ID
	 */
	int s_id = 0;
	int d_id = 0;
	unsigned char id_msg[4] = {0};


	switch(frame_type->data_type)
	{

	case EVENT_DATA:
	{
		frame_type->name_type[0] = msg[0];
		frame_type->code_type[0] = msg[1];
		frame_type->evt_data.value = msg[2];

		switch(frame_type->name_type[0])
		{

			/*
			 * 发言请求，主机收到后需要显示，还要将此信息发送给主席单元
			 */
			case WIFI_MEETING_EVT_SPK:
				tcp_ctrl_uevent_request_spk(frame_type,msg);
				break;
			/*
			 * 投票结果请求上报
			 */
			case WIFI_MEETING_EVT_VOT:
				tcp_ctrl_uevent_request_vote(frame_type,msg);
				break;
			/*
			 * 服务请求
			 */
			case WIFI_MEETING_EVT_SER:

				break;
			/*
			 * 签到管理
			 */
			case WIFI_MEETING_EVT_CHECKIN:
				tcp_ctrl_uevent_request_checkin(frame_type,msg);
				break;
		}
	}
		break;

	case CONFERENCE_DATA:
		break;
	}


	return 0;

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
	Pconference_info confer_info;
	int tc_index = 0;
	int i;
	int ret = 0;
	int str_len = 0;

	printf("msg: ");
	for(i=0;i<sizeof(msg);i++)
	{
		printf("0x%02x ",msg[i]);
	}
	printf("\n");



	switch(frame_type->msg_type)
	{
	/*
	 * 控制类应答是设置失败，单元机返回的
	 * 所以，在接收到此消息后，主机要进行显示，还要将信息上报给上位机
	 */
	case W_REPLY_MSG:
	{
		/*
		 * 会议类控制应答，单元机只返回失败情况0xe3
		 * 上报消息进入队列
		 */
		if(msg[tc_index++] == WIFI_MEETING_ERROR)
		{
			switch(msg[tc_index++])
			{
			case TCP_C_ERR_SUCCESS:
				tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_CONF_WREP_SUCCESS);
				break;
			case TCP_C_ERR_HEAD:
			case TCP_C_ERR_LENGTH:
			case TCP_C_ERR_CHECKSUM:
				tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_CONF_WREP_ERR);
				break;
			}
		}
	}
		break;
	case R_REPLY_MSG:
	{
		/*
		 * 会议查询类应答，单元机应答全状态
		 * 对于单主机情况，主机只关心ID 和席别
		 * 有上位机情况，将应答信息转发给上位机
		 */
		if(msg[tc_index] == WIFI_MEETING_CON_ID)
		{
			frame_type->name_type[0] = msg[tc_index++];
			frame_type->code_type[0] = msg[tc_index++];

			 frame_type->con_data.id = (msg[tc_index++] << 24);
			 frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 16);
			 frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 8);
			 frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);


		 }else if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){
			 tc_index--;
			 frame_type->name_type[0] = msg[tc_index];
			 tc_index++;
			 frame_type->code_type[0] = msg[tc_index++];
			 frame_type->con_data.seat = msg[tc_index++];

		 }
		//end
		 else if(msg[tc_index++] == WIFI_MEETING_CON_NAME){
			 tc_index--;
			 frame_type->name_type[0] = msg[tc_index];
			 tc_index++;
			 frame_type->code_type[0] = msg[tc_index++];
			 str_len = msg[tc_index++];
			 memcpy(frame_type->con_data.name,&msg[tc_index],str_len);

			 tc_index = tc_index+msg[tc_index-1];



		 }else if(msg[tc_index++] == WIFI_MEETING_CON_SUBJ){
			 tc_index--;
			 frame_type->name_type[0] = msg[tc_index];
			 tc_index++;
			 frame_type->code_type[0] = msg[tc_index++];
			 str_len = msg[tc_index++];
			 memcpy(frame_type->con_data.subj[0],&msg[tc_index],str_len);
			 tc_index = tc_index+msg[tc_index-1];


		 }

		/*
		 * 更新conference_head链表的内容
		 *
		 */
		confer_info = (Pconference_info)malloc(sizeof(conference_info));
		memset(confer_info,0,sizeof(conference_info));
		confer_info->fd = frame_type->fd;
		confer_info->con_data.id =  frame_type->con_data.id;
		confer_info->con_data.seat =  frame_type->con_data.seat;
		memcpy(confer_info->con_data.name,frame_type->con_data.name,strlen(frame_type->con_data.name));
		memcpy(confer_info->con_data.subj[0],frame_type->con_data.subj[0],strlen(frame_type->con_data.subj[0]));
		/*
		 * 增加到会议信息链表中
		 */

		ret = tcp_ctrl_refresh_conference_list(confer_info);
		if(ret)
			return ERROR;
		/*
		 * 消息进行处理
		 * 如果有PC就直接将数据传递给PC
		 * 单主机的话，就需要上报给应用
		 */
		tcp_ctrl_msg_send_to(frame_type,msg,WIFI_MEETING_CONF_RREP);
	}
		break;
	}



	return 0;

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

	int value = 0;

	frame_type->name_type[0] = msg[0];
	frame_type->code_type[0] = msg[1];
	frame_type->evt_data.value = msg[2];

	switch(frame_type->name_type[0])
	{
	/*
	 * 单元机电源
	 */
	case WIFI_MEETING_EVT_PWR:
	{	switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_PWR_ON:
			value = WIFI_MEETING_EVENT_POWER_ON;
			break;
		case WIFI_MEETING_EVT_PWR_OFF:
			value = WIFI_MEETING_EVENT_POWER_OFF;
			break;
		}

		break;
	}
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


	}



	tcp_ctrl_msg_send_to(frame_type,msg,value);
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
int tcp_ctrl_from_unit( const unsigned char* handlbuf,Pframe_type frame_type)
{
	int i;
	int tc_index = 0;

	printf("handlbuf: ");
	for(i=0;i<sizeof(handlbuf);i++)
	{
		printf("0x%02x ",handlbuf[i]);
	}
	printf("\n");


	/*
	 * 消息类型分类
	 * 具体细分为控制类应答和查询类应答
	 */
	switch(frame_type->msg_type)
	{
		/*
		 * 请求类消息
		 */
		case REQUEST_MSG:
		{
			tcp_ctrl_unit_request_msg(handlbuf,frame_type);
			break;
		}
		/*
		 * 控制类应答
		 * 查询类应答
		 */
		case W_REPLY_MSG:
		case R_REPLY_MSG:
		{
			if(frame_type->data_type == CONFERENCE_DATA)
			{
				tcp_ctrl_unit_reply_conference(handlbuf,frame_type);
			}
			else if(frame_type->data_type == EVENT_DATA){
				tcp_ctrl_unit_reply_event(handlbuf,frame_type);
			}
			break;
		}
		/*
		 * 上线请求消息
		 */
		case ONLINE_REQ:
			tcp_ctrl_refresh_client_list(handlbuf,frame_type);
			break;
	}

	return 0;
}




int tcp_ctrl_pc_read_msg(const unsigned char* msg,Pframe_type frame_type)
{

	switch(frame_type->data_type)
	{
	case EVENT_DATA:
		frame_type->name_type[0] = msg[0];
		frame_type->code_type[0] = msg[1];
		frame_type->evt_data.value = msg[2];
		switch(frame_type->evt_data.value)
		{
		case WIFI_MEETING_EVT_ALL_STATUS:

			break;

		}
		break;
	case CONFERENCE_DATA:
		break;

	}


	return SUCCESS;
}

/*
 * tcp_ctrl_from_pc
 * 上位机消息数据
 * 1、上位机上线后先发送宣告在线消息，主机保存上位机连接信息至链表
 * 2、宣告上线后，上位机会发查询(单元机扫描功能)信息(全状态)，获取主机和单元机的状态@client_info
 *
 */
int tcp_ctrl_from_pc(const unsigned char* handlbuf,Pframe_type frame_type)
{
	int i;
	int tc_index = 0;

	printf("handlbuf: ");
	for(i=0;i<sizeof(handlbuf);i++)
	{
		printf("0x%02x ",handlbuf[i]);
	}
	printf("\n");


	switch(frame_type->msg_type)
	{
		case WRITE_MSG:
			break;
		case READ_MSG:
			tcp_ctrl_pc_read_msg(handlbuf,frame_type);
			break;
		case REQUEST_MSG:
			break;
		case ONLINE_REQ:
			tcp_ctrl_refresh_client_list(handlbuf,frame_type);
			break;

	}
	return 0;
}
