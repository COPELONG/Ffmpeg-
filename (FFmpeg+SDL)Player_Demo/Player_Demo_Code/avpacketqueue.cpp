

#include "avpacketqueue.h"
#include "log.h"
AVPacketQueue::AVPacketQueue()
{

}

AVPacketQueue::~AVPacketQueue()
{

}

void AVPacketQueue::Abort()
{
    release();
    queue_.Abort();
}


int AVPacketQueue::Size()
{
   return  queue_.Size();
}

int AVPacketQueue::Push(AVPacket *val)
{
    AVPacket *tmp_pkt = av_packet_alloc();
    av_packet_move_ref(tmp_pkt, val);
    return queue_.Push(tmp_pkt);
}

AVPacket *AVPacketQueue::Pop(const int timeout)
{
    AVPacket *tmp_pkt = NULL;
    int ret = queue_.Pop(tmp_pkt, timeout);
    if(ret < 0) {
        if(ret == -1)
            LogError("AVPacketQueue::Pop failed");
    }
    return tmp_pkt;
}

void AVPacketQueue::release()
{
    while (true) {
        AVPacket *packet = NULL;
        int ret = queue_.Pop(packet, 1);
        if(ret < 0) {
            break;
        } else {
            av_packet_free(&packet);
            continue;
        }
    }
}
