#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H
#include "queue.h"

#ifdef __cplusplus  ///
extern "C"
{
// 包含ffmpeg头文件
//#include "libavutil/avutil.h"
//#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#endif


class AVPacketQueue
{
public:
    AVPacketQueue();
    ~AVPacketQueue();
    void Abort();

    int Size();
    int Push(AVPacket *val);
    AVPacket *Pop(const int timeout);
private:
    void release();
    Queue<AVPacket *> queue_;
};

#endif // AVPACKETQUEUE_H
