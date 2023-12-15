#include "videoencoder.h"
extern "C"
{
#include "libavutil/imgutils.h"
}
VideoEncoder::VideoEncoder()
{

}

VideoEncoder::~VideoEncoder()
{
    if(codec_ctx_) {
        DeInit();
    }
}

int VideoEncoder::InitH264(int width, int height, int fps, int bit_rate)
{
    width_ = width;
    height_ = height;
    fps_ = fps;
    bit_rate_ = bit_rate;

    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!codec) {
        printf("avcodec_find_encoder AV_CODEC_ID_H264 failed\n");
        return -1;
    }
    codec_ctx_ = avcodec_alloc_context3(codec);
    if(!codec_ctx_) {
        printf("avcodec_alloc_context3 AV_CODEC_ID_H264 failed\n");
        return -1;
    }

    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_ctx_->bit_rate = bit_rate_;
    codec_ctx_->width = width_;
    codec_ctx_->height = height_;
    codec_ctx_->framerate = {fps_, 1};
    codec_ctx_->time_base = {1, 1000000};   // 单位为微妙

    codec_ctx_->gop_size = fps_;
    codec_ctx_->max_b_frames = 0;
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;

    int ret = avcodec_open2(codec_ctx_, NULL, NULL);
    if(ret != 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        printf("avcodec_open2 failed:%s\n", errbuf);
        return -1;
    }

    frame_ = av_frame_alloc();
    if(!frame_) {
        printf("av_frame_alloc failed\n");
        return -1;
    }
    frame_->width = width_;
    frame_->height = height_;
    frame_->format = codec_ctx_->pix_fmt;

    printf("Inith264 success\n");
    return 0;
}

void VideoEncoder::DeInit()
{
    if(codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    if(frame_) {
        av_frame_free(&frame_);
    }
}

AVPacket *VideoEncoder::Encode(uint8_t *yuv_data, int yuv_size, int stream_index, int64_t pts, int64_t time_base)
{
    if(!codec_ctx_) {
        printf("codec_ctx_ null\n");
        return NULL;
    }
    int ret = 0;

    pts = av_rescale_q(pts, AVRational{1, (int)time_base}, codec_ctx_->time_base);
    frame_->pts = pts;
    if(yuv_data) {
        int ret_size = av_image_fill_arrays(frame_->data, frame_->linesize,
                                            yuv_data, (AVPixelFormat)frame_->format,
                                            frame_->width, frame_->height, 1);
        if(ret_size != yuv_size) {
            printf("ret_size:%d != yuv_size:%d -> failed\n", ret_size, yuv_size);
            return NULL;
        }
        ret = avcodec_send_frame(codec_ctx_, frame_);
    } else {
        ret = avcodec_send_frame(codec_ctx_, NULL);
    }

    if(ret != 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        printf("avcodec_send_frame failed:%s\n", errbuf);
        return NULL;
    }
    AVPacket *packet = av_packet_alloc();
    ret = avcodec_receive_packet(codec_ctx_, packet);
    if(ret != 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        printf("h264 avcodec_receive_packet failed:%s\n", errbuf);
        av_packet_free(&packet);
        return NULL;
    }
    packet->stream_index = stream_index;
    return packet;
}

AVCodecContext *VideoEncoder::GetCodecContext()
{
    return codec_ctx_;
}
