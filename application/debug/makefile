OBJS=*.o
ODIR=obj
$(ROOT_DIR)/$(BIN_DIR)/$(BIN):$(ODIR)/$(OBJS)
#	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	$(CC) -shared $^ -lc -o $@ $(LIBS)

#编译为库文件
#libtest.so:libtest.o
#	$(CC) -shared libtest.o -lc -o libtest.so
#libtest.o:pthread.c test.h
#	$(CC) -fPIC -c pthread.c -o libtest.o
#all:libtest.so