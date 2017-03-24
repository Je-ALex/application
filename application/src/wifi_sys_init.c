/*
 * wifi_sys_init.c
 *
 *  Created on: 2016年11月10日
 *      Author: leon
 * 系统初始化文件
 * 系统初始化接口函数
 * 主要实现系统的功能初始化，变量和参数的初始化
 *
 */

#include "wifi_sys_init.h"

#include "udp_ctrl_server.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_data_process.h"
#include "audio_server.h"
#include "tcp_ctrl_device_status.h"

#include "sys_uart_init.h"
#include "sys_status_detect.h"

extern sys_info sys_in;
extern Pglobal_info node_queue;

pthread_t ctrl_udp;
pthread_t ctrl_tcpr;
pthread_t ctrl_tcps;
pthread_t ctrl_procs;
pthread_t ctrl_heart;

pthread_t audp_recv[AUDP_RECV_NUM] = {0};
pthread_t audp_send;


/*
 * wifi_sys_signal_init
 * 系统状态与信号初始化
 * 互斥锁的定义可以参考CTRL_TCP_SQUEUE_MUTEX定义
 * 信号量参考CTRL_TCP_RECV_SEM定义
 *
 * 返回值：
 * 成功-SUCCESS
 * 失败-ERROR
 *
 */
static int wifi_sys_signal_init()
{
	int i;
	int ret = -1;

	for(i=0;i<=CTRL_TCP_SQUEUE_MUTEX;i++)
	{
		pthread_mutex_init(&sys_in.sys_mutex[i], NULL);
		usleep(1000);
	}

	for(i=0;i<=CTRL_TCP_RECV_SEM;i++)
	{
		ret = sem_init(&sys_in.sys_sem[i], 0, 0);
		if (ret != 0)
		{
			 printf("sys_in.sys_sem[%d] initialization failed",i);
			 return ERROR;
		}
		usleep(1000);
	}
	return SUCCESS;
}


/*
 * wifi_sys_val_init
 *
 * 系统中链表、队列和全局变量初始化
 *
 * 1、队列初始化 主要有是状态上报队列  数据接收队列和数据发送队列
 * 2、链表初始化 设备连接链表/心跳链表、发言管理和会议信息链表
 * 3、系统全局变量初始化 相关功能参数初始化默认发言人数4人/FIFO模式/摄像跟踪关闭
 * 4、连接信息的文本文件初始化
 */
static int wifi_sys_val_init()
{
	int i;
	FILE* cfile;

	node_queue = (Pglobal_info)malloc(sizeof(global_info));
	memset(node_queue,0,sizeof(global_info));

	node_queue->sys_queue = (Plinkqueue*)malloc(sizeof(linkqueue)*(CTRL_TCP_SEND_QUEUE+1));
	memset(node_queue->sys_queue,0,sizeof(linkqueue)*(CTRL_TCP_SEND_QUEUE+1));
	for(i=0;i<=CTRL_TCP_SEND_QUEUE;i++)
	{
		node_queue->sys_queue[i] = queue_init();
		if(node_queue->sys_queue[i] == NULL)
		{
#if TCP_DBG
			printf("%s-%s-%d node_queue->sys_queue[%d] init failed\n",__FILE__,__func__,__LINE__,i);

#endif
			return INIT_QUEUE_ERR;

		}
		else{
#if TCP_DBG

			printf("%s-%s-%d node_queue->sys_queue[%d] init success\n",__FILE__,__func__,__LINE__,i);
#endif
		}
		msleep(1);

	}

	node_queue->sys_list = (pclient_node*)malloc(sizeof(client_node)*(CONFERENCE_LIST+1));
	memset(node_queue->sys_list,0,sizeof(client_node)*(CONFERENCE_LIST+1));

	for(i=0;i<=CONFERENCE_LIST;i++)
	{
		node_queue->sys_list[i] = list_head_init();
		if(node_queue->sys_list[i] == NULL)
		{
#if TCP_DBG
			printf("%s-%s-%d node_queue->sys_list[%d] init failed\n",__FILE__,__func__,__LINE__,i);
#endif
			return INIT_LIST_ERR;
		}
		else{
#if TCP_DBG
			printf("%s-%s-%d node_queue->sys_list[%d] init success\n",__FILE__,__func__,__LINE__,i);
#endif
		}
		msleep(1);

	}


	node_queue->con_status = (Pconference_status)malloc(sizeof(conference_status));
	memset(node_queue->con_status,0,sizeof(conference_status));

	node_queue->con_status->spk_number = DEF_SPK_NUM;

	node_queue->con_status->mic_mode = DEF_MIC_MODE;

	conf_status_set_camera_track(0);

	cfile = fopen(CONNECT_FILE,"w+");
	fclose(cfile);




	return SUCCESS;
}



