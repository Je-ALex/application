//File   : sndwav_common.c

#include "../audio-module/sndwav_common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>


int  time_substract(timetime *result, struct timeval *begin,struct timeval *end)

{

    if(begin->tv_sec > end->tv_sec)
    	return -1;

    if((begin->tv_sec == end->tv_sec) && (begin->tv_usec > end->tv_usec))
    	return -2;

    result->time.tv_sec = (end->tv_sec - begin->tv_sec);
    result->time.tv_usec = (end->tv_usec - begin->tv_usec);
    if(result->time.tv_usec < 0)
    {
    	result->time.tv_sec--;
    	result->time.tv_usec += 1000000;
    }
    result->ms = result->time.tv_usec / 1000;
    result->time.tv_usec = result->time.tv_usec % 1000;
    return 0;

}


int SNDWAV_P_GetFormat(WAVContainer_t *wav, snd_pcm_format_t *snd_format)
{
//	if (LE_SHORT(wav->format.format) != WAV_FMT_PCM)
//		return -1;

	switch (LE_SHORT(wav->format.sample_length)) {
	case 16:
		*snd_format = SND_PCM_FORMAT_S16_LE;
		break;
	case 8:
		*snd_format = SND_PCM_FORMAT_U8;
		break;
	default:
		*snd_format = SND_PCM_FORMAT_UNKNOWN;
		break;
	}

	return 0;
}

ssize_t SNDWAV_ReadPcm(snd_data_format *sndpcm,snd_data_format *sndpcm_p,  size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;
	uint8_t *data = sndpcm->data_buf;


	if (count != sndpcm->period_size) {
		count = sndpcm->period_size;
	}

	while (count > 0) {

//		printf("snd_pcm_readi  count = %zu\n",count);
		r = snd_pcm_readi(sndpcm->handle, data, count);


//		printf("snd_pcm_readi%d  r = %zu\n",__LINE__,r);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
				fprintf(stderr, "<<<<<<<<<<<<<<< EAGAIN >>>>>>>>>>>>>>>\n");
				snd_pcm_wait(sndpcm->handle, 1000); 
		} else if (r == -EPIPE) {

				snd_pcm_prepare(sndpcm->handle); 
				fprintf(stderr, "<<<<<<<<<<<<<<<snd_pcm_readi Buffer overrun >>>>>>>>>>>>>>>\n");
		} else if (r == -ESTRPIPE) {

				fprintf(stderr, "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
		} else if (r < 0) {
				fprintf(stderr, "Error snd_pcm_writei: [%s]", snd_strerror(r));
				exit(-1);
		}

		if (r > 0) {
			result += r;
			count -= r;
			data += r * sndpcm->bits_per_frame / 8;
		}
	}


	return rcount;
}

ssize_t SNDWAV_WritePcm(snd_data_format *sndpcm, size_t wcount)
{
	ssize_t r;
	ssize_t result = 0;
	uint8_t *data = sndpcm->data_buf;

	if (wcount < sndpcm->period_size) {
		snd_pcm_format_set_silence(sndpcm->format,
			data + wcount * sndpcm->bits_per_frame / 8,
			(sndpcm->period_size - wcount) * sndpcm->channels);
		wcount = sndpcm->period_size;
	}
//	printf("sndpcm->chunk_size  = %zu\n",sndpcm->chunk_size);
	while (wcount > 0) {

//		printf("snd_pcm_writei  count = %zu\n",wcount);
		r = snd_pcm_writei(sndpcm->handle, data, wcount);

		if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {
			snd_pcm_wait(sndpcm->handle, 1000);
					} else if (r == -EPIPE) {
			snd_pcm_prepare(sndpcm->handle);
			fprintf(stderr, "<<<<<<<<<<<<<<<snd_pcm_writei Buffer Underrun >>>>>>>>>>>>>>>\n");
					} else if (r == -ESTRPIPE) {
			fprintf(stderr, "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
					} else if (r < 0) {
			fprintf(stderr, "Error snd_pcm_writei: [%s]", snd_strerror(r));
			exit(-1);
					}

		if (r > 0) {
			result += r;
			wcount -= r;
			data += r * sndpcm->bits_per_frame / 8;
		}
	}
	return result;
}
/*
 * 函数：硬件参数配置
 */
int SNDWAV_SetParams(snd_data_format *sndpcm, WAVContainer_t *wav)
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_format_t format;
	uint32_t exact_rate;
	uint32_t buffer_time, period_time,period_size;
	int dir = 0;

	/*为硬件参数申请内存*/
	snd_pcm_hw_params_alloca(&hwparams);

	/*初始化为默认参数*/
	if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_any/n");
		goto ERR_SET_PARAMS;
	}
	/*设置为交错模式*/
	if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_access/n");
		goto ERR_SET_PARAMS;
	}

	/*获取本地设置的format length*/
	if (SNDWAV_P_GetFormat(wav, &format) < 0) {
		fprintf(stderr, "Error get_snd_pcm_format/n");
		goto ERR_SET_PARAMS;
	}
	/*设置format参数*/
	if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_format/n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->format = format;

	/*设置声道参数*/
	if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(wav->format.channels)) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_channels/n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->channels = LE_SHORT(wav->format.channels);

	/*设置采样率*/
	exact_rate = LE_INT(wav->format.sample_rate);
	if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_rate_near/n");
		goto ERR_SET_PARAMS;
	}

	if (LE_INT(wav->format.sample_rate) != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported by your hardware./n ==> Using %d Hz instead./n",
			LE_INT(wav->format.sample_rate), exact_rate);
	}
	//获取缓冲区支持的最大time （us）
	if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max/n");
		goto ERR_SET_PARAMS;
	}

	if (buffer_time > 500000) buffer_time = 500000;
	buffer_time = 44000;
	period_time = buffer_time / 4;//11ms

	//设置buffer_time的值，dir(-1,0,1 exact value is <,=,>)
	if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, &dir) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_buffer_time_near/n");
		goto ERR_SET_PARAMS;
	}

	//设置周期时间 exact value is <,=,> val following dir (-1,0,1)
	if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, &dir) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near/n");
		goto ERR_SET_PARAMS;
	}
