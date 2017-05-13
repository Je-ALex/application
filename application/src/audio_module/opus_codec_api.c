

#include "audio_codec_api.h"


/*
 * 压缩
 */
static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}


int encoder_deinit(OpusEncoder *enc,Pframecontent frame_prop)
{

    opus_encoder_destroy(enc);
    free(frame_prop->data);
    free(frame_prop->in);
    free(frame_prop->enodata);

	return 0;
}

OpusEncoder* encoder_init(Pframecontent frame_prop)
{
    OpusEncoder *enc=NULL;
	int err;
	const char *bandwidth_string;

    /* defaults: */
	frame_prop->application = OPUS_APPLICATION_AUDIO;
	frame_prop->cvbr = 0;
	frame_prop->use_vbr = 1;
	frame_prop->max_payload_bytes = MAX_PACKET;
	frame_prop->complexity = 10;
	frame_prop->use_inbandfec = 0;
	frame_prop->forcechannels = OPUS_AUTO;
	frame_prop->use_dtx = 0;
	frame_prop->packet_loss_perc = 0;

	frame_prop->sampling_rate = 48000;
	frame_prop->frame_size = frame_prop->sampling_rate/400;
	frame_prop->bitrate_bps = 256000;
	frame_prop->channels=2;
	frame_prop->bandwidth = OPUS_AUTO;
	frame_prop->skip = 0;
	frame_prop->variable_duration = OPUS_FRAMESIZE_ARG;

	enc = opus_encoder_create(frame_prop->sampling_rate, frame_prop->channels,
			frame_prop->application, &err);

	if (err != OPUS_OK)
	{
	  printf("Cannot create encoder: %s\n", opus_strerror(err));
	  return NULL;
	}
	opus_encoder_ctl(enc, OPUS_SET_BITRATE(frame_prop->bitrate_bps));
	opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(frame_prop->bandwidth));
	opus_encoder_ctl(enc, OPUS_SET_VBR(frame_prop->use_vbr));
	opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(frame_prop->cvbr));
	opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(frame_prop->complexity));
	opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(frame_prop->use_inbandfec));
	opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(frame_prop->forcechannels));
	opus_encoder_ctl(enc, OPUS_SET_DTX(frame_prop->use_dtx));
	opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(frame_prop->packet_loss_perc));

	opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&frame_prop->skip));
	opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));
	opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(frame_prop->variable_duration));

    switch(frame_prop->bandwidth)
    {
    case OPUS_BANDWIDTH_NARROWBAND:
         bandwidth_string = "narrowband";
         break;
    case OPUS_BANDWIDTH_MEDIUMBAND:
         bandwidth_string = "mediumband";
         break;
    case OPUS_BANDWIDTH_WIDEBAND:
         bandwidth_string = "wideband";
         break;
    case OPUS_BANDWIDTH_SUPERWIDEBAND:
         bandwidth_string = "superwideband";
         break;
    case OPUS_BANDWIDTH_FULLBAND:
         bandwidth_string = "fullband";
         break;
    case OPUS_AUTO:
         bandwidth_string = "auto bandwidth";
         break;
    default:
         bandwidth_string = "unknown";
         break;
    }

   fprintf(stderr, "Encoding %ld Hz input at %.3f kb/s "
			   "in %s with %d-sample frames.\n",
			   (long)frame_prop->sampling_rate,frame_prop->bitrate_bps*0.001,
			   bandwidth_string,frame_prop->frame_size);

   frame_prop->in = (short*)malloc(frame_prop->frame_size*frame_prop->channels*sizeof(short));
   frame_prop->data = (unsigned char*)calloc(frame_prop->max_payload_bytes,sizeof(unsigned char));
   frame_prop->enodata = (unsigned char*)calloc(frame_prop->max_payload_bytes,sizeof(unsigned char));

   return enc;
}

