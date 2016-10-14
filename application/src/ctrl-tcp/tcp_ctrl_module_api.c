/*
 * tcp_ctrl_module_api.c
 *
 *  Created on: 2016年10月13日
 *      Author: leon
 */



#include "../../header/ctrl-tcp.h"
#include "../../header/tc_list.h"

extern pclient_node list_head;

/*
 * 会议类类信息(0x02)设置
 *
 * 此API是事件类数据统一接口
 *
 * 编辑终端属性，设置id，seats等
 *
 * 输入：
 * @fd--设置终端的fd
 * @id--分配的id
 * @seats--分配的席位,主席机，客席机，旁听机
 * @name--对应终端的姓名，单主机情况是不需要设置姓名
 * @msg_ty结构体--下发的消息类型(控制消息，请求消息，应答消息，查询消息)
 */
int ctrl_module_edit_info(Pframe_type type)
{

	pclient_node tmp = NULL;
	Pclient_info pinfo;

	char find_fd = -1;
	unsigned char buf[256] = {0};
	unsigned char s_buf[256] = {0};

	/**************************************
	 * 将数据信息进行组包
	 * 参照通讯协议中会议类参数信息进行编码
	 *
	 * 名称		编码格式	数据格式	值
	 * seat		0x01	char	1,2,3
	 * id		0x02	char 	1,2,3...
	 * name		0x03	char	"hushan"
	 ************************************/
	int i;
	/*
	 * 判断是否有此客户端
	 */
	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == type->fd)
		{
			find_fd = 1;
			break;
		}
		tmp = tmp->next;

	}
	if(find_fd < 0)
	{
		printf("please input the right fd..\n");
		return ERROR;
	}
	/*
	 * 发送消息组包
	 * 对数据内容进行封装，增数据头等信息
	 */
	tc_frame_compose(type,buf,s_buf);

	tmp = list_head->next;
	while(tmp != NULL)
	{
		pinfo = tmp->data;
		if(pinfo->client_fd == type->fd)
		{
			pthread_mutex_lock(&mutex);
			write(pinfo->client_fd, s_buf, type->frame_len);
			pthread_mutex_unlock(&mutex);
			break;
		}
		tmp = tmp->next;

	}

	return SUCCESS;
}


/*
 * 设置会议参数
 *
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj)
{

	frame_type data_type;

	/*
	 * 将参数保存
	 */
	data_type.fd = fd_value;
	data_type.con_data.id = client_id;
	data_type.con_data.seat = client_seat;
	memcpy(data_type.con_data.name,client_name,strlen(client_name));
	memcpy(data_type.con_data.subj,client_name,strlen(subj));

	data_type.msg_type = Write_msg;
	data_type.data_type = CONFERENCE_DATA;
	data_type.dev_type = HOST_CTRL;

	ctrl_module_edit_info(&data_type);

	return SUCCESS;

}
/*
 * 查询会议参数
 *
 */
int get_the_conference_parameters(int fd_value,int* client_id,char* client_seat,
		char* client_name,char* subj)
{

	frame_type data_type;

	/*
	 * 将参数保存
	 */
	data_type.fd = fd_value;

	data_type.msg_type = Read_msg;
	data_type.data_type = CONFERENCE_DATA;
	data_type.dev_type = HOST_CTRL;

	ctrl_module_edit_info(&data_type);


	data_type.con_data.id = client_id;
	data_type.con_data.seat = client_seat;
	memcpy(data_type.con_data.name,client_name,strlen(client_name));
	memcpy(data_type.con_data.subj,client_name,strlen(subj));

	return SUCCESS;

}
