#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
sdk=${1:?SDK include directory required}
out=${2:-build/tests}
cc=${HOST_CC:-cc};cxx=${HOST_CXX:-c++}
mkdir -p "$out"
src=src
flags="-O2 -Wall -Wextra -Werror -fsanitize=address,undefined"
for name in engine processing pipeline identity bridge; do
 extra=
 case "$name" in pipeline) extra="$src/rec-pipeline.c";;identity) extra="$src/rec-identity.c";;bridge) extra="$src/rec-identity.c $src/rec-pipeline.c $src/rec-bridge.c";;esac
 "$cc" -std=c11 $flags -I"$src" "$src/rec-engine.c" $extra "tests/test-$name.c" -lm -o "$out/$name"
 "$out/$name"
done
for name in rec-engine rec-monitor rec-telemetry rec-display rec-observer rec-storage rec-wav rec-lastwav rec-file-meter; do
 "$cc" -std=c11 $flags -I"$sdk" -c "$src/$name.c" -o "$out/$name.o"
done
for spec in 'storage:rec-storage' 'native-wav:rec-wav' 'observer:rec-observer rec-storage rec-wav' 'telemetry:rec-telemetry' 'display:rec-display' 'monitor:rec-monitor rec-telemetry rec-display' 'lastwav:rec-lastwav rec-storage rec-wav rec-file-meter' 'file-meter:rec-file-meter rec-wav' 'meter-graph:'; do
 name=${spec%%:*};objects=
 for object in ${spec#*:};do objects="$objects $out/$object.o";done
 "$cc" -std=c11 $flags -pthread -I"$sdk" -I"$src" "tests/test-$name.c" $objects -lm -o "$out/$name"
 "$out/$name"
done
for name in call-scope dynamics-sessions native-lifecycle;do
 "$cxx" -std=c++17 $flags -pthread -I"$sdk" -I"$src" "tests/test-$name.cpp" "$out/rec-engine.o" -o "$out/$name"
 "$out/$name"
done
for name in native-probe native-headroom native-dynamics;do
 features=-DSR_NATIVE_HEADROOM
 if [ "$name" = native-dynamics ];then features="$features -DSR_NATIVE_LIFECYCLE -DSR_NATIVE_DYNAMICS";fi
 "$cxx" -std=c++17 $flags -pthread $features -I"$sdk" -I"$src" "$src/rec-native-probe.cpp" "tests/test-$name.cpp" "$out/rec-engine.o" -o "$out/$name"
 "$out/$name"
done
features="-DSR_NATIVE_PROBE -DSR_NATIVE_HEADROOM -DSR_NATIVE_LIFECYCLE -DSR_NATIVE_DYNAMICS"
"$cxx" -std=c++17 $flags $features -I"$sdk" -c "$src/rec-native-probe.cpp" -o "$out/probe.o"
"$cc" -std=c11 $flags $features -I"$sdk" -I"$src" -c tests/test-native-panel.c -o "$out/panel.o"
"$cxx" $flags -pthread "$out/panel.o" "$out/probe.o" "$out/rec-engine.o" "$out/rec-monitor.o" "$out/rec-telemetry.o" "$out/rec-display.o" "$out/rec-observer.o" "$out/rec-storage.o" "$out/rec-wav.o" "$out/rec-lastwav.o" "$out/rec-file-meter.o" -lm -o "$out/native-panel"
"$out/native-panel"
