# Makefile for develop user space application in cross compiling 

CROSS_PATH=/home/novtech/Projects/buildroot-2014.08/output/host/usr/bin/arm-buildroot-linux-gnueabihf-

CROSS_CC=${CROSS_PATH}gcc -Wall
CROSS_LD=${CROSS_PATH}ld

all: socket_tcp_server_thread

socket_tcp_server_thread: socket_tcp_server_thread.o
	${CROSS_CC} socket_tcp_server_thread.c -o socket_tcp_server_thread -lpthread

clean: 
	rm -f *.o 
	rm -f socket_tcp_server_thread

install: 
	-@if [ ! -d ${PWD}/rootfs   ]; then mkdir -p ${PWD}/rootfs; fi
	cp socket_tcp_server_thread ${PWD}/rootfs

uninstall:
	rm -fR rootfs

