
#ifndef __WAV_PARSER_H
#define __WAV_PARSER_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)           (v)
#define LE_INT(v)               (v)
#define BE_SHORT(v)           bswap_16(v)
#define BE_INT(v)               bswap_32(v)

#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d) ((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)           bswap_16(v)
#define LE_INT(v)               bswap_32(v)
#define BE_SHORT(v)           (v)
#define BE_INT(v)               (v)
#else
#error "Wrong endian"
#endif

#define WAV_RIFF        COMPOSE_ID('R','I','F','F')
#define WAV_WAVE        COMPOSE_ID('W','A','V','E')
#define WAV_FMT         COMPOSE_ID('f','m','t',' ')
#define WAV_DATA        COMPOSE_ID('d','a','t','a')

#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe


#define WAV_GUID_TAG "/x00/x00/x00/x00/x10/x00/x80/x00/x00/xAA/x00/x38/x9B/x71"


#define DEFAULT_CHANNELS         (2)
#define DEFAULT_SAMPLE_RATE      (48000)
#define DEFAULT_SAMPLE_LENGTH    (16)
#define DEFAULT_DURATION_TIME    (10000)

typedef struct WAVHeader {
	uint32_t magic;
	uint32_t length;
	uint32_t type;
} WAVHeader_t;

typedef struct WAVFmt {
	uint32_t magic;
	uint32_t fmt_size;
	uint16_t format;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t bytes_p_second;
	uint16_t blocks_align;
	uint16_t sample_length;
} WAVFmt_t;

typedef struct WAVFmtExtensible {
	WAVFmt_t format;
	uint16_t ext_size;
	uint16_t bit_p_spl;
	uint32_t channel_mask;
	uint16_t guid_format;
	uint8_t guid_tag[14];
} WAVFmtExtensible_t;

typedef struct WAVChunkHeader {
	uint32_t type;
	uint32_t length;
} WAVChunkHeader_t;

typedef struct WAVContainer {
	WAVHeader_t header;
	WAVFmt_t format;
	WAVChunkHeader_t chunk;
} WAVContainer_t;

int WAV_ReadHeader(WAVContainer_t *container);
int audio_parameters_init(WAVContainer_t *container);

int WAV_WriteHeader(int fd, WAVContainer_t *container);

#endif /* #ifndef __WAV_PARSER_H */
