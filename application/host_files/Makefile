
CC=arm-linux-gcc


ARMCFLAGS = -g -Wall -I/home/wang/work/hushan/alsa_install/alsa_lib/include \
		-I$(PWD)
	
ARMLIBS = -L/home/wang/work/hushan/alsa_install/alsa_lib/lib \
		-L$(PWD)/ -lhost \
		-lasound -lpthread -lm 

arm-lib: lib_test.o
	${CC} -o arm-lib lib_test.o $(ARMCFLAGS) $(ARMLIBS)

lib_test.o:lib_test.c
	${CC} -o lib_test.o -c lib_test.c $(ARMCFLAGS) $(ARMLIBS)