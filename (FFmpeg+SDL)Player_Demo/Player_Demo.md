

# FFMpeg+SDL播放器框架

![image-20231031100609186](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231031100609186.png)

# 线程模块

作为解复用模块、解码模块的父类线程，提供线程基本函数。

```c++
class Thread{
 public:
  int Stop(){
      m_abort = 1 ;
      if(m_thread){
          m_thread->join();
          delete m_thread;
          m_thread = nullptr;
      }
  }
  int Start();
  virtual void Run() = 0;
 private:
    int m_abort = 0;
    std :: thread *m_thread =  nullptr ;
}
```



## 解复用线程模块

初始化解复用器并获取解码器参数等信息，将解复用出来的音频和视频包分别插入队列。

![image-20231101141939213](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101141939213.png)

### 解复用模块

```c++
int DemuxThread::Start(){
    m_thread = new std::thread(&DemuxThread::Run, this);
}
int DemuxThread::Stop()
{
    Thread::Stop();
    avformat_close_input(&ifmt_ctx_);
}
//解复用：将解复用出来的音频包和视频包插入队列中
void DemuxThread::Run(){
    AVPacket pkt = av_packet_alloc();
    while(m_abort != 1){
        
    av_init_packet(&pkt);
    ret = av_read_frame(ifmt_ctx, &pkt);
    if(ret <0){
        m_abort = 1 ;
        break ;
    }
    //插入队列
    if(pkt.stream_index == audio_index_){
        audio_queue_->Push(&pkt);
    }else if(pkt.stream_index == video_index_) {
        video_queue_->Push(&pkt);
    }else{
        av_packet_unref(&pkt);
    }
  }
}
```



### 初始化Init

find 流信息，并且初始化fmt_ctx

```c++
Init(const char *url){
    
    ifmt_ctx = avformat_alloc_context();
    ret = avformat_open_input(&ifmt_ctx,url.c_str(),NULL,NULL);
    
    avformat_find_stream_info(ifmt_ctx_, NULL);
    audio_index_ = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    video_index_ = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
       
}
//关联AVPacketQueue   *audio_queue和*video_queue队列
DemuxThread::DemuxThread(AVPacketQueue *audio_queue, AVPacketQueue *video_queue)
    :audio_queue_(audio_queue), video_queue_(video_queue){}
```

### 参数获取

```c++
AVCodecParameters *DemuxThread::AudioCodecParameters()
{
    if(audio_index_ != -1) {
        return ifmt_ctx_->streams[audio_index_]->codecpar;
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
```



## 解码线程模块

逻辑思路大致和解复用线程模块类似

用来取出 packet 队列中的pkt进行解码操作，并且插入到frame队列中

关联packet_queue和frame_queue，用来取和进。

![image-20231101145756159](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101145756159.png)

### 初始化Init

初始化解码器和解码器上下文

```c++
DecodeThread(AVPacketQueue *packet_queue, AVFrameQueue *frame_queue):packet_queue_(packet_queue), frame_queue_(frame_queue){}
//从pkt 队列中取完整的包，直接就可以使用解码器进行解析，不需要使用解析器（针对从.aac裸流中提取信息）
//需要获取解复用器的参数传给解码器，因为多媒体文件信息共享
Init(AVCodecParameters *par){//*par = ifmt_ctx_->streams[audio_index_]->codecpar;
    m_codec_ctx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(m_codec_ctx, par);
    // av_codec_set_pkt_timebase(m_codec_ctx, ic->streams[stream_index]->time_base);
    AVCodec *codec = avcodec_find_decoder(m_codec_ctx->codec_id);
    avcodec_open2(codec_ctx_, codec, NULL);
}
```



### 解码模块

循环从pkt队列中读取pkt 进行解码

```c++
void DecodeThread::Run(){
     AVFrame *frame = av_frame_alloc();
    
  while(m_abort !=1){
     AVPacket *pkt = packet_queue_->Pop(10);
     ret  =  avcodec_send_packet(codec_ctx_, pkt);
      
      if (ret < 0) {
         m_abort = 1;
         av_strerror(ret, err2str, sizeof(err2str));
         LogError("avcodec_send_packet failed, ret:%d, err2str:%s", ret, err2str);
         break;
      }
      
     av_packet_free(&pkt);
     while(true){
          avcodec_receive_frame(codec_ctx_, frame);
          if(ret == 0) {
              frame_queue_->Push(frame);
          }else if(AVERROR(EAGAIN) == ret){
              break;
          }else{
              //终止线程
              m_abort = 1;
              break;
          }                 
     }
  }
}
```



