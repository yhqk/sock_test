# Makefile for develop user space application in cross compiling 
# 
# Yuhong Qiu-Kaikkonen 2015

CROSS_PATH=/home/novtech/Projects/buildroot-2014.08/output/host/usr/bin/arm-buildroot-linux-gnueabihf-

CROSS_CC=${CROSS_PATH}gcc -Wall -Wextra
CROSS_LD=${CROSS_PATH}ld

all: socket_tcp_server_thread socket_udp_server_thread udp_server_stdio udp_file1_server

socket_tcp_server_thread: socket_tcp_server_thread.o
	${CROSS_CC} socket_tcp_server_thread.c -o socket_tcp_server_thread -lpthread

socket_udp_server_thread: socket_udp_server_thread.o
	${CROSS_CC} socket_udp_server_thread.c -o socket_udp_server_thread -lpthread

udp_server_stdio: udp_server_stdio.o
	${CROSS_CC} udp_server_stdio.c -o udp_server_stdio -lpthread

udp_file1_server: udp_file1_server.o
	${CROSS_CC} udp_file1_server.c -o udp_file1_server

clean: 
	rm -f *.o 
	rm -f socket_tcp_server_thread socket_udp_server_thread udp_server_stdio udp_file1_server

install: 
	-@if [ ! -d ${PWD}/rootfs   ]; then mkdir -p ${PWD}/rootfs; fi
	mv socket_tcp_server_thread socket_udp_server_thread udp_server_stdio udp_file1_server ${PWD}/rootfs

uninstall:
	rm -fR rootfs

