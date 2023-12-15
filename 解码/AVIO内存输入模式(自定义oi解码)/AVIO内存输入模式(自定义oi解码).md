# AVIO内存输入模式



![image-20231009102025775](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009102025775.png)

![image-20231009105749769](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009105749769.png)

![image-20231009105720038](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009105720038.png)

```
int ret = avio_open(&fmt_ctx_->pb, url_.c_str(), AVIO_FLAG_WRITE);
可以将 avio_open 视为一个自动化的文件读写接口，它会自动打开、读取和写入文件，你只需要提供文件路径和文件操作模式。这是一个简便的方法，适用于许多常见的文件操作
```

![image-20231016094641958](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231016094641958.png)



![image-20231009111057138](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009111057138.png)

![image-20231009111035955](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009111035955.png)

编码器需要open函数打开才能使用。

------------

![image-20231009105749769](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009105749769.png)

![image-20231009113311160](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009113311160.png)

![image-20231009113246015](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009113246015.png)

------------

<img src="https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231009120630251.png" alt="image-20231009120630251"  />

-----------

-----------

# AVIO自定义IO操作代码

```c++
//打开输入输出文件
in_file = fopen(in_file_name, "rb");
out_file = fopen(out_file_name, "wb");

//自定义IO
//1.创建format_ctx上下文 、解码器 、pkt 、 frame、
AVFormatContext *format_ctx = avformat_alloc_context();

AVCodec * codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
avcodec_open2(codec_ctx, codec, NULL);

AVPacket *packet = av_packet_alloc();
AVFrame *frame = av_frame_alloc();


//2.自定义IO：创建avio_ctx、自定义读写数据函数
uint8_t *io_buffer = av_malloc(BUF_SIZE);
AVIOContext *avio_ctx = avio_alloc_context(io_buffer, BUF_SIZE, 0, (void *)in_file,    \
                                               read_packet, NULL, NULL);
//3.关联format_ctx和avio_ctx，并且open输入文件：关联。
format_ctx->pb = avio_ctx;
avformat_open_input(&format_ctx, NULL, NULL, NULL);//后三个参数都为NULL就可以。因为avio_ctx已经关联。

//4.读写数据
av_read_frame(format_ctx, packet);//数据流向：io_buffer->avio_ctx->format_ctx->pkt.

//5.解码pkt
decode(codec_ctx, packet, frame, out_file);

//6.冲刷解码器
pkt->data = NULL;
avcodec_send_packet(dec_ctx, pkt);

//7.关闭文件和释放之前创建对象的内存
```



```c++
decode(codec_ctx, pkt, decoded_frame, outfile)函数分析
/**当收到pkt后，都要进行send和receive操作，比如bsf的av_bsf_send_packet(bsf_ctx, pkt);
所以解码send举一反三：avcodec_send_packet(codec_ctx,pkt);
然后send一次，while要receive接收**/    
ret = avcodec_send_packet(dec_ctx, pkt);    
while (ret >= 0){
ret = avcodec_receive_frame(dec_ctx, frame); 
/**可以对frame直接写入outfile**/
    
1.直接写入
    // 计算音频样本数据的总大小
    int sample_size = av_get_bytes_per_sample(frame->format);
    int num_samples = frame->nb_samples;
    int data_size = num_samples * frame->channels * sample_size;

    // 写入音频样本数据到文件
    fwrite(frame->data[0], 1, data_size, outfile);
------------------------------------------------------------------------
    
2.更改数据排列方式再写入
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        for (i = 0; i < frame->nb_samples; i++)
        {
            for (ch = 0; ch < dec_ctx->channels; ch++)  // 交错的方式写入, 大部分float的格式输出
                fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);
        }    
}
3.视频写入方式 YUV
        for(int j=0; j<frame->height; j++)
            fwrite(frame->data[0] + j * frame->linesize[0], 1, frame->width, outfile);
        for(int j=0; j<frame->height/2; j++)
            fwrite(frame->data[1] + j * frame->linesize[1], 1, frame->width/2, outfile);
        for(int j=0; j<frame->height/2; j++)
            fwrite(frame->data[2] + j * frame->linesize[2], 1, frame->width/2, outfile);
}
```

```c++
AVIOContext *avio_ctx = avio_alloc_context(io_buffer, BUF_SIZE, 0, (void *)in_file,    \
                                               read_packet, NULL, NULL);
//自定义read_pkt函数
static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    FILE *in_file = (FILE *)opaque;
    
    int read_size = fread(buf, 1, buf_size, in_file);//fread存入buf
    
    printf("read_packet read_size:%d, buf_size:%d\n", read_size, buf_size);
    if(read_size <=0) {
        return AVERROR_EOF;     // 数据读取完毕
    }
    return read_size;
}
```

