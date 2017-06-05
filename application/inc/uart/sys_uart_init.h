/*
 * sys_uart_init.h
 *
 *  Created on: 2016年12月8日
 *      Author: leon
 */

#ifndef INC_UART_SYS_UART_INIT_H_
#define INC_UART_SYS_UART_INIT_H_


#include "wifi_sys_init.h"

/*
 * 波特率
 */
typedef enum{
	UART_B_9600=9600,
	UART_B_19200=19200,
	UART_B_38400=38400,
	UART_B_57600=57600,
	UART_B_115200=115200,
}baudrate;

/*
 * 数据位
 */
typedef enum{
	UART_CS_5,
	UART_CS_6,
	UART_CS_7,
	UART_CS_8,
}data_b;

/*
 * 停止位
 */
typedef enum{
	UART_STOP_ONE,
	USRT_STOP_TWO,
}stop_b;

/*
 * 校验位
 */
typedef enum{
	UART_PAR_NONE,
	UART_PAR_EVEN,
	UART_PAR_ODD,
}parity_b;

/*
 * 串口参数结构体
 */
typedef struct {

	int baudrate;
	int cs;
	int stop;
	int parity;

}uart_params,*Puart_params;

/*
 * 串口设备参数结构体
 */
typedef struct {

	char* dev;
	int handle;
	uart_params params;
	pthread_mutex_t umutex;
	sem_t uart_sem;

}udev,*Pudev;

typedef struct{
	int to_sec;
	int to_msec;
	int to_usec;
}timeout,*Ptimeout;



typedef enum {

	WIFI_MEETING_EVT_SEFC_AFC_MODE = 0X01,
	WIFI_MEETING_EVT_SEFC_ANC_MODE = 0X0e,


	WIFI_MEETING_EVT_SEFC_VALUE_ON = 0X01,
	WIFI_MEETING_EVT_SEFC_VALUE_OFF = 0X00,

	WIFI_MEETING_EVT_SEFC_AFC_BIT = 0X01,
	WIFI_MEETING_EVT_SEFC_ANC_BIT_ONE = 0X02,
	WIFI_MEETING_EVT_SEFC_ANC_BIT_TWO = 0X04,
	WIFI_MEETING_EVT_SEFC_ANC_BIT_THREE = 0X08,

	WIFI_MEETING_EVT_SEFC_NO_REPLY = 0,
	WIFI_MEETING_EVT_SEFC_NEED_REPLY,

	WIFI_MEETING_EVT_SEFC_AFC_OFF ,
	WIFI_MEETING_EVT_SEFC_AFC_ON ,

	WIFI_MEETING_EVT_SEFC_ANC_OFF ,
	WIFI_MEETING_EVT_SEFC_ANC_ON ,

	WIFI_MEETING_EVT_SEFC_ANC_ONE ,
	WIFI_MEETING_EVT_SEFC_ANC_TWO ,
	WIFI_MEETING_EVT_SEFC_ANC_THREE ,

}event_snd_effect;





int wifi_sys_uaudio_init();
int uart_snd_effect_set(int value);

int sys_uart_video_set(unsigned short id,int value);
int wifi_sys_uvedio_init();



int sys_uart_init(Pudev pdev);

int sys_uart_read_data(Pudev pdev,char* msg,int bytes,Ptimeout time_out);

int sys_uart_write_data(Pudev pdev,char* msg,int len);



#endif /* INC_UART_SYS_UART_INIT_H_ */
