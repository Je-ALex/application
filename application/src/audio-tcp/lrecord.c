//File   : lrecord.c

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
#include <sys/socket.h>//socket():bind();listen():accept();inet_addr();listen():accept();connect();
#include <arpa/inet.h>//htons();inet_addr():
#include <netinet/in.h>//inet_addr():

#include "wav_parser.h"
#include "sndwav_common.h"



#define DEST_PORT 8080
//#define DEST_IP_ADDRESS "192.168.10.50"
#ifndef DEST_IP_ADDRESS
#define DEST_IP_ADDRESS "127.0.0.1"
#endif
int tcp_client(char* url)
{
    int sock_fd;
    struct sockaddr_in addr_serv;//服务器端地址

    sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd < 0){
            perror("sock");
            exit(1);
    } else {
            printf("sock sucessful:\n");
    }
    memset(&addr_serv,0,sizeof(addr_serv));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port =  htons(DEST_PORT);
    addr_serv.sin_addr.s_addr = inet_addr(url);
    int nSendBuf=32*1024;//设置为32K
    setsockopt(sock_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
   if( connect(sock_fd,(struct sockaddr *)&addr_serv,sizeof(struct sockaddr)) < 0){
           perror("connect");
            printf("connect (%d)\n",errno);
            exit(1);
   } else {
            printf("connect sucessful\n");
   }

    return sock_fd;


}

int SNDWAV_PrepareWAVParams(WAVContainer_t *wav)
{
	assert(wav);

	uint16_t channels = DEFAULT_CHANNELS;
	uint16_t sample_rate = DEFAULT_SAMPLE_RATE;
	uint16_t sample_length = DEFAULT_SAMPLE_LENGTH;
	uint32_t duration_time = DEFAULT_DURATION_TIME;

	wav->header.magic = WAV_RIFF;
	wav->header.type = WAV_WAVE;
	wav->format.magic = WAV_FMT;
	wav->format.fmt_size = LE_INT(16);
	wav->format.format = LE_SHORT(WAV_FMT_PCM);
	wav->chunk.type = WAV_DATA;


	wav->format.channels = LE_SHORT(channels);
	wav->format.sample_rate = LE_INT(sample_rate);
	wav->format.sample_length = LE_SHORT(sample_length);

	wav->format.blocks_align = LE_SHORT(channels * sample_length / 8);
	wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * sample_rate);

	wav->chunk.length = LE_INT(duration_time * (uint32_t)(wav->format.bytes_p_second));
	wav->header.length = LE_INT((uint32_t)(wav->chunk.length) + sizeof(wav->chunk) + sizeof(wav->format) + sizeof(wav->header) - 8);

	return 0;
}

void SNDWAV_Record(snd_data_format *sndpcm, snd_data_format *sndpcm_p,WAVContainer_t *wav, int fd,char* url)
{
	off64_t rest;
	size_t c, frame_size;
	struct timeval start,stop;
	timetime diff;
	int sockfd;
	int send_num;
	int n;
//	if (WAV_WriteHeader(fd, wav) < 0) {
//		exit(-1);
//	}
	/*
	 * tcp send data
	 */
//	sockfd = tcp_client(url);

	rest = wav->chunk.length;
	while (rest > 0) {
//		printf("rest %lld\n",rest);

		c = (rest <= (off64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes;
//		printf("c %zu\n",c);
//		c = sndpcm->chunk_bytes;
		frame_size = c * 8 / sndpcm->bits_per_frame;
//		printf("frame_size %zu\n",frame_size);
		gettimeofday(&start,0);
		if (SNDWAV_ReadPcm(sndpcm, NULL, frame_size) != frame_size)
			break;

		/*
		 * play
		 */
		sndpcm_p->data_buf = sndpcm->data_buf;
		SNDWAV_WritePcm(sndpcm_p, frame_size);


		/*write to note*/
//		if (write(fd, sndpcm->data_buf, c) != c) {
//			fprintf(stderr, "Error SNDWAV_Record[write]/n");
//			exit(-1);
//		}



//	    char send_buf[]="00000000";
//        send_num = send(sockfd,send_buf,sizeof(send_buf),0);

//	    send_num = send(sockfd,sndpcm->data_buf,c,0);
//        if (send_num < 0){
//               perror("send");
//                exit(1);
//        } else {
////              printf("send sucess:%s\n",sndpcm->data_buf);
//       }
//	    gettimeofday(&stop,0);
//	    time_substract(&diff,&start,&stop);
////		printf("send_num = %d\n",send_num);
//		printf("Total time : %d s,%d ms,%d us\n",(int)diff.time.tv_sec,diff.ms,(int)diff.time.tv_usec);


	    rest -= c;
	}
    close(sockfd);
}

int main(int argc, char *argv[])
{
	char *filename;
	char *devicename = "hw:0,0";
	int fd;
	WAVContainer_t wav;
	snd_data_format record;
	snd_data_format playback;

//	if (argc != 2) {
//		fprintf(stderr, "Usage: ./lrecord <file name>/n");
//		return -1;
//	}

	memset(&record, 0x0, sizeof(record));
	memset(&playback, 0x0, sizeof(playback));

//	filename = argv[1];
//	remove(filename);
//	if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) {
//		fprintf(stderr, "Error open: [%s]/n", filename);
//		return -1;
//	}

	if (snd_output_stdio_attach(&record.log, stderr, 0) < 0) {
		fprintf(stderr, "Error snd_output_stdio_attach/n");
		goto Err;
	}

	if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_open [ %s]/n", devicename);
		goto Err;
	}

	if (SNDWAV_PrepareWAVParams(&wav) < 0) {
		fprintf(stderr, "Error SNDWAV_PrepareWAVParams/n");
		goto Err;
	}

	if (SNDWAV_SetParams(&record, &wav) < 0) {
		fprintf(stderr, "Error set_snd_pcm_params/n");
		goto Err;
	}
	/*
	 * play part
	 */
	if (snd_output_stdio_attach(&playback.log, stderr, 0) < 0) {
         fprintf(stderr, "Error snd_output_stdio_attach/n");
         goto Err;
     }

     if (snd_pcm_open(&playback.handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
         fprintf(stderr, "Error snd_pcm_open [ %s]/n", devicename);
         goto Err;
     }

     if (play_SetParams(&playback, &wav) < 0) {
         fprintf(stderr, "Error set_snd_pcm_params/n");
         goto Err;
     }
     snd_pcm_dump(playback.handle, playback.log);

     /*
      * end
      */
 	snd_pcm_dump(record.handle, record.log);
 	if(argc > 1)
 		SNDWAV_Record(&record, &playback,&wav, fd,argv[1]);
 	else
 		SNDWAV_Record(&record, &playback,&wav, fd, DEST_IP_ADDRESS);

	snd_pcm_drain(record.handle);


	/*
	 *
	 */
    snd_pcm_drain(playback.handle);
    snd_output_close(playback.log);
    /*
     * end
     */

	close(fd);
	free(playback.data_buf);
	snd_output_close(record.log);
	snd_pcm_close(record.handle);
	return 0;

Err:
	close(fd);
	remove(filename);
	if (record.data_buf) free(record.data_buf);
	if (record.log) snd_output_close(record.log);
	if (record.handle) snd_pcm_close(record.handle);
	return -1;
}