int do_encode(OpusEncoder *enc,Pframecontent frame_prop, char* buf,int framelen,FILE* fout)
{

	int i,len;
	opus_uint32 enc_final_range;

	//转为16bits
	for(i=0;i<framelen*frame_prop->channels;i++)
	{
		opus_int32 s;
		s=buf[2*i+1]<<8 | buf[2*i];
		s=((s&0xFFFF)^0x8000)-0x8000;
		frame_prop->in[i]=s;
	}

	len = opus_encode(enc,frame_prop->in,frame_prop->frame_size,
			frame_prop->data,frame_prop->max_payload_bytes);

	opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
	if (len < 0)
	{
		fprintf (stderr, "opus_encode() returned %d\n", len);
		return -1;
	}

	if(fout != NULL)
	{
		//写入文本
		unsigned char int_field[4];

		int_to_char(len, int_field);
		if (fwrite(int_field, 1, 4, fout) != 4)
		{
		   fprintf(stderr, "Error writing.\n");
		   return -1;
		}
		int_to_char(enc_final_range, int_field);
		if (fwrite(int_field, 1, 4, fout) != 4)
		{
		   fprintf(stderr, "Error writing.\n");
		   return -1;
		}

		if (fwrite(frame_prop->data, 1, len, fout) != (unsigned)len)
		{
		   fprintf(stderr, "Error writing.\n");
		   return -1;
		}
	}else{
		unsigned char int_field[4];

		int_to_char(len, int_field);
		memcpy(frame_prop->enodata,int_field,4);

		int_to_char(enc_final_range, int_field);
		memcpy(&frame_prop->enodata[4],int_field,4);

		memcpy(&frame_prop->enodata[8],frame_prop->data,len);

	}

	return len+8;
}


/*
 * 解压缩
 */

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

int decoder_deinit(OpusDecoder *dec,Pframecontent frame_prop)
{

	opus_decoder_destroy(dec);
    free(frame_prop->data);
    free(frame_prop->out);
    free(frame_prop->deodata);

	return 0;
}
OpusDecoder* decoder_init(Pframecontent frame_prop)
{
    OpusDecoder *dec=NULL;
	int err;

    /* defaults: */
	frame_prop->max_payload_bytes = MAX_PACKET;
	frame_prop->sampling_rate = 48000;
	frame_prop->frame_size = frame_prop->sampling_rate/400;
	frame_prop->channels=2;


    dec = opus_decoder_create(frame_prop->sampling_rate, frame_prop->channels, &err);
    if (err != OPUS_OK)
    {
       fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(err));

       return NULL;
    }
    frame_prop->data = (unsigned char*)calloc(frame_prop->max_payload_bytes,sizeof(unsigned char));
    frame_prop->out = (short*)malloc(frame_prop->frame_size*frame_prop->channels*sizeof(short));
    frame_prop->deodata = (unsigned char*)malloc(frame_prop->frame_size*frame_prop->channels*sizeof(short));
    fprintf(stderr, "Decoding with %ld Hz output (%d channels)\n",
                    (long)frame_prop->sampling_rate, frame_prop->channels);

    return dec;
}


int do_decode(OpusDecoder *dec,Pframecontent frame_prop,int len,int enc_final_range,
		FILE* fin)
{
	int err;
    int output_samples;
    opus_uint32 dec_final_range;
    int max_frame_size = 48000*2;


    if(fin != NULL)
    {
        unsigned char ch[4];
        err = fread(ch, 1, 4, fin);
        if (feof(fin))
        	return -1;
        len = char_to_int(ch);
        if (len>frame_prop->max_payload_bytes || len<0)
        {
            fprintf(stderr, "Invalid payload length: %d\n",len);
            return -1;
        }
        err = fread(ch, 1, 4, fin);
        enc_final_range = char_to_int(ch);

        err = fread(frame_prop->data, 1, len, fin);
        if (err<len)
        {
            fprintf(stderr, "Ran out of input, "
                            "expecting %d bytes got %d\n",
                            len,err);
            return -1;
        }
    }


	output_samples = max_frame_size;

	output_samples = opus_decode(dec, frame_prop->data, len, frame_prop->out,
			output_samples, 0);

	if (output_samples>0)
	{
	   int i;
	   for(i=0;i<output_samples*frame_prop->channels;i++)
	   {
		  short s;
		  s=frame_prop->out[i];
		  frame_prop->deodata[2*i]=s&0xFF;
		  frame_prop->deodata[2*i+1]=(s>>8)&0xFF;
	   }

	} else {
	   fprintf(stderr, "error decoding frame: %s\n",
					   opus_strerror(output_samples));
	}

	opus_decoder_ctl(dec, OPUS_GET_FINAL_RANGE(&dec_final_range));

	if( enc_final_range!=0 && dec_final_range != enc_final_range ) {

		fprintf (stderr, "Error: Range coder state mismatch "
                     "between encoder and decoder "
                     "in frame 0x%8lx vs 0x%8lx\n",

                 (unsigned long)enc_final_range,
                 (unsigned long)dec_final_range);

		return -1;
	}

	return 0;
}


