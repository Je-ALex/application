/*
 * tcp_ctrl_queue.h
 *
 *  Created on: 2016年10月19日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_QUEUE_H_
#define HEADER_TCP_CTRL_QUEUE_H_

#include "tcp_ctrl_server.h"

typedef struct queue_node{

	void* data;
    struct queue_node *next;
}linknode,*Plinknode;

typedef struct
{
	int size;
	Plinknode front,rear;
}linkqueue,*Plinkqueue;



Plinkqueue queue_init();
int enter_queue(Plinkqueue queue, void* element);
int out_queue(Plinkqueue queue,Plinknode* ret);
int clear_queue(Plinkqueue queue);




#endif /* HEADER_TCP_CTRL_QUEUE_H_ */
