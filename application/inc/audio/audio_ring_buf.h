/*
 * audio_ring_buf.h
 *
 *  Created on: 2016年11月23日
 *      Author: leon
 */

#ifndef INC_AUDIO_AUDIO_RING_BUF_H_
#define INC_AUDIO_AUDIO_RING_BUF_H_

#include "audio_tcp_server.h"

typedef struct
{
	void** elements;
    int size;
    int head;
    int tail;

}audio_queue,*Paudio_queue;


Paudio_queue audio_queue_init(int size);

int audio_enqueue(Paudio_queue queue, void* element);

void* audio_dequeue(Paudio_queue queue);

int audio_queue_empty(Paudio_queue queue);
int audio_queue_full(Paudio_queue queue);

void audio_queue_free(Paudio_queue queue);

int audio_queue_len(Paudio_queue queue);

void* audio_queue_element(Paudio_queue queue, int index);


#endif /* INC_AUDIO_AUDIO_RING_BUF_H_ */
