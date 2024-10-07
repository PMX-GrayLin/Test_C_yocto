
#CC = g++
#CFLAGS = -ansi -O -Wall -std=c++11
#LDFLAGS = -lpthread

# CC = ${CXX}

# header dir
INCLUDES += -I$(BB_INCDIR)/json-c

# lib dir
INCLUDES += -L$(BB_LIBDIR)

# lib
LIBS += -ljson-c


CFLAGS += ${CXXFLAGS}
CFLAGS += ${LDFLAGS}
CFLAGS += ${INCLUDES}
CFLAGS += ${LIBS}

# CFLAGS += -I$(BB_INCDIR)/json-c
# CFLAGS += -ljson-c

# LDFLAG += ${LDFLAGS}
# LDFLAG += -L$(BB_LIBDIR)
# LDFLAG += -L$(BB_LIBDIR)/json-c
# LDFLAG += -L$(BB_LIBDIR)libjson-c.a
# LDFLAG += -ljson-c

# ${CXX} $(CFLAGS) -o test test.o $(LDFLAGS)
all: test.o
	${CXX} $(CFLAGS) -o test test.o
test.o: 
	${CXX} $(CFLAGS) test.cpp -c

.PHONY : clean 
clean:
	rm -rf *.o *.exe test
