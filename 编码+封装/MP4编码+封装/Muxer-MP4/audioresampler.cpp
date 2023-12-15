#include "audioresampler.h"

AudioResampler::AudioResampler()
{

}

AudioResampler::~AudioResampler()
{
    if(ctx_) {
        DeInit();
    }
}

int AudioResampler::InitFromS16ToFLTP(int in_channels, int in_sample_rate, int out_channels, int out_sample_rate)
{
    in_channels_ = in_channels;
    in_sample_rate_ = in_sample_rate;
    out_channels_ = out_channels;
    out_sample_rate_ = out_sample_rate;

    ctx_ = swr_alloc_set_opts(ctx_,
                              av_get_default_channel_layout(out_channels_),
                              AV_SAMPLE_FMT_FLTP,
                              out_sample_rate_,
                              av_get_default_channel_layout(in_channels_),
                              AV_SAMPLE_FMT_S16,
                              in_sample_rate_,
                              0,
                              NULL);
    if(!ctx_) {
        printf("swr_alloc_set_opts failed\n");
        return -1;
    }
    int ret = swr_init(ctx_);
    if(ret < 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        printf("swr_init  failed:%s\n",  errbuf);
        return -1;
    }
    return 0;
}

int AudioResampler::ResampleFromS16ToFLTP(uint8_t *in_data, AVFrame *out_frame)
{
    const uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
    indata[0] = in_data;
    int samples = swr_convert(ctx_, out_frame->data, out_frame->nb_samples,
                              indata, out_frame->nb_samples);
    if(samples <= 0) {
        return -1;
    }
    return samples;
}

void AudioResampler::DeInit()
{
    if(ctx_) {
        swr_free(&ctx_);
    }
}

AVFrame *AllocFltpPcmFrame(int channels, int nb_samples)
{
    AVFrame *pcm = NULL;
    pcm = av_frame_alloc();
    pcm->format = AV_SAMPLE_FMT_FLTP;
    pcm->channels = channels;
    pcm->channel_layout = av_get_default_channel_layout(channels);
    pcm->nb_samples = nb_samples;

    int ret = av_frame_get_buffer(pcm, 0);
    if(ret != 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        printf("av_frame_get_buffer failed:%s\n", errbuf);
        av_frame_free(&pcm);
        return NULL;
    }
    return pcm;
}

void FreePcmFrame(AVFrame *frame)
{
    if(frame)
        av_frame_free(&frame);
}