# 队列模块



## Queue.h

父队列：实现push、pop、front、size、互斥锁操作



```c++
template <typename T>
class Queue{
    public:
    void abort(){
        m_abort = 1;
        m_cond.notify_all();
    }
    int push(T & data){
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_abort==1){
            return -1;
        }
        m_queue.push(data);
        cond.notify_one();
        return 0;
    }
    int pop(T & data , const int timeout = 0){
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_queue.empty()){
            cond.wait_for(lock,std::chrono::milliseconds(timeout),[this]{
                return m_abort||!m_queue.empty();
            });
        }
        if(m_abort==1){
            return -1;
        }
        if(m_queue.empty()){
            return -2;
        }
        data = m_queue.font();
        m_.queue.pop();
        return 0;
    }
    int front(T &data){
        //条件判断
        ......
         return-1;
        
        data =  m_queue.front();
        return 0;
    }
    private:
    m_abort = 0;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::queue<T> m_queue;
}
```

## AVPacketQueue.h

组合Queue.h

![image-20231101170923829](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101170923829.png)

push、pop都调用Queue<AVPacket *>  m_queue;

```c++
int AVPacketQueue::Push(AVPacket *val)
{
    AVPacket *tmp_pkt = av_packet_alloc();
    int ret = av_packet_move_ref(tmp_pkt, val);
    if(ret < 0) {
        if(ret == -1)
            LogError("AVPacketQueue::Push failed");
    }
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

//释放资源，循环把pkt队列中的所有pkt全部pop并且free
void release(){
    while(true){
        AVPacket pkt = nullptr;
        m_queue.pop(pkt,1);
        av_packet_free(&pkt);
    }
}
```

## AVFramequeue.h

![image-20231101194541587](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101194541587.png)



# 输出模块



## audio_output.h

使用SDL进行输出

连接FreamQueue队列，因为从其抽取Frame进行播放。

![image-20231101194721869](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101194721869.png)

![image-20231101194825525](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101194825525.png)

```c++
AudioOutput *audio_output = new AudioOutput(&avsync, demux_thread->AudioStreamTimebase(),audio_params, &audio_frame_queue);

获得一帧数据后PTS进行转化
//frame->pts = av_rescale_q(frame->pts, decode_avctx->pkt_timebase, tb);
    音频
//AVRational tb = {1, frame->sample_rate};  
//pts = frame->pts * av_q2d(tb)   
    视频：利用流的time_base转化
//AVRational tb = is->video_st->time_base;    
//frame->pts = frame->pkt_dts
//pts = frame->pts * av_q2d(tb)
```

### 初始化InIt

```c++
int AudioOutput::Init(){
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec wanted_spec;
    wanted_spec.channels = 2;// 只支持2channel的输出
    
    //SDL的要求输出格式
    wanted_spec.freq = src_tgt_.freq;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.XXX = YYY;
    
    wanted_spec.callback = fill_audio_pcm;//回调函数
    wanted_spec.userdata = this;
    
    //记录保存上面SDL的输出格式 :  AudioParams dst_tgt_; // SDL实际输出的格式
    dst_tgt_.channels = wanted_spec.channels; //spec.channels;
    dst_tgt_.fmt = AV_SAMPLE_FMT_S16;
    dst_tgt_.freq = wanted_spec.freq ;// spec.freq;
    dst_tgt_.channel_layout = av_get_default_channel_layout(2);
    dst_tgt_.frame_size = 1024;//src_tgt_.frame_size;
    
    SDL_PauseAudio(0);
}
```

### SDL回调函数

​       一旦你的回调函数填充了音频数据，SDL会负责将这些数据发送到音频设备，并输出声音。你不需要手动处理音频输出，SDL会自动处理。你的任务是确保在回调函数中提供足够的音频数据以保持音频的流畅播放。这种机制使得音频处理和输出变得相对简单，同时也确保了音频和视频的同步。



