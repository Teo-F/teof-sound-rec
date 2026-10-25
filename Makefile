SDK_INCLUDE ?= /sdk/include
OUT ?= /build
CC = gcc
CXX = g++
WARN = -Wall -Wextra -Werror
BASE = -O2 -fno-omit-frame-pointer -fexceptions -fasynchronous-unwind-tables
SAN ?=

.PHONY: all test
all:
	mkdir -p $(OUT)
	$(CC) -std=c11 $(WARN) $(BASE) -fPIC -fvisibility=hidden -I$(SDK_INCLUDE) -shared -static-libgcc -Wl,--no-undefined -Wl,--version-script=exports.map -o $(OUT)/teof-sound-rec.so sound-rec.c recinfo-hook.S
	nm -D --defined-only $(OUT)/teof-sound-rec.so

test:
	mkdir -p $(OUT)
	$(CC) -std=c11 $(WARN) $(BASE) $(SAN) -c test-adapter.c -o $(OUT)/test-adapter.o
	$(CC) $(BASE) -c recinfo-hook.S -o $(OUT)/recinfo-hook.o
	$(CC) $(BASE) -c test-fixture.S -o $(OUT)/test-fixture.o
	$(CXX) -std=c++11 $(WARN) $(BASE) $(SAN) test-exceptions.cpp $(OUT)/test-adapter.o $(OUT)/recinfo-hook.o $(OUT)/test-fixture.o -o $(OUT)/test-adapter
	$(OUT)/test-adapter
