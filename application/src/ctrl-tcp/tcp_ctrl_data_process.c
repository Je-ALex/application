/*
 * tcp_ctrl_data_process.c
 *
 *  Created on: 2016年10月14日
 *      Author: leon
 */


#include "../../header/tcp_ctrl_data_process.h"


/*
 * 定义两个结构体
 * 存取前状态，存储当前状态
 *
 * 创建查询线程，轮训比对，有差异，就返回新状态
 */
frame_type new_uint_data = {0};
frame_type last_uint_data = {0};



/*
 * 数据包解析函数，将有效数据提出到handlbuf中
 */
char* tc_frame_analysis(int* fd,const unsigned char* buf,int* len,Pframe_type frame_type)
{

	int i;
	unsigned short package_len = 0;
	unsigned char sum = 0;
	int length = *len;

	char* buffer = NULL;

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
		return NULL;
	}
	/*
	 * 计算帧总长度buf[5]-buf[6]
	 * 小端模式：低位为高，高位为低
	 */
	package_len = buf[5] & 0xff;
	package_len = package_len << 8;
	package_len = package_len + buf[6] & 0xff;

	printf("package_len: %d\n",package_len);

	/*
	 * 长度最小不小于12(帧信息字节)+1(data字节，最小为一个名称码)
	 */
	if(length < 0x0d || (length != package_len))
	{
		printf( "%s not legal length\n", __FUNCTION__);

		return NULL;
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

		return NULL;
	}
	/*
	 * buf[4]
	 * 判断数据类型
	 * 消息类型(0xe0)4.5-4.7：控制消息(0x01)，请求消息(0x02)，应答消息(0x03)
	 * 数据类型(0x1c)4.2-4.4：事件型数据(0x01)，会议型单元参数(0x02)，会议型会议参数(0x03)
	 * 设备类型数据(0x03)4.0-4.1：上位机发送(0x01)，主机发送(0x02)，单元机发送(0x03)
	 */
	frame_type->msg_type = buf[4] & 0xe0;
	frame_type->data_type = buf[4] & 0x1c;
	frame_type->dev_type = buf[4] & 0x03;

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
	buffer = (char*)calloc(data_len,sizeof(char));

	memcpy(buffer,buf+11,data_len);

	printf("buffer: ");
	for(i=0;i<sizeof(buffer);i++)
	{
		printf("0x%02x ",buffer[i]);
	}
	printf("\n");

	return buffer;
}



/*
 * 主机发送数据组包函数
 * 首先确认组包消息的格式
 * 输入：
 * 帧数据类型：消息类型，数据类型，设备类型
 * 帧数据内容：控制类数据类型，会议型数据
 *
 * 输出：
 * 发送数据包内容，发送数据包长度
 *
 * 返回值：
 * 成功，失败
 */
int tc_frame_compose(Pframe_type type,char* params,unsigned char* result_buf)
{

	unsigned char msg,data,machine,info;
	int length = 0;
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

	int tc_index = 0;

	msg=data=machine=info=0;

	if(result_buf == NULL)
		return -1;

	/*
	 * HEAD
	 */
	result_buf[tc_index++] = 'D';
	result_buf[tc_index++] = 'S';
	result_buf[tc_index++] = 'D';
	result_buf[tc_index++] = 'S';

	/*
	 * edit the frame information to the buf
	 * 三类info的地址分别是
	 * xxx 	xxx 	xx
	 * 0xe0	0x1c	0x03
	 * 每个type分别进行移位运算后进行位与，得到第四字节的信息
	 *
	 */
	msg = type->msg_type;
	printf("msg = %x\n",msg);
	msg = (msg << 5) & 0xe0;
	printf("msg = %x\n",msg);

	data = type->data_type;
	printf("data = %x\n",data);
	data = (data << 2) & 0x1c;
	printf("data = %x\n",data);

	machine = type->dev_type;
	printf("machine = %x\n",machine);
	machine = machine & 0x03;
	printf("machine = %x\n",machine);

	info = msg+data+machine;
	printf("info = %x\n",info);

	/*
	 * 存储第四字节的信息
	 */
	result_buf[tc_index++] = info;

	/*
	 * LENGTH
	 * 数据长度加上固定长度
	 * 固定长度为4个头字节+1个消息类+2个长度+4个目标地址+1个校验和，共12个固定字节
	 */
	length = type->data_len + 12;
	result_buf[tc_index++] = (length >> 8) & 0xff;
	result_buf[tc_index++] = length & 0xff;

	getpeername(type->fd,(struct sockaddr*)&cli_addr,
								&clilen);

	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 0 ) & 0xff);
	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 8 ) & 0xff);
	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 16  ) & 0xff);
	result_buf[tc_index++] = (unsigned char) (( cli_addr.sin_addr.s_addr >> 24  ) & 0xff);

	/*
	 * 数据拷贝
	 */
	memcpy(&result_buf[tc_index],params,type->data_len);

	tc_index = tc_index + type->data_len;

	int i;
	unsigned char sum = 0;
	/*
	 * 计算校验和
	 */
	for(i=0;i<tc_index;i++)
	{
		sum += result_buf[i];
	}
	result_buf[tc_index] = (unsigned char) (sum & 0xff);

	type->frame_len = tc_index+1;

	printf("result data: ");
	for(i=0;i<tc_index+1;i++)
	{
		printf("%x ",result_buf[i]);
	}
	printf("\n");

	return 0;
}




