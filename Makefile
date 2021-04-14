#   $@   :   匹配目标文件
#   $<   :   依赖中的第一个文件
#   %.o 类似 *.o  : 以.o结尾的所有文件
CROSS_COMPILE =/usr/local/arm/arm-gcc-v5.3.1/bin/arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

TARGET = edge-scan  #目标变量

CFLAGS = -g -Wall

_ALLDIRS := $(shell ls -R | grep '^\./.*:$$' | awk '{gsub(":","");print}')
#$(info "ALLDIRS:$(_ALLDIRS)")
ALLDIRS := $(_ALLDIRS) .
INCLUDE := $(patsubst %,-I%,$(ALLDIRS))
#$(info "INCLUDE:$(INCLUDE)")
#	获取SRC_PATH目录及子目录获取其中的.c文件
SRC = $(foreach dir,$(ALLDIRS),$(wildcard $(dir)/*.c)) $(foreach dir,$(ALLDIRS),$(wildcard $(dir)/*.cpp))
$(info "SRC:$(SRC)")
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))
$(info "OBJS:$(OBJS)")
OBJS_WITHOUT_PATH = $(notdir $(OBJS))
#$(info "OBJS_WITHOUT_PATH:$(OBJS_WITHOUT_PATH)")

#OBJS_WITH_PATH = $(addprefix $(OBJS_PATH),$(OBJS_WITHOUT_PATH))
#$(info "OBJS_WITH_PATH:$(OBJS_WITH_PATH)")

LIBS_PATH = -L./lib
LIBS = -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lpthread -lrt -ldl -lopencv_imgcodecs -lopencv_imgproc -lopencv_core
#libs for ffmpeg
#-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale
#libs for opencv
#-lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core

#$(INCLUDE)
#all : $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS_WITHOUT_PATH) $(LIBS_PATH) $(LIBS)
$(OBJS): $(SRC)
	$(CXX) $(CFLAGS) -I. -I./include -I./lcd -I./ffmpeg -I./opencv -I./lib -c $^

Info:
$(info "INCLUDE:$(INCLUDE)")
$(info "SRC:$(SRC)")
$(info "OBJS:$(OBJS)")

.PHONY: clean
clean:
	rm *.o $(TARGET)

