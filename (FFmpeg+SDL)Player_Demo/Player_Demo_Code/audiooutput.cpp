#include "audiooutput.h"
#include "log.h"

AudioOutput::AudioOutput(AVSync *avsync, AVRational time_base, const AudioParams &audio_params, AVFrameQueue *frame_queue)
    :avsync_(avsync), time_base_(time_base), src_tgt_(audio_params), frame_queue_(frame_queue)
{

}

AudioOutput::~AudioOutput()
{

}
FILE *dump_pcm = NULL;
void fill_audio_pcm(void *udata, Uint8 *stream, int len) {

    // 1. 从frame queue读取解码后的PCM的数据，填充到stream
    // 2. len = 4000字节， 一个frame有6000字节， 一次读取了4000， 这个frame剩了2000字节
    AudioOutput *is = (AudioOutput *)udata;
    int len1 = 0;
    int audio_size = 0;

    if(!dump_pcm) {
           dump_pcm = fopen("dump.pcm", "wb");
        }
//    LogInfo("fill pcm len:%d", len);
    while (len > 0) {
        if(is->audio_buf_index == is->audio_buf_size) {
            is->audio_buf_index = 0;
            AVFrame *frame = is->frame_queue_->Pop(10);
            if(frame) {
                is->pts_ = frame->pts;
                // 读到解码后的数据
                // 怎么判断要不要做重采样
                if( ((frame->format != is->dst_tgt_.fmt)
                        || (frame->sample_rate != is->dst_tgt_.freq)
                        ||  (frame->channel_layout != is->dst_tgt_.channel_layout))
                        && (!is->swr_ctx_)) {
                    is->swr_ctx_ = swr_alloc_set_opts(NULL,
                                                      is->dst_tgt_.channel_layout,
                                                      (enum AVSampleFormat)is->dst_tgt_.fmt,
                                                      is->dst_tgt_.freq,
                                                      frame->channel_layout,
                                                      (enum AVSampleFormat)frame->format,
                                                      frame->sample_rate,
                                                      0, NULL);
                    if (!is->swr_ctx_ || swr_init(is->swr_ctx_) < 0) {
                        LogError(
                               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                               frame->sample_rate,
                               av_get_sample_fmt_name((enum AVSampleFormat)frame->format),
                               frame->channels,
                               is->dst_tgt_.freq,
                               av_get_sample_fmt_name((enum AVSampleFormat)is->dst_tgt_.fmt),
                               is->dst_tgt_.channels);
                        swr_free((SwrContext **)(&is->swr_ctx_));
                        return;
                    }
                }
                if(is->swr_ctx_) { // 重采样
                    const uint8_t **in = (const uint8_t **)frame->extended_data;
                    uint8_t **out = &is->audio_buf1_;
                    int out_samples = frame->nb_samples * is->dst_tgt_.freq/frame->sample_rate + 256;
                    int out_bytes = av_samples_get_buffer_size(NULL, is->dst_tgt_.channels, out_samples, is->dst_tgt_.fmt, 0);
                    if(out_bytes <0) {
                        LogError("av_samples_get_buffer_size failed");
                        return;
                    }
                    av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, out_bytes);

                    int len2 = swr_convert(is->swr_ctx_, out, out_samples, in, frame->nb_samples); // 返回样本数量
                    if(len2 <0) {
                        LogError("swr_convert failed");
                        return;
                    }
                    is->audio_buf_ = is->audio_buf1_;
                    is->audio_buf_size = av_samples_get_buffer_size(NULL, is->dst_tgt_.channels, len2, is->dst_tgt_.fmt, 1);
                } else { // 没有重采样
                    audio_size = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (enum AVSampleFormat)frame->format, 1);
                    av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, audio_size);
                    is->audio_buf_ = is->audio_buf1_;
                    is->audio_buf_size = audio_size;
                    memcpy(is->audio_buf_, frame->data[0], audio_size);
                }
                av_frame_free(&frame);
            }else {
                // 没有读到解码后的数据
                is->audio_buf_ = NULL;
                is->audio_buf_size = 512;
            }
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if(len1 > len)
            len1 = len;
        if(!is->audio_buf_) {
            memset(stream, 0, len1);
        } else {
            // 真正拷贝有效的数据
            memcpy(stream, is->audio_buf_ + is->audio_buf_index, len1);
            //fwrite((uint8_t *)is->audio_buf_ + is->audio_buf_index, 1, len1, dump_pcm);
                        //fflush(dump_pcm);
        }
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }

    // 设置时钟
    if(is->pts_ != AV_NOPTS_VALUE) {
        double pts = is->pts_ * av_q2d(is->time_base_);
        LogInfo("audio pts:%0.3lf\n", pts);
        is->avsync_->SetClock(pts);
    }
}

int AudioOutput::Init()
{
    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        LogError("SDL_Init failed");
        return -1;
    }

    SDL_AudioSpec wanted_spec, spec;
    wanted_spec.channels = 2;// 只支持2channel的输出
    wanted_spec.freq = src_tgt_.freq;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.callback = fill_audio_pcm;
    wanted_spec.userdata = this;
    wanted_spec.samples = 1024; // 采样数量

    int ret = SDL_OpenAudio(&wanted_spec, NULL);  // 更正下这里，如果设置的参数不支持我们就退出
    if(ret != 0) {
        LogError("SDL_OpenAudio failed");
        return -1;
    }

    dst_tgt_.channels = wanted_spec.channels; //spec.channels;
    dst_tgt_.fmt = AV_SAMPLE_FMT_S16;
    dst_tgt_.freq = wanted_spec.freq ;// spec.freq;
    dst_tgt_.channel_layout = av_get_default_channel_layout(2);
    dst_tgt_.frame_size = 1024;//src_tgt_.frame_size;
    SDL_PauseAudio(0);
    LogInfo("AudioOutput::Init() leave");
}

int AudioOutput::DeInit()
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    LogInfo("AudioOutput::DeInit() leave");
}
