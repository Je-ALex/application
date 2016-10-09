/*
 * File   : main.c
 * 应用入口函数
 * 日期：2016年7月
 */

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <pthread.h>
#include "wav_parser.h"
#include "sndwav_common.h"

#include <sys/socket.h>//socket():bind();listen():accept();inet_addr();listen():accept();connect();
#include <arpa/inet.h>//htons();inet_addr():
#include <netinet/in.h>//inet_addr():
#include <signal.h>

#define  QUEUE_LINE  12
#define  SOURCE_PORT 8080



/* The name of this program. */
const char* program_name;


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
        err = snd_pcm_sw_params_set_start_threshold(sndpcm->handle, swparams, (sndpcm->buffer_size / sndpcm->chunk_size) * sndpcm->chunk_size);
        if (err < 0) {
                printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
        err = snd_pcm_sw_params_set_avail_min(sndpcm->handle, swparams,sndpcm->chunk_size);
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
static int snd_init(snd_data_format* play,WAVContainer_t* wav)
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
static int tcp_params_init()
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
    addr_serv.sin_port = htons(SOURCE_PORT);
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
/*
 * 音频处理线程
 * 包含了tcp的维护，audio音频源的播放
 * 2016 alex
 */
static void* audio_thread(void* data)
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

	printf("enter the audio thread\n");
    /*
     * 音频设备初始化及音源参数初始化
     */
    ret = snd_init(&playback, &wav);
    if(ret < 0)
    {
		printf("snd_init error [%d]/n",ret);
		goto Err;
    }

    socket_fd = tcp_params_init();

    while(1){

             printf("begin accept:\n");
             conec_fd = accept(socket_fd,(struct sockaddr *)&addr_client,&client_len);
            if(conec_fd < 0){
                   perror("accept");
                   continue;
             }

            printf("accept a new client,ip:%s\n",inet_ntoa(addr_client.sin_addr));

        	while(1){

        		printf("begin recv:\n");
        		gettimeofday(&start,0);
        		recv_num = recv(conec_fd,playback.data_buf,playback.chunk_bytes,0);

        		gettimeofday(&stop,0);
        		time_substract(&recv_t,&start,&stop);
        		printf("recv_num time : %d s,%d ms,%d us\n",(int)recv_t.time.tv_sec,recv_t.ms,(int)recv_t.time.tv_usec);


        		if(recv_num <= 0){
        			perror("recv");
        			playback.recv_falg = 0;
        			break;
        		} else {

        //			sndpcm->recv_falg = 1;
        //			printf(" recv_num:%d\n",recv_num);
        //			sndpcm->recv_unm = recv_num;
        //			sndpcm->recv_buf = sndpcm->data_buf;
        			/* Transfer to size frame */
        			recv_num = recv_num * 8 / playback.bits_per_frame;
        			gettimeofday(&start_,0);
        			ret = SNDWAV_WritePcm(&playback, recv_num);
        			gettimeofday(&stop_,0);
        			printf(" ret:%d\n",ret);
        		}

        			time_substract(&diff,&start_,&stop_);
        			printf("ret time : %d s,%d ms,%d us\n",(int)diff.time.tv_sec,diff.ms,(int)recv_t.time.tv_usec);
        	}
    }


	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);

	return 0;

Err:
	snd_output_close(playback.log);
	if (playback.data_buf) free(playback.data_buf);
	if (playback.handle) snd_pcm_close(playback.handle);
	return -1;
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

	printf("enter the main\n");

	ret = pthread_create (&th_a, NULL, audio_thread, 0);
	if (ret != 0)
	{
	 perror ("creat audio thread error");
	}
	pthread_join(th_a, &retval);
	return 0;


}
