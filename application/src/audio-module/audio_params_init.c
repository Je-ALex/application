/*
 * audio_parameter_init.c
 *
 *  Created on: 2016年11月7日
 *      Author: leon
 */



#include "../../inc/audio/audio_parameter_init.h"


/*
 * 函数：音频文件数据格式初始化
 */
int audio_format_init(PWAVContainer wav)
{

	wav->format.channels = DEFAULT_CHANNELS;
	wav->format.sample_rate = DEFAULT_SAMPLE_RATE;
	wav->format.sample_length = DEFAULT_SAMPLE_LENGTH;

//	wav->format.blocks_align = LE_SHORT(wav->format.channels * wav->format.sample_length / 8);
//	wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * wav->format.sample_rate);
//	wav->chunk.length = LE_INT(DEFAULT_DURATION_TIME * (uint32_t)(wav->format.bytes_p_second));
//	wav->header.length = LE_INT((uint32_t)(wav->chunk.length) + sizeof(wav->chunk) +
//			sizeof(wav->format) + sizeof(wav->header) - 8);

#ifdef WAV_PRINT_MSG
	WAV_P_PrintHeader(wav);
#endif
	return 0;

}

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


int audio_get_format(PWAVContainer wav, snd_pcm_format_t *snd_format)
{

	switch (LE_SHORT(wav->format.sample_length)) {

	case 32:
		*snd_format = SND_PCM_FORMAT_S32_LE;
		break;
	case 24:
		*snd_format = SND_PCM_FORMAT_S24_LE;
		break;
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

	return SUCCESS;
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

int audio_module_data_write(Psnd_data_format sndpcm, int frame_len)
{
	int r;
	int result = 0;
	uint8_t *data = sndpcm->data_buf;

	if (frame_len < sndpcm->period_size) {
		snd_pcm_format_set_silence(sndpcm->format,
			data + frame_len * sndpcm->bits_per_frame / 8,
			(sndpcm->period_size - frame_len) * sndpcm->channels);
		frame_len = sndpcm->period_size;
	}

	while (frame_len > 0) {

		r = snd_pcm_writei(sndpcm->handle, sndpcm->data_buf, frame_len);

		if (r == -EAGAIN || (r >= 0 && r < frame_len))
		{
			snd_pcm_wait(sndpcm->handle, 1000);
			printf("--------EAGAIN--------\n");
		} else if (r == -EPIPE)
		{
			snd_pcm_prepare(sndpcm->handle);
			printf("------snd_pcm_writei Buffer Underrun------\n");

		} else if (r == -ESTRPIPE)
		{
			printf("--------Need suspend--------\n");

		} else if (r < 0)
		{
			printf( "Error snd_pcm_writei: [%s]", snd_strerror(r));
			return ERROR;
		}

		if (r > 0) {
			result += r;
			frame_len -= r;
			data += r * sndpcm->bits_per_frame / 8;
		}
	}
	return result;
}


/*
 * 函数：硬件参数配置
 */
int audio_snd_params_init(Psnd_data_format sndpcm, PWAVContainer wav)
{

	snd_pcm_hw_params_t *hwparams;

	snd_pcm_format_t format;
	uint32_t exact_rate;
	uint32_t buffer_time, period_time;
	int dir = 0;

	/*为硬件参数申请内存*/
	snd_pcm_hw_params_alloca(&hwparams);

	/*初始化为默认参数*/
	if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {
		printf("Error snd_pcm_hw_params_any\n");
		goto ERR_SET_PARAMS;
	}
	/*设置为交错模式*/
	if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		printf("Error snd_pcm_hw_params_set_access\n");
		goto ERR_SET_PARAMS;
	}

	/*获取本地设置的format length*/
	if (audio_get_format(wav, &format) < 0) {
		printf("Error get_snd_pcm_format\n");
		goto ERR_SET_PARAMS;
	}
	/*设置format参数*/
	if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {
		printf("Error snd_pcm_hw_params_set_format--%d\n",format);
		goto ERR_SET_PARAMS;
	}
	sndpcm->format = format;

	/*设置声道参数*/
	if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(wav->format.channels)) < 0) {
		printf("Error snd_pcm_hw_params_set_channels\n");
		goto ERR_SET_PARAMS;
	}
	sndpcm->channels = LE_SHORT(wav->format.channels);

	/*设置采样率*/
	exact_rate = LE_INT(wav->format.sample_rate);
	if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {
		printf("Error snd_pcm_hw_params_set_rate_near\n");
		goto ERR_SET_PARAMS;
	}

	if (LE_INT(wav->format.sample_rate) != exact_rate) {
		printf( "The rate %d Hz is not supported by your hardware./n ==> Using %d Hz instead.\n",
			LE_INT(wav->format.sample_rate), exact_rate);
	}
	//获取缓冲区支持的最大time （us）
	if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
		printf( "Error snd_pcm_hw_params_get_buffer_time_max\n");
		goto ERR_SET_PARAMS;
	}

	if (buffer_time > 500000) buffer_time = 500000;
	buffer_time = 48000;
	period_time = buffer_time / 4;//11ms

	//设置buffer_time的值，dir(-1,0,1 exact value is <,=,>)
	if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, &dir) < 0) {
		printf("Error snd_pcm_hw_params_set_buffer_time_near\n");
		goto ERR_SET_PARAMS;
	}

	//设置周期时间 exact value is <,=,> val following dir (-1,0,1)
	if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, &dir) < 0) {
		printf("Error snd_pcm_hw_params_set_period_time_near\n");
		goto ERR_SET_PARAMS;
	}

	/*将硬件参数设置到PCM设备*/
	if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {
		printf( "Error snd_pcm_hw_params(handle, params)\n");
		goto ERR_SET_PARAMS;
	}

	/*从配置空间中获取周期大小*/
	snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->period_size, &dir);

	/*从配置空间中获取buffer_size大小*/
	snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);

	if (sndpcm->period_size == sndpcm->buffer_size) {
		printf(("Can't use period equal to buffer size (%lu == %lu)\n"),
				sndpcm->period_size, sndpcm->buffer_size);
		goto ERR_SET_PARAMS;
	}

	sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);
	sndpcm->chunk_bytes = sndpcm->period_size * sndpcm->bits_per_frame / 8;

	/* alloc mem save the block*/
	sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);

	if (!sndpcm->data_buf) {
		printf("Error malloc: [data_buf]\n");
		goto ERR_SET_PARAMS;
	}

	return SUCCESS;

ERR_SET_PARAMS:
	return ERROR;
}

