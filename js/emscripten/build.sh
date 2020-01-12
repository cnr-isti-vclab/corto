#!/bin/bash
source /home/ponchio/devel/emsdk/emsdk_env.sh 

emcc -std=c++11 emcorto.cpp \
../../src/cstream.cpp \
../../src/bitstream.cpp \
../../src/tunstall.cpp \
../../src/normal_attribute.cpp \
../../src/color_attribute.cpp \
../../src/decoder.cpp \
-O3 -DNDEBUG \
--debug \
-s EXPORTED_FUNCTIONS='["_ngroups", "_groups", "_nvert", "_nface", "_decode", "_malloc", "_free", "_sbrk","__start"]' \
-s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=24576 -s TOTAL_MEMORY=65536 -o decoder_base.wasm

#emcc -std=c++11 emcorto.cpp \
#../../src/cstream.cpp \
#../../src/bitstream.cpp \
#../../src/tunstall.cpp \
#../../src/normal_attribute.cpp \
#../../src/color_attribute.cpp \
#../../src/decoder.cpp \
#-O3 -DNDEBUG \
#--debug \
#-s EXPORTED_FUNCTIONS='["_ngroups", "_groups", "_nvert", "_nface", "_decode", "_malloc", "_free", "_sbrk","__start"]' \
#-s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=24576 -s TOTAL_MEMORY=65536 -o decoder_simd.wasm \
#-munimplemented-simd128 -mbulk-memory

sed -i "s#\(var wasm_base = \)\".*\";#\\1\"$(cat decoder_base.wasm | hexdump -v -e '1/1 "%02X"')\";#" corto.em.js
#sed -i "s#\(var wasm_simd = \)\".*\";#\\1\"$(cat decoder_simd.wasm | hexdump -v -e '1/1 "%02X"')\";#" corto.em.js

#closure has a bug with ccall function not being hashed correctly. saves only 10KB
#--closure 1
#-s MALLOC="emmalloc" #no diff it seems.
#--memoryprofiler

#exception chactching would be expensive!
#-s NO\_FILESYSTEM=1  #no diff


#serve pages for testing as:
#emrun --no_browser --port 8080 .
