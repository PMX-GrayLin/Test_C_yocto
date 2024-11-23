
#CC = g++
#CFLAGS = -ansi -O -Wall -std=c++11
#LDFLAGS = -lpthread

# CC = ${CXX}

# header dir
INCLUDES_HEADER += -I$(BB_INCDIR)/json-c
INCLUDES_HEADER += -I$(BB_INCDIR)/gstreamer-1.0
INCLUDES_HEADER += -I$(BB_INCDIR)/glib-2.0
INCLUDES_HEADER += -I$(BB_LIBDIR)/glib-2.0/include

CFLAG += ${CXXFLAGS}
CFLAG += ${INCLUDES_HEADER}

# lib dir
INCLUDES_LIB += -L$(BB_LIBDIR)
# INCLUDES += -L$(BB_LIBDIR)/json-c

# lib
LINK_LIBS += -ljson-c

LDFLAG += ${LDFLAGS}
LDFLAG += ${INCLUDES_LIB}
LDFLAG += ${LINK_LIBS}

all: test.o
	@echo "========== Build all start =========="
	@echo ">>>> CXX:${CXX}"
	@echo ">>>> CFLAG:${CFLAG}"
	@echo ">>>> LDFLAG:${LDFLAG}"
	@echo ">>>> "
	${CXX} $(CFLAG) -o test test.o $(LDFLAG)
	@echo "========== Build all end =========="

test.o: 
	@echo "========== Build test.o start =========="
	${CXX} $(CFLAG) test.cpp -c
	@echo "========== Build test.o end =========="

.PHONY : clean 

clean:
	rm -rf *.o *.exe test
