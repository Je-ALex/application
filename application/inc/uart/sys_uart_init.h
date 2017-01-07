/*
 * sys_uart_init.h
 *
 *  Created on: 2016年12月8日
 *      Author: leon
 */

#ifndef INC_UART_SYS_UART_INIT_H_
#define INC_UART_SYS_UART_INIT_H_

#include "../audio/audio_server.h"

typedef enum{
	UART_B_9600=9600,
	UART_B_19200=19200,
	UART_B_38400=38400,
	UART_B_57600=57600,
	UART_B_115200=115200,
}baudrate;

typedef enum{
	UART_CS_5,
	UART_CS_6,
	UART_CS_7,
	UART_CS_8,
}data_b;
typedef enum{
	UART_STOP_ONE,
	USRT_STOP_TWO,
}stop_b;
typedef enum{
	UART_PAR_NONE,
	UART_PAR_EVEN,
	UART_PAR_ODD,
}parity_b;

typedef struct {

	int baudrate;
	int cs;
	int stop;
	int parity;

}uart_params,*Puart_params;

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

int uart_snd_effect_init();
int uart_snd_effect_set(int value);

int sys_uart_video_set(unsigned short id,int value);
int sys_video_uart_init();














/*
 * sys_uart_init
 * 系统串口初始化
 *
 * @Pudev设备信息
 *
 * 返回值：
 * 设备号
 *
 */
int sys_uart_init(Pudev pdev);

int sys_uart_read_data(Pudev pdev,char* msg,int bytes,Ptimeout time_out);

int sys_uart_write_data(Pudev pdev,char* msg,int len);






#endif /* INC_UART_SYS_UART_INIT_H_ */
