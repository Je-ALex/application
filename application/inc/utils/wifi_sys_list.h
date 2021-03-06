/*
 * tc_list.h
 *
 *  Created on: 2016年10月10日
 *      Author: leon
 */

#ifndef INC_TCP_CTRL_LIST_H_
#define INC_TCP_CTRL_LIST_H_


typedef struct node{
	int size;
    void* data;
    struct node* next;

} client_node, *pclient_node;

int sys_list_add(pclient_node head,void* data);
int sys_list_delete(pclient_node head,int pos,pclient_node* del);

pclient_node sys_list_head_init();

#endif /* INC_TCP_CTRL_LIST_H_ */
