SDK_INCLUDE ?= /sdk/include
OUT ?= build
CC = gcc
CXX = g++
NM ?= nm
CROSS ?=
FLAGS = -O2 -Wall -Wextra -Werror -fPIC -fvisibility=hidden -fno-omit-frame-pointer -fexceptions -fasynchronous-unwind-tables
FEATURES = -DSR_NATIVE_PROBE -DSR_NATIVE_HEADROOM -DSR_NATIVE_LIFECYCLE -DSR_NATIVE_DYNAMICS
SOURCES = rec-engine sound-rec-unified rec-monitor rec-observer rec-wav rec-storage rec-telemetry rec-display rec-lastwav rec-file-meter
LINK_SOURCES = sound-rec-unified rec-monitor rec-observer rec-wav rec-storage rec-telemetry rec-display rec-lastwav rec-engine rec-file-meter
OBJECTS = $(addprefix $(OUT)/,$(addsuffix .o,$(LINK_SOURCES)))

.PHONY: all test package clean
all:
	mkdir -p $(OUT)
	@for name in $(SOURCES); do \
	  $(CC) -std=c11 $(FLAGS) $(FEATURES) -I$(SDK_INCLUDE) -c src/$$name.c -o $(OUT)/$$name.o || exit $$?; \
	done
	$(CC) -fPIC -c src/recinfo-hook.S -o $(OUT)/recinfo-hook.o
	$(CXX) -std=c++17 $(FLAGS) $(FEATURES) -I$(SDK_INCLUDE) -c src/rec-native-probe.cpp -o $(OUT)/native-probe.o
	$(CXX) -shared -static-libgcc -pthread -Wl,--no-undefined -Wl,--version-script=exports.map $(OBJECTS) $(OUT)/recinfo-hook.o $(OUT)/native-probe.o -lm -o $(OUT)/teof-sound-rec.so
	$(NM) -D --defined-only $(OUT)/teof-sound-rec.so

test:
	HOST_CC="$(HOST_CC)" HOST_CXX="$(HOST_CXX)" sh tools/test.sh "$(SDK_INCLUDE)" "$(OUT)/tests"

package: all
	python3 tools/package.py "$(OUT)"

clean:
	rm -rf $(OUT)
