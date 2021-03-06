/*
 * audio_parameter_init.c
 *
 *  Created on: 2016年11月7日
 *      Author: leon
 */


#include "audio_server.h"
#include "audio_params_init.h"


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
    return SUCCESS;

}

/*
 * 函数：音频文件数据格式初始化
 */
int audio_format_init(PWAVContainer wav)
{
	wav->format.channels = DEFAULT_CHANNELS;
	wav->format.sample_rate = DEFAULT_SAMPLE_RATE;
	wav->format.sample_length = DEFAULT_SAMPLE_LENGTH;

	return SUCCESS;

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


ssize_t audio_module_data_read(snd_data_format *sndpcm,snd_data_format *sndpcm_p,  size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;
	char *data = sndpcm->data_buf;


	if (count != sndpcm->period_size) {
		count = sndpcm->period_size;
	}

	while (count > 0) {

//		printf("snd_pcm_readi  count = %zu\n",count);
		r = snd_pcm_readi(sndpcm->handle, data, count);


//		printf("snd_pcm_readi%d  r = %zu\n",__LINE__,r);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
				printf("<<<<<<<<<<<<<<< EAGAIN >>>>>>>>>>>>>>>\n");
				snd_pcm_wait(sndpcm->handle, 100);
		} else if (r == -EPIPE) {

				snd_pcm_prepare(sndpcm->handle);
				printf("<<<<<<<<<<<<<<<snd_pcm_readi Buffer overrun >>>>>>>>>>>>>>>\n");
		} else if (r == -ESTRPIPE) {
				printf("<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
		} else if (r < 0) {

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
	char* data = sndpcm->data_buf;

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
			snd_pcm_prepare(sndpcm->handle);
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



int audio_data_read(snd_data_format *sndpcm, char* buf,size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;
	char* data = buf;


	if (count != sndpcm->period_size) {
		count = sndpcm->period_size;
	}

	while (count > 0) {

		r = snd_pcm_readi(sndpcm->handle, data, count);

		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
				printf("<<<<<<<<<<<<<<< EAGAIN >>>>>>>>>>>>>>>\n");
				snd_pcm_wait(sndpcm->handle, 100);
		} else if (r == -EPIPE) {

				snd_pcm_prepare(sndpcm->handle);
				printf("<<<<<<<<<<<<<<<snd_pcm_readi Buffer overrun >>>>>>>>>>>>>>>\n");
		} else if (r == -ESTRPIPE) {
				printf("<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
		} else if (r < 0) {
				exit(-1);
		}

		if (r > 0) {
			result += r;
			count -= r;
			data += r * sndpcm->bits_per_frame / 8;
		}
	}
	return result;
}

int audio_data_write(Psnd_data_format sndpcm, char* buf,int frame_len)
{
	int r;
	int result = 0;
//	char* data = buf;

//	if (frame_len < sndpcm->period_size) {
//		snd_pcm_format_set_silence(sndpcm->format,
//			data + frame_len * sndpcm->bits_per_frame / 8,
//			(sndpcm->period_size - frame_len) * sndpcm->channels);
//
//		frame_len = sndpcm->period_size;
//	}

//	while (frame_len > 0) {

		r = snd_pcm_writei(sndpcm->handle, buf, frame_len);

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
//			frame_len -= r;
//			data += r * sndpcm->bits_per_frame / 8;
		}
//	}

	return result;
}



/*
 * audio_snd_params_init
 * alsa参数配置
 *
 * 输入
 * Psnd_data_format
 * PWAVContainer
 * 输出
 * Psnd_data_format
 *
 * 返回值
 * 成功
 * 失败
 */
int audio_snd_params_init(Psnd_data_format sndpcm, PWAVContainer wav)
{

	snd_pcm_hw_params_t *hwparams;

	snd_pcm_format_t format;
	uint32 exact_rate;
	uint32 buffer_time, period_time;
	int dir = 0;

	/*为硬件参数申请内存*/
	snd_pcm_hw_params_alloca(&hwparams);

	/*初始化为默认参数*/
	if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_any Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}
	/*设置为交错模式*/
	if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_set_access Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	/*获取本地设置的format length*/
	if (audio_get_format(wav, &format) < 0)
	{
		printf("%s-%s-%d audio_get_format Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}
	/*设置format参数*/
	if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_set_format Error --%d\n",__FILE__,__func__,
				__LINE__,format);
		goto ERR_SET_PARAMS;
	}
	sndpcm->format = format;

	/*设置声道参数*/
	if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams,
			LE_SHORT(wav->format.channels)) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_set_channels Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}
	sndpcm->channels = LE_SHORT(wav->format.channels);

	/*设置采样率*/
	exact_rate = LE_INT(wav->format.sample_rate);
	if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_set_rate_near Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	if (LE_INT(wav->format.sample_rate) != exact_rate)
	{
		printf("%s-%s-%d The rate %d Hz is not supported by your hardware ==> Using %d Hz instead.\n",__FILE__,__func__,__LINE__,
				LE_INT(wav->format.sample_rate), exact_rate);
	}
	//获取缓冲区支持的最大time （us）
	if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_get_buffer_time_max Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	if(buffer_time > 500000)
		buffer_time = 500000;

//	buffer_time = 42666;//延时长
//	buffer_time = 31999;//中间丢包
//	buffer_time = 26666;//比较稳定
//	buffer_time = 21333;//抖动
	buffer_time = 10666;//抖动
//	buffer_time = 10000;//抖动
	period_time = buffer_time / 4;//5.3ms/1024B

	//设置buffer_time的值，dir(-1,0,1 exact value is <,=,>)
	if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, &dir) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_set_buffer_time_near Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	//设置周期时间 exact value is <,=,> val following dir (-1,0,1)
	if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, &dir) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params_set_period_time_near Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	/*将硬件参数设置到PCM设备*/
	if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0)
	{
		printf("%s-%s-%d snd_pcm_hw_params Error\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	/*从配置空间中获取周期大小*/
	snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->period_size, &dir);

	/*从配置空间中获取buffer_size大小*/
	snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);

	if (sndpcm->period_size == sndpcm->buffer_size)
	{
		printf("%s-%s-%d Can't use period equal to buffer size (%lu == %lu)\n",__FILE__,__func__,
				__LINE__,sndpcm->period_size, sndpcm->buffer_size);
		goto ERR_SET_PARAMS;
	}

	sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);
	sndpcm->chunk_bytes = sndpcm->period_size * sndpcm->bits_per_frame / 8;

	/* alloc mem save the block*/
	sndpcm->data_buf = (char*)malloc(sndpcm->chunk_bytes);

	if (!sndpcm->data_buf) {
		printf("%s-%s-%d Error malloc: [data_buf]\n",__FILE__,__func__,__LINE__);
		goto ERR_SET_PARAMS;
	}

	return SUCCESS;

