#!/bin/bash
source /home/ponchio/devel/emsdk/emsdk_env.sh 
emcc -std=c++11 emcorto.cpp \
../../src/cstream.cpp \
../../src/bitstream.cpp \
../../src/tunstall.cpp \
../../src/normal_attribute.cpp \
../../src/color_attribute.cpp \
../../src/decoder.cpp \
-o emcorto.html --post-js post.js \
-O3 \
-s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" \
--memory-init-file 0 \
-s DISABLE_EXCEPTION_CATCHING=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s EXPORT_NAME="'Corto'"


#closure has a bug with ccall function not being hashed correctly. saves only 10KB
#--closure 1
#-s MALLOC="emmalloc" #no diff it seems.
#--memoryprofiler

#exception chactching would be expensive!
#-s NO\_FILESYSTEM=1  #no diff


#serve pages for testing as:
#emrun --no_browser --port 8080 .
