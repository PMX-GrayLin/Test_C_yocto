
#CC = g++
#CFLAGS = -ansi -O -Wall -std=c++11
#LDFLAGS = -lpthread

CC = ${CXX}

CFLAGS += ${CXXFLAGS}
CFLAGS += -I$(BB_INCDIR)/json-c

LDFLAG += ${LDFLAGS}
LDFLAG += -L$(BB_LIBDIR)
LDFLAG += -L$(BB_LIBDIR)/json-c
# LDFLAG += -L$(BB_LIBDIR)libjson-c.a
LDFLAG += -ljson-c


all: test.o
	${CC} $(CFLAGS) -o test test.o $(LDFLAGS)
test.o: 
	${CC} $(CFLAGS) test.cpp -c $(LDFLAGS)
lib_crc.o: 
	${CC} $(CFLAGS) lib_crc.c -c
.PHONY : clean 
clean:
	rm -rf *.o *.exe test
