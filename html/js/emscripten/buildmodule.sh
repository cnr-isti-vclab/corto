#!/bin/bash

#adjust this path before running. requires emsdk installed.
source "../../../../../Tools/emsdk/emsdk_env.sh"

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
-o emcorto.mjs --post-js post.js \
-O3 \
-I ../../../include/corto/ ../../../deps/lz4/ \
-s "EXPORTED_RUNTIME_METHODS=['ccall']" \
--memory-init-file 0 \
-s DISABLE_EXCEPTION_CATCHING=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s EXPORT_NAME="'Corto'" \
-s EXPORT_ES6=1 \
-s MODULARIZE=1


#closure has a bug with ccall function not being hashed correctly. saves only 10KB
#--closure 1
#-s MALLOC="emmalloc" #no diff it seems.
#--memoryprofiler

#exception chactching would be expensive!
#-s NO\_FILESYSTEM=1  #no diff


#serve pages for testing as:
#emrun --no_browser --port 8080 .

exec $SHELL