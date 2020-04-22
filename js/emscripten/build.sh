#!/bin/bash

#adjust this path before running. requires emsdk installed.
source ~/devel/emsdk/emsdk_env.sh 

emcc -std=c++11 emcorto.cpp \
../../src/cstream.cpp \
../../src/bitstream.cpp \
../../src/tunstall.cpp \
../../src/normal_attribute.cpp \
../../src/color_attribute.cpp \
../../src/decoder.cpp \
-O3 -DNDEBUG \
-s EXPORTED_FUNCTIONS='["_ngroups", "_groups", "_nvert", "_nface", "_decode", "_malloc", "_free", "_sbrk","__start"]' \
-s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=24576 -s TOTAL_MEMORY=1048576 -o decoder_base.wasm 

#-g \
#--debug \
#-fsanitize=undefined \
#--source-map-base "http://localhost/devel/nexus/html/js/decoder_base.wasm.map" \
#-s SAFE_HEAP=1 \
#-O3 -DNDEBUG \


# TODO: check simd support
#-munimplemented-simd128 -mbulk-memory
echo -n "s#\(var wasm_base = \)\".*\";#\\1\"" > sed_command.txt
hexdump -v -e '1/1 "%02X"' decoder_base.wasm >> sed_command.txt
echo "\";#" >> sed_command.txt
sed -f sed_command.txt corto.em.proto.js > ../corto.em.js

#serve pages for testing as:
#emrun --no_browser --port 8080 .
