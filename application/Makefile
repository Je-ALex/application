#设置编译器
#
#INCLUDE = ./inc
#
#CC=gcc
#LIBS = -lpthread
#CFLAGS = -g -Wall -I$(INCLUDE)
#
##debug文件夹里的makefile文件需要最后执行，
##所以这里需要执行的子目录要排除debug文件夹，这里使用awk排除了debug文件夹，读取剩下的文件夹
#SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "debug") print $$9}')
#
##无需下一行的注释代码，因为我们已经知道debug里的makefile是最后执行的，
##所以最后直接去debug目录下执行指定的makefile文件就行，具体下面有注释
##DEBUG=$(shell ls -l | grep ^d | awk '{if($$9 == "debug") print $$9}')
##记住当前工程的根目录路径
#ROOT_DIR=$(shell pwd)
#
##最终bin文件的名字，可以更改为自己需要的
#BIN=myapp.so
#
##目标文件所在的目录
#OBJS_DIR=debug/obj
#
##bin文件所在的目录
#BIN_DIR=debug/bin
#
##获取当前目录下的c文件集，放在变量CUR_SOURCE中
#CUR_SOURCE=${wildcard *.c}
#
##将对应的c文件名转为o文件后放在下面的CUR_OBJS变量中
#CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
#
##将以下变量导出到子shell中，本次相当于导出到子目录下的makefile中
#export CC BIN OBJS_DIR BIN_DIR ROOT_DIR LIBS CFLAGS S INFLAGS
#
##注意这里的顺序，需要先执行SUBDIRS最后才能是DEBUG
#all:$(SUBDIRS) $(CUR_OBJS) DEBUG
#
##递归执行子目录下的makefile文件，这是递归执行的关键
#$(SUBDIRS):ECHO
#	$(MAKE) -C $@
##将c文件编译为o文件，并放在指定放置目标文件的目录中即OBJS_DIR
#$(CUR_OBJS):%.o:%.c
#	$(CC) $(CFLAGS) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
#	
#DEBUG:ECHO
##直接去debug目录下执行makefile文件
#	$(MAKE) -C debug
#ECHO:
#	@echo $(SUBDIRS)
#
#clean:
#	@rm $(OBJS_DIR)/*.o
#	@rm -rf $(BIN_DIR)/*

#all:
#	${CC} -o server src/ctrl-tcp/tcp_ctrl_*.c src/udp/ctrl_udp.c ${CFLAGS} 
#
#
#clean:
#	rm server

#using for testing some code in Linux 
#
#CC=gcc
#SUBSRC = /src
#INCLUDE= /inc
#VPATH = src:inc
#
#TEST_SOURCE = ctrl-tcp/tcp_ctrl_api.c \
#			ctrl-tcp/tcp_ctrl_data_process.c \
#			ctrl-tcp/tcp_ctrl_data_compose.c \
#			ctrl-tcp/tcp_ctrl_staus.c \
#			ctrl-tcp/tcp_ctrl_list.c \
#			ctrl-tcp/tcp_ctrl_queue.c \
#			ctrl-tcp/tcp_ctrl_server.c\
#			udp/ctrl_udp.c
#				  
#TEST_SOURCE_SO =  $(SUBSRC)/ctrl-tcp/tcp_ctrl_api.c \
#			$(SUBSRC)/ctrl-tcp/tcp_ctrl_data_process.c \
#			$(SUBSRC)/ctrl-tcp/tcp_ctrl_data_compose.c \
#			$(SUBSRC)/ctrl-tcp/tcp_ctrl_staus.c \
#			$(SUBSRC)/ctrl-tcp/tcp_ctrl_list.c \
#			$(SUBSRC)/ctrl-tcp/tcp_ctrl_queue.c \
#			$(SUBSRC)/ctrl-tcp/tcp_ctrl_server.c\
#			$(SUBSRC)/udp/ctrl_udp.c
#
#OBJ=${patsubst %.c, %.o,$(TEST_SOURCE_SO)}
#	
#		
#testso:$(OBJ)
#	$(CC) -c $^ -lpthread -fPIC -shared -o libtest.so
#
#
#echo: 
#	@echo $(TEST_SOURCE_SO)
#	@echo $(OBJ)
#	gcc -MM src/ctrl-tcp/tcp_ctrl_api.c
#	
#$(OBJ):%.o:%.c
#	$(CC) -fPIC -o $@ -c $<
#	
#
#test:
#	$(CC) $(TEST_SOURCE) -g -DDEBUG -I$(INCLUDE_PATH) -lpthread  -fPIC -o test
#
#print:*.c
#	lpr -p $?
#	touch print
##testso:
##	$(CC) $(TEST_SOURCE_SO) -g -DDEBUG -I$(INCLUDE) -lpthread -fPIC  -shared -o libtest.so
#
#	
#.PHONY:test testso echo
#
#CUR_DIR=$(shell pwd)
#
#OBJS_DIR=debug/obj
#
###bin文件所在的目录
#BIN_DIR=debug/bin
#
#SRC_TCP_DIR=${CUR_DIR}/src/ctrl-tcp
#SRC_UDP_DIR=${CUR_DIR}/src/udp

