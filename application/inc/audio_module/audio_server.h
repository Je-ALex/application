/*
 * audio_tcp_server.h
 *
 *  Created on: 2016年10月29日
 *      Author: leon
 */

#ifndef INC_AUDIO_MODULE_AUDIO_SERVER_H_
#define INC_AUDIO_MODULE_AUDIO_SERVER_H_

#include <alsa/asoundlib.h>
#include "wifi_sys_init.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char  	uint8_t;

typedef unsigned short 	uint16_t;

typedef unsigned int   	uint32_t;

typedef long long 		off64_t;

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)           (v)
#define LE_INT(v)               (v)
#define BE_SHORT(v)           bswap_16(v)
#define BE_INT(v)               bswap_32(v)

#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d) ((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)           bswap_16(v)
#define LE_INT(v)               bswap_32(v)
#define BE_SHORT(v)           (v)
#define BE_INT(v)               (v)
#else
#error "Wrong endian"
#endif

#define WAV_RIFF        COMPOSE_ID('R','I','F','F')
#define WAV_WAVE        COMPOSE_ID('W','A','V','E')
#define WAV_FMT         COMPOSE_ID('f','m','t',' ')
#define WAV_DATA        COMPOSE_ID('d','a','t','a')


#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe


#define DEFAULT_CHANNELS         (2)
#define DEFAULT_SAMPLE_RATE      (48000)
#define DEFAULT_SAMPLE_LENGTH    (16)


#define		RS_NUM 	50
#define 	LOG_NAME 	"audio.log"

/*
 * timeofday
 */
typedef struct {
	struct timeval time;
	int ms;

}timetime;

typedef struct  {
	uint32_t magic;
	uint32_t length;
	uint32_t type;
} WAVHeader;

typedef struct  {
	uint32_t magic;
	uint32_t fmt_size;
	uint16_t format;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t bytes_p_second;
	uint16_t blocks_align;
	uint16_t sample_length;
} WAVFmt;

typedef struct  {
	uint32_t type;
	uint32_t length;
} WAVChunkHeader;

typedef struct  {
	WAVHeader header;
	WAVFmt format;
	WAVChunkHeader chunk;
} WAVContainer,*PWAVContainer;


typedef struct  {

	snd_pcm_t *handle;
	snd_output_t *log;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_format_t format;

	uint16_t channels;
	size_t chunk_bytes;
	size_t bits_per_sample;
	size_t bits_per_frame;

	int recv_num;
	uint8_t *data_buf;

} snd_data_format,*Psnd_data_format;


typedef struct{

	int len;
	char* msg;

}audio_frame,*Paudio_frame;

typedef struct{

	sem_t audio_mix_sem[10];
	sem_t audio_recv_sem[10];

}audio_signal;



int wifi_sys_audio_init();


int audio_spk_recv_thread_create(int port);

int audio_spk_recv_thread_delete();



#ifdef __cplusplus
}
#endif

#endif /* INC_AUDIO_MODULE_AUDIO_SERVER_H_ */
