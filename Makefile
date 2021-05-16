CC := gcc

CCFLAGS := -L/opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app/lib
CCFLAGS += -pthread -llocalsdk -l_hiae -livp -live -lmpi -lmd -l_hiawb -lisp -lsecurec -lsceneauto -lVoiceEngine -lupvqe -l_hidehaze -l_hidrc -l_hildci -ldnvqe -lsns_f22 -lpaho-mqtt3c -lyyjson

TOOLCHAIN := arm-himix100-linux-
ARCH := -march=armv7-a -mfpu=neon-vfpv4 -funsafe-math-optimizations

OUTPUT := ./bin

all: mkdirs mjsxj02hl

mjsxj02hl: ./mjsxj02hl.c logger.o init.o configs.o inih.o video.o audio.o speaker.o alarm.o night.o mqtt.o rtsp.o
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) ./mjsxj02hl.c $(OUTPUT)/objects/*.o ./rtsp/librtsp.a -o $(OUTPUT)/mjsxj02hl

logger.o: ./logger/logger.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./logger/logger.c -o $(OUTPUT)/objects/logger.o

configs.o: ./configs/configs.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./configs/configs.c -o $(OUTPUT)/objects/configs.o

inih.o: ./configs/inih/ini.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./configs/inih/ini.c -o $(OUTPUT)/objects/inih.o

init.o: ./localsdk/init.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./localsdk/init.c -o $(OUTPUT)/objects/init.o

video.o: ./localsdk/video/video.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./localsdk/video/video.c -o $(OUTPUT)/objects/video.o

audio.o: ./localsdk/audio/audio.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./localsdk/audio/audio.c -o $(OUTPUT)/objects/audio.o

speaker.o: ./localsdk/speaker/speaker.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./localsdk/speaker/speaker.c -o $(OUTPUT)/objects/speaker.o

alarm.o: ./localsdk/alarm/alarm.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./localsdk/alarm/alarm.c -o $(OUTPUT)/objects/alarm.o

night.o: ./localsdk/night/night.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./localsdk/night/night.c -o $(OUTPUT)/objects/night.o

mqtt.o: ./mqtt/mqtt.c
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./mqtt/mqtt.c -o $(OUTPUT)/objects/mqtt.o

librtsp.a:
	make -C ./rtsp

rtsp.o: ./rtsp/rtsp.c librtsp.a
	$(TOOLCHAIN)$(CC) $(CCFLAGS) $(ARCH) -c ./rtsp/rtsp.c -o $(OUTPUT)/objects/rtsp.o

mkdirs: clean
	mkdir -p $(OUTPUT)/objects

clean:
	make clean -C ./rtsp
	rm -rf $(OUTPUT)/*
