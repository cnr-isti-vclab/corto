QT += core
QT -= gui

TARGET = nxz
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    nxzdecoder.cpp \
    nxzencoder.cpp \
    tunstall.cpp \
    bitstream.cpp \
    cstream.cpp \
    color_attribute.cpp \
    normal_attribute.cpp \
    ../../../vcglib/wrap/ply/plylib.cpp \
    tinyply.cpp \
    meshloader.cpp
HEADERS += \
    nxzdecoder.h \
    nxzencoder.h \
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
    ../../../vcglib/wrap/ply/plylib.h \
    timer.h \
    tinyply.h \
    meshloader.h \
    objload.h

DISTFILES += \
    plan.md

#uncomment this for tests with other entropy coders.
#DEFINES += ENTROPY_TESTS
#LIBS += -lz $$PWD/lz4/liblz4.a
