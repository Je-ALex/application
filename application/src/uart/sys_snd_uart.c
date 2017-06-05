/*
 * sys_snd_uart.c
 *
 *  Created on: 2016年12月8日
 *      Author: leon
 *
 *
 * 音频接口使用说明
 * 主机使用UART与DSP模块的串口进行通讯，主要功能时实现音效的设置
 * DSP端串口使用说明
 * 1.第一步必须先发送一个是否需要回复的指令
 * 是否需要回复命令
 * 不需要回复命令: A5AC00000000000000000000
 * 需要回复命令:  A5AC00000001000000000001
 *
 * 2.AFC(反馈抑制器)控制逻辑
 * AFC直通     A5AC0F000101000000000011
 * AFC非直通    A5AC0F000100000000000010
 *
 * 3.ANC(噪声消除)控制逻辑
 * 直通    A5AC1B00010100000000001D
 * 非直通    A5AC1B00010000000000001C
 * 这个是会保持为上次的状态，所以在等级3后，先发送直通，再发个等级1.这样显示才会和按键的匹配正常
 * 等级1    A5AC1B00020100000000001E
 * 等级2    A5AC1B000203000000000020
 * 等级3    A5AC1B000206000000000023
 */

#include "sys_uart_init.h"


udev pdev_snd;
#define SND_MODE 	9
#define NUM 		12

int old_mode = 0;


char init_off[NUM]	=	{0xa5,0xac,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
char init_on[NUM]	=	{0xa5,0xac,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x01};

char afc_off[NUM]	=	{0xa5,0xac,0x0f,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x11};
char afc_on[NUM]	=	{0xa5,0xac,0x0f,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x10};

char anc_off[NUM]	= 	{0xa5,0xac,0x1b,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x1d};
char anc_on[NUM]	=	{0xa5,0xac,0x1b,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x1c};

char anc1_on[NUM]	=	{0xa5,0xac,0x1b,0x00,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x1e};
char anc2_on[NUM]	= 	{0xa5,0xac,0x1b,0x00,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x20};
char anc3_on[NUM]	= 	{0xa5,0xac,0x1b,0x00,0x02,0x06,0x00,0x00,0x00,0x00,0x00,0x23};

char snd_effect_mode[SND_MODE][NUM] =
{
		{0xa5,0xac,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},//no need reply
		{0xa5,0xac,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x01},//need reply

		{0xa5,0xac,0x0f,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x11},//afc off
		{0xa5,0xac,0x0f,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x10},//afc on

		{0xa5,0xac,0x1b,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x1d},//anc off
		{0xa5,0xac,0x1b,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x1c},//anc on

		{0xa5,0xac,0x1b,0x00,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x1e},//anc 1 on
		{0xa5,0xac,0x1b,0x00,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x20},//anc 2 on
		{0xa5,0xac,0x1b,0x00,0x02,0x06,0x00,0x00,0x00,0x00,0x00,0x23} //anc 3 on
};

char reply_type[SND_MODE][NUM] =
{
		//no need reply
		{},
		//need reply
		{0xA5,0xAC,0x00,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x00,0x02},
		//afc off
		{0xA5,0xAC,0x0F,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x00,0x12},
		//afc on
		{0xA5,0xAC,0x0F,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x11},
		//anc off
		{0xA5,0xAC,0x1B,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x00,0x1E},
		//anc on
		{0xA5,0xAC,0x1B,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x1D},
		//anc 1 on
		{0xA5,0xAC,0x1B,0x00,0x02,0x01,0x00,0x00,0x01,0x00,0x00,0x1F},
		//anc 2 on
		{0xA5,0xAC,0x1B,0x00,0x02,0x03,0x00,0x00,0x01,0x00,0x00,0x21},
		//anc 3 on
		{0xA5,0xAC,0x1B,0x00,0x02,0x06,0x00,0x00,0x01,0x00,0x00,0x24}
};


/*
 * uart_snd_effect_set
 * 音效设置接口函数
 *
 * 设置即将设置的音效参数
 *
 * 输入参数：
 * @value 音效状态采用位表示，分别8421对应的位状态来表示音效状态，为开的则对应为为1，否则为0
 *
 * 返回值：
 * 成功-SUCCESS
 * 失败-ERROR
 *
 */
int uart_snd_effect_set(int value)
{
	char buf[24] = {0};
	int ret = 0;

	int afc_mode,anc_mode = 0;


	/*
	 * AFC控制部分
	 */
	afc_mode = old_mode & WIFI_MEETING_EVT_SEFC_AFC_MODE;

	if(afc_mode != (value & WIFI_MEETING_EVT_SEFC_AFC_MODE))
	{
		afc_mode = value & WIFI_MEETING_EVT_SEFC_AFC_MODE;

		switch(afc_mode)
		{
		case WIFI_MEETING_EVT_SEFC_VALUE_ON:
			afc_mode = WIFI_MEETING_EVT_SEFC_AFC_ON;
			break;
		case WIFI_MEETING_EVT_SEFC_VALUE_OFF:
			afc_mode = WIFI_MEETING_EVT_SEFC_AFC_OFF;
			break;
		}

		switch(afc_mode)
		{
		case WIFI_MEETING_EVT_SEFC_AFC_OFF:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_AFC_OFF],NUM);
			break;
		case WIFI_MEETING_EVT_SEFC_AFC_ON:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_AFC_ON],NUM);
			break;
		}
		ret = sys_uart_write_data(&pdev_snd,buf,NUM);
	}


	/*
	 * ANC控制部分
	 */
	anc_mode = old_mode & WIFI_MEETING_EVT_SEFC_ANC_MODE;

	if(anc_mode != (value & WIFI_MEETING_EVT_SEFC_ANC_MODE))
	{
		anc_mode = value & WIFI_MEETING_EVT_SEFC_ANC_MODE;

		if(anc_mode == WIFI_MEETING_EVT_SEFC_VALUE_OFF)
		{
			anc_mode = WIFI_MEETING_EVT_SEFC_ANC_OFF;
		}else if(anc_mode == WIFI_MEETING_EVT_SEFC_ANC_BIT_ONE){
			anc_mode = WIFI_MEETING_EVT_SEFC_ANC_ON;
		}else if(anc_mode == WIFI_MEETING_EVT_SEFC_ANC_BIT_TWO){
			anc_mode = WIFI_MEETING_EVT_SEFC_ANC_TWO;
		}else if(anc_mode == WIFI_MEETING_EVT_SEFC_ANC_BIT_THREE){
			anc_mode = WIFI_MEETING_EVT_SEFC_ANC_THREE;
		}

		switch(anc_mode)
		{
		case WIFI_MEETING_EVT_SEFC_ANC_OFF:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_ONE],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			msleep(50);
			memset(buf,0,sizeof(buf));
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_OFF],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			break;
		case WIFI_MEETING_EVT_SEFC_ANC_ON:
		case WIFI_MEETING_EVT_SEFC_ANC_ONE:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_ON],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			msleep(50);
			memset(buf,0,sizeof(buf));
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_ONE],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			break;
		case WIFI_MEETING_EVT_SEFC_ANC_TWO:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_ON],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			msleep(50);
			memset(buf,0,sizeof(buf));
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_TWO],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			break;
		case WIFI_MEETING_EVT_SEFC_ANC_THREE:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_ON],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			msleep(50);
			memset(buf,0,sizeof(buf));
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_THREE],NUM);
			ret = sys_uart_write_data(&pdev_snd,buf,NUM);
			break;
		default:
			printf("%s-%s-%d not legal value\n",__FILE__,__func__,__LINE__);
			return ERROR;
		}

	}
	old_mode = value;

	sem_post(&pdev_snd.uart_sem);

	return ret;
}

