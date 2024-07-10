# MP4合成

## 1.muxer结构

![image-20231013165853953](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231013165853953.png)

![image-20231015161241422](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231015161241422.png)

![image-20231015152250036](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231015152250036.png)

![image-20231015152229465](D:\typora-image\image-20231015152229465.png)

-----------------

![image-20231015154819100](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231015154819100.png)

一个视频编码器可能使用以微秒为单位的时间基，而一个封装器（如MP4封装器）可能使用以毫秒为单位的时间基。在这种情况下，编码器生成的时间戳需要转换为容器的时间基，以确保视频帧在容器中的正确定位和同步。

![image-20231015195802442](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231015195802442.png)

![image-20231015195752536](D:\typora-image\image-20231015195752536.png)

音视频帧经过编码编码：需要确保音频帧的时间戳与编码器的时间基匹配，从而在编码过程中正确地设置时间戳。

frame->packet.

封装过程时：对编码器的时间基等信息也进行重新设置。

packer->fmt_ctx.

--------------------------

![image-20231015155308614](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231015155308614.png)

`av_interleaved_write_frame` 函数的作用是将 `packet` 中的音视频帧写入 `fmt_ctx_` 表示的媒体文件，并且在写入之前，它会对帧的时间戳进行排序以保持时间同步。此函数会将 `packet` 写入媒体文件的正确位置，并将时间戳信息与 `fmt_ctx_` 中的信息相匹配，以确保生成的媒体文件是正确的和具有正确的时间同步。

---------------------------------------------

--------------------------------

## 2.音视频编码

![image-20231015212123690](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231015212123690.png)

用于填充图像帧 (`frame_`) 的数据和属性。它主要使用 `av_image_fill_arrays` 函数，将 YUV 数据填充到图像帧的不同平面中，以准备后续的图像处理或编码。需要独立创建frame，并且用来填充。

--------------

## 3.合成



# MP4合成代码



```c++
//1.打开YUV、PCM数据文件 || 初始化编码器和帧
in_yuv_fd = fopen(in_yuv_name, "rb");
in_pcm_fd = fopen(in_pcm_name, "rb");

AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
codec_ctx_ = avcodec_alloc_context3(codec);
codec_ctx_->XXX = XXX；......
avcodec_open2(codec_ctx_, NULL, NULL);

frame_ = av_frame_alloc();
frame_->XXX = XXX；

//2.因为要对PCM和YUV编码，所以进行编码初始化操作：分配PCM、YUV的buf、创建并配置重采样器
uint8_t *pcm_frame_buf = (uint8_t *)malloc(pcm_frame_size);
uint8_t *yuv_frame_buf = (uint8_t *)malloc(yuv_frame_size);

swr_ctx_ = swr_alloc_set_opts();
swr_init(swr_ctx_);

//3.mp4初始化 包括新建流，open io, send header
avformat_alloc_output_context2(&fmt_ctx_, NULL, NULL,url);
  //3.1创建音视频流
  mp4_muxer.AddStream(video_encoder.GetCodecContext());
  mp4_muxer.AddStream(audio_encoder.GetCodecContext());
      //流内部创建逻辑
        AVStream *st = avformat_new_stream(fmt_ctx_, NULL);
        avcodec_parameters_from_context(st->codecpar, codec_ctx);
  //3.2 open
  avio_open(&fmt_ctx_->pb, url_.c_str(), AVIO_FLAG_WRITE);
  //3.3 SendHeader
  avformat_write_header(fmt_ctx_, NULL);

// 4. 在while循环读取yuv、pcm进行编码然后发送给MP4 muxer
while (1) {
    fread(yuv_frame_buf, 1, yuv_frame_size, in_yuv_fd);
    video_encoder.Encode(yuv_frame_buf, yuv_frame_size, video_index,video_pts, video_time_base);
        //video   Encode内部核心代码：
        // 时间基转换：确保视频帧的时间戳与编码器期望的时间基一致，以便正确编码。
        pts = av_rescale_q(pts, AVRational{1, (int)time_base}, codec_ctx_->time_base);
        frame_->pts = pts;
        //YUV填充frame
        av_image_fill_arrays(frame_->data, frame_->linesize,yuv_data, 
                             (AVPixelFormat)frame_->format,frame_->width, frame_->height, 1);
        avcodec_send_frame(codec_ctx_, frame_);
        AVPacket *packet = av_packet_alloc();
        avcodec_receive_packet(codec_ctx_, packet);
        packet->stream_index = stream_index;
        return packet;
    
      //音频重采样，并且填充数据
      swr_convert(ctx_, out_frame->data, out_frame->nb_samples,indata, out_frame->nb_samples);
    
// 5. 发送包数据
mp4_muxer.SendPacket(packet);
        // 时间基转换：确保音频包的时间戳与输出容器的时间基一致，以便在正确的时间点写入文件。
        //vid_stream_->time_base通常在创建编码器和封装器以及调用 'avformat_write_header' 函数时进行设置。
        src_time_base = vid_codec_ctx_->time_base;
        dst_time_base = vid_stream_->time_base;
    
        packet->pts = av_rescale_q(packet->pts, src_time_base, dst_time_base);
        packet->dts = av_rescale_q(packet->dts, src_time_base, dst_time_base);
        packet->duration = av_rescale_q(packet->duration, src_time_base, dst_time_base);
        //写入文件
        av_interleaved_write_frame(fmt_ctx_, packet); 
        av_packet_free(&packet);
    
//6. 发送尾数据
av_write_trailer(fmt_ctx_);    
    
```

```c++
av_rescale_q
将一个时间戳从一个时间基转换到另一个时间基。例如，如果有一个时间戳在基于 1/25 的时间基中，你想转换到基于 1/1000 的时间基中。

AVRational src_tb = {1, 25};    // 原始时间基（每帧40毫秒）
AVRational dst_tb = {1, 1000};  // 目标时间基（每毫秒）
int64_t timestamp = 40;         // 在原始时间基中的时间戳
int64_t new_timestamp = av_rescale_q(timestamp, src_tb, dst_tb);

//**********************************************************************************************
av_rescale_rnd： nt64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd);
参数:

a: 要转换的时间戳或其他数值。
b: 乘数（通常是目标时间基的分母）。
c: 除数（通常是原始时间基的分母）。
rnd: 舍入方式，可以是 AV_ROUND_ZERO, AV_ROUND_INF, AV_ROUND_DOWN, AV_ROUND_UP, AV_ROUND_NEAR_INF, AV_ROUND_PASS_MINMAX 等。

int64_t timestamp = 40;        // 时间戳
int64_t new_timestamp = av_rescale_rnd(timestamp, 1000, 25, AV_ROUND_NEAR_INF);


```