int wifi_sys_net_thread_init()
{
	int ret = -1;
	int i;

	/*
	 * 控制相关的网络线程
	 */
	ret = pthread_create(&ctrl_udp,NULL,wifi_sys_ctrl_udp_server,NULL);
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_ctrl_udp_server failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}

	ret = pthread_create(&ctrl_tcpr,NULL,wifi_sys_ctrl_tcp_recv,NULL);
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_ctrl_tcp_recv failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}

	/*
	 * 音频相关的网络创建线程
	 */

	//语音接收线程
    int port = AUDIO_RECV_PORT;
    for(i=0;i<AUDP_RECV_NUM;i++)
    {
    	ret = pthread_create(&audp_recv[i], NULL, audio_recv_thread, (void*)port);
    	if (ret != 0)
    	{
    		printf("%s-%s-%d audio_recv_thread[%d] failed\n",__FILE__,__func__,__LINE__,i);
    	}
    	port +=2;
    }
	//语音发送线程
	ret = pthread_create(&audp_send, NULL, audio_send_thread,NULL);
	if (ret != 0)
	{
		printf("%s-%s-%d audio_send_thread failed\n",__FILE__,__func__,__LINE__);
	}

	return SUCCESS;
}

int wifi_sys_net_thread_deinit()
{
	int i;

//	pthread_exit(ctrl_udp);
	printf("%s-%s-%d\n",__FILE__,__func__,__LINE__);

	sys_net_status_set(1);
//	pthread_kill(ctrl_udp,SIGUSR1);
//	pthread_kill(ctrl_tcpr,SIGUSR1);
//
//
//    for(i=0;i<AUDP_RECV_NUM;i++)
//    {
//    	pthread_kill(audp_recv[i],SIGUSR1);
//    }
//
//    pthread_kill(audp_send,SIGUSR1);


	return SUCCESS;
}
/*
 * wifi_sys_thread_init
 * 系统线程初始化函数
 */
static int wifi_sys_thread_init()
{
	int ret;


	wifi_sys_net_thread_init();


	ret = pthread_create(&ctrl_procs,NULL,wifi_sys_ctrl_tcp_procs_data,NULL);
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_ctrl_tcp_procs_data failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}

	ret = pthread_create(&ctrl_heart,NULL,wifi_sys_ctrl_tcp_heart_state,NULL);
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_ctrl_tcp_heart_state failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}

	ret = pthread_create(&ctrl_tcps,NULL,wifi_sys_ctrl_tcp_send,NULL);
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_ctrl_tcp_send failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}


	return SUCCESS;
}



/* TODO
 * wifi_conference_sys_init
 * 主机系统初始化接口函数
 *
 * 网络成功连接后，调用此函数
 *
 * 主要负责系统的功能初始化
 *
 * 返回值：
 * 成功-0
 * 失败-ERROR
 *
 */
int wifi_conference_sys_init()
{
	int ret;

	/*
	 * 对标准输出进行重定向，指定到LOG文件
	 */
//	fflush(stdout);
//	char* path = "/hushan/LOG.txt";
//	freopen(path,"w",stdout);
//	setvbuf(stdout,NULL,_IONBF,0);


	printf("%s-%s-%d-%s-%s-%s\n",__FILE__,__func__,__LINE__,
			__DATE__,__TIME__,VERSION);

	ret = sys_status_detect_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d sys_status_detect_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}

	ret = wifi_sys_signal_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_signal_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}

	ret = wifi_sys_val_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_val_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}

	ret = uart_snd_effect_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d uart_snd_effect_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}
	ret = sys_video_uart_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d sys_video_uart_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}

	ret = wifi_sys_audio_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_audio_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}

	ret = wifi_sys_thread_init();
	if (ret != SUCCESS)
	{
		printf("%s-%s-%d wifi_sys_thread_init failed\n",__FILE__,__func__,__LINE__);
		goto ERR;
	}

	return SUCCESS;

ERR:
	free(node_queue->sys_list);
	free(node_queue->sys_queue);
	free(node_queue->con_status);
	free(node_queue);

	return ERROR;

}


//int main(void)
//{
//	pthread_t ctrl_tcps;
//	int ret = 0;
//	host_info net_info;
//	void *retval;
//	//首先检测网络是否启动
//	ret = host_info_get_network_info(&net_info);
//	if(ret == ERROR){
//		printf("%s-%s-%d-network err=%d\n",__FILE__,__func__,__LINE__,ret);
//	}
//	ret = wifi_conference_sys_init();
//	printf("%s-%s-%d-ret=%d\n",__FILE__,__func__,__LINE__,ret);
//
//	//测试线程
//	pthread_create(&ctrl_tcps,NULL,control_tcp_send,NULL);
//	pthread_create(&ctrl_tcps,NULL,control_tcp_queue,NULL);
//
//	pthread_join(ctrl_tcps, &retval);
//
//
//    return SUCCESS;
//}


