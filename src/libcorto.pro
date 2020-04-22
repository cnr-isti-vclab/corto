TEMPLATE = lib

TARGET = corto
CONFIG += staticlib

win32:QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic
unix:QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra -pedantic -Wimplicit





QMAKE_STRIP = echo

SOURCES += main.cpp \
    decoder.cpp \
    encoder.cpp \
    tunstall.cpp \
    bitstream.cpp \
    cstream.cpp \
    color_attribute.cpp \
    normal_attribute.cpp \
    tinyply.cpp \
    meshloader.cpp

HEADERS += \
    decoder.h \
    encoder.h \
    point.h \
    zpoint.h \
    cstream.h \
    tunstall.h \
    bitstream.h \
    cstream.h \
    color_attribute.h \
    normal_attribute.h \
    index_attribute.h \
    vertex_attribute.h \
    timer.h \
    corto.h

lib.path = /usr/local/lib
lib.files = libcorto.a

include.path = /usr/local/include/corto
include.files = $$HEADERS

INSTALLS += include lib
