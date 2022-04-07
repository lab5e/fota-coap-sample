LIBS  =  -l coap-2-openssl
CFLAGS = -Wall -g

SRC=$(wildcard *.c)

all: $(OBJS)
	gcc -o fota-sample $(SRC) $(CFLAGS) $(LIBS)
