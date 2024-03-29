# 音视频FLV合成实战

  AVOutputFormat *fmt;  进行复用，解复用则是AVInputFormat *fmt

`AVInputFormat` 结构通常在解复用过程中内部使用，而不需要显式地指定。你可以通过 `avformat_open_input` 函数来打开输入文件，并在其内部使用合适的 `AVInputFormat` 结构来正确解析多媒体数据。

![image-20231012212012001](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231012212012001.png)

![image-20231012210415458](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231012210415458.png)

```c++
AVOutputFormat * fmt = AVFormatContext * oc->oformat; // 获取绑定的AVOutputFormat            ***反之***
```

分离音频和视频流通常是在解复用（Demuxing）过程中，通过遍历 `AVFormatContext` 结构的各个媒体流，根据媒体流的类型来实现的。`AVInputFormat` 结构主要用于表示文件格式，而 `AVFormatContext` 结构包含了媒体流的具体信息。

-------------

```c++
 OutputStream * ost->st =  avformat_new_stream(oc, NULL);    // 创建一个流成分
 解复用：从ctx分离两个音频和视频流
     
 反之，复用：创建两个流重新绑定到ctx中。
     
```

-----------

![image-20231012203536263](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231012203536263.png)

![image-20231012203807106](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231012203807106.png)

--------------

---------

自定义创建输出流数据结构

![image-20231012205008816](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231012205008816.png)

--------------------

-------------

![image-20231013100421367](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231013100421367.png)

![image-20231013100904589](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231013100904589.png)

-----------------------

-------------------------

```c++
    // 新建码流 绑定到 AVFormatContext stream->index 有设置
    ost->st = avformat_new_stream(oc, NULL);    // 创建一个流成分
```

FFmpeg会为新创建的媒体流自动分配一个合适的 `index`

![image-20231013111540130](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231013111540130.png)



# 合成FLV代码



```c++
//1.创建音视频对象
AVOutputFormat *output_fmt;
AVFormatContext *format_ctx;
OutputStream video_st = { 0 }; // 封装视频编码相关的
OutputStream audio_st = { 0 }; // 封装音频编码相关的
AVCodec *audio_codec, *video_codec;

//2.分配AVFormatContext并根据filename绑定合适的AVOutputFormat
avformat_alloc_output_context2(&format_ctx, NULL, NULL, filename);

//3.为自己创建的AVOutputFormat *output_fmt进行初始化
output_fmt = format_ctx->oformat;// 获取绑定的AVOutputFormat
output_fmt->video_codec = AV_CODEC_ID_H264;    // 指定编码器
output_fmt->audio_codec = AV_CODEC_ID_AAC;     // 指定编码器

//4.根据音视频编码格式创建音频流、视频流
add_stream(&video_st, format_ctx, &video_codec, output_fmt->video_codec);
add_stream(&audio_st, format_ctx, &audio_codec, output_fmt->audio_codec);
   //4.1 add_stream()函数解析：
   1.'find'根据传入的编码器、设参、赋值         codec_id = output_fmt->video_codec
   *codec = avcodec_find_encoder(codec_id);    //通过codec_id找到编码器 
    codec_ctx = avcodec_alloc_context3(*codec); // 创建编码器上下文
    video_st->enc = codec_ctx;
    codec_ctx->codec_id = codec_id;
    codec_ctx->XXX = XXX;
    ......
        
   2.根据 'AVFormatContext *format_ctx' 创建流、并对流进行处理：设置参数等信息，
   video_st->st = avformat_new_stream(format_ctx, NULL);    // 创建一个流成分
   video_st->st->id = format_ctx ->nb_streams - 1;
   

//5.open：关联编码器，创建数据......
open_video(format_ctx, video_codec, &video_st, opt);
     //5.1 关联编码器
     avcodec_open2(codec_ctx, codec, &opt);
     //5.2 创建帧buffer
     video_st->frame = alloc_picture(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
     //5.3 在关联编码器open后调用，因为ctx参数可能还会被codec改变，       copy
     avcodec_parameters_from_context(video_st->st->codecpar, codec_ctx);//编码器上下参数copy
open_audio（）需要设置重采样器

//6.打开输出文件，写入头、中、尾 数据
avio_open(&format_ctx->pb, filename, AVIO_FLAG_WRITE);
avformat_write_header(format_ctx, &opt);
  //创建生成测试frame 
   ......
  //编码frame(send+recive) 
   ......
  //时间基转换
av_packet_rescale_ts(pkt, *time_base, st->time_base);
av_interleaved_write_frame(fmt_ctx, &pkt);//必须在之前调用avformat_alloc_output_context2() 然后会写入到输出文件中
av_write_trailer(format_ctx);

//7.close and  free
close_stream(fmt_ctx, &video_st);
......
avformat_free_context(fmt_ctx);
......
    
```



