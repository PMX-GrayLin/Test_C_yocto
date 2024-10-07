
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


# CFLAGS += -I$(BB_INCDIR)/json-c
# CFLAGS += -ljson-c

# LDFLAG += ${LDFLAGS}
# LDFLAG += -L$(BB_LIBDIR)
# LDFLAG += -L$(BB_LIBDIR)/json-c
# LDFLAG += -L$(BB_LIBDIR)libjson-c.a
# LDFLAG += -ljson-c


# ${CXX} $(CFLAGS) -o test test.o $(LDFLAGS)
all: test.o
	${CXX} $(CFLAG) -o test test.o $(LDFLAG)
test.o: 
	${CXX} $(CFLAG) test.cpp -c

.PHONY : clean 
clean:
	rm -rf *.o *.exe test
