//File   : wav_parser.c



#include "../../inc/audio/audio_tcp_server.h"

#define WAV_PRINT_MSG

char *WAV_P_FmtString(uint16_t fmt)
{
	switch (fmt) {
	case WAV_FMT_PCM:
		return "PCM";
		break;
	case WAV_FMT_IEEE_FLOAT:
		return "IEEE FLOAT";
		break;
	case WAV_FMT_DOLBY_AC3_SPDIF:
		return "DOLBY AC3 SPDIF";
		break;
	case WAV_FMT_EXTENSIBLE:
		return "EXTENSIBLE";
		break;
	default:
		break;
	}

	return "NON Support Fmt";
}

void WAV_P_PrintHeader(WAVContainer_t *wav_header)
{
	printf("+++++++++++++++++++++++++++\n");
	printf("\n");

	printf("File Magic:         [%c%c%c%c]\n",
		(char)(wav_header->header.magic),
		(char)(wav_header->header.magic>>8),
		(char)(wav_header->header.magic>>16),
		(char)(wav_header->header.magic>>24));
	printf("File Length:        [%d]\n", wav_header->header.length);
	printf("File Type:          [%c%c%c%c]\n",
	(char)(wav_header->header.type),
	  (char)(wav_header->header.type>>8),
	  (char)(wav_header->header.type>>16),
	  (char)(wav_header->header.type>>24));
	printf("\n");
	printf("Fmt Magic:          [%c%c%c%c]\n",
	(char)(wav_header->format.magic),
	  (char)(wav_header->format.magic>>8),
	  (char)(wav_header->format.magic>>16),
	  (char)(wav_header->format.magic>>24));
	printf("Fmt Size:           [%d]\n", wav_header->format.fmt_size);
	printf("Fmt Format:         [%s]\n", WAV_P_FmtString(wav_header->format.format));
	printf("Fmt Channels:       [%d]\n", wav_header->format.channels);
	printf("Fmt Sample_rate:    [%d](HZ)\n", wav_header->format.sample_rate);
	printf("Fmt Bytes_p_second: [%d]\n", wav_header->format.bytes_p_second);
	printf("Fmt Blocks_align:   [%d]\n", wav_header->format.blocks_align);
	printf("Fmt Sample_length:  [%d]\n", wav_header->format.sample_length);

	printf("\n");

	printf("Chunk Type:         [%c%c%c%c]\n",
		(char)(wav_header->chunk.type),
		(char)(wav_header->chunk.type>>8),
		(char)(wav_header->chunk.type>>16),
		(char)(wav_header->chunk.type>>24));
	printf("Chunk Length:       [%d]\n", wav_header->chunk.length);

	printf("\n");
	printf("++++++++++++++++++++++++++++++++++++++\n");
}

int WAV_P_CheckValid(WAVContainer_t *container)
{
	if (container->header.magic != WAV_RIFF ||
		container->header.type != WAV_WAVE ||
		container->format.magic != WAV_FMT ||
		container->format.fmt_size != LE_INT(16) ||
(container->format.channels != LE_SHORT(1) && container->format.channels != LE_SHORT(2))
 || container->chunk.type != WAV_DATA) {
		fprintf(stderr, "non standard wav file./n");
		return -1;
	}

	return 0;
}
/*
 * 函数：音频文件数据格式初始化
 */
int audio_parameters_init(WAVContainer_t *wav)
{
	wav->header.magic = WAV_RIFF;
	wav->header.type = WAV_WAVE;
	wav->format.magic = WAV_FMT;
	wav->format.fmt_size = LE_INT(16);
	wav->format.format = LE_SHORT(WAV_FMT_PCM);
	wav->chunk.type = WAV_DATA;

	wav->format.channels = DEFAULT_CHANNELS;
	wav->format.sample_rate = DEFAULT_SAMPLE_RATE;
	wav->format.sample_length = DEFAULT_SAMPLE_LENGTH;
	wav->format.blocks_align = LE_SHORT(wav->format.channels * wav->format.sample_length / 8);
	wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * wav->format.sample_rate);
	wav->chunk.length = LE_INT(DEFAULT_DURATION_TIME * (uint32_t)(wav->format.bytes_p_second));
	wav->header.length = LE_INT((uint32_t)(wav->chunk.length) + sizeof(wav->chunk) + sizeof(wav->format) + sizeof(wav->header) - 8);

#ifdef WAV_PRINT_MSG
	WAV_P_PrintHeader(wav);
#endif
	return 0;

}

int WAV_WriteHeader(int fd, WAVContainer_t *container)
{
	assert((fd >=0) && container);

	if (WAV_P_CheckValid(container) < 0)
		return -1;

	if (write(fd,&container->header,sizeof(container->header))!=sizeof(container->header)
	||write(fd,&container->format,sizeof(container->format))!=sizeof(container->format)
	||write(fd,&container->chunk,sizeof(container->chunk))!=sizeof(container->chunk)) {
			fprintf(stderr, "Error WAV_WriteHeader/n");
			return -1;
		}

#ifdef WAV_PRINT_MSG
	WAV_P_PrintHeader(container);
#endif

	return 0;
}
