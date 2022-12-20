#!/bin/bash

#adjust this path before running. requires emsdk installed.
source "../../../../../Tools/emsdk/emsdk_env.sh"

# Split LZ4 and corto, link at the end

emcc -std=c++11 emcorto.cpp \
../../../src/cstream.cpp \
../../../src/bitstream.cpp \
../../../src/tunstall.cpp \
../../../src/normal_attribute.cpp \
../../../src/color_attribute.cpp \
../../../src/decoder.cpp \
../../../deps/lz4/lib/lz4.c \
../../../deps/lz4/lib/lz4file.c \
../../../deps/lz4/lib/lz4frame.c \
../../../deps/lz4/lib/lz4hc.c \
../../../deps/lz4/lib/xxhash.c \
-I ../../../include/corto/ ../../../deps/lz4/ \
-O3 \
-DNDEBUG \
-s EXPORTED_FUNCTIONS='["_ngroups", "_groups", "_nvert", "_nface", "_decode", "_malloc", "_free", "_sbrk","__initialize"]' \
-s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=24576 -s TOTAL_MEMORY=1048576 -o decoder_base.wasm \
--no-entry

#-g \
#--debug \
#-fsanitize=undefined \
#--source-map-base "http://localhost/devel/nexus/html/js/decoder_base.wasm.map" \
#-s SAFE_HEAP=1 \
#-O3 -DNDEBUG \


# TODO: check simd support
#-munimplemented-simd128 -mbulk-memory
printf "s/var wasm_base = \".*\";/var wasm_base=\"" > sed_command.txt
xxd -ps -c 0 decoder_base.wasm | tr -d '\n' >> sed_command.txt
printf "\";/" >> sed_command.txt
sed -f sed_command.txt corto.em.proto.js > ../corto.em.js

#serve pages for testing as:
#emrun --no_browser --port 8080 .

exec $SHELL