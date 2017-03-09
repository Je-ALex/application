/*
 * uart_snd_effect.c
 *
 *  Created on: 2016年12月2日
 *      Author: leon
 */


#include "wifi_sys_init.h"
#include "sys_uart_init.h"


int sys_uart_read_data(Pudev pdev,char* msg,int bytes,Ptimeout time_out)
{
	fd_set rd;
	struct timeval timeout;

    FD_ZERO(&rd);
    FD_SET(pdev->handle,&rd);

    int num,ret;

    timeout.tv_sec = time_out->to_sec;
    timeout.tv_usec = time_out->to_usec;

    ret = select (pdev->handle+1,&rd,NULL,NULL,&timeout);

    switch (ret)
    {
        case 0:
           // printf("no data input within  1s.\n");
        	 return 0;
        case -1:
            perror("select");
            return 0;
        default:
        {
    		if((num = read(pdev->handle,msg,bytes)) > 0)

    		return num;
        }

    }

    return SUCCESS;
}

int sys_uart_write_data(Pudev pdev,char* msg,int len)
{
	int i;

	write(pdev->handle, msg, len);

	printf("uart send:\n");
	for(i=0;i<len;i++)
		printf("0x%02x ",msg[i]);
	printf("\n");

	return SUCCESS;
}

static speed_t sys_uart_get_baudrate(int baudrate)
{
	switch(baudrate) {

	case 0: return B0;
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	case 460800: return B460800;
	case 500000: return B500000;
	case 576000: return B576000;
	case 921600: return B921600;
	case 1000000: return B1000000;
	case 1152000: return B1152000;
	case 1500000: return B1500000;
	case 2000000: return B2000000;
	case 2500000: return B2500000;
	case 3000000: return B3000000;
	case 3500000: return B3500000;
	case 4000000: return B4000000;

	default: return -1;
	}
}

int set_uart_opt(Pudev pdev)
{
    int fl=fcntl(pdev->handle,F_GETFL);
    if(fl<0)
    {
        perror("fcntl");
        return SETOPTERRO;
    }
    if(fcntl(pdev->handle,F_SETFL,0))
    {
        perror("fcntl");
        return SETOPTERRO;
    }

	return SUCCESS;

}
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
int sys_uart_init(Pudev pdev)
{
	speed_t speed;
	tcflag_t flag;

    int ret;
    struct termios old_cfg,new_cfg;

    pthread_mutex_init(&pdev->umutex,NULL);
    sem_init(&pdev->uart_sem, 0, 0);

	speed = sys_uart_get_baudrate(pdev->params.baudrate);

	pdev->handle = open(pdev->dev,O_RDWR | O_NONBLOCK| O_NOCTTY | O_NDELAY);
//	pdev->handle = open(pdev->dev,O_RDWR | O_NOCTTY );
    if(pdev->handle<0)
    {
        perror(pdev->dev);
        return ERROR;
    }


    //获取当前状态的配置参数
    ret = tcgetattr(pdev->handle,&old_cfg);
    if(ret)
    {
        perror(pdev->dev);
        return ERROR;
    }
    //初始化新参数
    bzero(&new_cfg,sizeof(new_cfg));

    //数据位
	switch(pdev->params.cs) {

	case UART_CS_5:
		flag = CS5;
		break;
	case UART_CS_6:
		flag = CS6;
		break;
	case UART_CS_7:
		flag = CS7;
		break;
	case UART_CS_8:
		flag = CS8;
		break;
	default:
		return ERROR;
	}

	new_cfg.c_cflag = (speed | flag);
	//停止位
	switch(pdev->params.stop) {

	case UART_STOP_ONE:
		new_cfg.c_cflag &= ~CSTOPB;
		break;
	case USRT_STOP_TWO:
		new_cfg.c_cflag |= CSTOPB;
		break;
	default:
		return ERROR;
	}
	//校验位
	switch(pdev->params.parity) {

	case UART_PAR_NONE:
		new_cfg.c_iflag = IGNPAR;
		break;
	case UART_PAR_EVEN:
		new_cfg.c_cflag |= PARENB;
		new_cfg.c_cflag &= ~PARODD;
		new_cfg.c_iflag |= INPCK;
		break;
	case UART_PAR_ODD:
		new_cfg.c_cflag |= (PARENB | PARODD);
		new_cfg.c_iflag |= INPCK;
		break;
	default:
		return ERROR;
	}

    new_cfg.c_cflag |= (CLOCAL | CREAD);
//    new_cfg.c_cflag = speed|CS8|CLOCAL|CREAD;
    new_cfg.c_oflag = 0;

    tcflush(pdev->handle,TCIFLUSH);

    ret = tcsetattr(pdev->handle,TCSANOW,&new_cfg);
    if(ret)
    {
        perror(pdev->dev);
        return ERROR;
    }
    tcgetattr(pdev->handle,&old_cfg);

//    set_uart_opt(pdev);

    return SUCCESS;
}



