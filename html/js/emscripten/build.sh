#!/bin/bash

#adjust this path before running. requires emsdk installed.
source "../../../../../Tools/emsdk/emsdk_env.sh"

# Split LZ4 and corto, link at the end
# Add lz4.c (similar to emcorto.cpp)
# Compress and compressBound probably shouldn't be part of the library

emcc -std=c++11 emcorto.cpp emlz4.cpp \
../../../src/cstream.cpp \
../../../src/bitstream.cpp \
../../../src/tunstall.cpp \
../../../src/normal_attribute.cpp \
../../../src/color_attribute.cpp \
../../../src/decoder.cpp \
-I ../../../include/corto/ \
-I ../../../deps/lz4/lib/ \
-o decoder_base.wasm \
-s EXPORTED_FUNCTIONS='["_ngroups", "_groups", "_getPropName", "_getPropValue", "_groupEnd", "_nprops", "_nvert", "_nface", "_decode", "_malloc", "_memcpy", "_free", "_sbrk","__initialize"]' \
-s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=24576 -s TOTAL_MEMORY=1048576 \
-O3 \
-DENABLE_LZ4=1 \
-DNDEBUG \
--no-entry -msimd128

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