//	period_size = 256;
//	snd_pcm_hw_params_set_period_size_near(sndpcm->handle, hwparams, &period_size, &dir);


	/*将硬件参数设置到PCM设备*/
	if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params(handle, params)/n");
		goto ERR_SET_PARAMS;
	}

	/*从配置空间中获取周期大小*/
	snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->period_size, &dir);

	/*从配置空间中获取buffer_size大小*/
	snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);

	if (sndpcm->period_size == sndpcm->buffer_size) {
		fprintf(stderr, ("Can't use period equal to buffer size (%lu == %lu)/n"), sndpcm->period_size, sndpcm->buffer_size);
		goto ERR_SET_PARAMS;
	}

	sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);
	sndpcm->chunk_bytes = sndpcm->period_size * sndpcm->bits_per_frame / 8;

	/* alloc mem save the block*/
	sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);
	if (!sndpcm->data_buf) {
		fprintf(stderr, "Error malloc: [data_buf]/n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->recv_buf = (uint8_t *)malloc(sndpcm->chunk_bytes*2);
	if (!sndpcm->recv_buf) {
		fprintf(stderr, "Error malloc: [recv_buf]/n");
		goto ERR_SET_PARAMS;
	}
	return 0;

ERR_SET_PARAMS:
	return -1;
}
//ying jian can shu she zhi
int play_SetParams(snd_data_format *sndpcm, WAVContainer_t *wav)
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_format_t format;
	uint32_t exact_rate;
	uint32_t buffer_time, period_time;

	/* snd_pcm_hw_params_t*/
	snd_pcm_hw_params_alloca(&hwparams);

	/* init paramers*/
	if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_any/n");
		goto ERR_SET_PARAMS;
	}

	if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_access/n");
		goto ERR_SET_PARAMS;
	}

	/* get format type */
	if (SNDWAV_P_GetFormat(wav, &format) < 0) {
		fprintf(stderr, "Error get_snd_pcm_format/n");
		goto ERR_SET_PARAMS;
	}
	if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_format/n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->format = format;

	/*set channel */
	if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(wav->format.channels)) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_channels/n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->channels = LE_SHORT(wav->format.channels);

	/* set sample rate */
	exact_rate = LE_INT(wav->format.sample_rate);
	if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_rate_near/n");
		goto ERR_SET_PARAMS;
	}
	if (LE_INT(wav->format.sample_rate) != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported by your hardware./n ==> Using %d Hz instead./n",
			LE_INT(wav->format.sample_rate), exact_rate);
	}

	if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max/n");
		goto ERR_SET_PARAMS;
	}
	if (buffer_time > 500000) buffer_time = 500000;
	buffer_time =50000;
	period_time = buffer_time / 4;

	if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_buffer_time_near/n");
		goto ERR_SET_PARAMS;
	}

	if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near/n");
		goto ERR_SET_PARAMS;
	}

	/* set paramers to snd card */
	if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {
		fprintf(stderr, "Error snd_pcm_hw_params(handle, params)/n");
		goto ERR_SET_PARAMS;
	}
	/*get period size */
	snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->period_size, 0);
	/* */
	snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);
	if (sndpcm->period_size == sndpcm->buffer_size) {
		fprintf(stderr, ("Can't use period equal to buffer size (%lu == %lu)/n"), sndpcm->period_size, sndpcm->buffer_size);
		goto ERR_SET_PARAMS;
	}

	sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);

	sndpcm->chunk_bytes = sndpcm->period_size * sndpcm->bits_per_frame /8;

	/* alloc mem save the block*/
	sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);
	if (!sndpcm->data_buf) {
		fprintf(stderr, "Error malloc: [data_buf]/n");
		goto ERR_SET_PARAMS;
	}

	return 0;

ERR_SET_PARAMS:
	return -1;
}
