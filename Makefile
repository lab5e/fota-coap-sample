LIBS  =  -l coap-2-openssl
CFLAGS = -Wall -g

SRC=$(wildcard *.c)
VERSION=1.0.0

all: $(OBJS)
	gcc -DVERSION=\"$(VERSION)\" -o fota-sample $(SRC) $(CFLAGS) $(LIBS) 

