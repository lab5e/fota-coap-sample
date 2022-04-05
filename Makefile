LIBS  =  -l coap-2-openssl
CFLAGS = -Wall

SRC=$(wildcard *.c)

all: $(OBJS)
	gcc -o fota-sample $(SRC) $(CFLAGS) $(LIBS)
