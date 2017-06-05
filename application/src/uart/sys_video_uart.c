/*
 * sys_video_uart.c
 *
 *  Created on: 2017年1月6日
 *      Author: leon
 *
 *  摄像跟踪处理文件
 *  主要有初始化摄像跟踪的串口配置，摄像跟踪状态发送接口，GPIO管理接口
 *  因为是RS485接口，所以在处理上，需要在发送数据时，对相应的GPIO管脚进行拉高拉低处理
 *  相应的GPIO管脚配置参考DTS文件
 *
 */

#include "sys_uart_init.h"
#include "tcp_ctrl_server.h"
#include "tcp_ctrl_device_status.h"

udev pdev_video;


int gpio_pull_high(int fd)
{

//	char command[128];
//	FILE *fp=NULL;
//	snprintf(command,sizeof(command),"echo 0 > /sys/class/leds/RS485/brightness");
//	fp=popen(command,"r");
//	if(fp==NULL)
//    {
//	    printf("gpio_pull_high error\n");
//	    return -1;
//	}
//	fclose(fp);

	int ret = 0;
	ret = write(fd,"0",1);

	return ret;
}

int gpio_pull_low(int fd)
{

//	char command[128];
//	FILE *fp=NULL;
//	snprintf(command,sizeof(command),"echo 1 > /sys/class/leds/RS485/brightness");
//	fp=popen(command,"r");
//	if(fp==NULL)
//    {
//	    printf("gpio_pull_low error\n");
//	    return -1;
//	}
//	fclose(fp);

	int ret = 0;

	ret = write(fd,"1",1);


	return ret;
}

/*
 * sys_uart_video_set
 * 摄像跟踪状态设置函数
 *
 * 判断摄像跟踪是否打开，进行参数配置后，送入串口
 *
 * 输入参数：
 * @id会议单元的ID号，@value开关值，1为开，0为关
 *
 * 返回值：
 * 成功-SUCCESS
 * 失败-ERROR
 */
int sys_uart_video_set(unsigned short id,int value)
{
	int ret = 0;
	char buf[3] = {0xb0,0x00,0x00};

	buf[1] = value;
	buf[2] = id;

	int fd = -1;
	char gpioname[64] = "/sys/class/leds/RS485/brightness";

	fd = open(gpioname,O_RDWR);
	if(fd < 0)
    {
		printf("%s-%s-%d r485 open failed \n",__FILE__,__func__,__LINE__);
	    return ERROR;
	}


//	if(conf_status_get_camera_track())
//	{
		gpio_pull_high(fd);
		usleep(100);
		ret = sys_uart_write_data(&pdev_video,buf,sizeof(buf));
		gpio_pull_low(fd);
//	}

	close(fd);

	return ret;
}




/*
 * wifi_sys_uvedio_init
 * 摄像跟踪接口初始化函数
 *
 * 初始化摄像跟踪接口参数
 * 使用UART2，波特率为9600，8N1模式
 *
 * 返回值：
 * 成功-SUCCESS
 * 失败-ERROR
 *
 */
int wifi_sys_uvedio_init()
{
	int ret = 0;

	pdev_video.dev = "/dev/ttymxc1";
	pdev_video.params.baudrate = UART_B_9600;
	pdev_video.params.cs = UART_CS_8;
	pdev_video.params.stop = UART_STOP_ONE;
	pdev_video.params.parity = UART_PAR_NONE;


	ret = sys_uart_init(&pdev_video);
	if(ret)
	{
		printf("%s-%s-%d sys_uart_init failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}

	return ret;
}
