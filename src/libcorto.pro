TEMPLATE = lib

TARGET = corto
CONFIG += staticlib

win32:QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic
unix:QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra -pedantic -Wimplicit



INCLUDEPATH += ../include/corto

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
    ../include/corto/decoder.h \
    ../include/corto/encoder.h \
    ../include/corto/point.h \
    ../include/corto/zpoint.h \
    ../include/corto/cstream.h \
    ../include/corto/tunstall.h \
    ../include/corto/bitstream.h \
    ../include/corto/color_attribute.h \
    ../include/corto/normal_attribute.h \
    ../include/corto/index_attribute.h \
    ../include/corto/vertex_attribute.h \
    ../include/corto/corto.h

lib.path = /usr/local/lib
lib.files = libcorto.a

include.path = /usr/local/include/corto
include.files = $$HEADERS

INSTALLS += include lib
