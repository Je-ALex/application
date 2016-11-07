/*
 * File   : main.c
 * 应用入口函数
 * 日期：2016年7月
 */


#include "audio_tcp_server.h"

#include "../../inc/tcp_ctrl_queue.h"

#define  QUEUE_LINE  12
#define  SOURCE_PORT 8080



snd_data_format playback;
WAVContainer wav;


Plinkqueue* queue;

sem_t report_sem[10];

typedef struct{

	int status;
	int len;
	char* msg;

}audio_frame,*Paudio_frame;

typedef struct{

	sem_t audio_sem[10];

}audio_sem;

audio_sem sem;

/*
 * tcp_ctrl_report_enqueue
 * 上报数据数据发送队列
 * 将消息数据送入发送队列等待发送
 *
 * in:
 * @Pframe_type 数据信息结构体
 * @value 事件信息
 *
 * out:
 * @NULL
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
static int tcp_audio_enqueue(sem_t* en_sem,Plinkqueue tcp_send_queue,Paudio_frame data)
{

	Paudio_frame tmp;
	tmp = (Paudio_frame)malloc(sizeof(audio_frame));
	memset(tmp,0,sizeof(audio_frame));


	tmp->msg = data->msg;
	tmp->len = data->len;

	enter_queue(tcp_send_queue,tmp);

//	tmp->len = tmp->len * 8 / playback.bits_per_frame;
//
//	printf("audio_tcp_mix recv_num=%d\n",tmp->len);
//
//
//	playback.data_buf = tmp->msg;
//	SNDWAV_WritePcm(&playback, tmp->len);

	sem_post(en_sem);

	return SUCCESS;
}

/*
 * tcp_ctrl_tpsend_dequeue
 * tcp控制模块的数据出队列
 * 将消息数据送入发送队列等待发送
 *
 * in/out:
 * @Ptcp_send 数据信息结构体
 *
 * return：
 * @ERROR
 * @SUCCESS
 */
static int tcp_audio_dequeue(sem_t* out_sem,Plinkqueue tcp_send_queue,Paudio_frame event_tmp)
{

	int ret = -1;

	Plinknode node;
	Paudio_frame tmp;

//	printf("%s\n",__func__);

	sem_wait(out_sem);

	ret = out_queue(tcp_send_queue,&node);

	if(ret == 0)
	{
		tmp = node->data;

		event_tmp->len = tmp->len;
		event_tmp->msg = tmp->msg;

//		tmp->len = tmp->len * 8 / playback.bits_per_frame;
//
//		printf("audio_tcp_mix recv_num=%d\n",tmp->len);
//
//		playback.data_buf = tmp->msg;
//		SNDWAV_WritePcm(&playback, tmp->len);

		free(tmp->msg);
		free(tmp);
		free(node);
	}else{
		return ERROR;
	}

	return SUCCESS;
}


