/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */


#include "../../header/tcp_ctrl_data_process.h"
#include "../../header/tcp_ctrl_queue.h"

/*
 * 定义两个结构体
 * 存取前状态，存储当前状态
 *
 * 创建查询线程，轮训比对，有差异，就返回新状态
 */
extern frame_type new_uint_data;
extern pclient_node list_head;
extern pclient_node confer_head;


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
	//printf("%c,%c,%c,%c\n",buf[0],buf[1],buf[2],buf[3]);

	while (buf[0] != 'D' || buf[1] != 'S'
			|| buf[2] != 'D' || buf[3] != 'S') {

		printf( "%s not legal headers\n", __FUNCTION__);
		return ERROR;
	}
	/*
	 * 计算帧总长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[5] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + buf[6] & 0xff;

	printf("package_len: %d\n",package_len);

	/* fixme
	 * 长度最小不小于12(帧信息字节)+1(data字节，最小为一个名称码) = 13
	 */
	if(length < 0x0d || (length != package_len))
	{
		printf( "%s not legal length\n", __FUNCTION__);

		return ERROR;
	}
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
	/*
	 * buf[4]
	 * 判断数据类型
	 * 消息类型(0xf0)4.5-4.7：控制消息(0x01)，请求消息(0x02)，应答消息(0x03)
	 * 数据类型(0x0c)4.2-4.4：事件型数据(0x01)，会议型单元参数(0x02)，会议型会议参数(0x03)
	 * 设备类型数据(0x03)4.0-4.1：上位机发送(0x01)，主机发送(0x02)，单元机发送(0x03)
	 */
	frame_type->msg_type = (buf[4] & MSG_TYPE) >> 4;
	frame_type->data_type = (buf[4] & DATA_TYPE) >> 2;
	frame_type->dev_type = buf[4] & MACHINE_TYPE;

	frame_type->fd = *fd;
	/*
	 * 计算data内容长度
	 * package_len减去帧信息长度4字节头，1字节信息，2字节长度，4字节目标地址，1字节校验和
	 *
	 * data_len = package_len - 12
	 */
	int data_len = package_len - 12;
	frame_type->data_len = data_len;

	/*
	 * 保存有效数据
	 * 将有数据另存为一个内存区域
	 */
	buffer = (unsigned char*)calloc(data_len,sizeof(char));

	memcpy(buffer,buf+11,data_len);

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

	if(frame_type->evt_data.value == WIFI_MEETING_EVT_SPK_REQ_SPK)
	{
		tmp=confer_head->next;
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
		 * 将事件信息发送至消息队列
		 */
		frame_type->fd = tmp_fd;
		tcp_ctrl_enter_queue(frame_type);
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

	pclient_node tmp = NULL;
	Pclient_info tmp_type;
	int status = 0;

	switch(frame_type->evt_data.value)
	{
	case WIFI_MEETING_EVT_VOT_ASSENT:
		new_uint_data.con_data.v_result.assent++;
		break;
	case WIFI_MEETING_EVT_VOT_NAY:
		new_uint_data.con_data.v_result.nay++;
		break;
	case WIFI_MEETING_EVT_VOT_WAIVER:
		new_uint_data.con_data.v_result.waiver++;
		break;
	case WIFI_MEETING_EVT_VOT_TOUT:
		new_uint_data.con_data.v_result.timeout++;
		break;
	}

	/*
	 * 查询是否有上位机连接，若有，将投票信息上报给上位机
	 */
	tmp=list_head->next;
	while(tmp != NULL)
	{
		tmp_type = tmp->data;

		if(tmp_type->client_name == PC_CTRL)
		{
			frame_type->fd = tmp_type->client_fd;
			tcp_ctrl_module_edit_info(frame_type,msg);
			status++;
			break;
		}
		tmp = tmp->next;
	}
	if(!status)
		printf("there is no pc\n");

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

	pclient_node tmp = NULL;
	Pclient_info tmp_type;
	int status = 0;
	/*
	 * 查询是否有上位机连接，若有，将投票信息上报给上位机
	 */
	tmp=list_head->next;
	while(tmp != NULL)
	{
		tmp_type = tmp->data;

		if(tmp_type->client_name == PC_CTRL)
		{
			frame_type->fd = tmp_type->client_fd;
			tcp_ctrl_module_edit_info(frame_type,msg);
			status++;
			break;
		}
		tmp = tmp->next;
	}

	if(!status)
		printf("there is no pc\n");

	return SUCCESS;
}


