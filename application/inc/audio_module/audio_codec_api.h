/*
 * audio_codec_api.h
 *
 *  Created on: 2017年5月2日
 *      Author: leon
 */

#ifndef INC_AUDIO_MODULE_AUDIO_CODEC_API_H_
#define INC_AUDIO_MODULE_AUDIO_CODEC_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#include "opus.h"
#include "opus_types.h"
#include "opus_multistream.h"


#define MAX_PACKET 1500

typedef struct {

	int application;
	int channels;
	int sampling_rate;
	int frame_size;
	int bitrate_bps;
	int bandwidth;

	int cvbr ;
	int use_vbr ;
	int max_payload_bytes ;
	int complexity ;
	int use_inbandfec ;
	int forcechannels ;
	int use_dtx ;
	int packet_loss_perc ;
	int variable_duration;
	int skip;

	short* in;
	short* out;
	unsigned char* data;

	unsigned char* enodata;
	unsigned char* deodata;
}framecontent,*Pframecontent;

int encoder_deinit(OpusEncoder *enc,Pframecontent frame_prop);
OpusEncoder* encoder_init(Pframecontent frame_prop);
int do_encode(OpusEncoder *enc,Pframecontent frame_prop, char* buf,int framelen,FILE* fout);

int decoder_deinit(OpusDecoder *dec,Pframecontent frame_prop);
OpusDecoder* decoder_init(Pframecontent frame_prop);
int do_decode(OpusDecoder *dec,Pframecontent frame_prop,int len,int enc_final_range,
		FILE* fin);


#endif /* INC_AUDIO_MODULE_AUDIO_CODEC_API_H_ */
