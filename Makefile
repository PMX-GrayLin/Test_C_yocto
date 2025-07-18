
# Version
FW_VERSION := $(shell expr $$(date +%Y) - 2024).$$(date +%m).$$(date +%d)
VERSION_FLAG += -DFW_VERSION=\"$(FW_VERSION)\"
DEFINES += $(VERSION_FLAG)

# APP_NAME=test
APP_NAME=fw_daemon

# add define to enable/disable functions
ENABLE_Gige ?= 0
ENABLE_CIS ?= 0
ENABLE_OST = 0
ENABLE_TestCode = 0

ifeq ($(ENABLE_CIS),1)
DEFINES += -DENABLE_CIS
INCLUDES_HEADERs += -Icam_omnivision
endif
ifeq ($(ENABLE_Gige),1)
DEFINES += -DENABLE_Gige
INCLUDES_HEADERs += -Icam_gige
endif
ifeq ($(ENABLE_OST),1)
DEFINES += -DENABLE_OST
INCLUDES_HEADERs += -Iost
endif
ifeq ($(ENABLE_TestCode),1)
DEFINES += -DENABLE_TestCode
INCLUDES_HEADERs += -Itemp
endif

# header dir
INCLUDES_HEADERs += -I$(BB_INCDIR)
INCLUDES_HEADERs += -I.
INCLUDES_HEADERs += -Iinclude
INCLUDES_HEADERs += -Icam_uvc
INCLUDES_HEADERs += -I$(BB_INCDIR)/json-c
INCLUDES_HEADERs += -I$(BB_INCDIR)/gstreamer-1.0
INCLUDES_HEADERs += -I$(BB_INCDIR)/glib-2.0
INCLUDES_HEADERs += -I$(BB_LIBDIR)/glib-2.0/include
INCLUDES_HEADERs += -I$(BB_INCDIR)/opencv4 -I$(BB_INCDIR)/opencv4/opencv

CFLAG += ${CXXFLAGS}
CFLAG += ${DEFINES}
CFLAG += ${INCLUDES_HEADERs}

# lib dir
INCLUDES_LIB += -L$(BB_LIBDIR)
INCLUDES_LIB += -L$(BB_LIBDIR)/../../lib
INCLUDES_LIB += -L$(BB_LIBDIR)/gstreamer-1.0

# lib
LINK_LIBS += -ljson-c
LINK_LIBS += -lgstreamer-1.0
LINK_LIBS += -lglib-2.0
LINK_LIBS += -lgobject-2.0
LINK_LIBS += -lgstapp-1.0
LINK_LIBS += -lmosquitto -lmosquittopp
LINK_LIBS += -lgpiod
LINK_LIBS += -lcurl
LINK_LIBS += -ludev
# LINK_LIBS += -lft4222 -lftd2xx

OCVLDFLAG +=-lopencv_core 
OCVLDFLAG +=-lopencv_imgproc
OCVLDFLAG +=-lopencv_imgcodecs 
OCVLDFLAG +=-lopencv_videoio 
OCVLDFLAG +=-lopencv_highgui 
# OCVLDFLAG +=-lopencv_features2d
# OCVLDFLAG +=-lopencv_video
# OCVLDFLAG +=-lopencv_objdetect 
# OCVLDFLAG +=-lopencv_photo 
# OCVLDFLAG +=-lopencv_flann
# OCVLDFLAG +=-lopencv_ml
# OCVLDFLAG +=-lopencv_dnn

LDFLAG += ${LDFLAGS}
LDFLAG += ${INCLUDES_LIB}
LDFLAG += ${LINK_LIBS}
LDFLAG += ${OCVLDFLAG}

# add source files
CPPSOURCEFILES = $(wildcard *.cpp)
ifeq ($(ENABLE_CIS),1)
	CPPSOURCEFILES += $(wildcard cam_omnivision/*.cpp)
endif
ifeq ($(ENABLE_Gige),1)
	CPPSOURCEFILES += $(wildcard cam_gige/*.cpp)
endif
ifeq ($(ENABLE_OST),1)
	CPPSOURCEFILES += $(wildcard ost/*.cpp)
endif
ifeq ($(ENABLE_TestCode),1)
	CPPSOURCEFILES += $(wildcard temp/*.cpp)
endif
CPPSOURCEFILES += $(wildcard cam_uvc/*.cpp)

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
