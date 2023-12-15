#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#ifdef __cplusplus  ///
extern "C"
{
// 包含ffmpeg头文件
//#include "libavutil/avutil.h"
#include "SDL.h"
#include "libswresample/swresample.h"
}
#endif

#include "avsync.h"
#include "avframequeue.h"
typedef struct AudioParams
{
    int freq; //采样率
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
}AudioParams;


class AudioOutput
{
public:
    AudioOutput(AVSync *avsync, AVRational time_base, const AudioParams &audio_params, AVFrameQueue *frame_queue);
    ~AudioOutput();
    int Init();
    int DeInit();
public:
    int64_t pts_ = AV_NOPTS_VALUE;
    AudioParams src_tgt_; // 解码后的参数
    AudioParams dst_tgt_; // SDL实际输出的格式
    AVFrameQueue *frame_queue_ = NULL;
    struct SwrContext *swr_ctx_ = NULL;
    uint8_t *audio_buf_ = NULL;
    uint8_t *audio_buf1_ = NULL;
    uint32_t audio_buf_size = 0;
    uint32_t audio_buf1_size = 0;
    uint32_t audio_buf_index = 0;
    AVSync *avsync_ = NULL;
    AVRational time_base_;
};

#endif // AUDIOOUTPUT_H
