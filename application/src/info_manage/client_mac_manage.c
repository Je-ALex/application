/*
 * client_mac_manage.c
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#include "sys_uart_init.h"
#include "client_mac_manage.h"
#include "audio_ring_buf.h"
#include "tcp_ctrl_data_compose.h"
#include "tcp_ctrl_data_process.h"
#include "tcp_ctrl_device_status.h"

extern Pglobal_info node_queue;


int cmm_search_client_mac(Pclient_info info)
{
	pclient_node tmp_node = NULL;
	Pconnect_mac_table macinfo;
	Pconnect_mac_table tmp_info;

	macinfo = (Pconnect_mac_table)malloc(sizeof(connect_mac_table));
	memset(macinfo,0,sizeof(connect_mac_table));

	int state = 0;
	unsigned short tmp_mac = 1;

	tmp_node = node_queue->sys_list[CONNECT_MAC_TABLE]->next;
	do
	{
		if(tmp_node == NULL)
		{
			break;
		}else{
			tmp_info = tmp_node->data;
			if(tmp_info->mac_number == info->mac_addr)
			{
				state++;
//				printf("%s-%s-%d,the MAC is exist\n",__FILE__,__func__,__LINE__);
//				free(info);
//
//				return ERROR;

			}
			if(tmp_info->mac_table > tmp_mac)
			{
				tmp_mac = tmp_info->mac_table;
			}

		}
		tmp_node = tmp_node->next;

	}while(tmp_node != NULL);

	if(state == 0)
	{
		macinfo->mac_table = tmp_mac+1;
		macinfo->mac_number = info->mac_addr;
		list_add(node_queue->sys_list[CONNECT_MAC_TABLE],macinfo);

	}

	return SUCCESS;
}

