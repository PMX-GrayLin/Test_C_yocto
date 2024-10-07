
#CC = g++
#CFLAGS = -ansi -O -Wall -std=c++11
#LDFLAGS = -lpthread

# CC = ${CXX}

# header dir
INCLUDES_HEADER += -I$(BB_INCDIR)/json-c

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
	@echo "CXX:${CXX}"
	@echo "CXX:${CFLAG}"
	@echo "CXX:${LDFLAG}"
	${CXX} $(CFLAG) -o test test.o $(LDFLAG)
	@echo "========== Build all end =========="

test.o: 
	@echo "========== Build test.o start =========="
	${CXX} $(CFLAG) test.cpp -c
	@echo "========== Build test.o end =========="

.PHONY : clean 

clean:
	rm -rf *.o *.exe test
