LIBS  =  -l coap-2-openssl
CFLAGS = -Wall -g

SRC=$(wildcard *.c)
VERSION=1.0.0

all: image

image: $(OBJS)
	gcc -DVERSION=\"$(VERSION)\" -o fota-sample $(SRC) $(CFLAGS) $(LIBS)  && cp fota-sample fota-sample.$(VERSION)

device: image server
	@mkdir -p run && \
		cp fota-sample run && \
		cp cert.crt run && \
		cp key.pem run && \
		cd run && \
		../server
