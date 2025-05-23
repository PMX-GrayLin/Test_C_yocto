
APP_NAME=test
# APP_NAME=fw_daemon

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

# add define to enable/disable functions
ENABLE_OST = 0
ENABLE_TestCode = 0

ifeq ($(ENABLE_OST),1)
	DEFINES += -DENABLE_OST
endif

ifeq ($(ENABLE_TestCode),1)
	DEFINES += -DENABLE_TestCode
endif

CFLAG += ${CXXFLAGS}
CFLAG += ${INCLUDES_HEADER}
CFLAG += ${DEFINES}

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

# CPPSOURCEFILES = $(wildcard *.cpp) $(wildcard ost/*.cpp) $(wildcard temp/*.cpp)
CPPSOURCEFILES = $(wildcard *.cpp)
ifeq ($(ENABLE_OST),1)
	CPPSOURCEFILES += $(wildcard ost/*.cpp)
endif
ifeq ($(ENABLE_TestCode),1)
	CPPSOURCEFILES += $(wildcard temp/*.cpp)
endif
CPPOBJECTS = $(patsubst %.cpp,%.o,$(CPPSOURCEFILES))

%.o: %.cpp
	@echo "========== Build $< to $@ start =========="
	$(CXX) $(CFLAG) -c $< -o $@
	@echo "========== Build $< to $@ end =========="

# for ost/
ost/%.o: ost/%.cpp
	@echo "========== Build $< to $@ start =========="
	$(CXX) $(CFLAG) -c $< -o $@
	@echo "========== Build $< to $@ end =========="

all: $(APP_NAME)
$(APP_NAME): ${CPPOBJECTS}
	@echo "========== Build all start =========="
	@echo ">>>> CXX:${CXX}"
	@echo ">>>> CFLAG:${CFLAG}"
	@echo ">>>> LDFLAG:${LDFLAG}"
	@echo ">>>> "
	${CXX} -o $@ ${CPPOBJECTS} $(LDFLAG)
	@echo "========== Build all end =========="

.PHONY : clean 

clean:
	rm -rf *.o *.exe $(APP_NAME)
