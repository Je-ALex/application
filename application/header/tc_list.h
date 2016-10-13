/*
 * tc_list.h
 *
 *  Created on: 2016年10月10日
 *      Author: leon
 */

#ifndef HEADER_TC_LIST_H_
#define HEADER_TC_LIST_H_




typedef struct node{
	int size;
    void* data;
    struct node* next;

} client_node, *pclient_node;

pclient_node list_add(pclient_node head,void* data);
pclient_node list_delete(pclient_node head,int fd);

pclient_node list_head_init();

#endif /* HEADER_TC_LIST_H_ */
