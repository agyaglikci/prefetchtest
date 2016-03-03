CC=g++
CFLAGS=-O0
LIB=/usr/local/lib/libpapi.a -lm -lpthread
INCLUDE=-I/usr/local/include
all:
	${CC} -g ${CFLAGS} ${INCLUDE} -std=c++11 -o prefetchtest main.cpp ${LIB}
clean: 
	${RM} *.o test