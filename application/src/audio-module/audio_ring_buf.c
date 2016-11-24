/*
 * util_queue.c
 *
 *  Created on: 2012-5-18
 *      Author: van
 */

#include "audio_ring_buf.h"
#include <stdlib.h>
#include <stdio.h>



// 初始化队列
Paudio_queue newQueue(int queueSize)
{
	Paudio_queue queue = (Paudio_queue)malloc(sizeof(audio_queue));
    if (NULL == queue)
    {
        printf("in newQueue queue malloc error.\n");
        return NULL;
    }

    queue->elements = (void**)malloc(sizeof(void*) * (queueSize + 1));
    if (NULL == queue->elements)
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
void freeQueue(Paudio_queue queue)
{
    free(queue->elements);
    free(queue);
}

// 入队
int enqueue(Paudio_queue queue, void* element)
{
    if (queueIsFull(queue))
    {
        queue->head = 0;
        queue->tail = 0;
        printf("queue is full.\n");
//        return -1;
    }

    queue->elements[queue->tail] = element;
    queue->tail = (queue->tail + 1) % queue->size;

    return 0;
}

// 出队
void* dequeue(Paudio_queue queue)
{
	void* result;

    if (queueIsEmpty(queue))
    {
        // printf("queue is empty.\n");
        return NULL;
    }

    result = queue->elements[queue->head];

    queue->head = (queue->head + 1) % queue->size;

    return result;
}

// 队列是否为空
int queueIsEmpty(Paudio_queue queue)
{
    return (queue->head == queue->tail);
}

// 队列是否已满
int queueIsFull(Paudio_queue queue)
{
    return (((queue->tail + 1) % queue->size) == queue->head);
}

// 队列大小
int queueLength(Paudio_queue queue)
{
    return (((queue->tail + queue->size) + queue->head) % queue->size);
}

// 获取队列指定元素
void* getQueueElement(Paudio_queue queue, int index)
{
    if (index < 0 || index >= queueLength(queue))
    {
        printf("queue element is out of range.\n");
        return NULL;
    }

    return (queue->elements[(queue->head + index) % queue->size]);
}
