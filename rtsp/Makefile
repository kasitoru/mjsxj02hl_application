DEBUG = -D_DEBUG

TARGET = ../lib/librtspserver.so

OBJS_PATH = ../bin/objects/librtspserver

CROSS_COMPILE = arm-himix100-linux-
CXX   = $(CROSS_COMPILE)g++

INC  = -I$(shell pwd)/RtspServer/src/ -I$(shell pwd)/RtspServer/src/net -I$(shell pwd)/RtspServer/src/xop -I$(shell pwd)/RtspServer/src/3rdpart

LD_FLAGS  = -lrt -pthread -lpthread -ldl -lm $(DEBUG)
CXX_FLAGS = -std=c++11 -fPIC -march=armv7-a -mfpu=neon-vfpv4 -funsafe-math-optimizations

SRC1  = $(notdir $(wildcard ./RtspServer/src/net/*.cpp))
OBJS1 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC1))

SRC2  = $(notdir $(wildcard ./RtspServer/src/xop/*.cpp))
OBJS2 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC2))

SRC3  = $(notdir $(wildcard ./libRtspServer.cpp))
OBJS3 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC3))

all: BUILD_DIR $(TARGET)

BUILD_DIR:
	@-mkdir -p $(OBJS_PATH)

$(TARGET) : $(OBJS1) $(OBJS2) $(OBJS3)
	$(CXX) $^ -o $@ -shared $(LD_FLAGS)
 
$(OBJS_PATH)/%.o : ./%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./RtspServer/src/net/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./RtspServer/src/xop/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)

clean:
	-rm -rf $(OBJS_PATH)
