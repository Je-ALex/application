
#CC = $(CROSS_COMPILE)gcc
CC = arm-fsl-linux-gnueabi-gcc
#CC = gcc
CFLAGS = -g -Wall -I/home/wang/work/hushan/alsa_install/alsa_lib/include
LIBS = -L/home/wang/work/hushan/alsa_install/alsa_lib/lib -lasound -lpthread
all:lplay lrecord

lplay: lplay.o sndwav_common.o wav_parser.o
	$(CC) $(CFLAGS) lplay.o sndwav_common.o wav_parser.o -o lplay $(LIBS) 
lplay.o: lplay.c  sndwav_common.h wav_parser.h
	$(CC) $(CFLAGS) -c lplay.c  


lrecord: lrecord.o sndwav_common.o wav_parser.o 
	$(CC) $(CFLAGS) lrecord.o sndwav_common.o wav_parser.o -o lrecord $(LIBS)
lrecord.o: lrecord.c sndwav_common.h  wav_parser.h
	$(CC) $(CFLAGS) -c lrecord.c 


sndwav_common.o: sndwav_common.c sndwav_common.h
	$(CC) $(CFLAGS) -c sndwav_common.c 


wav_parser.o: wav_parser.c wav_parser.h
	$(CC) $(CFLAGS) -c wav_parser.c

#tcp.o: client.c
#	$(CC) $(CFLAGS) -c $^
clean:
	rm lplay lrecord *.o
