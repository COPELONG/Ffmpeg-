TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    log.cpp \
    demuxthread.cpp \
    avpacketqueue.cpp \
    avframequeue.cpp \
    decodethread.cpp \
    audiooutput.cpp \
    videooutput.cpp


win32 {
INCLUDEPATH += $$PWD/ffmpeg-4.2.1-win32-dev/include \
            $$PWD/SDL2-2.0.10/include
LIBS += $$PWD/ffmpeg-4.2.1-win32-dev/lib/avformat.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avcodec.lib    \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avdevice.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avfilter.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avutil.lib     \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/postproc.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swresample.lib \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swscale.lib

LIBS += $$PWD/SDL2-2.0.10/lib/x86/SDL2.lib
}

HEADERS += \
    avsync.h \
    log.h \
    thread.h \
    demuxthread.h \
    queue.h \
    avpacketqueue.h \
    avframequeue.h \
    decodethread.h \
    audiooutput.h \
    videooutput.h
