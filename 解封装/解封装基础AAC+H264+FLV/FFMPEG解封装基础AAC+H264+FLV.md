# FFMPEG解封装

![image-20230926200248431](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926200248431.png)

##   AAC ADTS

用于音频压缩

![image-20230926211822817](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926211822817.png)

![image-20230926211957843](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230926211957843.png)

![image-20231007095446271](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231007095446271.png)

![image-20231007095552756](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231007095552756.png)

![image-20231007110031198](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231007110031198.png)

## H264 NALU

h264（别称AVC）

 ![image-20230927100656456](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927100656456.png)

GOP 表示一组相互依赖的视频帧，这些帧在压缩和解压缩过程中一起处理。

![image-20230927101530080](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927101530080.png)

![image-20230927101929676](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927101929676.png)



NALU 是视频编码中的基本单元，它将视频帧分割成一个个小的单元，以便进行压缩、传输和解压缩。每个 NALU 包含了一部分视频数据，以及一些元数据，用于描述数据的类型和编码参数。

NALU 的存在使得视频数据能够以单元的方式进行传输和解码，以及在网络上进行流式传输。它是视频编码和解码的基础构建块之一。

NALU（Network Abstraction Layer Unit）是视频编码过程中的基本单位，它是在视频流中进行编码和解码的基本单元。视频编码器将原始的视频帧分割成一个个的NALU，并对每个NALU进行压缩编码，最终形成压缩后的视频流。在解码端，解码器会逐个解码这些NALU，还原出原始的视频帧。

因此，NALU与视频帧之间的关系可以描述为：

1. **NALU是视频帧的编码表示**：原始的视频帧经过编码器处理后，被分割成一个个的NALU。每个NALU包含了编码器对视频帧进行压缩编码后的数据。
2. **多个NALU组成一个完整的视频帧**：一个完整的视频帧可能会被分割成多个NALU进行编码，这些NALU经过解码后能够还原出原始的视频帧。
3. **视频帧由NALU组装而成**：在解码端，解码器会逐个解码接收到的NALU，并根据视频编码格式的规范将这些NALU组装成原始的视频帧。

综上所述，NALU是视频编码过程中的编码单元，而视频帧是在解码过程中由NALU组装而成的原始视频数据单元。在视频编解码过程中，NALU和视频帧之间存在着密切的关系，NALU是视频帧经过编码后的表示形式，而视频帧则是NALU经过解码后还原出的原始视频数据。

![image-20230927103606861](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927103606861.png)

![image-20230927104733017](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927104733017.png)

解复用（Demuxing）的过程可以从多媒体文件中提取出音频、视频和其他数据流，通常这些流是裸流（Elementary Stream）的一部分。裸流是原始的音频或视频数据，通常不包含封装格式的额外信息。

裸流（Elementary Stream）通常是原始的音频或视频数据，没有包含封装格式或其他附加信息，因此无法直接播放。裸流是原始的编码数据，通常需要进一步处理才能进行播放或解码。

AVPacket前4个字表示的是nalu的长度，从第5个字节开始才是nalu的数据。所以直接将AVPacket前4个字节替换为0x00000001即可得到标准的nalu数据。

```c
const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
//是获取名为 "h264_mp4toannexb" 的过滤器。
h264_mp4toannexb比特流过滤器的主要作用是将H.264视频流从MP4封装格式中提取，并将其重新封装为Annex B格式的比特流。Annex B格式是一种H.264编码视频的特定封装方式，它将视频帧分隔开，并为每个帧添加了起始码（start code），以便解码器可以正确识别帧的开始和结束。
    ret = av_bsf_send_packet(bsf_ctx, &pkt);
    ret = av_bsf_receive_packet(bsf_ctx, &pkt))
```

```
FLV/MP4/MKV等结构中，h264需要h264_mp4toannexb处理。添加信息
```

如果想在FLV/MP4/MKV等结构中提取h264流，并且进行播放，需要先使用h264_mp4toannexb过滤器对h264裸流中的每一个pkt数据包进行数据处理：比如添加sps等信息，然后才可以输出一个.h264文件，ffplay才可以播放



## FLV

  ![image-20230927164506002](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927164506002.png)

![image-20230927170846822](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927170846822.png)

![image-20230927171210222](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927171210222.png)

![image-20231016201524068](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231016201524068.png)

![image-20230927212038694](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927212038694.png)

`_pTagData` 指向FLV标签的原始媒体数据，而 `_pMedia` 指向标签的元数据，后者通常包含有关媒体内容的描述信息。

![image-20230927212833394](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927212833394.png)

![image-20230927212845202](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927212845202.png)

`pParser->_nNalUnitLength` 的值表示了在解析 H.264 视频流时应该用多少字节来表示 NALU 数据的长度。

通常情况下，它可以为 1、2 或 4，具体取决于视频流的编码参数和配置。

```c
        // 获取NALU的起始码
        memcpy(_pMedia + _nMediaLen, &nH264StartCode, 4);
        // 复制NALU的数据
        memcpy(_pMedia + _nMediaLen + 4, pd + nOffset + pParser->_nNalUnitLength, nNaluLen);
        // 解析NALU
//        pParser->_vjj->Process(_pMedia+_nMediaLen, 4+nNaluLen, _header.nTotalTS);
        _nMediaLen += (4 + nNaluLen);
        nOffset += (pParser->_nNalUnitLength + nNaluLen);

一个tag可能包含多个nalu, 所以每个nalu前面有NalUnitLength字节表示每个nalu的长度
这段代码中：pMieda没有存NalUnitLength的数据，跳过了，并且更新offset偏移量。
    存入的不包含4字节的头部信息，只是解析，存入的是statcode  +   nalu data   
```

