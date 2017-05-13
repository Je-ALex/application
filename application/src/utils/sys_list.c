/*
 * tc_list.c
 *
 *  Created on: 2016年10月10日
 *      Author: leon
 */


#include "wifi_sys_list.h"
#include "wifi_sys_init.h"

/*
 * list_add
 * 将新元素加入链表中
 *
 * 输入
 * pclient_node/void*
 * 输出
 * 无
 * 返回值
 * 成功
 * 失败
 */
int sys_list_add(pclient_node head,void* data)
{
	pclient_node newnode = NULL;
	pclient_node tail = NULL;

	newnode = (pclient_node)malloc(sizeof(client_node));
	memset(newnode,0,sizeof(client_node));

	newnode->data = data;
	newnode->next = NULL;

	tail = head;

	if(head == NULL)
	{
		return ERROR;
	}
	else{
		while(tail->next != NULL)
		{
			tail = tail->next;
		}
	}
	tail->next = newnode;
	head->size++;

	return SUCCESS;
}

/*
 * list_delete
 * 删除链表上的元素
 *
 * 删除结点主要分为删除位置为头结点和中尾部结点
 * 使用变量num确定删除位置
 * 删除可能是顺序，乱序，倒序
 * 所以
 */
int sys_list_delete(pclient_node head,int pos,pclient_node* del)
{
	pclient_node tmp = NULL,tmp1 = NULL;

	int num = 0;

	if (pos < 0 || pos >= head->size)
	    return ERROR;

	if(head == NULL){
		return ERROR;
	}

	tmp1 = head;

	tmp = head->next;

	while(tmp != NULL)
	{
			/*
			 * 删除结点位于头部
			 */
			if(pos == 0){

				*del = tmp;
				tmp1->next = tmp->next;
				//printf("data in the first\n");
				head->size--;
				break;
			}
			/*
			 * 删除结点在中尾部
			 * 顺序，倒序删除均可
			 */
			else{

				//printf("delete data last...\n");
				if(pos == num)
				{
					*del = tmp;
					tmp1->next = tmp->next;
					head->size--;
					break;
				}
			}
			tmp1=tmp;
			tmp=tmp->next;
			num++;
	}

	return SUCCESS;

}

/*
 * list_head_init
 * 创建单链表头指针
 * 输入
 * 无
 * 输出
 * 无
 * 返回值
 * 成功-头指针
 * 失败
 */
pclient_node sys_list_head_init()
{
	pclient_node head = NULL;

	head = (pclient_node)malloc(sizeof(client_node));

	if(head == NULL)
		return NULL;

	memset(head,0,sizeof(client_node));

	return head;
}


