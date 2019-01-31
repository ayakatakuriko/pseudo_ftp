CC = gcc

CFLAGS = -g -ggdb
LDFLAGS =

TARGET1 = myftpc
SRCS1 = myftpcli.c my_socket.c my_file.c myftp.c io.c
OBJS1 = myftpcli.o my_socket.o my_file.o myftp.o io.o

TARGET2 = myftps
SRCS2 = myftpsrv.c my_socket.c my_file.c myftp.c io.c
OBJS2 = myftpsrv.o my_socket.o my_file.o myftp.o io.o

RM = rm -f

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(TARGET2): $(OBJS2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c $<

myftpcli.o: param.h utility.h my_socket.h myftp.h my_file.h	io.h
myftpsrv.o: param.h utility.h my_socket.h myftp.h my_file.h
my_socket.o: my_socket.h
my_file.o: my_file.h
myftp.o: param.h utility.h my_socket.h myftp.h my_file.h
io.o: io.h utility.h

clean:
	$(RM) $(OBJS1) $(OBJS2)

clean_target:
	$(RM) $(TARGET1) $(TARGET2)

clean_all:
	$(RM) $(TARGET1) $(OBJS1) $(TARGET2) $(OBJS2)
