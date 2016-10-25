#
#udp目录中生成本模块的链接文件
#看情况需要可以编译为动态库文件，也可以编译到可执行文件
#
#
#CC:=arm-linux-gcc
#CFLAGS = -g -Wall
#CUR_DIR = $(shell pwd)
#OBJ_DIR = $(CUR_DIR)/obj
#
#SRC = $(wildcard *.c)  
#OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,${SRC})
#
#udpserver:${OBJ}
#	$(CC) $(OBJ) -o udpserver -lpthread
#$(OBJ_DIR)/%.o:${CUR_DIR}/%.c
#	$(CC) $(CFLAGS) -c $< -o $@
#
#all: udpserver
#
#clean:
#	rm -f *.o *.so udpserver

#子目录的Makefile直接读取其子目录就行
SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
#以下同根目录下的makefile的相同代码的解释
CUR_SOURCE=${wildcard *.c}
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
all:$(SUBDIRS) $(CUR_OBJS)
$(SUBDIRS):ECHO
	make -C $@
$(CUR_OBJS):%.o:%.c
#	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
	$(CC) -fPIC -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
ECHO:
	@echo $(SUBDIRS)