/*
 * uart_snd_effect_get
 * 音频接口串口接收线程
 *
 * 主要是用于接收串口数据
 *
 */
static void* uart_snd_effect_get(void* p)
{
	timeout time_out;
	char buf[NUM] = {0};
	int i,ret = 0;

	time_out.to_sec = 0;
	time_out.to_usec = 100;

	pthread_detach(pthread_self());
	while(1)
	{
		sem_wait(&pdev_snd.uart_sem);
//		pthread_mutex_lock(&pdev.umutex);
		ret = sys_uart_read_data(&pdev_snd,buf,NUM,&time_out);
//		pthread_mutex_unlock(&pdev.umutex);

		if(ret)
		{
			printf("uart recv:\n");
			for(i=0;i<ret;i++)
				printf("0x%02x ",buf[i]);
			printf("\n");

			memset(buf,0,sizeof(buf));

		}

	}
	pthread_exit(0);
}

/*
 * wifi_sys_uaudio_init
 * 音频控制接口初始化函数
 *
 * 1、串口配置初始化，使用UART4，配置为8N1模式，波特率为57600
 * 2、串口接收线程初始化
 *
 * 返回值：
 * 成功-SUCCESS
 * 失败-ERROR
 *
 */
int wifi_sys_uaudio_init()
{
	pthread_t snd_uart;
	int ret = 0;
	char buf[24] = {0};

	pdev_snd.dev = "/dev/ttymxc3";
	pdev_snd.params.baudrate = UART_B_57600;
	pdev_snd.params.cs = UART_CS_8;
	pdev_snd.params.stop = UART_STOP_ONE;
	pdev_snd.params.parity = UART_PAR_NONE;


	ret = sys_uart_init(&pdev_snd);
	if(ret)
	{
		printf("%s-%s-%d sys_uart_init failed\n",__FILE__,__func__,__LINE__);
		return ret;
	}
	memcpy(buf,snd_effect_mode[1],NUM);

	ret = sys_uart_write_data(&pdev_snd,buf,NUM);


	pthread_create(&snd_uart,NULL,uart_snd_effect_get,NULL);

	return ret;
}
