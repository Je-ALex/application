/*
 * sys_snd_uart.c
 *
 *  Created on: 2016年12月8日
 *      Author: leon
 */

#include "sys_uart_init.h"


/*
是否需要回复命令
不需要回复命令: A5AC00000000000000000000
需要回复命令:  A5AC00000001000000000001

反馈抑制器/AFC
AFC直通     A5AC0F000101000000000011
AFC非直通    A5AC0F000100000000000010

噪声消除/ANC
  直通    A5AC1B00010100000000001D
非直通    A5AC1B00010000000000001C
这个是会保持为上次的状态，所以在等级3后，先发送直通，再发个等级1.这样显示才会和按键的匹配正常z
 *等级1    A5AC1B00020100000000001E
 *等级2    A5AC1B000203000000000020
 *等级3    A5AC1B000206000000000023

*/

udev pdev;
#define SND_MODE 	9
#define NUM 		12



//char* init_off=	"A5AC00000000000000000000";
//char* init_on	=	"A5AC00000001000000000001";
//char* afc_on 	= 	"A5AC0F000100000000000010";
//char* afc_off	= 	"A5AC0F000101000000000011";
//char* anc_off = 	"A5AC1B00010100000000001D";
//char* anc_on	=  	"A5AC1B00010000000000001C";
//char* anc1_on = 	"A5AC1B00020100000000001E";
//char* anc2_on = 	"A5AC1B000203000000000020";
//char* anc3_on = 	"A5AC1B000206000000000023";

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
 * 发送控制信号
 *
 *
 */
int old_mode = 0;
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
		ret = sys_uart_write_data(&pdev,buf,NUM);
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
			break;
		case WIFI_MEETING_EVT_SEFC_ANC_ON:
		case WIFI_MEETING_EVT_SEFC_ANC_ONE:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_ON],NUM);
			break;
		case WIFI_MEETING_EVT_SEFC_ANC_TWO:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_TWO],NUM);
			break;
		case WIFI_MEETING_EVT_SEFC_ANC_THREE:
			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_THREE],NUM);
			break;
		default:
			printf("not legal value\n");
			return ERROR;
		}

		if(anc_mode == WIFI_MEETING_EVT_SEFC_ANC_OFF)
		{
			ret = sys_uart_write_data(&pdev,buf,NUM);

			msleep(50);
			memset(buf,0,sizeof(buf));

			memcpy(buf,snd_effect_mode[WIFI_MEETING_EVT_SEFC_ANC_OFF],NUM);
			ret = sys_uart_write_data(&pdev,buf,NUM);
		}else{

			ret = sys_uart_write_data(&pdev,buf,NUM);
		}

	}
	old_mode = value;

	sem_post(&pdev.uart_sem);

	return ret;
}

void* uart_snd_effect_get(void* p)
{
	timeout time_out;
	char buf[NUM] = {0};
	int i,ret = 0;

	time_out.to_sec = 0;
	time_out.to_usec = 100;

	pthread_detach(pthread_self());
	while(1)
	{
		sem_wait(&pdev.uart_sem);
//		pthread_mutex_lock(&pdev.umutex);
		ret = sys_uart_read_data(&pdev,buf,NUM,&time_out);
//		pthread_mutex_unlock(&pdev.umutex);

		if(ret)
		{
			for(i=0;i<ret;i++)
				printf("0x%02x ",buf[i]);
			printf("\n");

			memset(buf,0,sizeof(buf));

		}else{


		}

	}
	pthread_exit(0);
}

int uart_snd_effect_init()
{
	pthread_t snd_uart;
	int ret = 0;
	char buf[24] = {0};

	pdev.dev = "/dev/ttymxc3";

	pdev.params.baudrate = UART_B_57600;
	pdev.params.cs = UART_CS_8;
	pdev.params.stop = UART_STOP_ONE;
	pdev.params.parity = UART_PAR_NONE;


	ret = sys_uart_init(&pdev);
	if(ret)
	{
		printf("sys_uart_init failed\n");
		return ret;
	}
	memcpy(buf,init_on,NUM);

	ret = sys_uart_write_data(&pdev,buf,NUM);


	pthread_create(&snd_uart,NULL,uart_snd_effect_get,NULL);

	return ret;
}