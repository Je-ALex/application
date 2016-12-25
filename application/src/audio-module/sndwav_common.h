//File   : sndwav_common.h

#ifndef __SNDWAV_COMMON_H
#define __SNDWAV_COMMON_H

#include "../../inc/audio/audio_server.h"



typedef long long off64_t;


ssize_t SNDWAV_ReadPcm(snd_data_format *sndpcm,snd_data_format *sndpcm_p,  size_t rcount);


ssize_t SNDWAV_WritePcm(snd_data_format *sndpcm, size_t wcount);

int SNDWAV_SetParams(snd_data_format *sndpcm, WAVContainer_t *wav);
int play_SetParams(snd_data_format *sndpcm, WAVContainer_t *wav);



#endif /* #ifndef __SNDWAV_COMMON_H */