static int set_swparams(snd_data_format *sndpcm, snd_pcm_sw_params_t *swparams)
{
        int err;

        snd_pcm_sw_params_alloca(&swparams);
        /* get the current swparams */
        err = snd_pcm_sw_params_current(sndpcm->handle, swparams);
        if (err < 0) {
                printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* start the transfer when the buffer is almost full: */
        /* (buffer_size / avail_min) * avail_min */
        err = snd_pcm_sw_params_set_start_threshold(sndpcm->handle, swparams, (sndpcm->buffer_size / sndpcm->period_size) * sndpcm->period_size);
        if (err < 0) {
                printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
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
        return 0;
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
	snd_pcm_sw_params_t *swparams;

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
static void audio_data_mix(char** sourseFile,char *objectFile,int number,int length)
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
static int audio_udp_init(int* port)
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
	addr_serv.sin_port = htons(*port);

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
/*
 * 音频处理线程
 * 包含了tcp的维护，audio音频源的播放
 * 2016 alex
 */
static void* audio_recv_thread(void* p)
{

	struct timeval start,stop;
	timetime diff;

	int ret;
	int socket_fd;

	int port = (int)p;

	int queue_num = port-SOURCE_PORT;
	if(queue_num > 0)
		queue_num = queue_num/2;

	struct sockaddr_in fromAddr;
	int fromLen = sizeof(fromAddr);

	printf("port--%d\n",port);

	socket_fd = audio_udp_init(&port);

	if(socket_fd < 0)
	{
		printf("audio_udp_init failed\n");
		pthread_exit(0);
	}

	char* buffer;

	Paudio_frame data;
	data = malloc(sizeof(audio_frame));


    while(1){

//    	gettimeofday(&start,0);

    	buffer = malloc(playback.chunk_bytes);
    	if(buffer == NULL)
    	{
    		printf("buffer failed\n");
    		pthread_exit(0);
    	}


		playback.recv_num = recvfrom(socket_fd,buffer,playback.chunk_bytes,
						0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);


    	data->msg = buffer;
    	data->len = playback.recv_num;

    	printf("%d:recv_num = %d\n",port,playback.recv_num);

		tcp_audio_enqueue(&sem.audio_sem[queue_num],queue[queue_num],data);

//		gettimeofday(&stop,0);
//		time_substract(&diff,&start,&stop);
//		printf("%s : %d s,%d ms,%d us\n",__func__,(int)diff.time.tv_sec,
//				diff.ms,(int)diff.time.tv_usec);

    }

    free(data);
    pthread_exit(0);
}

static void* audio_data_mix_thread(void* data)
{
	struct timeval start,stop;
	timetime diff;

    Paudio_frame* tmp;
    tmp = malloc(sizeof(Paudio_frame)*8);

	Plinknode* node;
	node = malloc(sizeof(Plinknode)*8);

    char** recvbuf ;
    recvbuf = malloc(sizeof(char*)*8);
	memset(recvbuf,0,sizeof(recvbuf));

	int i,j;
	int length = 0;
    int ret;

    while(1)
    {

    	j=0;

    	/*
    	 * i的范围会根据控制模块设置获得
    	 * 发言的顺序是从低到高排序
    	 */
    	for(i=0;i<2;i++)
    	{
    		sem_wait(&sem.audio_sem[i]);
    		ret = out_queue(queue[i],&node[i]);

     		if(ret == 0)
			{
				tmp[i] = node[i]->data;
				recvbuf[j] = tmp[i]->msg;
				/*
				 * 算法找出最大值
				 */
				if(length < tmp[i]->len)
					length = tmp[i]->len;

				j++;

			}else{
				break;
			}
    	}

		if(j>0)
		{
//	    	gettimeofday(&start,0);

			audio_data_mix(recvbuf,playback.data_buf,j,length);

			length = length * 8 / playback.bits_per_frame;

			printf("audio_tcp_mix recv_num=%d\n",length);

			audio_module_data_write(&playback,length);

//			gettimeofday(&stop,0);
//			time_substract(&diff,&start,&stop);
//			printf("%s : %d s,%d ms,%d us\n",__func__,(int)diff.time.tv_sec,
//					diff.ms,(int)diff.time.tv_usec);

			for(i=0;i<j;i++)
			{
				if(tmp[i]->msg != NULL)
				{
					free(tmp[i]->msg);
				}
				if(tmp[i]!= NULL)
				{
					free(tmp[i]);
				}
				if(node[i] != NULL)
				{
					free(node[i]);
				}

			}

		}

    }

}

#define SEM_NUM 	8
#define QUEUE_NUM 	8
#define THREAD_NUM 	8

/*
 * 音频模块初始化线程
 * 初步设想此模块是上电后就会初始化，创建最大支持人数的线程数
 *
 * 信号量需要八组，队列需要八组
 */
int main(int argc, char *argv[])
{
	pthread_t  th_a= 0;

	void *retval;
	int ret = 0;
	int i = 0;

	printf("audio %s \n",__func__);

    /*
     * 音频设备初始化及音源参数初始化
     */
    ret = audio_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("audio_tcp_snd_init error [%d]/n",ret);

    }

	/*
	 * 信号量初始化
	 */
	for(i=0;i<SEM_NUM;i++){

		ret = sem_init(&sem.audio_sem[i], 0, 0);
		if (ret != 0)
		{
		 perror("initialization failed\n");
		}
	}
	/*
	 * 队列初始化
	 */
	queue = malloc(sizeof(Plinkqueue)*QUEUE_NUM);
	for(i=0;i<QUEUE_NUM;i++)
	{
		queue[i] = queue_init();

	}

    int port = SOURCE_PORT;
    for(i=0;i<THREAD_NUM;i++)
    {

    	ret = pthread_create(&th_a, NULL, audio_recv_thread, (void*)port);
    	if (ret != 0)
    	{
    	 perror ("creat audio thread error");
    	}
    	port +=2;
    }
	/*
	 * 混音线程
	 */
	ret = pthread_create(&th_a, NULL, audio_data_mix_thread,NULL);
	if (ret != 0)
	{
	 perror ("creat audio thread error");
	}

	pthread_join(th_a, &retval);


Err:
	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

	return 0;


}