/*
 * 处理应答类会议消息
 *
 * 应答类消息内容：
 * 终端状态应答，分为错误码和正常码
 *
 *
 */
int tc_unit_reply_conference(const char* msg,Pframe_type frame_type)
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

//		 if(msg[tc_index++] == WIFI_MEETING_CON_ID)
//		 {
//			 	frame_type->name_type[num] = msg[tc_index-1];
//				frame_type->code_type[num] = msg[tc_index++];
//
//				frame_type->con_data.id = (msg[tc_index++] << 24);
//				frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 16);
//				frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 8);
//				frame_type->con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);
//
//				num++;
//
//		 }else if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){
//
//			 	 frame_type->name_type[num] = msg[tc_index-1];
//			 	 frame_type->code_type[num] = msg[tc_index++];
//
//			 	 frame_type->con_data.seat = msg[tc_index++];
//
//			 	 num++;
//
//		 }else if(msg[tc_index++] == WIFI_MEETING_CON_NAME){
//
//		 	 frame_type->name_type[num] = msg[tc_index-1];
//		 	 frame_type->code_type[num] = msg[tc_index++];
//		 	 tc_index++;
//		 	 memcpy(frame_type->con_data.name,msg[tc_index],msg[tc_index-1]);
//
//		 	 tc_index = tc_index+msg[tc_index-1];
//		 	 num++;
//
//
//		 }else if(msg[tc_index++] == WIFI_MEETING_CON_SUBJ){
//
//			 frame_type->name_type[num] = msg[tc_index-1];
//			 frame_type->code_type[num] = msg[tc_index++];
//		 	 tc_index++;
//		 	 memcpy(frame_type->con_data.subj,msg[tc_index],msg[tc_index-1]);
//
//		 	 tc_index = tc_index+msg[tc_index-1];
//			 num++;
//
//		 }

		 if(msg[tc_index++] == WIFI_MEETING_CON_ID)
		 {
			 new_uint_data.name_type[num] = msg[tc_index-1];
			 new_uint_data.code_type[num] = msg[tc_index++];

			 new_uint_data.con_data.id = (msg[tc_index++] << 24);
			 new_uint_data.con_data.id = frame_type->con_data.id+(msg[tc_index++] << 16);
			 new_uint_data.con_data.id = frame_type->con_data.id+(msg[tc_index++] << 8);
			 new_uint_data.con_data.id = frame_type->con_data.id+(msg[tc_index++] << 0);

				num++;

		 }else if(msg[tc_index++] == WIFI_MEETING_CON_SEAT){

			 new_uint_data.name_type[num] = msg[tc_index-1];
			 new_uint_data.code_type[num] = msg[tc_index++];

			 new_uint_data.con_data.seat = msg[tc_index++];

				 num++;

		 }else if(msg[tc_index++] == WIFI_MEETING_CON_NAME){

			 new_uint_data.name_type[num] = msg[tc_index-1];
			 new_uint_data.code_type[num] = msg[tc_index++];
			 tc_index++;
			 memcpy(new_uint_data.con_data.name,&msg[tc_index],msg[tc_index-1]);

			 tc_index = tc_index+msg[tc_index-1];
			 num++;


		 }else if(msg[tc_index++] == WIFI_MEETING_CON_SUBJ){

			 new_uint_data.name_type[num] = msg[tc_index-1];
			 new_uint_data.code_type[num] = msg[tc_index++];
			 tc_index++;
			 memcpy(new_uint_data.con_data.subj,&msg[tc_index],msg[tc_index-1]);

			 tc_index = tc_index+msg[tc_index-1];
			 num++;

		 }



	return 0;

}
/*
 * 处理单元机数据
 * 在此函数下，将消息分为请求类和应答类
 *
 */
int tc_from_unit(const char* handlbuf,Pframe_type frame_type)
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
			if(frame_type->data_type == CONFERENCE_DATA)
			{
			}
			else{

			}
			break;
			/*
			 * 此类应答消息只包含终端应答的错误消息
			 */
		case REPLY_MSG:

			if(frame_type->data_type == CONFERENCE_DATA)
			{
				tc_unit_reply_conference(handlbuf,frame_type);
			}
			else{
//				tc_unit_reply_event(handlbuf,frame_type);
			}
			break;

	}

	return 0;
}
/*
 * 处理上位机数据
 */
int tc_from_pc(const char* handlbuf,Pframe_type frame_type)
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

	}
	return 0;
}
