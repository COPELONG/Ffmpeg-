#include "demuxthread.h"
#include "log.h"
DemuxThread::DemuxThread(AVPacketQueue *audio_queue, AVPacketQueue *video_queue)
    :audio_queue_(audio_queue), video_queue_(video_queue)
{
    LogInfo("DemuxThread");
}

DemuxThread::~DemuxThread()
{
    LogInfo("~DemuxThread");
    if(thread_) {
        Stop();
    }
}

int DemuxThread::Init(const char *url)
{
    LogInfo("url:%s", url);
    int ret = 0;
    url_ = url;

    ifmt_ctx_ = avformat_alloc_context();

    ret = avformat_open_input(&ifmt_ctx_, url_.c_str(), NULL, NULL);
    if(ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LogError("avformat_open_input failed, ret:%d, err2str:%s", ret, err2str);
        return -1;
    }

    ret = avformat_find_stream_info(ifmt_ctx_, NULL);
    if(ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LogError("avformat_find_stream_info failed, ret:%d, err2str:%s", ret, err2str);
        return -1;
    }
    av_dump_format(ifmt_ctx_, 0, url_.c_str(), 0);

    audio_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    video_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    LogInfo("audio_index_:%d, video_index_:%d", audio_index_, video_index_);
    if(audio_index_ < 0 || video_index_ < 0) {
        LogError("no audio or no video");
        return -1;
    }

    LogInfo("Init leave");
}

int DemuxThread::Start()
{
    thread_ = new std::thread(&DemuxThread::Run, this);
    if(!thread_) {
        LogError("new std::thread(&DemuxThread::Run, this) failed");
        return -1;
    }
    return 0;
}

int DemuxThread::Stop()
{
    Thread::Stop();
    avformat_close_input(&ifmt_ctx_);
}

void DemuxThread::Run()
{
    LogInfo("Run into");
    int ret = 0;
    AVPacket pkt;

    while (abort_ != 1) {

        if(audio_queue_->Size() > 100 || video_queue_->Size() > 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        ret = av_read_frame(ifmt_ctx_, &pkt);
        if(ret < 0) {
            av_strerror(ret, err2str, sizeof(err2str));
            LogError("av_read_frame failed, ret:%d, err2str:%s", ret, err2str);
            break;
        }
        if(pkt.stream_index == audio_index_) {
            ret = audio_queue_->Push(&pkt);
//            av_packet_unref(&pkt);
//            LogInfo("audio pkt queue size:%d", audio_queue_->Size());
        } else if(pkt.stream_index == video_index_) {
            ret = video_queue_->Push(&pkt);
//            av_packet_unref(&pkt);
//            LogInfo("video pkt queue size:%d", video_queue_->Size());
        } else {
            av_packet_unref(&pkt);
        }
    }
    LogInfo("Run finish");
}

AVCodecParameters *DemuxThread::AudioCodecParameters()
{
    if(audio_index_ != -1) {
        return ifmt_ctx_->streams[audio_index_]->codecpar;
    } else {
        return NULL;
    }
}

AVCodecParameters *DemuxThread::VideoCodecParameters()
{
    if(video_index_ != -1) {
        return ifmt_ctx_->streams[video_index_]->codecpar;
    } else {
        return NULL;
    }
}

AVRational DemuxThread::AudioStreamTimebase()
{
    if(audio_index_ != -1) {
        return ifmt_ctx_->streams[audio_index_]->time_base;
    } else {
        return AVRational{0, 0};
    }
}

AVRational DemuxThread::VideoStreamTimebase()
{
    if(video_index_ != -1) {
        return ifmt_ctx_->streams[video_index_]->time_base;
    } else {
        return AVRational{0, 0};
    }
}

