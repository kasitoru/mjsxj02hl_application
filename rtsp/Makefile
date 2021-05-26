############################################################################
#
# Makefile for librtsp
#
# (C) Copyright 2008 to 2020 Hangzhou Xiongmai Technology Stock CO., LTD.
#
# Fri Jun 24 20:09:01 CST 2016		xcz		File created
#
############################################################################

CROSS = arm-himix100-linux-

CFLAGS = -std=gnu99
CFLAGS += -Ilibrtsp/include

SRCDIR = librtsp/src
OUTDIR = ../bin/objects/librtsp
TARGET = $(OUTDIR)/librtsp.a

SOURCE = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%.o,$(SOURCE))

all: mkdirs $(TARGET)

$(TARGET): $(OBJS)
	$(CROSS)ar -rv $@ $^

$(OBJS):
	$(CROSS)gcc $(CFLAGS) -c $(patsubst $(OUTDIR)/%.o,$(SRCDIR)/%.c,$@) -o $@

mkdirs: clean
	mkdir -p $(OUTDIR)

clean:
	rm -f $(TARGET) $(OBJS)

