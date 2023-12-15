#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#ifdef __cplusplus  ///
extern "C"
{
// 包含ffmpeg头文件
//#include "libavutil/avutil.h"
#include "SDL.h"
#include "libavutil/time.h"
}
#endif


#include "avframequeue.h"
#include "avsync.h"


class VideoOutput
{
public:
    VideoOutput(AVSync *avsync, AVRational time_base, AVFrameQueue *frame_queue, int video_width, int video_height);
    int Init();
    int MainLoop();
    void RefreshLoopWaitEvent(SDL_Event *event);
private:
    void videoRefresh(double *remaining_time);
    AVFrameQueue *frame_queue_ = NULL;
    SDL_Event event_; // 事件
    SDL_Rect rect_;
    SDL_Window *win_ = NULL;
    SDL_Renderer *renderer_ = NULL;
    SDL_Texture *texture_ = NULL;
    AVSync *avsync_ = NULL;
    AVRational time_base_;
    int video_width_ = 0;
    int video_height_ = 0;
    uint8_t *yuv_buf_ = NULL;
    int      yuv_buf_size_ = 0;
//    SDL_mutex mutex_;
};

#endif // VIDEOOUTPUT_H
