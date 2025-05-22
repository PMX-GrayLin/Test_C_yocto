
APP_NAME=test

# header dir
INCLUDES_HEADER += -I$(BB_INCDIR)
INCLUDES_HEADER += -I.
INCLUDES_HEADER += -Iinclude
INCLUDES_HEADER += -Iost
INCLUDES_HEADER += -I$(BB_INCDIR)/json-c
INCLUDES_HEADER += -I$(BB_INCDIR)/gstreamer-1.0
INCLUDES_HEADER += -I$(BB_INCDIR)/glib-2.0
INCLUDES_HEADER += -I$(BB_LIBDIR)/glib-2.0/include
INCLUDES_HEADER += -I$(BB_INCDIR)/opencv4 -I$(BB_INCDIR)/opencv4/opencv

CFLAG += ${CXXFLAGS}
CFLAG += ${INCLUDES_HEADER}

# lib dir
INCLUDES_LIB += -L$(BB_LIBDIR)
INCLUDES_LIB += -L$(BB_LIBDIR)/gstreamer-1.0

# lib
LINK_LIBS += -ljson-c
LINK_LIBS += -lgstreamer-1.0
LINK_LIBS += -lglib-2.0
LINK_LIBS += -lgobject-2.0
LINK_LIBS += -lmosquitto -lmosquittopp
LINK_LIBS += -lgpiod
# LINK_LIBS += -lft4222 -lftd2xx

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

CPPSOURCEFILES = $(wildcard *.cpp) $(wildcard ost/*.cpp) $(wildcard test/*.cpp)
CPPOBJECTS = $(patsubst %.cpp,%.o,$(CPPSOURCEFILES))

# for ost/
ost/%.o: ost/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES_HEADER) -c $< -o $@

all: ${CPPOBJECTS}
	@echo "========== Build all start =========="
	@echo ">>>> CXX:${CXX}"
	@echo ">>>> CFLAG:${CFLAG}"
	@echo ">>>> LDFLAG:${LDFLAG}"
	@echo ">>>> "
	${CXX} $(CFLAG) -o $(APP_NAME) ${CPPOBJECTS} $(LDFLAG)
	@echo "========== Build all end =========="

%.o: %.cpp
	@echo "========== Build $< to $@ start =========="
	$(CXX) $(CFLAG) $< -c
	@echo "========== Build $< to $@ end =========="

.PHONY : clean 

clean:
	rm -rf *.o *.exe $(APP_NAME)
