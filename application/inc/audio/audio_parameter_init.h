/*
 * audio_parameter_init.h
 *
 *  Created on: 2016年11月7日
 *      Author: leon
 */

#ifndef INC_AUDIO_AUDIO_PARAMETER_INIT_H_
#define INC_AUDIO_AUDIO_PARAMETER_INIT_H_

#include "../audio/audio_tcp_server.h"
#include "tcp_ctrl_queue.h"


int audio_snd_params_init(Psnd_data_format sndpcm, PWAVContainer wav);

int audio_format_init(PWAVContainer wav);

int audio_module_data_write(Psnd_data_format sndpcm, int frame_len);

#endif /* INC_AUDIO_AUDIO_PARAMETER_INIT_H_ */
