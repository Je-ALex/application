

CC = gcc

HOSTCFLAGS = -g -Wall -I/home/wang/work/hushan/alsa-install-gcc/alsa_lib/include -I../../inc/audio-tcp
HOSTLIBS = -L/home/wang/work/hushan/alsa-install-gcc/alsa_lib/lib -lasound -lpthread

ARMCFLAGS = -g -Wall -I/home/wang/work/hushan/alsa_install/alsa_lib/include -I../../inc/audio-tcp
ARMLIBS = -L/home/wang/work/hushan/alsa_install/alsa_lib/lib -lasound -lpthread

.PHNOY: server client

server:
	$(CC) -o server audio_tcp_server.c sndwav_common.c wav_parser.c \
	../ctrl-tcp/tcp_ctrl_queue.c $(HOSTCFLAGS) $(HOSTLIBS) 
client:
	$(CC) -o client lrecord.c sndwav_common.c wav_parser.c $(HOSTCFLAGS) $(HOSTLIBS) 
	
arm-server:
	$(CC) -o arm-server audio_tcp_server.c sndwav_common.c \
	wav_parser.c ../ctrl-tcp/tcp_ctrl_queue.c $(ARMCFLAGS) $(ARMLIBS) 
#all:lplay lrecord
#
#lplay: lplay.o sndwav_common.o wav_parser.o
#	$(CC) $(CFLAGS) lplay.o sndwav_common.o wav_parser.o -o lplay $(LIBS) 
#lplay.o: lplay.c  sndwav_common.h wav_parser.h
#	$(CC) $(CFLAGS) -c lplay.c  
#
#
#lrecord: lrecord.o sndwav_common.o wav_parser.o 
#	$(CC) $(CFLAGS) lrecord.o sndwav_common.o wav_parser.o -o lrecord $(LIBS)
#lrecord.o: lrecord.c sndwav_common.h  wav_parser.h
#	$(CC) $(CFLAGS) -c lrecord.c 
#
#
#sndwav_common.o: sndwav_common.c sndwav_common.h
#	$(CC) $(CFLAGS) -c sndwav_common.c 
#
#
#wav_parser.o: wav_parser.c wav_parser.h
#	$(CC) $(CFLAGS) -c wav_parser.c

#tcp.o: client.c
#	$(CC) $(CFLAGS) -c $^
clean:
	-rm server client

#子目录的Makefile直接读取其子目录就行
#SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
##以下同根目录下的makefile的相同代码的解释
#CUR_SOURCE=${wildcard *.c}
#CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
#all:$(SUBDIRS) $(CUR_OBJS)
#$(SUBDIRS):ECHO
#	make -C $@
#$(CUR_OBJS):%.o:%.c
#	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
#ECHO:
#	@echo $(SUBDIRS)