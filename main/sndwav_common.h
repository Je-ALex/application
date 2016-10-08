//File   : sndwav_common.h

#ifndef __SNDWAV_COMMON_H
#define __SNDWAV_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include "wav_parser.h"



typedef long long off64_t;

typedef struct  {
	snd_pcm_t *handle;
	snd_output_t *log;
	snd_pcm_uframes_t chunk_size;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_format_t format;
	uint16_t channels;
	size_t chunk_bytes;
	size_t bits_per_sample;
	size_t bits_per_frame;

	int recv_unm;
	int recv_falg;
	uint8_t* recv_buf;

	uint8_t *data_buf;
} snd_data_format;

/*
 * timeofday
 */
typedef struct {
	struct timeval time;
	int ms;

}timetime;

ssize_t SNDWAV_ReadPcm(snd_data_format *sndpcm,snd_data_format *sndpcm_p,  size_t rcount);


ssize_t SNDWAV_WritePcm(snd_data_format *sndpcm, size_t wcount);

int SNDWAV_SetParams(snd_data_format *sndpcm, WAVContainer_t *wav);
int play_SetParams(snd_data_format *sndpcm, WAVContainer_t *wav);

int  time_substract( timetime *result, struct timeval *begin,struct timeval *end);
#endif /* #ifndef __SNDWAV_COMMON_H */
