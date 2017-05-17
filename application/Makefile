CC=gcc

HOSTCFLAGS = -g -Wall \
	-I./inc \
	-I./inc/audio_module \
	-I./inc/device_manage \
	-I./inc/utils \
	-I./inc/uart \
	-I./inc/info_manage \
	-I./inc/recv_msg_procs \
	-I./inc/send_msg_procs \
	-I./inc/run_status_info \
	-I./inc/app_interface \
	-I./inc/sys_status_detect 

HOSTLIBS = 	-lasound -lpthread -lm

ARMCFLAGS = -g -Wall -I/home/wang/work/hushan/alsa-install-gcc-1.0.24/alsa-lib/include \
	-I./inc \
	-I./inc/audio_module \
	-I./inc/device_manage \
	-I./inc/utils \
	-I./inc/uart \
	-I./inc/info_manage \
	-I./inc/recv_msg_procs \
	-I./inc/send_msg_procs \
	-I./inc/run_status_info \
	-I./inc/app_interface \
	-I./inc/sys_status_detect \

ARMLIBS = -L/home/wang/work/hushan/alsa-install-gcc-1.0.24/alsa-lib/lib \
	-lasound -lpthread -lm

CSOFLAGS= -shared -lc -fPIC


TEST_SOURCE = src/wifi_sys_init.c \
			src/app_interface/tcp_ctrl_api.c \
			src/device_manage/udp_ctrl_server.c \
			src/device_manage/tcp_ctrl_server.c \
			src/recv_msg_procs/tcp_ctrl_data_process.c \
			src/recv_msg_procs/tcp_ctrl_data_process_unit.c \
			src/recv_msg_procs/tcp_ctrl_data_process_pc.c \
			src/send_msg_procs/tcp_ctrl_data_compose.c \
			src/run_status_info/tcp_ctrl_device_status.c \
			src/utils/sys_list.c \
			src/utils/sys_queue.c \
			src/audio_module/audio_server.c \
			src/audio_module/audio_params_init.c \
			src/audio_module/audio_ring_buf.c \
			src/uart/sys_uart_init.c \
			src/uart/sys_snd_uart.c \
			src/uart/sys_video_uart.c \
			src/info_manage/client_mac_manage.c \
			src/info_manage/client_connect_manage.c \
			src/info_manage/client_mic_status_manage.c \
			src/info_manage/client_heart_manage.c \
			src/sys_status_detect/sys_status_detect.c
			

.PHONY: libso bin arm-bin clean

libso:
	${CC} -o libhost.so $(TEST_SOURCE) $(ARMLIBS) $(ARMCFLAGS) $(CSOFLAGS)
	
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