#SUB_DIR=${CUR_DIR}/sub
#MUL_DIR=${CUR_DIR}/multis
#MAIN_DIR=${CUR_DIR}/main

#INC_DIR= -I${CUR_DIR}/inc
#
#SRC = ${wildcard  ${SRC_TCP_DIR}/*.c} \
#      ${wildcard  ${SRC_UDP_DIR}/*.c}
#      
#OBJ = ${patsubst %.c, %.o, ${SRC}}
#
#NOT_DIR = $(notdir $(OBJ))
#
#TARGET=main
#CC=gcc
#CCFLAGS=${INC_DIR}
#
#
#	
#${TARGET}:
#	@echo "Compiling $< ==> $@"
#	${CC} -o $@ $(CUR_DIR)/$(OBJS_DIR)/$(notdir $(OBJ)) -lpthread
#	@echo "Compile done."
#
#${OBJ}:%.o:%.c
#	@echo "Compiling $< ==> $(CUR_DIR)/$(OBJS_DIR)/$(notdir $@)"
#	${CC} ${CCFLAGS} -c $^ -o $(CUR_DIR)/$(OBJS_DIR)/$(notdir $@)
#
#.PHONY:$(TARGET) clean 
#
#clean:
#	@rm -f ${OBJ}
#	@echo "Clean object files done."
#
#	@rm -f *~
#	@echo "Clean tempreator files done."
#
#	@rm -f ${TARGET}
#	@echo "Clean target files done."
#
#	@echo "Clean done."

CC=gcc

HOSTCFLAGS = -g -Wall -I/home/wang/work/hushan/alsa-install-gcc/alsa_lib/include \
	-I./inc -I./inc/audio -I./inc/control -I./inc/utils

HOSTLIBS = -L/home/wang/work/hushan/alsa-install-gcc/alsa_lib/lib \
	-lasound -lpthread -L/home/wang/work/hushan/rtp/install-pc/lib -lspeexdsp -lm

ARMCFLAGS = -g -Wall -I/home/wang/work/hushan/alsa_install/alsa_lib/include \
	-I./inc -I./inc/audio -I./inc/control -I./inc/utils
	
ARMLIBS = -L/home/wang/work/hushan/alsa_install/alsa_lib/lib \
	-lasound -lpthread -lm

CSOFLAGS= -shared -lc -fPIC -I./inc


TEST_SOURCE = wifi_sys_init.c \
			src/ctrl-module/tcp_ctrl_server.c \
			src/scanf-module/ctrl_udp.c \
			src/ctrl-module/tcp_ctrl_api.c \
			src/ctrl-module/tcp_ctrl_data_process.c \
			src/ctrl-module/tcp_ctrl_data_process_pc.c \
			src/ctrl-module/tcp_ctrl_data_compose.c \
			src/ctrl-module/tcp_ctrl_device_status.c \
			src/utils/sys_list.c \
			src/utils/sys_queue.c \
			src/audio-module/audio_server.c \
			src/audio-module/audio_params_init.c \
			src/audio-module/audio_ring_buf.c 

#.PHONY: libso bin arm-bin clean

libso:
	${CC} -o libctrl.so src/ctrl-tcp/tcp_ctrl_*.c src/udp/ctrl_udp.c $(CSOFLAGS)
	
bin:
	${CC} -o server $(TEST_SOURCE) $(HOSTCFLAGS) $(HOSTLIBS)
	
arm-bin:
	${CC} -o arm-server $(TEST_SOURCE) $(ARMCFLAGS) $(ARMLIBS)
	
clean:
	rm server arm-server
	
#编译为库文件
#libtest.so:libtest.o
#	gcc -shared libtest.o -lc -o libtest.so
#libtest.o:pthread.c
#	gcc -fPIC -c pthread.c -o libtest.o
#all:libtest.so

