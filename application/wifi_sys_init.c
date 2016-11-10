/*
 * wifi_sys_init.c
 *
 *  Created on: 2016年11月10日
 *      Author: leon
 */

#include "tcp_ctrl_server.h"
#include "scanf_md_udp.h"


extern sys_info sys_in;


/*
 * wifi_sys_signal_init
 * 系统状态与信号初始化
 *
 * 1、设备控制模块中TCP数据收发互斥锁
 * 2、本地状态上报QT消息队列互斥锁
 * 3、设备控制模块消息进出队列互斥锁
 * 4、设备控制模块状态上报上位机互斥锁
 *
 *
 */
int wifi_sys_signal_init()
{
	int i;
	int ret = -1;

	for(i=0;i<=PC_REP_MUTEX;i++)
	{
		pthread_mutex_init(&sys_in.sys_mutex[i], NULL);
	}

//	pthread_mutex_init(&sys_in.ctrl_tcp_mutex, NULL);
//	pthread_mutex_init(&sys_in.lreport_queue_mutex, NULL);
//	pthread_mutex_init(&sys_in.ctcp_queue_mutex, NULL);
//	pthread_mutex_init(&sys_in.pc_queue_mutex, NULL);

	for(i=0;i<PC_REP_MUTEX+1;i++)
	{
		ret = sem_init(&sys_in.sys_sem[i], 0, 0);
		if (ret != 0)
		{
			 perror("local_report_sem initialization failed");
			 return ERROR;
		}
	}

//	ret = sem_init(&sys_in.local_report_sem, 0, 0);
//	if (ret != 0)
//	{
//		 perror("local_report_sem initialization failed");
//		 return ERROR;
//	}
//	ret = sem_init(&sys_in.pc_queue_sem, 0, 0);
//	if (ret != 0)
//	{
//		 perror("Semaphore initialization failed");
//		 return ERROR;
//	}
//	ret = sem_init(&sys_in.tpsend_queue_sem, 0, 0);
//	if (ret != 0)
//	{
//		 perror("Semaphore tpsend_queue_sem initialization failed");
//		 return ERROR;
//	}


	return SUCCESS;
}


/*
 * init_control_tcp_module
 * 设备控制模块初始化接口
 * 此接口可以作为API接口供外部使用
 *
 */
int wifi_conference_sys_init()
{
	pthread_t ctrl_udp;
	pthread_t ctrl_tcpr;
	pthread_t ctrl_tcps;
	pthread_t queue_t;

	void*status;
	int ret;

	printf("%s,%d\n",__func__,__LINE__);

	wifi_sys_signal_init();

	/*
	 * 设备发现之UDP广播初始化
	 */
	ret = pthread_create(&ctrl_udp,NULL,wifi_sys_ctrl_udp_server,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_udp_server failed\n");
		 return ERROR;
	}
	/*
	 * TCP控制模块数据接收线程
	 */
	ret = pthread_create(&ctrl_tcpr,NULL,wifi_sys_ctrl_tcp_recv,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_udp_server failed\n");
		 return ERROR;
	}
	/*
	 * TCP控制模块数据发送线程
	 */
	ret = pthread_create(&ctrl_tcps,NULL,wifi_sys_ctrl_tcp_send,NULL);
	if (ret != 0)
	{
		 printf("wifi_sys_ctrl_udp_server failed\n");
		 return ERROR;
	}

	//测试线程
//	pthread_create(&tcp_send,NULL,control_tcp_send,NULL);
//	pthread_create(&queue_t,NULL,control_tcp_queue,NULL);

//	pthread_join(udp_t,&status);
	pthread_join(ctrl_tcps,&status);
//	pthread_join(tcp_send,&status);
//	pthread_join(queue_t,&status);

	return SUCCESS;
}





int main(void)
{

	wifi_conference_sys_init();


    return 0;
}