/*
 * tcp_ctrl_unit_request_event
 * 单元机事件类请求消息处理函数
 * 处理接收到的消息，将消息内容进行解析，存取到全局结构体中@new_uint_data
 *
 * 已增加发言，投票，签到，服务
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 */
int tcp_ctrl_unit_event_request(const unsigned char* msg,Pframe_type frame_type)
{
	int tc_index = 0;
	int num = 0;
	int i;


	frame_type->name_type[0] = msg[0];
	frame_type->code_type[0] = msg[1];

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


	return 0;

}
/*
 * tcp_ctrl_unit_reply_conference
 * 单元机会议类应答消息处理函数
 * 处理接收到的消息，将消息内容进行解析，存取到全局结构体中@new_uint_data
 *
 * in:
 * @msg数据内容
 * @Pframe_type帧信息
 *
 */
int tcp_ctrl_unit_reply_conference(const unsigned char* msg,Pframe_type frame_type)
{


	int tc_index = 0;
	int num = 0;
	int i;

	printf("msg: ");
	for(i=0;i<sizeof(msg);i++)
	{
		printf("0x%02x ",msg[i]);
	}
	printf("\n");

	new_uint_data.fd = frame_type->fd;

	if(msg[tc_index++] == WIFI_MEETING_CON_ID)
	{
		 new_uint_data.name_type[0] = msg[tc_index-1];
		 new_uint_data.code_type[0] = msg[tc_index++];

		 new_uint_data.con_data.id = (msg[tc_index++] << 24);
		 new_uint_data.con_data.id = frame_type->con_data.id+(msg[tc_index++] << 16);
		 new_uint_data.con_data.id = frame_type->con_data.id+(msg[tc_index++] << 8);
		 new_uint_data.con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);

			num++;

	 }else if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){

		 new_uint_data.name_type[0] = msg[tc_index-1];
		 new_uint_data.code_type[0] = msg[tc_index++];

		 new_uint_data.con_data.seat = msg[tc_index++];

			 num++;

	 }else if(msg[tc_index++] == WIFI_MEETING_CON_NAME){

		 new_uint_data.name_type[0] = msg[tc_index-1];
		 new_uint_data.code_type[0] = msg[tc_index++];
		 tc_index++;
		 memcpy(new_uint_data.con_data.name,&msg[tc_index],msg[tc_index-1]);

		 tc_index = tc_index+msg[tc_index-1];
		 num++;


	 }else if(msg[tc_index++] == WIFI_MEETING_CON_SUBJ){

		 new_uint_data.name_type[0] = msg[tc_index-1];
		 new_uint_data.code_type[0] = msg[tc_index++];
		 tc_index++;
		 memcpy(new_uint_data.con_data.subj[0],&msg[tc_index],msg[tc_index-1]);

		 tc_index = tc_index+msg[tc_index-1];
		 num++;

	 }



	return 0;

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
		case REQUEST_MSG:
		{
			if(frame_type->data_type == CONFERENCE_DATA)
			{
					;//暂时没有
			}
			else if(frame_type->data_type == EVENT_DATA){

				tcp_ctrl_unit_event_request(handlbuf,frame_type);
			}
			break;
		}
		case W_REPLY_MSG:
		{
			if(frame_type->data_type == CONFERENCE_DATA)
			{
				tcp_ctrl_unit_reply_conference(handlbuf,frame_type);
			}
			else if(frame_type->data_type == EVENT_DATA){
//				tc_unit_reply_event(handlbuf,frame_type);
			}
			break;
		}
		case R_REPLY_MSG:
		{
			if(frame_type->data_type == CONFERENCE_DATA)
			{
				tcp_ctrl_unit_reply_conference(handlbuf,frame_type);
			}
			else if(frame_type->data_type == EVENT_DATA){
//				tc_unit_reply_event(handlbuf,frame_type);
			}
			break;
		}
		case ONLINE_REQ:
			tcp_ctrl_refresh_client_list(frame_type);
			break;
	}

	return 0;
}
/*
 * 处理上位机数据
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
			break;
		case REQUEST_MSG:
			break;
		case ONLINE_REQ:
			tcp_ctrl_refresh_client_list(frame_type);
			break;

	}
	return 0;
}
