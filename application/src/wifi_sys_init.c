/*
 * wifi_sys_init.c
 *
 *  Created on: 2016年11月10日
 *      Author: leon
 */

#include "wifi_sys_init.h"

#include "udp_ctrl_server.h"
#include "tcp_ctrl_server.h"
#include "audio_server.h"
#include "tcp_ctrl_device_status.h"
#include "sys_uart_init.h"

extern sys_info sys_in;
extern Pglobal_info node_queue;


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
 */
static int wifi_sys_val_init()
{
	int i;
	FILE* cfile;

	node_queue = (Pglobal_info)malloc(sizeof(global_info));
	memset(node_queue,0,sizeof(global_info));

	/*
	 * 队列初始化
	 */
	node_queue->sys_queue = (Plinkqueue*)malloc(sizeof(linkqueue)*(CTRL_TCP_SEND_QUEUE+1));
	memset(node_queue->sys_queue,0,sizeof(linkqueue)*(CTRL_TCP_SEND_QUEUE+1));
	for(i=0;i<=CTRL_TCP_SEND_QUEUE;i++)
	{
		node_queue->sys_queue[i] = queue_init();
		if(node_queue->sys_queue[i] == NULL)
		{
#if TCP_DBG
			printf("node_queue->sys_queue[%d] init failed\n",i);
#endif
			return INIT_QUEUE_ERR;

		}
		else{
#if TCP_DBG
			printf("node_queue->sys_queue[%d] init success\n",i);
#endif
		}
		usleep(1000);

	}
	/*
	 * 链表初始化
	 */
	node_queue->sys_list = (pclient_node*)malloc(sizeof(client_node)*(CONFERENCE_LIST+1));
	memset(node_queue->sys_list,0,sizeof(client_node)*(CONFERENCE_LIST+1));

	for(i=0;i<=CONFERENCE_LIST;i++)
	{
		node_queue->sys_list[i] = list_head_init();
		if(node_queue->sys_list[i] == NULL)
		{
#if TCP_DBG
			printf("node_queue->sys_list[%d] init failed\n",i);
#endif
			return INIT_LIST_ERR;
		}
		else{
#if TCP_DBG
			printf("node_queue->sys_list[%d] init success\n",i);
#endif
		}
		usleep(1000);

	}

	/*
	 * 全局变量初始化
	 */
	node_queue->con_status = (Pconference_status)malloc(sizeof(conference_status));
	memset(node_queue->con_status,0,sizeof(conference_status));
	//设置默认发言人数为1
	node_queue->con_status->spk_number = DEF_SPK_NUM;
	//设置默认话筒模式为FIFO
	node_queue->con_status->mic_mode = DEF_MIC_MODE;
	//摄像跟踪打开
	conf_status_set_camera_track(0);
	/*
	 * 初始化连接文本信息
	 */
	cfile = fopen(CONNECT_FILE,"w+");
	fclose(cfile);

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
	pthread_t ctrl_udp;
	pthread_t ctrl_tcpr;
	pthread_t ctrl_tcps;
	pthread_t ctrl_procs;
	pthread_t ctrl_heart;

	int ret;

	printf("%s-%s-%d-%s-%s-%s\n",__FILE__,__func__,__LINE__,
			__DATE__,__TIME__,VERSION);


	ret = wifi_sys_signal_init();
	if (ret != 0)
	{
		 printf("wifi_sys_signal_init failed\n");
		 goto ERR;
	}

	ret = wifi_sys_val_init();
	if (ret != 0)
	{
		 printf("wifi_sys_val_init failed\n");
		 goto ERR;
	}

	ret = uart_snd_effect_init();
	if (ret != 0)
	{
		 printf("uart_snd_effect_init failed\n");
		 goto ERR;
	}
	ret = sys_video_uart_init();
	if (ret != 0)
	{
		 printf("sys_video_uart_init failed\n");
		 goto ERR;
	}
	/*
	 * 设备发现之UDP广播初始化
	 */
	ret = pthread_create(&ctrl_udp,NULL,wifi_sys_ctrl_udp_server,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_udp_server failed\n");
		 goto ERR;
	}
	/*
	 * TCP控制模块数据接收线程
	 */
	ret = pthread_create(&ctrl_tcpr,NULL,wifi_sys_ctrl_tcp_recv,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_tcp_recv failed\n");
		 goto ERR;
	}
	/*
	 * TCP控制模块接收数据处理线程
	 */
	ret = pthread_create(&ctrl_procs,NULL,wifi_sys_ctrl_tcp_procs_data,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_tcp_procs_data failed\n");
		 goto ERR;
	}
	/*
	 * TCP控制模块心跳控制线程
	 */
	ret = pthread_create(&ctrl_heart,NULL,wifi_sys_ctrl_tcp_heart_state,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_tcp_heat_sta failed\n");
		 goto ERR;
	}
	/*
	 * TCP控制模块数据发送线程
	 */
	ret = pthread_create(&ctrl_tcps,NULL,wifi_sys_ctrl_tcp_send,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_tcp_send failed\n");
		 goto ERR;
	}

	//音频初始化
	ret = wifi_sys_audio_init();
	if (ret != 0)
	{
		 printf("wifi_sys_audio_init failed\n");
		 goto ERR;
	}


//	pthread_join(ctrl_tcps, &retval);

//	free(node_queue->sys_list);
//	free(node_queue->sys_queue);
//	free(node_queue->con_status);
//	free(node_queue);

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


