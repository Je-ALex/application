/*
 * tc_list.c
 *
 *  Created on: 2016年10月10日
 *      Author: leon
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include "../../header/tcp_ctrl_list.h"
#include "../../header/tcp_ctrl_server.h"

/*
 * 将新结点加入链表中
 */
pclient_node list_add(pclient_node head,void* data)
{
	pclient_node newnode,tail;

	newnode = (pclient_node)malloc(sizeof(client_node));
	memset(newnode,0,sizeof(client_node));

	newnode->data = data;
	newnode->next = NULL;

	Pclient_info pinfo;

	tail = head;

	if(head == NULL)
		return NULL;
	else{
		while(tail->next != NULL)
		{
			tail = tail->next;
		}
	}

	tail->next = newnode;


	pclient_node tmep;

	tmep = head->next;

	printf("instert node success\n");

	while(tmep != NULL)
	{
		pinfo = tmep->data;
		printf("client_fd--%d ,client_id--%d ,"
				"client_mac--%s ,cli_addr--%s ",pinfo->client_fd,pinfo->client_id,
				pinfo->client_mac,inet_ntoa(pinfo->cli_addr.sin_addr));
		tmep = tmep->next;
		printf("\n");
	}

	return 0;


}

/*
 * 删除链表上的元素
 * 通过fd参数来判断
 *
 * 删除结点主要分为删除位置为头结点和中尾部结点
 * 使用变量num确定删除位置
 * 删除可能是顺序，乱序，倒序
 * 所以
 */
pclient_node list_delete(pclient_node head,int fd)
{
	pclient_node tmp,tmp2,tmp3;
	Pclient_info pinfo;
	int num = 0;

	char state = -1;
	tmp3 = head;

	tmp=tmp3->next;

	printf("delete fd = %d\n",fd);

	if(head == NULL){
		return NULL;
	}
	else{

		while(tmp != NULL)
		{
			pinfo = tmp->data;
			tmp2 =tmp;

			/*
			 * 删除结点位于头部
			 */
			if(pinfo->client_fd == fd && num == 0){

				state ++;
				printf("data in the first\n");
				tmp3->next = tmp2->next;
				free(tmp2);
				break;

			}
			/*
			 * 删除结点在中尾部
			 * 顺序，倒序删除均可
			 */
			else{

				printf("delete data last...\n");
				tmp2 = tmp2->next;

				if(tmp2 != NULL)
					pinfo = tmp2->data;

				if(pinfo->client_fd == fd)
				{
					state++;
					printf("data in the last\n");
					tmp->next = tmp2->next;
					free(tmp2);
					break;

				}
			}

			tmp=tmp->next;
			num++;
		}

	}
	if(state < 0)
		printf("there is no data in the list\n");

	return 0;

}

/*
 * 创建单链表头指针
 */
pclient_node list_head_init()
{

	pclient_node head;

	head = (pclient_node)malloc(sizeof(client_node));
	if(head == NULL)
		return NULL;

	memset((void*)head,0,sizeof(client_node));

	printf("head init success\n");

	return head;

}