```c++

void fill_audio_pcm(void *userdata, Uint8 *stream, int len){
     // 1. 从frame queue读取解码后的PCM的数据，填充到stream（指向音频数据缓冲区的指针）
     // 2. len = 4000字节， 一个frame有6000字节， 一次读取了4000， 这个frame剩了2000字节
    AudioOutput *is = (AudioOutput *)userdata;  //关联用户自定义的数据对象，从而操作数据信息。
    int len1 = 0;
    int audio_size = 0;
    
    SDL stream拷贝原理：检查Frame是否需要重采样，存入缓冲区is->audio_buf中，然后is->audio_buf--->stream中
  只有当 is->audio_buf 数据全部流向stream后，才继续取下一个Frame并且重新更新buf。
    while(len>0){//  len是stream需要的字节数，加入一个frame len1 = 6000，那么stream只要4000.
          if(is->audio_buf_index == is->audio_buf_size){//buf数据读取完毕，重新拷贝Fream buf
             is->audio_buf_index = 0;
             AVFrame *frame = is->frame_queue_->Pop(10);
             if(frame){//如果为NULL，则memset(stream, 0, len1);
                 is->pts_ = frame->pts;
                 //判断是否需要重采样：frame的参数和SDL的参数不一致时。
                 if((frame->format != is->dst_tgt_.fmt)||(frame->sample_rate != is->dst_tgt_.freq)||(frame->channel_layout != is->dst_tgt_.channel_layout)){
                     //需要重采样，进行init  and  set_opt
                     is->swr_ctx_ = swr_alloc_set_opts(NULL,
                                                      is->dst_tgt_.channel_layout,
                                                      (enum AVSampleFormat)is->dst_tgt_.fmt,
                                                      is->dst_tgt_.freq,
                                                      frame->channel_layout,
                                                      (enum AVSampleFormat)frame->format,
                                                      frame->sample_rate,
                                                      0, NULL);
                     swr_init(is->swr_ctx_)
                 }
                 if(is->swr_ctx_) { // 重采样
                     const uint8_t **in = (const uint8_t **)frame->extended_data;
                     uint8_t **out = &is->audio_buf1_;
                     int out_samples = frame->nb_samples * is->dst_tgt_.freq/frame->sample_rate + 256;
                     int out_bytes = av_samples_get_buffer_size(NULL, is->dst_tgt_.channels, out_samples, is->dst_tgt_.fmt, 0);
                     av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, out_bytes);
                     int len2 = swr_convert(is->swr_ctx_, out, out_samples, in, frame->nb_samples);
                     is->audio_buf_ = is->audio_buf1_;
                 }else{//不需要重采样
                     av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, audio_size);
                     is->audio_buf_ = is->audio_buf1_;
                     memcpy(is->audio_buf_, frame->data[0], audio_size);
                 }
                 av_frame_free(&frame);//frame数据都已经存入到buf中，可以free
             }
              //计算要拷贝多少数据、以及从buf中的哪个位置拷贝数据
              len1 = is->audio_buf_size - is->audio_buf_index;//代表可以拷贝这么多的数据
              if(len1 > len){
                  len1 = len ;//说明只需要len长的数据，多了不需要
              }
              //拷贝数据buf到stream
              memcpy(stream, is->audio_buf_ + is->audio_buf_index, len1);
              len-=len1;
              stream+=len1;
              is->audio_buf_index+=len1;
        }
    }
    
}
```



## video_output.h

关联VideoFrameQueue队列，读取帧数据并且输出

![image-20231101211037731](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101211037731.png)

### 初始化Init

![image-20231101212437474](D:\typora-image\image-20231101212437474.png)

```c++
int VideoOutput::Init(){
      SDL_Init(SDL_INIT_VIDEO)；
      win_ = SDL_CreateWindow("player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            video_width_, video_height_, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
      renderer_ = SDL_CreateRenderer(win_, -1, 0);
      texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,                                   video_width_, video_height_); 
    
      yuv_buf_size_ = video_width_ * video_height_ * 1.5;//YUV
      yuv_buf_ = (uint8_t *)malloc(yuv_buf_size_);
    
}
```

### 输出显示

![image-20231101213303909](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231101213303909.png)

```c++
void VideoOutput::RefreshLoopWaitEvent(SDL_Event *event){
    //每隔X时间刷新画面
        while(!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)){
            videoRefresh(&remaining_time);
        }
}
void VideoOutput::videoRefresh(double *remaining_time){
    AVFrame *frame = NULL;
    frame = frame_queue_->Front();
    //同步音频操作：具体见 音视频同步 讲解
    
    ......
        
    // 有就渲染
        rect_.x = 0;
        rect_.y = 0;
        rect_.w = video_width_;
        rect_.h = video_height_;
        SDL_UpdateYUVTexture(texture_, &rect_, frame->data[0], frame->linesize[0],
                frame->data[1], frame->linesize[1],
                frame->data[2], frame->linesize[2]);
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, NULL, &rect_);
        SDL_RenderPresent(renderer_);
        frame = frame_queue_->Pop(1);
        av_frame_free(&frame);    
}
```



