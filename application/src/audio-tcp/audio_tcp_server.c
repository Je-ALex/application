/*
 * File   : main.c
 * 应用入口函数
 * 日期：2016年7月
 */


#include "audio_tcp_server.h"
#include "sndwav_common.h"
#include "../../inc/tcp_ctrl_queue.h"

#define  QUEUE_LINE  12
#define  SOURCE_PORT 8080


/* The name of this program. */
const char* program_name;

unsigned char aodio_buf[10][2048];
unsigned int num[10] = {0};

pthread_mutex_t tcp_mutex;
sem_t local_report_sem;

Plinkqueue tcp_send_queue;
typedef struct{
	int len;
	char* msg;

}audio_frame,*Paudio_frame;
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
int tcp_audio_enqueue(int len ,char* buf)
{

	Paudio_frame tmp;

	tmp = (Paudio_frame)malloc(sizeof(audio_frame));
	memset(tmp,0,sizeof(audio_frame));

	tmp->len = len;
	tmp->msg = buf;

//	pthread_mutex_lock(&sys_in.tcp_mutex);
	enter_queue(tcp_send_queue,tmp);
//	pthread_mutex_unlock(&sys_in.tcp_mutex);

	sem_post(&local_report_sem);

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
int tcp_audio_dequeue(Paudio_frame* event_tmp)
{

	int ret;
	int value;

	Plinknode node;
	Paudio_frame tmp;

	sem_wait(&local_report_sem);

	ret = out_queue(tcp_send_queue,&node);
//	pthread_mutex_unlock(tpsend_queue_mutex);

	if(ret == 0)
	{
		tmp = node->data;
		memcpy(*event_tmp,tmp,sizeof(audio_frame));

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
	char *devicename = "default";
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

#define SIZE_AUDIO_FRAME (2)
//归一化算法
void Mix(char sourseFile[10][SIZE_AUDIO_FRAME],char *objectFile,int number,int length)
{

    int const MAX=32767;
    int const MIN=-32768;

    double f=1;
    int output;
    int i = 0,j = 0;
    for (i=0;i<length/2;i++)
    {
        int temp=0;
        for (j=0;j<number;j++)
        {
            temp+=*(short*)(sourseFile[j]+i*2);
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

int status = 0;
/*
 * 音频处理线程
 * 包含了tcp的维护，audio音频源的播放
 * 2016 alex
 */
static void* audio_tcp_thread1(void* data)
{
	snd_data_format playback;
	WAVContainer_t wav;

	int ret;
	int socket_fd,conec_fd;
    socklen_t client_len;
    struct sockaddr_in addr_serv,addr_client;

    int recv_num;
	struct timeval start,stop;
	struct timeval start_,stop_;
	timetime diff;
	timetime recv_t;

	memset(&playback, 0x0, sizeof(playback));

	printf("%s\n",__func__);

	memset(&wav,0,sizeof(WAVContainer_t));

    /*
     * 音频设备初始化及音源参数初始化
     */
//    ret = audio_tcp_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("audio_tcp_snd_init error [%d]/n",ret);
		goto Err;
    }
    socket_fd = tcp_params_init(SOURCE_PORT);

//	char* buf = malloc(1920);
//	int i;
//	int fd=-1;
//	char *path="1.txt";
//	FILE* fp;
    char buf[2048] = {0};
	char r_buf[2048] = {0};
	Paudio_frame msg;
	msg = malloc(sizeof(audio_frame));
	memset(msg,0,sizeof(audio_frame));


    while(1){

			printf("begin accept:\n");
			status = 0;
			num[0] = 0;
			conec_fd = accept(socket_fd,(struct sockaddr *)&addr_client,&client_len);
            if(conec_fd < 0){

                   perror("accept");
                   continue;
             }

            printf("accept a new client,ip:%s\n",inet_ntoa(addr_client.sin_addr));

//            fd = open(path, O_RDWR | O_CREAT, 0665);
//            fp = fopen(path,"w+");
        	while(1){
//              	pthread_mutex_lock(&tcp_mutex);
//        		printf("begin recv:\n");
//        		gettimeofday(&start,0);
        		recv_num = recv(conec_fd,r_buf,2048,0);

//        		gettimeofday(&stop,0);
//        		time_substract(&recv_t,&start,&stop);
//        		printf("recv_num time : %d s,%d ms,%d us\n",(int)recv_t.time.tv_sec,recv_t.ms,(int)recv_t.time.tv_usec);


        		if(recv_num <= 0){
        			perror("recv");
        			playback.recv_falg = 0;
//        			close(fd);
        			break;
        		} else {

        			printf("1recv_num = %d\n",recv_num);
//        			if(write(fd,buf,1920)!=recv_num)
//					{
//					  printf("write error\n");
//					}
//          			if((ret=fwrite(buf,1,1920,fp)) != recv_num)
//					{
//					  printf("write error ret =%d\n",ret);
//					}
        			status = 1;
        //			sndpcm->recv_falg = 1;
        //			printf(" recv_num:%d\n",recv_num);
        //			sndpcm->recv_unm = recv_num;
        //			sndpcm->recv_buf = sndpcm->data_buf;


        			/* Transfer to size frame */
//        			recv_num = recv_num * 8 / playback.bits_per_frame;
//
//        			ret = SNDWAV_WritePcm(&playback, recv_num);
//
//        			printf(" 1ret:%d\n",ret);

        			tcp_audio_enqueue(recv_num,r_buf);

//        			num[0] = recv_num;
//        			memcpy(aodio_buf[0],r_buf,recv_num);
//
//                	Mix(aodio_buf,buf,1,recv_num);
//
//    				memcpy(playback.data_buf,buf,recv_num);

//					recv_num = recv_num * 8 / playback.bits_per_frame;
//
//					ret = SNDWAV_WritePcm(&playback, recv_num);

//        			pthread_mutex_unlock(&tcp_mutex);
        		}

//        			time_substract(&diff,&start_,&stop_);
//        			printf("ret time : %d s,%d ms,%d us\n",(int)diff.time.tv_sec,diff.ms,(int)recv_t.time.tv_usec);
        	}
    }




Err:
	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

    pthread_exit(0);
}

static void* audio_tcp_thread2(void* data)
{
	snd_data_format playback;
	WAVContainer_t wav;

	int ret;
	int socket_fd,conec_fd;
    socklen_t client_len;
    struct sockaddr_in addr_serv,addr_client;

    int recv_num;
	struct timeval start,stop;
	struct timeval start_,stop_;
	timetime diff;
	timetime recv_t;

	memset(&playback, 0x0, sizeof(playback));

	printf("%s\n",__func__);


	memset(&wav,0,sizeof(WAVContainer_t));

    /*
     * 音频设备初始化及音源参数初始化
     */
//    ret = audio_tcp_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("audio_tcp_snd_init error [%d]/n",ret);
		goto Err;
    }


    char* buf = malloc(1920);

    socket_fd = tcp_params_init(8081);
    int i;
    int fd=-1;
	char *path="1.txt";
	FILE* fp;

	char r_buf[1920] = {0};
    while(1){

			printf("begin accept:\n");
			status = 0;

			conec_fd = accept(socket_fd,(struct sockaddr *)&addr_client,&client_len);
            if(conec_fd < 0){

                   perror("accept");
                   continue;
             }

            printf("accept a new client,ip:%s\n",inet_ntoa(addr_client.sin_addr));

//            fd = open(path, O_RDWR | O_CREAT, 0665);
            fp = fopen(path,"w+");
        	while(1){

//        		printf("begin recv:\n");
//        		gettimeofday(&start,0);
        		recv_num = recv(conec_fd,r_buf,sizeof(r_buf),0);

//        		gettimeofday(&stop,0);
//        		time_substract(&recv_t,&start,&stop);
//        		printf("recv_num time : %d s,%d ms,%d us\n",(int)recv_t.time.tv_sec,recv_t.ms,(int)recv_t.time.tv_usec);


        		if(recv_num <= 0){
        			perror("recv");
        			playback.recv_falg = 0;
        			close(fd);
        			break;
        		} else {
//        			pthread_mutex_lock(&tcp_mutex);
        			printf("2recv_num = %d\n",recv_num);
//        			if(write(fd,buf,1920)!=recv_num)
//					{
//					  printf("write error\n");
//					}
//          			if((ret=fwrite(buf,1,1920,fp)) != recv_num)
//					{
//					  printf("write error ret =%d\n",ret);
//					}
//        			status = 1;

        			/* Transfer to size frame */
//        			recv_num = recv_num * 8 / playback.bits_per_frame;
//        			gettimeofday(&start_,0);
//        			ret = SNDWAV_WritePcm(&playback, recv_num);
//        			gettimeofday(&stop_,0);
//        			printf(" 2ret:%d\n",ret);

        			num[1] = recv_num;
        			memcpy(aodio_buf[1],r_buf,recv_num);
//        			pthread_mutex_unlock(&tcp_mutex);
        		}

//        			time_substract(&diff,&start_,&stop_);
//        			printf("ret time : %d s,%d ms,%d us\n",(int)diff.time.tv_sec,diff.ms,(int)recv_t.time.tv_usec);
        	}
    }




Err:
	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

    pthread_exit(0);
}

static void* audio_tcp_mix(void* data)
{
    int ret1,ret2,ret3,ret4;
    unsigned char buf[2048] = {0};
    int recv_num;
    int ret;

	snd_data_format playback;
	WAVContainer_t wav;
	memset(&playback, 0x0, sizeof(playback));
	memset(&wav,0,sizeof(WAVContainer_t));

    /*
     * 音频设备初始化及音源参数初始化
     */
    ret = audio_tcp_snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("audio_tcp_snd_init error [%d]/n",ret);

    }
    Paudio_frame tmp;
	tmp = (Paudio_frame)malloc(sizeof(audio_frame));

    while(1)
    {

    	tcp_audio_dequeue(&tmp);

    	printf("tcp_audio_dequeue\n");
    	if(tmp->len > 0)
    		recv_num = tmp->len;
    	else
    		;

        if(recv_num>0 )
        {
//        	pthread_mutex_lock(&tcp_mutex);

//        	memcpy(aodio_buf[0],tmp->msg,recv_num);

//        	Mix(aodio_buf,buf,4,recv_num);
        	memcpy(playback.data_buf,tmp->msg,recv_num);

			recv_num = recv_num * 8 / playback.bits_per_frame;
			printf("audio_tcp_mix recv_num=%d\n",recv_num);
			ret = SNDWAV_WritePcm(&playback, recv_num);

//			pthread_mutex_unlock(&tcp_mutex);
        }

    }




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
	ret = sem_init(&local_report_sem, 0, 0);
	if (ret != 0)
	{
	 perror("local_report_sem initialization failed");
	}
	/*
	 * 创建两个线程
	 * 1
	 * 2
	 */
	tcp_send_queue = queue_init();

	ret = pthread_create(&th_a, NULL, audio_tcp_thread1, NULL);
	if (ret != 0)
	{
	 perror ("creat audio thread error");
	}

//	ret = pthread_create(&th_a, NULL, audio_tcp_thread2, NULL);
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
