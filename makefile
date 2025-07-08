BUILD_DIR=./build
CC=gcc
CFLAGS=-I./include -lraylib

ifdef RELEASE
	CFLAGS+=-O2
else
	CFLAGS+=-ggdb
endif

ifdef RUN_TESTS
	CFLAGS+=-DRUN_TESTS
endif

ifdef STRICT
CFLAGS+=-Wall -Wextra -Wpedantic -Werror -fanalyzer -fstack-protector-strong \
		-D_FORTIFY_SOURCE=2
endif

build: skeleton gbemu

gbemu: gbemu.o cjson.o jtest.o util.o instr.o
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $(BUILD_DIR)/objs/gbemu.o \
		$(BUILD_DIR)/objs/cjson.o \
		$(BUILD_DIR)/objs/jtest.o \
		$(BUILD_DIR)/objs/util.o \
		$(BUILD_DIR)/objs/instr.o

instr.o: src/instr.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/objs/$@ $<

gbemu.o: src/main.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/objs/$@ $<

jtest.o: src/jtest.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/objs/$@ $<

cjson.o: vendor/cJSON/cJSON.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/objs/$@ $<

util.o: src/util.h
	$(CC) $(CFLAGS) -x c -DUTIL_IMPLEMENTATION -c -o $(BUILD_DIR)/objs/$@ $<

skeleton:
	mkdir -p ${BUILD_DIR}
	mkdir -p $(BUILD_DIR)/objs
	mkdir -p $(BUILD_DIR)/libs

clean:
	-rm --preserve-root ${BUILD_DIR} -rv