## 简单demo

```c++
#include <stdio.h>
#include <libavformat/avformat.h>

int main() {
    AVFormatContext *video_fmt_ctx = NULL;
    AVFormatContext *audio_fmt_ctx = NULL;
    AVFormatContext *out_fmt_ctx = NULL;
    AVPacket video_pkt, audio_pkt;
    int ret;

    // 打开视频文件
    if ((ret = avformat_open_input(&video_fmt_ctx, "input.h264", NULL, NULL)) < 0) {
        fprintf(stderr, "Error opening video input file\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(video_fmt_ctx, NULL)) < 0) {
        fprintf(stderr, "Error finding video stream information\n");
        return ret;
    }

    // 打开音频文件
    if ((ret = avformat_open_input(&audio_fmt_ctx, "input.aac", NULL, NULL)) < 0) {
        fprintf(stderr, "Error opening audio input file\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(audio_fmt_ctx, NULL)) < 0) {
        fprintf(stderr, "Error finding audio stream information\n");
        return ret;
    }

    // 创建 FLV 格式的输出文件
    if ((ret = avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, "output.flv")) < 0) {
        fprintf(stderr, "Error creating output context\n");
        return ret;
    }

    // 添加视频流和音频流到输出文件
    for (int i = 0; i < video_fmt_ctx->nb_streams; i++) {
        AVStream *stream = avformat_new_stream(out_fmt_ctx, NULL);
        avcodec_parameters_copy(stream->codecpar, video_fmt_ctx->streams[i]->codecpar);
    }
    for (int i = 0; i < audio_fmt_ctx->nb_streams; i++) {
        AVStream *stream = avformat_new_stream(out_fmt_ctx, NULL);
        avcodec_parameters_copy(stream->codecpar, audio_fmt_ctx->streams[i]->codecpar);
    }

    // 打开输出文件
    if ((ret = avio_open(&out_fmt_ctx->pb, "output.flv", AVIO_FLAG_WRITE)) < 0) {
        fprintf(stderr, "Error opening output file\n");
        return ret;
    }

    // 写入输出文件的头部
    avformat_write_header(out_fmt_ctx, NULL);

    // 将视频文件的数据写入输出文件
    while (av_read_frame(video_fmt_ctx, &video_pkt) >= 0) {
        av_interleaved_write_frame(out_fmt_ctx, &video_pkt);
        av_packet_unref(&video_pkt);
    }

    // 将音频文件的数据写入输出文件
    while (av_read_frame(audio_fmt_ctx, &audio_pkt) >= 0) {
        av_interleaved_write_frame(out_fmt_ctx, &audio_pkt);
        
        //AVPacket 的数据流向是先流入到 AVFormatContext 中，然后流向输出文件。通过 av_interleaved_write_frame() 函数，数据包被写入到 AVFormatContext 中，并最终写入到输出文件中。
        
        av_packet_unref(&audio_pkt);
        
    }

    // 写入输出文件的尾部
    av_write_trailer(out_fmt_ctx);

    // 释放资源
    avformat_close_input(&video_fmt_ctx);
    avformat_close_input(&audio_fmt_ctx);
    avformat_free_context(out_fmt_ctx);

    return 0;
}

```





