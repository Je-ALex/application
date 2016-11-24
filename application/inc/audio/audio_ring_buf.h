/*
 * audio_ring_buf.h
 *
 *  Created on: 2016年11月23日
 *      Author: leon
 */

#ifndef INC_AUDIO_AUDIO_RING_BUF_H_
#define INC_AUDIO_AUDIO_RING_BUF_H_


typedef struct
{
	void** elements;
    int size;
    int head;
    int tail;
}audio_queue,*Paudio_queue;


Paudio_queue newQueue(int queueSize); // 初始化队列
void freeQueue(Paudio_queue queue); // 释放队列

int enqueue(Paudio_queue queue, void* element); // 入队
void* dequeue(Paudio_queue queue); // 出队

int queueIsEmpty(Paudio_queue queue); // 队列是否为空
int queueIsFull(Paudio_queue queue); // 队列是否已满

int queueLength(Paudio_queue queue); // 队列大小

void* getQueueElement(Paudio_queue queue, int index); // 获取队列指定元素


#endif /* INC_AUDIO_AUDIO_RING_BUF_H_ */
