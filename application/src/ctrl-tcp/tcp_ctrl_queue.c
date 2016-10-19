/*
 * tcp_ctrl_queue.c
 *
 *  Created on: 2016年10月19日
 *      Author: leon
 */



#include "../../header/tcp_ctrl_queue.h"


/*

// 初始化队列
Plinkqueue queue_init(int queueSize)
{
	Plinkqueue queue = (Plinkqueue)malloc(sizeof(linkqueue));
    if (NULL == queue)
    {
        printf("in newQueue queue malloc error.\n");
        return NULL;
    }

    queue->data = (void**)malloc(sizeof(void*)); //* (queueSize + 1));
    if (NULL == queue->data)
    {
        printf("in newQueue queue->elements malloc error.\n");
        return NULL;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->size = queueSize;

    return queue;
}

// 释放队列
void freeQueue(Plinkqueue queue)
{
    free(queue->data);
    free(queue);
}
// 队列是否为空
int queueIsEmpty(Plinkqueue queue)
{
    return (queue->head == queue->tail);
}
// 入队
int enqueue(Plinkqueue queue, void* element)
{
    if (queueIsFull(queue))
    {
        printf("queue is full.\n");
        return -1;
    }

    queue->data[queue->tail] = element;
    queue->tail = (queue->tail + 1) % queue->size;

    return 0;
}

// 出队
void* dequeue(Plinkqueue queue)
{
    void* result;

    if (queueIsEmpty(queue))
    {
        // printf("queue is empty.\n");
        return NULL;
    }

    result = queue->data[queue->head];

    queue->head = (queue->head + 1) % queue->size;

    return result;
}


// 队列是否已满
int queueIsFull(Plinkqueue queue)
{
    return (((queue->tail + 1) % queue->size) == queue->head);
}

// 队列大小
int queueLength(Plinkqueue queue)
{
    return (((queue->tail + queue->size) + queue->head) % queue->size);
}

// 获取队列指定元素
void* getQueueElement(Plinkqueue queue, int index)
{
    if (index < 0 || index >= queueLength(queue))
    {
        printf("queue element is out of range.\n");
        return NULL;
    }

    return (queue->data[(queue->head + index) % queue->size]);
}
*/

Plinkqueue queue_init(){

	Plinkqueue head = (Plinkqueue)malloc(sizeof(linkqueue));
	if(head == NULL){

	  return NULL;
	}
	memset(head, 0, sizeof(linkqueue));

	head->front = head->rear;

	return head;
}

int enter_queue(Plinkqueue head,void* data)
{
	Plinknode tail;

	Plinknode node = (Plinknode) malloc(sizeof(linknode));
	memset(node, 0, sizeof(linknode));

	node->data = data;
	node->next = NULL;

	tail = head->rear;

	if(head == NULL)
		return ERROR;
	else
	{
		while(tail->next != NULL)
		{
			tail = tail->next;
		}
	}

	tail->next = node;
	tail = node;

	if(head->front->next == NULL)
		head->front = node;

	head->size++;

	printf("instert node success\n");
	return 0;
}

void* out_queue(Plinkqueue queue)
{
	Plinknode node;

	if(queue->front == queue->rear)
		return NULL;

	node = queue->front->next;

	queue->front->next = node->next;

	if(queue->rear == node)
		queue->rear = queue->front;


	return node->data;
}
















