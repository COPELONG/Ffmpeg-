#ifndef DECODETHREAD_H
#define DECODETHREAD_H
#include "thread.h"
#include "avpacketqueue.h"
#include "avframequeue.h"

class DecodeThread : public Thread
{
public:
    DecodeThread(AVPacketQueue *packet_queue, AVFrameQueue *frame_queue);
    ~DecodeThread();
    int Init(AVCodecParameters *par);
    int Start();
    int Stop();
    void Run();
private:
    char err2str[256] = {0};
    AVCodecContext *codec_ctx_ = NULL;
    AVPacketQueue *packet_queue_ = NULL;
    AVFrameQueue *frame_queue_ = NULL;
};

#endif // DECODETHREAD_H
