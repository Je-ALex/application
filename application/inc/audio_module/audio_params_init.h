/*
 * audio_parameter_init.h
 *
 *  Created on: 2016年11月7日
 *      Author: leon
 */

#ifndef INC_AUDIO_MODULE_AUDIO_PARAMS_INIT_H_
#define INC_AUDIO_MODULE_AUDIO_PARAMS_INIT_H_

#include "audio_server.h"

int set_params(Psnd_data_format sndpcm, PWAVContainer wav);

int audio_snd_params_init(Psnd_data_format sndpcm, PWAVContainer wav);

int audio_format_init(PWAVContainer wav);

ssize_t audio_module_data_read(snd_data_format *sndpcm,snd_data_format *sndpcm_p,
		size_t rcount);

int audio_module_data_write(Psnd_data_format sndpcm, int frame_len);

int  time_substract( timetime *result, struct timeval *begin,struct timeval *end);


int audio_data_read(snd_data_format *sndpcm, char* buf,size_t rcount);
int audio_data_write(Psnd_data_format sndpcm, char* buf,int frame_len);



#endif /* INC_AUDIO_MODULE_AUDIO_PARAMS_INIT_H_ */
