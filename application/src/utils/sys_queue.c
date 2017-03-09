/*
 * tcp_ctrl_queue.c
 *
 *  Created on: 2016年10月19日
 *      Author: leon
 */


#include "wifi_sys_queue.h"
#include "wifi_sys_init.h"

/*
 * 初始化队列
 */
Plinkqueue queue_init(){


	Plinkqueue head;
	head = (Plinkqueue)malloc(sizeof(linkqueue));
	head->front = head->rear = (Plinknode)malloc(sizeof(linknode));

	if(head == NULL){

		printf("queue init failed\n");
		return NULL;
	}

	head->front->next = NULL;


	return head;
}

/*
 * 消息入队
 */
int enter_queue(Plinkqueue queue,void* data)
{
	Plinknode node = (Plinknode) malloc(sizeof(linknode));
	memset(node, 0, sizeof(linknode));

	node->data = data;
	node->next = NULL;


	if(queue == NULL)
	{
		printf("queue is null\n");
		return ERROR;
	}

	queue->rear->next = node;
	queue->rear = node;

	queue->size++;

	return SUCCESS;
}

/*
 * 消息出队
 */
int out_queue(Plinkqueue queue,Plinknode* ret)
{
	Plinknode node = NULL;

	if(queue->front == queue->rear)
	{
		printf("queue is empty\n");
		return ERROR;
	}

	node = queue->front->next;

	queue->front->next = node->next;

	if(queue->rear == node)
		queue->rear = queue->front;

	queue->size--;

	*ret = node;

	return SUCCESS;
}
/*
 * 清除销毁队列
 */
int clear_queue(Plinkqueue queue)
{
	Plinknode temp = queue->front->next;
    while(temp)
    {
    	Plinknode tp = temp;
        temp = temp->next;
        free(tp);
    }
    temp = queue->front;
    queue->front = queue->rear = NULL;
    free(temp);

    return 0;
}















