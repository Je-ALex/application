/*
 *  Created on: 2017年3月14日
 *      Author: leon
 *
 *  sys_status_detect.c
 *  主要是关于系统运行状态的监测
 *  目前监测接口有
 *  1、网络状态监测，监测设备网络物理连接是否正常，主要是监测网口连接是否正常
 */
#include "wifi_sys_init.h"
#include "tcp_ctrl_device_status.h"


int rep_stat = 0;
int del_stat = 0;

/*
 * network_status_detection
 * 系统网络连接状态监测函数
 * 主要是监测系统网络接口是否正常
 * 通过监测/sys/class/net/eth0/carrier设备号的返回值判断连接是否正常
 *
 * 返回值-无
 *
 */
static void* network_status_detection(void* p)
{
	int fd = -1;
	char ethname[64] = "/sys/class/net/eth0/carrier";
	char value[3] = {0};
	frame_type type;
	int status = 0;

	pthread_detach(pthread_self());

	memset(&type,0,sizeof(frame_type));



	while(1)
	{
		fd = open(ethname,O_RDONLY);
		if(fd < 0)
	    {
			printf("%s-%s-%d eth0 open failed \n",__FILE__,__func__,__LINE__);
			pthread_exit(0);
		}

		read(fd,value,sizeof(value));

		if(strstr(value,"0"))
		{
			printf("%s-%s-%d value=%s\n",__FILE__,__func__,__LINE__,value);
			if(rep_stat == 0)
			{
				rep_stat = 1;
				//关闭本地网络线程
				wifi_sys_net_thread_deinit();

				system("udhcpc -b -i eth0 -q");

				msleep(30);
				//本地状态上报qt
				del_stat = 1;

				status = WIFI_MEETING_EVENT_NET_ERROR;
				tcp_ctrl_report_enqueue(&type,status);
			}
		}else{
			if(del_stat)
			{
				wifi_sys_net_thread_init();
				del_stat = 0;
			}
			rep_stat = 0;
		}

		close(fd);
		sleep(3);
	}

	pthread_exit(0);

}

/*
 * sys_status_detect_init
 * 系统状态监测函数
 *
 * 初始化系统状态监测线程
 * 返回值：
 * 成功-SUCCESS
 * 失败-ERROR
 */
int sys_status_detect_init()
{
	pthread_t nsdi;

	int ret = 0;

	ret = pthread_create(&nsdi,NULL,network_status_detection,NULL);

	return ret;
}