ERR_SET_PARAMS:
	return ERROR;
}


/*
int set_params(Psnd_data_format sndpcm, PWAVContainer wav)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;
	snd_pcm_format_t format;
	size_t n;

	int start_delay = 0,stop_delay = 0;
	unsigned int rate;
	unsigned period_time = 0;
	unsigned buffer_time = 0;
	snd_pcm_uframes_t period_frames = 0;
	snd_pcm_uframes_t buffer_frames = 0;
	snd_pcm_uframes_t chunk_size = 0;
	snd_pcm_uframes_t start_threshold, stop_threshold;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(sndpcm->handle, params);

	if (err < 0) {
//		error(_("Broken configuration for this PCM: no configurations available"));
		return ERROR;
	}

	err = snd_pcm_hw_params_set_access(sndpcm->handle, params,
						   SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
//		error(_("Access type not available"));
		return ERROR;
	}
	获取本地设置的format length
	if (audio_get_format(wav, &format) < 0) {
		printf("Error get_snd_pcm_format\n");
		return ERROR;
	}
	err = snd_pcm_hw_params_set_format(sndpcm->handle, params, format);
	if (err < 0) {
//		error(_("Sample format non available"));
//		show_available_sample_formats(params);
		return ERROR;
	}
	sndpcm->format = format;
	err = snd_pcm_hw_params_set_channels(sndpcm->handle, params,
			LE_SHORT(wav->format.channels));
	if (err < 0) {
//		error(_("Channels count non available"));
		return ERROR;
	}
	sndpcm->channels = LE_SHORT(wav->format.channels);
#if 0
	err = snd_pcm_hw_params_set_periods_min(handle, params, 2);
	assert(err >= 0);
#endif
	rate = wav->format.sample_rate;
	err = snd_pcm_hw_params_set_rate_near(sndpcm->handle, params,
			&rate, 0);

	if ((float)rate * 1.05 < wav->format.sample_rate || (float)rate * 0.95 > wav->format.sample_rate) {
		return ERROR;
	}
	rate = wav->format.sample_rate;
//	if (buffer_time == 0 && buffer_frames == 0) {
//		err = snd_pcm_hw_params_get_buffer_time_max(params,
//							    &buffer_time, 0);
//
//		if (buffer_time > 500000)
//			buffer_time = 500000;
//	}
	//获取缓冲区支持的最大time （us）
	if (snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0) < 0)
	{
		printf( "Error snd_pcm_hw_params_get_buffer_time_max\n");
		return ERROR;
	}
	if (buffer_time > 500000) buffer_time = 500000;
	buffer_time = 21333;
//	buffer_time = 42666;
//	buffer_time = 10666;
	period_time = buffer_time / 4;//5.3ms/1024B

//	if (period_time == 0 && period_frames == 0) {
//		if (buffer_time > 0)
//		{
//			buffer_time = 21333;
//		//	buffer_time = 42666;
//		//	buffer_time = 10666;
//			period_time = buffer_time / 4;//5.3ms/1024B
//		}
//		else
//			period_frames = buffer_frames / 4;
//	}
//	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(sndpcm->handle, params,
							     &period_time, 0);
//	else
//		err = snd_pcm_hw_params_set_period_size_near(sndpcm->handle, params,
//							     &period_frames, 0);

//	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, params,
							     &buffer_time, 0);
//	} else {
//		err = snd_pcm_hw_params_set_buffer_size_near(sndpcm->handle, params,
//							     &buffer_frames);
//	}

	err = snd_pcm_hw_params(sndpcm->handle, params);
	if (err < 0) {
//		error(_("Unable to install hw params:"));
		return ERROR;
	}
	snd_pcm_hw_params_get_period_size(params, &sndpcm->period_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &sndpcm->buffer_size);
	if (sndpcm->period_size == sndpcm->buffer_size) {
//		error(_("Can't use period equal to buffer size (%lu == %lu)"),
//		      chunk_size, buffer_size);
		return ERROR;
	}
	snd_pcm_sw_params_current(sndpcm->handle, swparams);

	n = sndpcm->period_size;

	err = snd_pcm_sw_params_set_avail_min(sndpcm->handle, swparams, n);

	 round up to closest transfer boundary
	n = sndpcm->buffer_size;
	if (start_delay <= 0) {
		start_threshold = n + (double) rate * start_delay / 1000000;
	} else
		start_threshold = (double) rate * start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(sndpcm->handle, swparams, start_threshold);

	if (stop_delay <= 0)
		stop_threshold = sndpcm->buffer_size + (double) rate * stop_delay / 1000000;
	else
		stop_threshold = (double) rate * stop_delay / 1000000;
	err = snd_pcm_sw_params_set_stop_threshold(sndpcm->handle, swparams, stop_threshold);


	if (snd_pcm_sw_params(sndpcm->handle, swparams) < 0) {
//		error(_("unable to install sw params:"));
		return ERROR;
	}

	sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
	sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);
	sndpcm->chunk_bytes = sndpcm->period_size * sndpcm->bits_per_frame / 8;

	 alloc mem save the block
//	sndpcm->data_buf = (uint8_t *)realloc(sndpcm->data_buf,sndpcm->chunk_bytes);
	sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);
	if (!sndpcm->data_buf) {
		printf("Error malloc: [data_buf]\n");
		return ERROR;
	}

//	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
//	significant_bits_per_sample = snd_pcm_format_width(hwparams.format);
//	bits_per_frame = bits_per_sample * hwparams.channels;
//	chunk_bytes = chunk_size * bits_per_frame / 8;
//	audiobuf = realloc(audiobuf, chunk_bytes);
//	if (audiobuf == NULL) {
////		error(_("not enough memory"));
//	}

	return SUCCESS;
}
*/



