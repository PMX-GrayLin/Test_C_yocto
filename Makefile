
#CC = g++
#CFLAGS = -ansi -O -Wall -std=c++11
#LDFLAGS = -lpthread

# CC = ${CXX}

# header dir
INCLUDES_HEADER += -I$(BB_INCDIR)/json-c
INCLUDES_HEADER += -I$(BB_INCDIR)/gstreamer-1.0
INCLUDES_HEADER += -I$(BB_INCDIR)/glib-2.0
INCLUDES_HEADER += -I$(BB_LIBDIR)/glib-2.0/include
INCLUDES_HEADER += -I$(BB_INCDIR)/opencv4 -I$(BB_INCDIR)/opencv4/opencv

CFLAG += ${CXXFLAGS}
CFLAG += ${INCLUDES_HEADER}

# lib dir
INCLUDES_LIB += -L$(BB_LIBDIR)
# INCLUDES += -L$(BB_LIBDIR)/json-c
INCLUDES_LIB += -L$(BB_LIBDIR)/gstreamer-1.0

# lib
LINK_LIBS += -ljson-c
LINK_LIBS += -lgstreamer-1.0
LINK_LIBS += -lglib-2.0
LINK_LIBS += -lmosquitto -lmosquittopp

OCVLDFLAG +=-lopencv_core 
OCVLDFLAG +=-lopencv_imgproc
OCVLDFLAG +=-lopencv_features2d
OCVLDFLAG +=-lopencv_video 
OCVLDFLAG +=-lopencv_videoio 
OCVLDFLAG +=-lopencv_imgcodecs 
OCVLDFLAG +=-lopencv_objdetect 
OCVLDFLAG +=-lopencv_highgui 
OCVLDFLAG +=-lopencv_photo 
OCVLDFLAG +=-lopencv_flann
OCVLDFLAG +=-lopencv_ml
OCVLDFLAG +=-lopencv_dnn

LDFLAG += ${LDFLAGS}
LDFLAG += ${INCLUDES_LIB}
LDFLAG += ${LINK_LIBS}
LDFLAG += ${OCVLDFLAG}

all: test_gst.o test_ocv.o aicamerag2.0 test.o
	@echo "========== Build all start =========="
	@echo ">>>> CXX:${CXX}"
	@echo ">>>> CFLAG:${CFLAG}"
	@echo ">>>> LDFLAG:${LDFLAG}"
	@echo ">>>> "
	${CXX} $(CFLAG) -o test test_gst.o test_ocv.o test.o $(LDFLAG)
	@echo "========== Build all end =========="

test.o: 
	@echo "========== Build test.o start =========="
	${CXX} $(CFLAG) test.cpp -c
	@echo "========== Build test.o end =========="

aicamerag2.o: 
	@echo "========== Build aicamerag2.o start =========="
	${CXX} $(CFLAG) aicamerag2.cpp -c
	@echo "========== Build aicamerag2.o end =========="

test_gst.o: 
	@echo "========== Build test_gst.o start =========="
	${CXX} $(CFLAG) test_gst.cpp -c
	@echo "========== Build test_gst.o end =========="

test_ocv.o: 
	@echo "========== Build test_ocv.o start =========="
	${CXX} $(CFLAG) test_ocv.cpp -c
	@echo "========== Build test_ocv.o end =========="

test_gst.o: 
	@echo "========== Build test_gst.o start =========="
	${CXX} $(CFLAG) test_gst.cpp -c
	@echo "========== Build test_gst.o end =========="
.PHONY : clean 

clean:
	rm -rf *.o *.exe test
