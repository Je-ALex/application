/*
 * lib_test.c
 *
 *  Created on: 2016年12月17日
 *      Author: leon
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


#include "tcp_ctrl_api.h"

void* control_tcp_queue(void* p)
{

	Prun_status event_tmp;
	event_tmp = (Prun_status)malloc(sizeof(run_status));

	while(1)
	{
		unit_info_get_running_status(event_tmp);
		printf("%s-%s-fd:%d,id:%d,seat:%d,value:%d\n",
				__FILE__,__func__,event_tmp->socket_fd,
				event_tmp->id,event_tmp->seat,event_tmp->value);

	}

	free(event_tmp);
}

int main()
{
	void *retval;
	pthread_t ctrl_tcps;

	wifi_conference_sys_init();

	pthread_create(&ctrl_tcps,NULL,control_tcp_queue,NULL);
	printf( "%s \n", __func__);

	pthread_join(ctrl_tcps, &retval);

	return 0;
}
