CC=g++
CFLAGS=-O0
LIB=/usr/local/lib/libpapi.a -lm -lpthread
INCLUDE=-I/usr/local/include
all:
	${CC} ${CFLAGS} -o flush_cache flush_cache.cpp -pthread -lpthread
	${CC} -g ${CFLAGS} ${INCLUDE} -std=c++11 -o prefetchtest main.cpp ${LIB}
clean: 
	${RM} *.o flush_cache prefetchtest