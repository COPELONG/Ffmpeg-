TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    FlvParser.cpp \
    vadbg.cpp \
    Videojj.cpp

HEADERS += \
    FlvParser.h \
    vadbg.h \
    Videojj.h
