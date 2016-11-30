
#include "audio_params_init.h"
#include "audio_ring_buf.h"


FILE* file;


snd_data_format playback;
WAVContainer wav;
Paudio_queue* queue;
audio_signal sem;

extern Pglobal_info node_queue;

static int set_swparams(snd_data_format *sndpcm, snd_pcm_sw_params_t *swparams)
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

	/*
	 * 配置音频文件
	 */
	if (audio_format_init(wav) < 0) {
		printf("Error WAV_Parse /n");
		return ERROR;
	}

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
    if ((err = set_swparams(play, swparams)) < 0) {
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
static void audio_data_mix(char** sourseFile,unsigned char *objectFile,int number,int length)
{

    int const MAX=32767;
    int const MIN=-32768;

    double f=1;
    int output;
    int i = 0,j = 0;
    int temp;

	 for (i=0;i<length/2;i++)
		{
			temp=0;

			for (j=0;j<number;j++)
			{
				temp+=*(short*)(*(sourseFile+j)+i*2);
			}
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
				f+=((double)1-f)/(double)32;
			}

			*(short*)(objectFile+i*2)=(short)output;
		}


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
		printf("bind error...\n");
	}


	return socket_fd;
}
/* audio_recv_thread
 * 音频处理线程
 * 共八个线程，用来接收音频数据
 * 默认发言人数为一人，为主席单元，默认为第一音频个端口
 * 在语音接收中，采用先进后出的模式，默认发言人数为一人
 *
 */
static void* audio_recv_thread(void* p)
{

	struct timeval start,stop;
	timetime diff;


	int i;
	int socket_fd;

	int port = (int)p;
	int queue_num = port-AUDIO_RECV_PORT;
	if(queue_num > 0)
		queue_num = queue_num/2;

	struct sockaddr_in fromAddr;
	int fromLen = sizeof(fromAddr);

	printf("port--%d\n",port);

	socket_fd = audio_udp_init(port);

	if(socket_fd < 0)
	{
		printf("audio_udp_init failed\n");
		pthread_exit(0);
	}

	Paudio_frame data[RECV_NUM];
	for(i=0;i<RECV_NUM;i++)
	{
		data[i] = malloc(sizeof(audio_frame));
		if(data[i] == NULL)
		{
			printf("data failed\n");
			pthread_exit(0);
		}
	}

	char* buffer[RECV_NUM];
	for(i=0;i<RECV_NUM;i++)
	{
		buffer[i] = malloc(playback.chunk_bytes);
		if(buffer[i] == NULL)
		{
			printf("buffer failed\n");
			pthread_exit(0);
		}
	}

	i=0;
    while(1){

		if(node_queue->con_status->debug_sw){

			gettimeofday(&start,0);
		}
		playback.recv_num = recvfrom(socket_fd,buffer[i],playback.chunk_bytes,
								0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);

		if(conf_info_get_spk_num() - queue_num)
		{
			data[i]->msg = buffer[i];
			data[i]->len = playback.recv_num;
//	    	printf("%d:recv_num = %d\n",port,playback.recv_num);
			audio_enqueue(queue[queue_num],data[i]);

			i++;
			if(i==10)
				i=0;
//			sem_wait(&sem.audio_recv_sem[queue_num]);
		}

//		playback.recv_num = playback.recv_num * 8 / playback.bits_per_frame;
//		playback.data_buf = buffer[i];
//		audio_module_data_write(&playback, playback.recv_num);


		if(node_queue->con_status->debug_sw){
			gettimeofday(&stop,0);
			time_substract(&diff,&start,&stop);
			printf("%d : %d s,%d ms,%d us\n",socket_fd,(int)diff.time.tv_sec,
					diff.ms,(int)diff.time.tv_usec);
//			fprintf(file,"%d : %d s,%d ms,%d us\n",socket_fd,(int)diff.time.tv_sec,
//								diff.ms,(int)diff.time.tv_usec);
		}
    }

	for(i=0;i<RECV_NUM;i++)
	{
		if(data[i] != NULL)
		    free(data[i]);

		if(buffer[i] != NULL)
		    free(buffer[i]);
	}

    pthread_exit(0);

}

/*
 * audio_data_mix_thread
 * 混音模块
 *
 */
static void* audio_data_mix_thread(void* data)
{
	struct timeval start,stop;
	timetime diff;

    Paudio_frame tmp[8];

    char** recvbuf ;
    recvbuf = malloc(sizeof(char*)*8);
	memset(recvbuf,0,sizeof(recvbuf));

	int i,j;
	int length = 0;


    while(1)
    {

		if(node_queue->con_status->debug_sw){

			gettimeofday(&start,0);
		}

    	for(i=0;i<conf_info_get_spk_num();i++)
    	{
    		tmp[i] = audio_dequeue(queue[i]);
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
			else{
				//printf("queue[%d] %s-%s-%d queue empty\n",i,__FILE__,__func__,__LINE__);

			}
    	}

		if(j)
		{
			/*
			 * 一个人的时候，就不用进行混音
			 */
			if(j == 1)
			{
				playback.data_buf = (unsigned char*)recvbuf[0];
			}else{
				audio_data_mix(recvbuf,playback.data_buf,j,length);
			}

			length = length * 8 / playback.bits_per_frame;
			audio_module_data_write(&playback,length);

			j=0;

			if(node_queue->con_status->debug_sw){
				gettimeofday(&stop,0);
				time_substract(&diff,&start,&stop);
				printf("audio_data_mix-%d s,%d ms,%d us\n",(int)diff.time.tv_sec,
						diff.ms,(int)diff.time.tv_usec);
			}
		}

    }

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
	pthread_t  th_a[MAX_SPK_NUM] = {0};
	pthread_t  mix_th = 0;

	void *retval;
	int ret = 0;
	int i = 0;

    /*
     * 音频设备初始化及音源参数初始化
     */
    ret = audio_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("%s-%s-%d audio_snd_init error\n",__FILE__,__func__,__LINE__);
    }

	/*
	 * 信号量初始化
	 */
	for(i=0;i<MAX_SPK_NUM;i++){

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

	file = fopen(LOG_NAME,"w+");
	/*
	 * 队列初始化
	 */
	queue = (Paudio_queue*)malloc(sizeof(Paudio_queue)*MAX_SPK_NUM);
	for(i=0;i<MAX_SPK_NUM;i++)
	{
		queue[i] = audio_queue_init(RECV_NUM);
	}
	/*
	 * 混音线程
	 */
	ret = pthread_create(&mix_th, NULL, audio_data_mix_thread,NULL);
	if (ret != 0)
	{
		perror ("creat audio thread error");
	}
	/*
	 * 语音接收线程
	 */
    int port = AUDIO_RECV_PORT;
    for(i=0;i<MAX_SPK_NUM;i++)
    {
    	ret = pthread_create(&th_a[i], NULL, audio_recv_thread, (void*)port);
    	if (ret != 0)
    	{
    	 perror ("creat audio thread error");
    	}
    	port +=2;
    }

    while(1){

    }

//	pthread_join(th_a, &retval);

	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

	return SUCCESS;


}