视频Tag Data开始的：
第⼀个字节包含视频数据的参数信息，
第⼆个字节开始为视频流数据，

如下图，1字节+1字节+3字节

故offset初始 = 5。

![image-20240725213439853](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20240725213439853.png)

![image-20231016215301114](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231016215301114.png)

![image-20230927215525858](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20230927215525858.png)

音频同理：

⾳频Tag Data区域开始的：
第⼀个字节包含了⾳频数据的参数信息，

第⼆个字节开始为⾳频流数据，如下图：

![image-20240725213208489](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20240725213208489.png)

![image-20231016220443838](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231016220443838.png)

![image-20231016220658073](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231016220658073.png)

#### Script tag  data 解析

![image-20231007103153746](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231007103153746.png)

![image-20231007104449288](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231007104449288.png)

通常情况下，从FLV文件中解析的音频AAC数据已经包含了ADTS（Audio Data Transport Stream）封装。ADTS是一种用于封装AAC音频数据的常见格式，它包含了音频帧的头部信息和原始音频数据，以便解码器能够正确地解释和播放音频。

<img src="https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20240320155238456.png" alt="image-20240320155238456" style="zoom:100%;" />

-------------

------------

![image-20231016204004948](https://my-figures.oss-cn-beijing.aliyuncs.com/Figures/image-20231016204004948.png)

----------

## MP4解封装（解复用）AAC代码流程：

```c++
aac_fd = fopen(aac_filename, "wb");

AVFormatContext * ifmt_ctx = avformat_alloc_context();
avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL);
/*在打开媒体文件时，avformat_open_input() 函数会根据文件的格式选择合适的 AVInputFormat。这个信息将存储在 AVFormatContext 结构体中的 iformat 字段中。
AVInputFormat 主要用于描述和标识媒体文件的输入格式的特性，而 AVFormatContext 包含了实际打开的媒体文件的详细信息。*/
avformat_find_stream_info(ifmt_ctx, NULL);//查找媒体流、编解码器等配置信息。赋值给ctx
//ifmt_ctx->streams[audio_index]->codecpar->codec_id != AV_CODEC_ID_AAC
//调用之后就知道每一道流中的具体信息,调用av_find_best_stream等函数

//创建接收packtet
AVPacket pkt = av_packet_alloc();
av_init_packet(&pkt);
//找到音频流index
av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
//读取数据到pkt
av_read_frame(ifmt_ctx,&pkt);
fwrite(adts_header_buf, 1, 7, aac_fd);//写入头数据
fwrite( pkt.data, 1, pkt.size, aac_fd);//写入body数据
av_packet_unref(&pkt);
```

## MP4解封装（解复用）H264代码流程：

```c++
aac_fd = fopen(aac_filename, "wb");

AVFormatContext * ctx = avformat_alloc_context();
avformat_open_input(&ctx, in_filename, NULL, NULL);
avformat_find_stream_info(ctx, NULL);//查找媒体流、编解码器等配置信息。赋值给ctx
//ifmt_ctx->streams[audio_index]->codecpar->codec_id != AV_CODEC_ID_AAC
//调用之后就知道每一道流中的具体信息,调用av_find_best_stream等函数

//创建接收packtet
AVPacket pkt = av_packet_alloc();;
av_init_packet(&pkt);
//找到视频流index
av_find_best_stream(ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
//创建过滤器and配置
const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
AVBSFContext *bsf_ctx = NULL;
av_bsf_alloc(bsfilter, &bsf_ctx); 
avcodec_parameters_copy(bsf_ctx->par_in, ifmt_ctx->streams[videoindex]->codecpar);
av_bsf_init(bsf_ctx);

while (0 == file_end)
{
//读取数据到pkt
av_read_frame(ctx,&pkt);
//读取后，判断pkt的index
if(pkt->stream_index == video_index) {
//把解析出的pkt传入过滤器中
av_bsf_send_packet(bsf_ctx, pkt);
//send后，需要while循环一直receive pkt
while(av_bsf_receive_packet(bsf_ctx, pkt) == 0)
{
fwrite( pkt.data, 1, pkt.size, aac_fd);//写入body数据
}
}
av_packet_unref(&pkt);
}
```



![image-20231017095117888](D:\typora-image\image-20231017095117888.png)

## FLV分析

当对FLV 文件进行解复用时，您实际上是在取出FLV 文件中的不同数据标记（tags），然后可以选择将这些标记保存为单独的文件或重新组合它们以创建新的FLV 文件。FLV 文件的基本结构包括一个包含各种类型标记的标记流。这些标记包括音频标记、视频标记和元数据标记等，它们分别包含音频、视频或其他元数据信息。

在解复用FLV 文件时，您可以选择提取特定类型的标记，例如音频标记和视频标记，将它们保存为独立的音频和视频文件。这些标记通常包含音频和视频数据以及时间戳信息，以便在后续处理中正确同步音频和视频。

如果需要重新组合标记以创建新的FLV 文件，您可以将这些标记按照正确的顺序重新组合，确保正确的时间戳和同步，然后将它们写入新的FLV 文件。这在某些情况下可能需要专业的多媒体处理工具或编程知识。

































