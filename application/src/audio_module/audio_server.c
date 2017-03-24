
#include "audio_params_init.h"
#include "audio_ring_buf.h"
#include "tcp_ctrl_device_status.h"

//pthread_t  th_a[MAX_SPK_NUM] = {0};
//pthread_t  mix_th = 0;
pthread_t  local_cap = 0;

snd_data_format playback;
snd_data_format capture;

WAVContainer wav;
Paudio_queue* rqueue;
Paudio_queue squeue;

audio_signal sem;

unsigned int snd_ts= 0;

/*
 * audio_set_swparams
 * 音频软件参数设置
 */
static int audio_set_swparams(snd_data_format *sndpcm, snd_pcm_sw_params_t *swparams)
{
        int err;

        snd_pcm_sw_params_alloca(&swparams);
        /* get the current swparams */
        err = snd_pcm_sw_params_current(sndpcm->handle, swparams);
        if (err < 0) {
                printf("Unable to determine current swparams for playback: %s\n",
                		snd_strerror(err));
                return err;
        }
        /* start the transfer when the buffer is almost full: */
        /* (buffer_size / avail_min) * avail_min */
        err = snd_pcm_sw_params_set_start_threshold(sndpcm->handle, swparams,
        		(sndpcm->buffer_size / sndpcm->period_size) * sndpcm->period_size);
        if (err < 0) {
                printf("Unable to set start threshold mode for playback: %s\n",
                		snd_strerror(err));
                return err;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is
         *  enabled (aka interrupt like style processing) */
        err = snd_pcm_sw_params_set_avail_min(sndpcm->handle, swparams,sndpcm->period_size);
        if (err < 0) {
                printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
                return err;
        }

        /* write the parameters to the playback device */
        err = snd_pcm_sw_params(sndpcm->handle, swparams);
        if (err < 0) {
                printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
                return err;
        }
        return SUCCESS;
}

/*
 * audio_snd_init_capture
 * 音频模块的声卡和alsa参数初始化
 */
static int audio_snd_init_capture(Psnd_data_format capt,PWAVContainer wav)
{
//	int err;
	char *devicename = "hw:0,0";
//	snd_pcm_sw_params_t *swparams = NULL;


//	if (snd_output_stdio_attach(&capt->log, stderr, 0) < 0) {
//		 printf("Error snd_output_stdio_attach\n");
//		 return ERROR;
//	 }

	/*
	 * 打开pcm设备
	 */
	if (snd_pcm_open(&capt->handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) {
		printf("Error snd_pcm_open [ %s]\n", devicename);
		return ERROR;
	}

	if (audio_snd_params_init(capt, wav) < 0) {
		printf("Error set_snd_pcm_params\n");
		return ERROR;
	}

//    if ((err = audio_set_swparams(capt, swparams)) < 0) {
//        printf("Setting of swparams failed: %s\n", snd_strerror(err));
//            return ERROR;
//    }
//	if (set_params(capt, wav) < 0) {
//		printf("Error set_snd_pcm_params\n");
//		return ERROR;
//	}

//	snd_pcm_dump(capt->handle, capt->log);

	return SUCCESS;
}

/*
 * audio_snd_init
 * 音频模块的声卡和alsa参数初始化
 *
 * @snd_data_format alsa参数结构体
 * @WAVContainer_t 音频数据参数结构体
 *
 * 返回值：
 * 成功或失败
 */
static int audio_snd_init(Psnd_data_format play,PWAVContainer wav)
{
	int err;
	char *devicename = "hw:0,0";
	snd_pcm_sw_params_t *swparams = NULL;


	if (snd_output_stdio_attach(&play->log, stderr, 0) < 0) {
		 printf("Error snd_output_stdio_attach\n");
		 return ERROR;
	 }
	/*
	 * 打开pcm设备
	 */
	if (snd_pcm_open(&play->handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		printf("Error snd_pcm_open [ %s]\n", devicename);
		return ERROR;
	}

	if (audio_snd_params_init(play, wav) < 0) {
		printf("Error set_snd_pcm_params\n");
		return ERROR;
	}
    if ((err = audio_set_swparams(play, swparams)) < 0) {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
            return ERROR;
    }
	snd_pcm_dump(play->handle, play->log);

	return SUCCESS;
}


/*
 * audio_data_mix
 * 归一化混音算法
 *
 * @sourseFile音频数据源
 * @objectFile混音后数据
 * @number混音通道数
 * @length音频源数据长度
 *
 * 返回值：
 * 无
 */
static void audio_data_mix( char** sourseFile, char* objectFile,
		int number,int length)
{


#if 1
		const short MAX = 0X7FFF;
		const short MIN = -0X8000;

		double f=1;
		int output;
		int i = 0,j = 0;
		int temp;

//		short _data[3] = {0};
//		int m,n;
//		m=n=0;

		for (i=0;i<length/2;i++)
		{
			temp=0;

			for (j=0;j<number;j++)
			{
				//只进行最多三路混音
//				if(tmp_data < *(short*)(*(sourseFile+j)+i*2))
//				{
//					tmp_data = *(short*)(*(sourseFile+j)+i*2);
//					_data[m] = tmp_data;
//					n++;
//				}

				temp+=*(short*)(*(sourseFile+j)+i*2);
			}
//			n = bubbleSort2((short*)(*sourseFile + i*2),number,_data);
//
//			for(m=0;m<n;m++)
//			{
//				temp+=_data[m];
//			}

			output=(int)(temp*f);

			if (output>MAX)
			{
				f=(double)MAX/(double)(output);
				output=MAX;
			}
			if (output<MIN)
			{
				f=(double)MIN/(double)(output);
				output=MIN;
			}
			if (f<1)
			{
				f+=((double)1-f)/(double)64;
			}

			*(short*)(objectFile+i*2)=(short)output;
		}

#else
	    int  MAX = 0X7FFFFFFF;
	    int  MIN = -0X80000000;

	    double f;
	    int output;
	    int i = 0,j = 0;


	    f=1.0;

		for (i=0;i<length/2;i++)
		{
			int temp = 0;

			for (j=0;j<number;j++)
			{
				temp += *(int*)(*(sourseFile+j)+i*4);
			}
			output = (int)(temp*f);

			if (output>MAX)
			{
				f=(double)MAX/(double)(output);
//				printf("output>MAX\n");
				output=MAX;
			}
			if (output<MIN)
			{
				f=(double)MIN/(double)(output);
//				printf("output<MIN\n");
				output=MIN;
			}

			if (f<1)
			{
				f+=((double)1-f)/(double)48;
			}

			*(int*)(objectFile+i*4) = (int)output;
		}

#endif

}

/*
 * audio_udp_init
 * 音频模块udp服务端初始化
 *
 * @port 服务端端口号
 *
 * 返回值：
 * 套接字号
 */
static int audio_udp_init(int port)
{
	int socket_fd = -1;
	/*
	 *configuration the socket  parameter
	 */
	struct sockaddr_in addr_serv;

	socket_fd = socket(AF_INET,SOCK_DGRAM,0);
	if(socket_fd < 0)
	{
		 printf("init socket failed\n");

	}

	memset(&addr_serv,0,sizeof(addr_serv));
	addr_serv.sin_family=AF_INET;
	addr_serv.sin_addr.s_addr=htonl(INADDR_ANY);
	addr_serv.sin_port = htons(port);

	/*
	 * bind the socket
	 */
	if(bind(socket_fd,(struct sockaddr *)&(addr_serv),
			sizeof(struct sockaddr_in)) == -1)
	{
		printf("%s-%s-%d bind failed\n",__FILE__,__func__,__LINE__);
	}

//	int nZero=0;
//	setsockopt(socket_fd,SOL_SOCKET,SO_RCVBUF,(char*)&nZero,sizeof(int));

	return socket_fd;
}

/* audio_data_local_capture
 * 本地采集
 *
 */
static void* audio_data_local_capture(void* p)
{
	size_t frame_size;
	Paudio_frame data[RS_NUM];
	Paudio_frame mixdata[RS_NUM];

    Paudio_frame tmp[10];

    char** recvbuf = NULL;
    recvbuf = malloc(sizeof(char*)*(MAX_SPK_NUM+1));
	memset(recvbuf,0,sizeof(char*)*(MAX_SPK_NUM+1));

	volatile int i,j,q;

	volatile int length = 0;
	volatile int frame_len = 0;

	pthread_detach(pthread_self());

	for(i=0;i<RS_NUM;i++)
	{
		data[i] = malloc(sizeof(audio_frame));
		if(data[i] == NULL)
		{
			printf("data failed\n");
			pthread_exit(0);
		}
	}
	char* buffer[RS_NUM];
	for(i=0;i<RS_NUM;i++)
	{
		buffer[i] = malloc(capture.chunk_bytes);
		if(buffer[i] == NULL)
		{
			printf("buffer failed\n");
			pthread_exit(0);
		}
		memset(buffer[i],0,capture.chunk_bytes);
	}

	for(i=0;i<RS_NUM;i++)
	{
		mixdata[i] = malloc(sizeof(audio_frame));
		if(mixdata[i] == NULL)
		{
			printf("data failed\n");
			pthread_exit(0);
		}
	}
	char* mixbuffer[RS_NUM];
	for(i=0;i<RS_NUM;i++)
	{
		mixbuffer[i] = malloc(playback.chunk_bytes);
		if(mixbuffer[i] == NULL)
		{
			printf("buffer failed\n");
			pthread_exit(0);
		}
		memset(mixbuffer[i],0,capture.chunk_bytes);
	}

	i=j=0;

	while (1) {

		frame_size = capture.chunk_bytes * 8 / capture.bits_per_frame;

		audio_data_read(&capture, buffer[i], frame_size);
#if 0
		data[i]->msg = buffer[i];
		data[i]->len = capture.chunk_bytes;
		audio_enqueue(rqueue[0],data[i]);

//		 printf("data[%d]->len = %d\n",i,data[i]->len);
//		/*
//		 * play
//		 */
//		playback.data_buf = buffer[i];
//		audio_module_data_write(&playback, frame_size);

		//start
#else
		recvbuf[0] = buffer[i];
		length = capture.chunk_bytes;


		for(q=1;q<=conf_status_get_spk_offset();q++)
		{
			tmp[q] = audio_dequeue(rqueue[q]);
			if(tmp[q] != NULL)
			{
//				frame_len = tmp[q]->len * 8 / playback.bits_per_frame;
//				audio_data_write(&playback,tmp[q]->msg,frame_len);
				recvbuf[j] = tmp[q]->msg;
				/*
				 * 算法找出最大值
				 */
				if(length < tmp[q]->len)
					 length = tmp[q]->len;

				j++;
			}
		}

		audio_data_mix(recvbuf,mixbuffer[i],j,length);

		frame_len = length * 8 / playback.bits_per_frame;

		audio_data_write(&playback,mixbuffer[i],frame_len);

		j=1;

//			if(conf_status_get_snd_brd())
//			{
//				/*
//				 * 送入发送队列
//				 */
//				memcpy(sendbuffer[i],mixbuffer[i],length);
				mixdata[i]->msg = mixbuffer[i];
				mixdata[i]->len = length;
				audio_enqueue(squeue,mixdata[i]);

//			}
#endif
		//end


		i++;
		if(i==RS_NUM)
			i=0;

		usleep(1);
	}
	pthread_exit(0);


}



int tcp_ctrl_data_char2int(unsigned int* value,unsigned char* buf)
{

	unsigned int tmp = 0;
	tmp = (buf[0] & 0xff) << 24 ;
	tmp = tmp + ((buf[1] & 0xff) << 16);
	tmp = tmp + ((buf[2] & 0xff) << 8);
	tmp = tmp + (buf[3] & 0xff);

	*value = tmp;

	return SUCCESS;
}

/* audio_recv_thread
 * 音频处理线程
 * 共八个线程，用来接收音频数据
 * 默认发言人数为一人，为主席单元，默认为第一音频个端口
 * 在语音接收中，采用先进后出的模式，默认发言人数为一人
 *
 */
void* audio_recv_thread(void* p)
{

//	struct timeval start,stop;
//	timetime diff;

	struct sockaddr_in fromAddr;
	int fromLen = sizeof(fromAddr);
	int socket_fd;

	int i;
	int port = (int)p;
	int queue_num = port-AUDIO_RECV_PORT;

	pthread_detach(pthread_self());

	queue_num = queue_num/2 + 1;

	printf("port--%d,num--%d\n",port,queue_num);
	socket_fd = audio_udp_init(port);

	if(socket_fd < 0)
	{
		printf("audio_udp_init failed\n");
		pthread_exit(0);
	}

	Paudio_frame data[RS_NUM];
	for(i=0;i<RS_NUM;i++)
	{
		data[i] = malloc(sizeof(audio_frame));
		if(data[i] == NULL)
		{
			printf("data failed\n");
			pthread_exit(0);
		}
	}

	char* buffer[RS_NUM];
	for(i=0;i<RS_NUM;i++)
	{
		buffer[i] = malloc(playback.chunk_bytes);
		if(buffer[i] == NULL)
		{
			printf("buffer failed\n");
			pthread_exit(0);
		}
		memset(buffer[i],0,playback.chunk_bytes);
	}


	i=0;
	unsigned int i_ts = 0;

    while(1)
    {

//		playback.recv_num = recvfrom(socket_fd,buffer[i],playback.chunk_bytes+12,
//								0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);
//
//		if(conf_status_get_spk_offset() - queue_num + 1)
//		{
//			if(conf_status_get_spk_buf_offset(queue_num) < 9)
//			{
//				conf_status_set_spk_buf_offset(queue_num,i);
//
//			}else
//			{
//				if(buffer[i][0] == 'D' && buffer[i][1] == 'S' &&
//					buffer[i][2] == 'D' && buffer[i][3] == 'S')
//				{
//					tcp_ctrl_data_char2int(&i_ts,&buffer[i][4]);
//
//					if(i_ts > conf_status_get_spk_timestamp(queue_num))
//					{
//						data[i]->msg = &buffer[i][12];
//						data[i]->len = playback.recv_num - 12;
//		//		    	printf("%d:recv_num = %d\n",port,playback.recv_num);
//						audio_enqueue(rqueue[queue_num],data[i]);
//						conf_status_set_spk_timestamp(queue_num,i_ts);
//
//					}
//
//				}
//
////				sem_post(&sem.audio_recv_sem[queue_num]);
//			}
//			i++;
//			if(i==RS_NUM)
//				i=0;
//		}

    	/*
    	 * test
    	 */
		if(sys_net_status_get() == 1)
			goto ERR;
#if 1
		playback.recv_num = recvfrom(socket_fd,buffer[i],playback.chunk_bytes,
								0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);

//		printf("%d:recv_num = %d\n",port,playback.recv_num);

		data[i]->msg = buffer[i];
		data[i]->len = playback.recv_num;
		audio_enqueue(rqueue[queue_num],data[i]);


		i++;
		if(i==RS_NUM)
			i=0;

#else
//	 	sem_wait(&sem.audio_recv_sem[queue_num]);

		playback.recv_num = recvfrom(socket_fd,playback.data_buf,playback.chunk_bytes,
								0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);

		playback.recv_num = playback.recv_num * 8 / playback.bits_per_frame;

		audio_module_data_write(&playback,playback.recv_num);

#endif
    }

ERR:
	for(i=0;i<RS_NUM;i++)
	{
		if(data[i] != NULL)
		    free(data[i]);

		if(buffer[i] != NULL)
		    free(buffer[i]);
	}
	close(socket_fd);

    pthread_exit(0);

}


/*
 * audio_data_mix_thread
 * 混音模块
 *
 */
static void* audio_data_mix_thread(void* p)
{

//	struct timeval start,stop;
//	timetime diff;


    Paudio_frame tmp[9];

    char** recvbuf ;
    recvbuf = malloc(sizeof( char*)*(MAX_SPK_NUM+1));
	memset(recvbuf,0,sizeof( char*)*(MAX_SPK_NUM+1));

	volatile int i,j;

	volatile int length = 0;
	volatile int frame_len = 0;


	pthread_detach(pthread_self());

	Paudio_frame data[RS_NUM];
	for(i=0;i<RS_NUM;i++)
	{
		data[i] = malloc(sizeof(audio_frame));
		if(data[i] == NULL)
		{
			printf("data failed\n");
			pthread_exit(0);
		}
	}

	char* buffer[RS_NUM];
	for(i=0;i<RS_NUM;i++)
	{
		buffer[i] = malloc(playback.chunk_bytes);
		if(buffer[i] == NULL)
		{
			printf("buffer failed\n");
			pthread_exit(0);
		}
	}
	int mix_i = 0;
	j=0;

	while(1)
    {

//		if(sys_debug_get_switch())
//		{
//			gettimeofday(&start,0);
//		}

		for(i=0;i<=conf_status_get_spk_offset();i++)
		{
//    		sem_wait(&sem.audio_recv_sem[i]);
			tmp[i] = audio_dequeue(rqueue[i]);
			if(tmp[i] != NULL)
			{
				recvbuf[j] = tmp[i]->msg;
				/*
				 * 算法找出最大值
				 */
				if(length < tmp[i]->len)
					length = tmp[i]->len;

				j++;
			}
//			else{
//				//printf("queue[%d] %s-%s-%d queue empty\n",i,__FILE__,__func__,__LINE__);
//			}

		}

//		if(j)
//		{
//			audio_data_mix(recvbuf,playback.data_buf,j,length);
//
//			frame_len = length * 8 / playback.bits_per_frame;
//
//			audio_module_data_write(&playback,frame_len);
//
//			j=0;


			audio_data_mix(recvbuf,buffer[mix_i],j,length);

			frame_len = length * 8 / playback.bits_per_frame;

			audio_data_write(&playback,buffer[mix_i],frame_len);

			j=0;

//			if(conf_status_get_snd_brd())
//			{
//				memcpy(buffer[m],playback.data_buf,length);
//				/*
//				 * 送入发送队列
//				 */
			data[mix_i]->msg = buffer[mix_i];
			data[mix_i]->len = length;
			audio_enqueue(squeue,data[mix_i]);

			mix_i++;
			if(mix_i==RS_NUM)
				mix_i=0;
//			}

//			if(sys_debug_get_switch())
//			{
//				gettimeofday(&stop,0);
//				time_substract(&diff,&start,&stop);
//				printf("audio_data_mix-%d s,%d ms,%d us\n",(int)diff.time.tv_sec,
//						diff.ms,(int)diff.time.tv_usec);
//			}

//		}
		usleep(1);
    }
    pthread_exit(0);
}


/* audio_send_thread
 * 音频下发线程
 *
 */
void* audio_send_thread(void* p)
{
	int sock;
	int ret = 0;
	int opt = 1;

    Paudio_frame send_msg;

	printf("%s,%d\n",__func__,__LINE__);

	pthread_detach(pthread_self());
	/*
	 * 创建socke套接字
	 */
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0)
	{
		 printf("init socket failed\n");
		 pthread_exit(0);
	}
	/*
	 * 设置套接字的属性
	 */
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			(char *)&opt, sizeof(opt));
	if(ret)
	{
		printf("setsockopt error...\n");
		pthread_exit(0);
	}
	/*
	 * 配置udp服务端参数
	 */
	struct sockaddr_in addr_serv;
	memset(&addr_serv,0,sizeof(addr_serv));
	addr_serv.sin_family=AF_INET;
	addr_serv.sin_addr.s_addr=htonl(INADDR_BROADCAST);
	//INADDR_BROADCAST
	addr_serv.sin_port = htons(AUDIO_SEND_PORT);

	/*
	 * 绑定套接字号
	 */
	if(bind(sock,(struct sockaddr *)&(addr_serv),
			sizeof(struct sockaddr_in)) == -1)
	{
		printf("%s-%s-%d bind failed\n",__FILE__,__func__,__LINE__);
		pthread_exit(0);
	}
	while(1){

		if(sys_net_status_get() == 1)
			goto ERR;
		if(conf_status_get_snd_brd())
		{
			send_msg = audio_dequeue(squeue);
			if(send_msg != NULL)
			{
				ret = sendto(sock,send_msg->msg,send_msg->len,0,
						(struct sockaddr*)&addr_serv,sizeof(addr_serv));
			}
			usleep(1);
		}else{
			msleep(2);
		}

	}
ERR:
	close(sock);
	pthread_exit(0);
}

/*
 * wifi_sys_audio_init
 * 音频模块初始化函数
 * 初步设想此模块是上电后就会初始化
 * 主要有声卡的初始化，信号量初始化，队列初始化，音频接收线程，混音线程
 *
 */
int wifi_sys_audio_init()
{

//	void *retval;
	int ret = 0;
	int i = 0;


	memset(&capture, 0x0, sizeof(snd_data_format));
	memset(&playback, 0x0, sizeof(snd_data_format));
	/*
	 * 配置音频文件
	 */
	if (audio_format_init(&wav) < 0) {
		printf("Error WAV_Parse\n");
		return ERROR;
	}
    /*
     * 音频设备拾音设置
     */
    ret = audio_snd_init_capture(&capture, &wav);
    if(ret < 0)
    {
		printf("%s-%s-%d audio_snd_init_capture error\n",__FILE__,__func__,__LINE__);
		return ERROR;
    }

    /*
     * 音频设备播放设置
     */
    ret = audio_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("%s-%s-%d audio_snd_init error\n",__FILE__,__func__,__LINE__);
		return ERROR;
    }

	/*
	 * 信号量初始化
	 */
	for(i=0;i<=MAX_SPK_NUM;i++){

		ret = sem_init(&sem.audio_mix_sem[i], 0, 0);
		if (ret != 0)
		{
		 perror("initialization failed\n");
		}

		ret = sem_init(&sem.audio_recv_sem[i], 0, 0);
		if (ret != 0)
		{
		 perror("initialization failed\n");
		}
	}

	/*
	 * 接收数据队列初始化
	 * 大小为10个，1个本地+9个UDP
	 */
	rqueue = (Paudio_queue*)malloc(sizeof(Paudio_queue)*(MAX_SPK_NUM+2));
	for(i=0;i<=(MAX_SPK_NUM+1);i++)
	{
		rqueue[i] = audio_queue_init(RS_NUM);
	}
	/*
	 * 发送数据队列初始化
	 */
	squeue = (Paudio_queue)malloc(sizeof(audio_queue));
	squeue = audio_queue_init(RS_NUM);

	/*
	 * 本地音频采集
	 */
	ret = pthread_create(&local_cap, NULL, audio_data_local_capture,NULL);
	if (ret != 0)
	{
		perror ("creat audio_data_local_capture error");
	}

	/*
	 * 语音接收线程
	 */
//    int port = AUDIO_RECV_PORT;
//    for(i=0;i<=MAX_SPK_NUM;i++)
//    {
//    	ret = pthread_create(&th_a[i], NULL, audio_recv_thread, (void*)port);
//    	if (ret != 0)
//    	{
//    	 perror ("creat audio_recv_thread error");
//    	}
//    	port +=2;
//    }

	/*
	 * 混音线程
	 */
//	pthread_t  mix_th = 0;
//	ret = pthread_create(&mix_th, NULL, audio_data_mix_thread,NULL);
//	if (ret != 0)
//	{
//		perror ("creat audio_data_mix_thread error");
//	}

//	/*
//	 * 语音发送线程
//	 */
//	ret = pthread_create(&mix_th, NULL, audio_send_thread,NULL);
//	if (ret != 0)
//	{
//		perror ("audio_send_thread error");
//	}


//	pthread_join(th_a[0], &retval);

//	snd_output_close(playback.log);
//	if (playback.data_buf) free(playback.data_buf);
//	if (playback.handle) snd_pcm_close(playback.handle);
//
//	snd_output_close(capture.log);
//	if (capture.data_buf) free(capture.data_buf);
//	if (capture.handle) snd_pcm_close(capture.handle);

	return SUCCESS;

}













