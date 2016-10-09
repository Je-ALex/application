#
#udp目录中生成本模块的链接文件
#看情况需要可以编译为动态库文件，也可以编译到可执行文件
#
#
CC:=arm-linux-gcc
CFLAGS = -g -Wall
CUR_DIR = $(shell pwd)
OBJ_DIR = $(CUR_DIR)/obj

SRC = $(wildcard *.c)  
OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,${SRC})

udpserver:${OBJ}
	$(CC) $(OBJ) -o udpserver -lpthread
$(OBJ_DIR)/%.o:${CUR_DIR}/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: udpserver

clean:
	rm -f *.o *.so udpserver