QT     += core
QT     -= gui

TARGET   = corto
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += warn_on
TEMPLATE = app

win32:QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic
unix:QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra -pedantic -Wimplicit

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
    tinyply.h \
    meshloader.h \
    objload.h \
    corto.h

DISTFILES += \
    plan.md

#uncomment this for tests with other entropy coders
#DEFINES += ENTROPY_TESTS
#LIBS += -lz $$PWD/lz4/liblz4.a
