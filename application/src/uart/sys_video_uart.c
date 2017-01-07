/*
 * sys_video_uart.c
 *
 *  Created on: 2017年1月6日
 *      Author: leon
 */

#include "sys_uart_init.h"
udev pdev;


int gpio_pull_high()
{
	usleep(10000);
	char command[128];
	FILE *fp=NULL;
	snprintf(command,sizeof(command),"echo 0 > /sys/class/leds/RS485/brightness");
	fp=popen(command,"r");
	if(fp==NULL)
    {
	    printf("gpio_pull_high error\n");
	    return -1;
	}
	fclose(fp);
	return 0;
}

int gpio_pull_low()
{
	usleep(10000);
	char command[128];
	FILE *fp=NULL;
	snprintf(command,sizeof(command),"echo 1 > /sys/class/leds/RS485/brightness");
	fp=popen(command,"r");
	if(fp==NULL)
    {
	    printf("gpio_pull_low error\n");
	    return -1;
	}
	fclose(fp);
	return 0;
}

int sys_uart_video_set(unsigned short id,int value)
{
	int ret = 0;

//	char buf[3] = {0xb0,0x00,0x00};
//
//	buf[1] = value;
//	buf[2] = id;
	char buf[2] = {0xfd,0xdd};
	gpio_pull_high();
	ret = sys_uart_write_data(&pdev,buf,sizeof(buf));
	gpio_pull_low();

	return ret;
}





int sys_video_uart_init()
{
	int ret = 0;

	pdev.dev = "/dev/ttymxc1";

	pdev.params.baudrate = UART_B_9600;
	pdev.params.cs = UART_CS_8;
	pdev.params.stop = UART_STOP_ONE;
	pdev.params.parity = UART_PAR_NONE;


	ret = sys_uart_init(&pdev);
	if(ret)
	{
		printf("sys_uart_init failed\n");
		return ret;
	}

	return ret;
}