```c++
SDL视频渲染输出和QT组合：
    
StartPlay(strFile, WId类型 ui->label->winId());
play_wid = ui->label->winId()
SDL_Window window = SDL_CreateWindowFrom((void *)play_wid);
```



# 音视频同步

  以音频为准，视频同步音频



## AVSync.h

```c++
 time_t GetMicroseconds() //用来获取程序开始到调用此函数所用的Time
     
     void SetClock(double pts) {
        double time = GetMicroseconds() / 1000000.0; // us -> s
       //GetMicroseconds() 返回的是微秒（1秒 = 1,000,000 微秒），而除以 1000000.0 就是将微秒转换为秒。
        SetClockAt(pts, time);
    }    

    void SetClockAt(double pts, double time) {
        pts_ = pts;
        pts_drift_ = pts_ - time;
    }
   
    double GetClock() {   //得到的结果意思是：应该在X时刻显示
        double time = GetMicroseconds() / 1000000.0;
        return pts_drift_ + time;
    }
```

在音频输出audio_output.h中进行Clock设置

```c++
  // SDL中的回调函数内部使用
       
if(is->pts_ != AV_NOPTS_VALUE) {
        double pts = is->pts_ * av_q2d(is->time_base_);  // 转化成s单位
        is->avsync_->SetClock(pts);
    }
```

在视频输出video_output.h中进行GetClock操作

```c++
    if(frame) {
        double pts = frame->pts * av_q2d(time_base_)；

        double diff = pts - avsync_->GetClock();

        if(diff > 0) { //说明当前视频帧的时间戳比同步时钟的时间晚，即视频帧需要稍后才能显示。这通常表示视频帧需要等待一段时间，以便与同步时钟同步。
            *remaining_time = FFMIN(*remaining_time, diff);
            //remaining_time 通常用于控制视频帧的刷新速度,当diff>0：说明需要早点更快刷新视频帧。
            
            return;//重新设置刷新时间便于同步。
        }
```



# 主流程Main.cpp



```c++
    // queue
    AVPacketQueue audio_packet_queue;
    AVPacketQueue video_packet_queue;

    AVFrameQueue audio_frame_queue;
    AVFrameQueue video_frame_queue;

    AVSync avsync;
    //1 .解复用：解出pkt放入到pkt_queue
    DemuxThread *demux_thread = new DemuxThread(&audio_packet_queue, &video_packet_queue);
    ret = demux_thread->Init(argv[1]);
    ret = demux_thread->Start();
    //2.音视解码、视频解码线程
    DecodeThread *audio_decode_thread = new DecodeThread(&audio_packet_queue, &audio_frame_queue);
    audio_decode_thread->Init(demux_thread->AudioCodecParameters());
    audio_decode_thread->Start();
    
    DecodeThread *video_decode_thread = new DecodeThread(&video_packet_queue, &video_frame_queue);
    video_decode_thread->Init(demux_thread->VideoCodecParameters());
    video_decode_thread->Start();
    
    //3.audio输出和video输出
    AudioParams audio_params = {0};
    memset(&audio_params, 0, sizeof(AudioParams));
    audio_params.channels = demux_thread->AudioCodecParameters()->channels;
    audio_params.channel_layout = demux_thread->AudioCodecParameters()->channel_layout;
    audio_params.fmt = (enum AVSampleFormat) demux_thread->AudioCodecParameters()->format;
    audio_params.freq = demux_thread->AudioCodecParameters()->sample_rate;
    audio_params.frame_size =demux_thread->AudioCodecParameters()->frame_size;
    //audio输出
    AudioOutput *audio_output = new AudioOutput(&avsync, demux_thread->AudioStreamTimebase(),                                                         audio_params, &audio_frame_queue);
    audio_output->Init();
    //video输出
    VideoOutput *video_output = new VideoOutput(&avsync, demux_thread->VideoStreamTimebase(),
                                &video_frame_queue, demux_thread->VideoCodecParameters()->width,
                                demux_thread->VideoCodecParameters()->height);
   video_output->Init();
   video_output->MainLoop();

    //4.停止线程并且释放资源
    demux_thread->Stop();
    delete demux_thread;
    
    audio_decode_thread->Stop();    
    delete audio_decode_thread;
    
    video_decode_thread->Stop();
    delete video_decode_thread;
```













