#ifndef MUXER_H
#define MUXER_H
#include <iostream>
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}


class Muxer
{
public:
    Muxer();
    ~Muxer();
    // 输出文件 返回<0值异常
    // 初始化
    int Init(const char *url);
    // 资源释放
    void DeInit();
    // 创建流
    int AddStream(AVCodecContext *codec_ctx);

    // 写流
    int SendHeader();
    int SendPacket(AVPacket *packet);
    int SendTrailer();

    int Open(); // avio open

    int GetAudioStreamIndex();
    int GetVideoStreamIndex();
private:
    AVFormatContext *fmt_ctx_ = NULL;
    std::string url_ = "";

    // 编码器上下文
    AVCodecContext *aud_codec_ctx_= NULL;
    AVStream *aud_stream_ = NULL;
    AVCodecContext *vid_codec_ctx_= NULL;
    AVStream *vid_stream_ = NULL;

    int audio_index_ = -1;
    int video_index_ = -1;
};

#endif // MUXER_H
