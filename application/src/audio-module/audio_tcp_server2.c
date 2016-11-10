/*
 * File   : main.c
 * 应用入口函数
 * 日期：2016年7月
 */


#include "../../inc/audio/audio_tcp_server.h"
#include "../../inc/tcp_ctrl_queue.h"
#include "../audio-module/sndwav_common.h"

#define  QUEUE_LINE  12
#define  SOURCE_PORT 8080


/* The name of this program. */
const char* program_name;

unsigned char aodio_buf[10][2048];
unsigned int num[10] = {0};

pthread_mutex_t tcp_mutex;
sem_t report_sem1;
sem_t report_sem2;


snd_data_format playback;
WAVContainer_t wav;


Plinkqueue* queue;

typedef struct{

	int status;
	int len;
	char* msg;


}audio_frame,*Paudio_frame;

typedef struct{

	sem_t* sem;
	sem_t* report_sem[10];

}sem,*Psem;

Psem test_sem;
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
static int tcp_audio_enqueue(Psem sem,Plinkqueue tcp_send_queue,Paudio_frame data)
{

	Paudio_frame tmp;
	tmp = (Paudio_frame)malloc(sizeof(audio_frame));
	memset(tmp,0,sizeof(audio_frame));


	tmp->msg = data->msg;
	tmp->len = data->len;

//	pthread_mutex_lock(&sys_in.tcp_mutex);
	enter_queue(tcp_send_queue,tmp);
//	pthread_mutex_unlock(&sys_in.tcp_mutex);


//	tmp->len = tmp->len * 8 / playback.bits_per_frame;
//
//	printf("audio_tcp_mix recv_num=%d\n",tmp->len);
//
//
//	playback.data_buf = tmp->msg;
//	SNDWAV_WritePcm(&playback, tmp->len);

	sem_post(sem->sem);

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
static int tcp_audio_dequeue(Psem sem,Plinkqueue tcp_send_queue,Paudio_frame event_tmp)
{

	int ret = -1;

	Plinknode node;
	Paudio_frame tmp;

//	printf("%s\n",__func__);

	sem_wait(sem->sem);

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
//

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
 * snd_card init
 *
 */
static int audio_tcp_snd_init(snd_data_format* play,WAVContainer_t* wav)
{
	int err;
	char *devicename = "hw:0,0";
	snd_pcm_sw_params_t *swparams;

	/*
	 * 配置音频文件
	 */
	if (audio_parameters_init(wav) < 0) {
		fprintf(stderr, "Error WAV_Parse /n");
		return -1;
	}

	if (snd_output_stdio_attach(&play->log, stderr, 0) < 0) {
		 fprintf(stderr, "Error snd_output_stdio_attach\n");
		 return -1;
	 }
	/*
	 * 打开pcm设备
	 */
	if (snd_pcm_open(&play->handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_open [ %s]\n", devicename);
		return -1;
	}

	if (SNDWAV_SetParams(play, wav) < 0) {
		fprintf(stderr, "Error set_snd_pcm_params\n");
		return -1;
	}
    if ((err = set_swparams(play, swparams)) < 0) {
            printf("Setting of swparams failed: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
    }
	snd_pcm_dump(play->handle, play->log);


	return 0;
}

static int tcp_params_init(int port)
{
	int sock_fd,conn_fd;
	unlink("server_socket");
	struct sockaddr_in addr_serv,addr_client;
	sock_fd = socket(AF_INET,SOCK_STREAM,0);

    if(sock_fd < 0){
            perror("socket");
            exit(1);
    } else {
            printf("sock sucessful\n");
    }
    memset(&addr_serv,0,sizeof(addr_serv));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(port);
    addr_serv.sin_addr.s_addr =htonl(INADDR_ANY);

    if(bind(sock_fd,(struct sockaddr *)&addr_serv,sizeof(struct sockaddr_in))<0){
            perror("bind");
            exit(1);
    } else {
           printf("bind sucess\n");
    }
    if (listen(sock_fd,QUEUE_LINE) < 0){
            perror("listen");
            exit(1);
   } else {
           printf("listen sucessful\n");
    }
    return sock_fd;

}

#define SIZE_AUDIO_FRAME (2212)
/*
 * 归一化混音算法
 * 算法的
 */
void Mix(char** sourseFile,char *objectFile,int number,int length)
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



Plinkqueue queue_test;
/*
 * 音频处理线程
 * 包含了tcp的维护，audio音频源的播放
 * 2016 alex
 */
static void* audio_tcp_thread1(void* p)
{

	SpeexPreprocessState* pstate;


	int ret;
//	int socket_fd,conec_fd;
    socklen_t client_len;
    struct sockaddr_in addr_client;

    int recv_num;
	struct timeval start,stop;
	struct timeval start_,stop_;
	timetime diff;
	timetime recv_t;

	int socket_fd[10];


    /*
     * 音频设备初始化及音源参数初始化
     */
//    ret = audio_tcp_snd_init(&playback, &wav);
//    if(ret < 0)
//    {
//		printf("audio_tcp_snd_init error [%d]/n",ret);
//		goto Err;
//    }
//    socket_fd = tcp_params_init(SOURCE_PORT);

	/*
	 *configuration the socket  parameter
	 */
	struct sockaddr_in addr_serv;
	struct sockaddr_in fromAddr;
    int i;
    for(i=0;i<1;i++)
    {
        socket_fd[i] = socket(AF_INET,SOCK_DGRAM,0);
    	if(socket_fd[i] < 0)
    	{
    		 printf("init socket failed\n");

    	}


    	memset(&addr_serv,0,sizeof(addr_serv));
    	addr_serv.sin_family=AF_INET;
    	addr_serv.sin_addr.s_addr=htonl(INADDR_ANY);
    	addr_serv.sin_port = htons(SOURCE_PORT+i);

    	/*
    	 * bind the socket
    	 */
        if(bind(socket_fd[i],(struct sockaddr *)&(addr_serv),
        		sizeof(struct sockaddr_in)) == -1)
        {
            printf("bind error...\n");

        }
    }

    int fromLen = sizeof(fromAddr);


    char** buf;
    buf = malloc(sizeof(char*)*10);
    *buf = malloc(sizeof(char)*playback.chunk_bytes);



	char* buffer;

	Paudio_frame data;
	data = malloc(sizeof(audio_frame));

    while(1){


//    	pthread_mutex_lock(&tcp_mutex);
    	for(i=0;i<1;i++)
    	{

        	buffer = malloc(playback.chunk_bytes);

    		playback.recv_num = recvfrom(socket_fd[i],buffer,playback.chunk_bytes,
        					0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);
//        	recvbuf[i] = buf[i];

    	}
//     	printf("1recv_num = %d\n",playback.recv_num);
    	data->msg = buffer;
    	data->len = playback.recv_num;

    	printf("1recv_num = %d\n",playback.recv_num);
//		recv_num = recv_num * 8 / playback.bits_per_frame;
//    	memcpy(playback.data_buf,buffer,recv_num);
//
//		recv_num = recv_num * 8 / playback.bits_per_frame;
//		printf("1recv_num = %d\n",recv_num);
//		SNDWAV_WritePcm(&playback,recv_num);
    	test_sem.sem = test_sem->report_sem[0];
		tcp_audio_enqueue(test_sem,queue[0],data);


//		tcp_audio_dequeue(queue[0],NULL);

//		pthread_mutex_unlock(&tcp_mutex);
//		sem_post(&local_report_sem);
//		Mix(recvbuf,playback.data_buf,1,playback.chunk_bytes);
//		//初始化
//		pstate=speex_preprocess_state_init(recv_num,
//				DEFAULT_SAMPLE_RATE);
//		//设置预处理器操作选项
//	    int denoise = 1;
//	    int noiseSuppress = -25;
//	    speex_preprocess_ctl(pstate, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //降噪
//	    speex_preprocess_ctl(pstate, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //设置噪声的dB
//
//		ret = speex_preprocess_run(pstate,playback.data_buf);


//		recv_num = recvfrom(socket_fd[0],playback.data_buf,playback.chunk_bytes,
//		        					0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);
//
//		recv_num = recv_num * 8 / playback.bits_per_frame;
//		printf("1recv_num = %d\n",recv_num);
//		SNDWAV_WritePcm(&playback,recv_num);






//		usleep(10000);
//			printf("begin accept:\n");
//			msg->len = 0;
//			status = 0;
//			num[0] = 0;
//			conec_fd = accept(socket_fd,(struct sockaddr *)&addr_client,&client_len);
//            if(conec_fd < 0){
//
//                   perror("accept");
//                   continue;
//             }
//
//            printf("accept a new client,ip:%s\n",inet_ntoa(addr_client.sin_addr));
//
////            fd = open(path, O_RDWR | O_CREAT, 0665);
////            fp = fopen(path,"w+");
//        	while(1){
////              pthread_mutex_lock(&tcp_mutex);
////        		printf("begin recv:\n");
////        		gettimeofday(&start,0);
//
//        		//0=两个都为空，1=1有数据，2=2有数据
//
////        		switch(msg->status)
////        		{
////        		case 0:
////        			recv_num = recv(conec_fd,buf1,2112,0);
//////        			msg->msg1 = buf1;
////        			memcpy(msg->msg1,buf1,recv_num);
////        			msg->status = 1;
////        			break;
////        		case 1:
////        			recv_num = recv(conec_fd,buf2,2112,0);
////        			memcpy(msg->msg2,buf2,recv_num);
//////        			msg->msg2 = buf2;
////        			msg->status = 2;
////        			break;
////        		default:
////        			memset(msg,0,sizeof(audio_msg));
////        			recv_num = recv(conec_fd,buf1,2112,0);
////        			memcpy(msg->msg1,buf1,recv_num);
//////        			msg->msg1 = buf1;
////        			msg->status = 1;
////        			break;
////        		}
//
//
//        		recv_num = recv(conec_fd,playback.data_buf,playback.chunk_bytes,0);
////        		gettimeofday(&stop,0);
////        		time_substract(&recv_t,&start,&stop);
////        		printf("recv_num time : %d s,%d ms,%d us\n",(int)recv_t.time.tv_sec,recv_t.ms,(int)recv_t.time.tv_usec);
//
//
//        		if(recv_num <= 0){
//        			perror("recv");
//        			playback.recv_falg = 0;
//
////        			close(fd);
//        			break;
//        		} else {
//
//        			printf("1recv_num = %d\n",recv_num);
//
//
////        			if(write(fd,buf,1920)!=recv_num)
////					{
////					  printf("write error\n");
////					}
////          			if((ret=fwrite(buf,1,1920,fp)) != recv_num)
////					{
////					  printf("write error ret =%d\n",ret);
////					}
//        //			sndpcm->recv_falg = 1;
//        //			printf(" recv_num:%d\n",recv_num);
//        //			sndpcm->recv_unm = recv_num;
//        //			sndpcm->recv_buf = sndpcm->data_buf;
//
//
//        			/* Transfer to size frame */
////        			recv_num = recv_num * 8 / playback.bits_per_frame;
////
////        			ret = SNDWAV_WritePcm(&playback, recv_num);
////
////        			printf(" 1ret:%d\n",ret);
//
////        			tcp_audio_enqueue(msg);
//
////        			num[0] = recv_num;
////        			memcpy(aodio_buf[0],r_buf,recv_num);
////
////                	Mix(aodio_buf,buf,1,recv_num);
////
////    				memcpy(playback.data_buf,buf1,recv_num);
//
//
//
//					recv_num = recv_num * 8 / playback.bits_per_frame;
//					//初始化
//					pstate=speex_preprocess_state_init(recv_num,
//							DEFAULT_SAMPLE_RATE);
//					//设置预处理器操作选项
//					speex_preprocess_ctl(pstate,SPEEX_PREPROCESS_SET_DENOISE,1);
//
//					ret = speex_preprocess_run(pstate,playback.data_buf);
//
//					SNDWAV_WritePcm(&playback, recv_num);
//
////        			pthread_mutex_unlock(&tcp_mutex);
//        		}
//        		msg->len = recv_num;
//
////        			time_substract(&diff,&start_,&stop_);
////        			printf("ret time : %d s,%d ms,%d us\n",(int)diff.time.tv_sec,diff.ms,(int)recv_t.time.tv_usec);
//        	}
    }

    free(data);


Err:
	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

    pthread_exit(0);
}

static void* audio_tcp_thread2(void* p)
{

	SpeexPreprocessState* pstate;


	int ret;
//	int socket_fd,conec_fd;
    socklen_t client_len;
    struct sockaddr_in addr_client;

    int recv_num;
	struct timeval start,stop;
	struct timeval start_,stop_;
	timetime diff;
	timetime recv_t;

	int socket_fd[10];


    /*
     * 音频设备初始化及音源参数初始化
     */
//    ret = audio_tcp_snd_init(&playback, &wav);
//    if(ret < 0)
//    {
//		printf("audio_tcp_snd_init error [%d]/n",ret);
//		goto Err;
//    }
//    socket_fd = tcp_params_init(SOURCE_PORT);

	/*
	 *configuration the socket  parameter
	 */
	struct sockaddr_in addr_serv;
	struct sockaddr_in fromAddr;
    int i;
    for(i=0;i<1;i++)
    {
        socket_fd[i] = socket(AF_INET,SOCK_DGRAM,0);
    	if(socket_fd[i] < 0)
    	{
    		 printf("init socket failed\n");

    	}


    	memset(&addr_serv,0,sizeof(addr_serv));
    	addr_serv.sin_family=AF_INET;
    	addr_serv.sin_addr.s_addr=htonl(INADDR_ANY);
    	addr_serv.sin_port = htons(SOURCE_PORT+2);

    	/*
    	 * bind the socket
    	 */
        if(bind(socket_fd[i],(struct sockaddr *)&(addr_serv),
        		sizeof(struct sockaddr_in)) == -1)
        {
            printf("bind error...\n");

        }
    }

    int fromLen = sizeof(fromAddr);

    char** buf;
    buf = malloc(sizeof(char*)*10);
    *buf = malloc(sizeof(char)*playback.chunk_bytes);

	char* buffer;

	Paudio_frame data;
	data = malloc(sizeof(audio_frame));

    while(1){


    	for(i=0;i<1;i++)
    	{

        	buffer = malloc(playback.chunk_bytes);

    		playback.recv_num = recvfrom(socket_fd[i],buffer,playback.chunk_bytes,
        					0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);


    	}
    	data->msg = buffer;
    	data->len = playback.recv_num;

//    	printf("2recv_num = %d\n",playback.recv_num);
//		recv_num = recv_num * 8 / playback.bits_per_frame;
//    	memcpy(playback.data_buf,buffer,recv_num);
//
//		recv_num = recv_num * 8 / playback.bits_per_frame;
//		printf("1recv_num = %d\n",recv_num);
//		SNDWAV_WritePcm(&playback,recv_num);
    	test_sem.sem = test_sem->report_sem[1];
		tcp_audio_enqueue(test_sem,queue[1],data);


//		tcp_audio_dequeue(queue[0],NULL);
//		pthread_mutex_unlock(&tcp_mutex);
//		sem_post(&local_report_sem);
//		Mix(recvbuf,playback.data_buf,1,playback.chunk_bytes);
//		//初始化
//		pstate=speex_preprocess_state_init(recv_num,
//				DEFAULT_SAMPLE_RATE);
//		//设置预处理器操作选项
//	    int denoise = 1;
//	    int noiseSuppress = -25;
//	    speex_preprocess_ctl(pstate, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //降噪
//	    speex_preprocess_ctl(pstate, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //设置噪声的dB
//
//		ret = speex_preprocess_run(pstate,playback.data_buf);


//		recv_num = recvfrom(socket_fd[0],playback.data_buf,playback.chunk_bytes,
//		        					0,(struct sockaddr*)&fromAddr,(socklen_t*)&fromLen);
//
//		recv_num = recv_num * 8 / playback.bits_per_frame;
//		printf("1recv_num = %d\n",recv_num);
//		SNDWAV_WritePcm(&playback,recv_num);






//			usleep(10000);
//			printf("begin accept:\n");
//			msg->len = 0;
//			status = 0;
//			num[0] = 0;
//			conec_fd = accept(socket_fd,(struct sockaddr *)&addr_client,&client_len);
//            if(conec_fd < 0){
//
//                   perror("accept");
//                   continue;
//             }
//
//            printf("accept a new client,ip:%s\n",inet_ntoa(addr_client.sin_addr));
//
////            fd = open(path, O_RDWR | O_CREAT, 0665);
////            fp = fopen(path,"w+");
//        	while(1){
////              pthread_mutex_lock(&tcp_mutex);
////        		printf("begin recv:\n");
////        		gettimeofday(&start,0);
//
//        		//0=两个都为空，1=1有数据，2=2有数据
//
////        		switch(msg->status)
////        		{
////        		case 0:
////        			recv_num = recv(conec_fd,buf1,2112,0);
//////        			msg->msg1 = buf1;
////        			memcpy(msg->msg1,buf1,recv_num);
////        			msg->status = 1;
////        			break;
////        		case 1:
////        			recv_num = recv(conec_fd,buf2,2112,0);
////        			memcpy(msg->msg2,buf2,recv_num);
//////        			msg->msg2 = buf2;
////        			msg->status = 2;
////        			break;
////        		default:
////        			memset(msg,0,sizeof(audio_msg));
////        			recv_num = recv(conec_fd,buf1,2112,0);
////        			memcpy(msg->msg1,buf1,recv_num);
//////        			msg->msg1 = buf1;
////        			msg->status = 1;
////        			break;
////        		}
//
//
//        		recv_num = recv(conec_fd,playback.data_buf,playback.chunk_bytes,0);
////        		gettimeofday(&stop,0);
////        		time_substract(&recv_t,&start,&stop);
////        		printf("recv_num time : %d s,%d ms,%d us\n",(int)recv_t.time.tv_sec,recv_t.ms,(int)recv_t.time.tv_usec);
//
//
//        		if(recv_num <= 0){
//        			perror("recv");
//        			playback.recv_falg = 0;
//
////        			close(fd);
//        			break;
//        		} else {
//
//        			printf("1recv_num = %d\n",recv_num);
//
//
////        			if(write(fd,buf,1920)!=recv_num)
////					{
////					  printf("write error\n");
////					}
////          			if((ret=fwrite(buf,1,1920,fp)) != recv_num)
////					{
////					  printf("write error ret =%d\n",ret);
////					}
//        //			sndpcm->recv_falg = 1;
//        //			printf(" recv_num:%d\n",recv_num);
//        //			sndpcm->recv_unm = recv_num;
//        //			sndpcm->recv_buf = sndpcm->data_buf;
//
//
//        			/* Transfer to size frame */
////        			recv_num = recv_num * 8 / playback.bits_per_frame;
////
////        			ret = SNDWAV_WritePcm(&playback, recv_num);
////
////        			printf(" 1ret:%d\n",ret);
//
////        			tcp_audio_enqueue(msg);
//
////        			num[0] = recv_num;
////        			memcpy(aodio_buf[0],r_buf,recv_num);
////
////                	Mix(aodio_buf,buf,1,recv_num);
////
////    				memcpy(playback.data_buf,buf1,recv_num);
//
//
//
//					recv_num = recv_num * 8 / playback.bits_per_frame;
//					//初始化
//					pstate=speex_preprocess_state_init(recv_num,
//							DEFAULT_SAMPLE_RATE);
//					//设置预处理器操作选项
//					speex_preprocess_ctl(pstate,SPEEX_PREPROCESS_SET_DENOISE,1);
//
//					ret = speex_preprocess_run(pstate,playback.data_buf);
//
//					SNDWAV_WritePcm(&playback, recv_num);
//
////        			pthread_mutex_unlock(&tcp_mutex);
//        		}
//        		msg->len = recv_num;
//
////        			time_substract(&diff,&start_,&stop_);
////        			printf("ret time : %d s,%d ms,%d us\n",(int)diff.time.tv_sec,diff.ms,(int)recv_t.time.tv_usec);
//        	}
    }

    free(data);


Err:
	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

    pthread_exit(0);
}

static void* audio_tcp_mix(void* data)
{

    int ret;
    int len[10];

    Paudio_frame tmp;
    tmp = malloc(sizeof(Paudio_frame));

    Paudio_frame tmp1;
    tmp1 = malloc(sizeof(Paudio_frame));


    char** recvbuf ;
    recvbuf = malloc(sizeof(char*)*10);
	memset(recvbuf,0,sizeof(recvbuf));

	int i,j;
	int length;
	int m,n;


    while(1)
    {
    	tcp_audio_dequeue(test_sem->report_sem[0],queue[0],tmp);

    	tcp_audio_dequeue(test_sem->report_sem[1],queue[1],tmp1);

    	if(tmp->len > tmp1->len)
    		;
    	else
    		tmp->len = tmp1->len;

    	recvbuf[0] = tmp->msg;
    	recvbuf[1] = tmp1->msg;
//    	printf("audio_tcp_mix recv_num=%d\n",tmp->len);
//    	for(i=0;i<10;i++)
//    	{
//        	ret = tcp_audio_dequeue(queue[i],&tmp[i]);
//        	if(ret == ERROR)
//        		break;
//
//        	recvbuf[i] = tmp[i]->msg;
//        	len[i] = tmp[i]->len;
//    	}

    	Mix(recvbuf,playback.data_buf,2,tmp->len);

    	tmp->len = tmp->len * 8 / playback.bits_per_frame;
//    	playback.data_buf = tmp->msg;
		printf("audio_tcp_mix recv_num=%d\n",tmp->len);

		ret = SNDWAV_WritePcm(&playback, tmp->len);

		free(tmp->msg);
		free(tmp1->msg);

    }

	free(tmp);
	free(tmp1);


}


/*
 * 函数：主进程
 * 说明：
 * 1、系统运行主进程
 * 2、主要分为控制类数据线程（tcp）、音频数据线程（tcp）、设备发现线程（udp）
 * 串行控制线程（uart）、错误监测线程
 * 2016 alex
 */
int main(int argc, char *argv[])
{
	pthread_t  th_a= 0;
	void *retval;
	int ret = 0;
	int i = 0;
	printf("audio %s \n",__func__);

	pthread_mutex_init(&tcp_mutex, NULL);


	ret = sem_init(test_sem->report_sem[0], 0, 0);
	if (ret != 0)
	{
	 perror("local_report_sem initialization failed");
	}
	ret = sem_init(test_sem->report_sem[1], 0, 0);
	if (ret != 0)
	{
	 perror("local_report_sem initialization failed");
	}

	queue = malloc(sizeof(Plinkqueue)*10);
	queue[0] = queue_init();
	queue[1] = queue_init();
    /*
     * 音频设备初始化及音源参数初始化
     */
    ret = audio_tcp_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("audio_tcp_snd_init error [%d]/n",ret);

    }
	/*
	 * 创建两个线程
	 * 1
	 * 2
	 */

	ret = pthread_create(&th_a, NULL, audio_tcp_thread1, NULL);
	if (ret != 0)
	{
	 perror ("creat audio thread error");
	}

	ret = pthread_create(&th_a, NULL, audio_tcp_thread2, NULL);
	if (ret != 0)
	{
	 perror ("creat audio thread error");
	}
	ret = pthread_create(&th_a, NULL, audio_tcp_mix,NULL);
	if (ret != 0)
	{
	 perror ("creat audio thread error");
	}

	pthread_join(th_a, &retval);


	return 0;


}
