
#include "audio_ring_buf.h"


/*
 * audio_queue_init
 * 音频数据队列初始化
 * @size初始化队列的大小
 */
Paudio_queue audio_queue_init(int size)
{
	Paudio_queue queue = (Paudio_queue)malloc(sizeof(audio_queue));
    if (NULL == queue)
    {
        printf("%s-%s-%d queue malloc error\n",__FILE__,__func__,__LINE__);
        return NULL;
    }

    queue->elements = (void**)malloc(sizeof(void*) * (size + 1));
    if (NULL == queue->elements)
    {
    	printf("%s-%s-%d queue->elements malloc error\n",__FILE__,__func__,__LINE__);
		return NULL;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->size = size;

    return queue;
}

/*
 * audio_queue_free
 * 释放音频数据队列
 * @消息队列句柄
 */
void audio_queue_free(Paudio_queue queue)
{
    free(queue->elements);
    free(queue);
}

/*
 * audio_enqueue
 * 音频数据进入队列
 * @消息队列句柄
 * @元素
 */
int audio_enqueue(Paudio_queue queue, void* element)
{
    if (audio_queue_full(queue))
    {
        queue->head = 0;
        queue->tail = 0;
//      printf("%s-%s-%d queue full\n",__FILE__,__func__,__LINE__);
    }

    queue->elements[queue->tail] = element;
    queue->tail = (queue->tail + 1) % queue->size;

    return SUCCESS;
}

/*
 * audio_dequeue
 * 音频数据出队列
 * @队列句柄
 */
void* audio_dequeue(Paudio_queue queue)
{
	void* result;

    if (audio_queue_empty(queue))
    {
        return NULL;
    }
    result = queue->elements[queue->head];

    queue->head = (queue->head + 1) % queue->size;

    return result;
}

/*
 * 判断队列是否为空
 */
int audio_queue_empty(Paudio_queue queue)
{
    return (queue->head == queue->tail);
}

/*
 * 判断队列是否已满
 */
int audio_queue_full(Paudio_queue queue)
{
    return (((queue->tail + 1) % queue->size) == queue->head);
}

/*
 * 判断队列大小
 */
int audio_queue_len(Paudio_queue queue)
{
    return (((queue->tail + queue->size) + queue->head) % queue->size);
}

/*
 * 获取队列指定元素
 */
void* audio_queue_element(Paudio_queue queue, int index)
{
    if (index < 0 || index >= audio_queue_len(queue))
    {
        printf("queue element is out of range.\n");
        return NULL;
    }

    return (queue->elements[(queue->head + index) % queue->size]);
}



