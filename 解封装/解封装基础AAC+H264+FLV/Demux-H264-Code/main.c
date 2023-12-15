#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>



static char err_buf[128] = {0};
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}

/*
AvCodecContext->extradata[]中为nalu长度
*   codec_extradata:
*   1, 64, 0, 1f, ff, e1, [0, 18], 67, 64, 0, 1f, ac, c8, 60, 78, 1b, 7e,
*   78, 40, 0, 0, fa, 40, 0, 3a, 98, 3, c6, c, 66, 80,
*   1, [0, 5],68, e9, 78, bc, b0, 0,
*/

//ffmpeg -i 2018.mp4 -codec copy -bsf:h264_mp4toannexb -f h264 tmp.h264
//ffmpeg 从mp4上提取H264的nalu h
int main(int argc, char **argv)
{
    AVFormatContext *ifmt_ctx = NULL;
    int             videoindex = -1;
    AVPacket        *pkt = NULL;
    int             ret = -1;
    int             file_end = 0; // 文件是否读取结束

    if(argc < 3)
    {
        printf("usage inputfile outfile\n");
        return -1;
    }
    FILE *outfp=fopen(argv[2],"wb");
    printf("in:%s out:%s\n", argv[1], argv[2]);

    // 分配解复用器的内存，使用avformat_close_input释放
    ifmt_ctx = avformat_alloc_context();
    if (!ifmt_ctx)
    {
        printf("[error] Could not allocate context.\n");
        return -1;
    }

    // 根据url打开码流，并选择匹配的解复用器
    ret = avformat_open_input(&ifmt_ctx,argv[1], NULL, NULL);
    if(ret != 0)
    {
        printf("[error]avformat_open_input: %s\n", av_get_err(ret));
        return -1;
    }

    // 读取媒体文件的部分数据包以获取码流信息
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if(ret < 0)
    {
        printf("[error]avformat_find_stream_info: %s\n", av_get_err(ret));
        avformat_close_input(&ifmt_ctx);
        return -1;
    }

    // 查找出哪个码流是video/audio/subtitles
    videoindex = -1;
    // 推荐的方式
    videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(videoindex == -1)
    {
        printf("Didn't find a video stream.\n");
        avformat_close_input(&ifmt_ctx);
        return -1;
    }

    // 分配数据包
    pkt = av_packet_alloc();
    av_init_packet(pkt);

    // 1 获取相应的比特流过滤器
    //FLV/MP4/MKV等结构中，h264需要h264_mp4toannexb处理。添加SPS/PPS等信息。
    // FLV封装时，可以把多个NALU放在一个VIDEO TAG中,结构为4B NALU长度+NALU1+4B NALU长度+NALU2+...,
    // 需要做的处理把4B长度换成00 00 00 01或者00 00 01
    const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
    AVBSFContext *bsf_ctx = NULL;
    // 2 初始化过滤器上下文
    av_bsf_alloc(bsfilter, &bsf_ctx); //AVBSFContext;
    // 3 添加解码器属性
    avcodec_parameters_copy(bsf_ctx->par_in, ifmt_ctx->streams[videoindex]->codecpar);
    av_bsf_init(bsf_ctx);

    file_end = 0;
    while (0 == file_end)
    {
        if((ret = av_read_frame(ifmt_ctx, pkt)) < 0)
        {
            // 没有更多包可读
            file_end = 1;
            printf("read file end: ret:%d\n", ret);
        }
        if(ret == 0 && pkt->stream_index == videoindex)
        {
#if 0
            int input_size = pkt->size;
            int out_pkt_count = 0;
            if (av_bsf_send_packet(bsf_ctx, pkt) != 0) // bitstreamfilter内部去维护内存空间
            {
                av_packet_unref(pkt);   // 你不用了就把资源释放掉
                continue;       // 继续送
            }
            av_packet_unref(pkt);   // 释放资源
            while(av_bsf_receive_packet(bsf_ctx, pkt) == 0)
            {
                out_pkt_count++;
                // printf("fwrite size:%d\n", pkt->size);
                size_t size = fwrite(pkt->data, 1, pkt->size, outfp);
                if(size != pkt->size)
                {
                    printf("fwrite failed-> write:%u, pkt_size:%u\n", size, pkt->size);
                }
                av_packet_unref(pkt);
            }
            if(out_pkt_count >= 2)
            {
                printf("cur pkt(size:%d) only get 1 out pkt, it get %d pkts\n",
                       input_size, out_pkt_count);
            }
#else       // TS流可以直接写入
            size_t size = fwrite(pkt->data, 1, pkt->size, outfp);
            if(size != pkt->size)
            {
                printf("fwrite failed-> write:%u, pkt_size:%u\n", size, pkt->size);
            }
            av_packet_unref(pkt);
#endif
        }
        else
        {
            if(ret == 0)
                av_packet_unref(pkt);        // 释放内存
        }
    }
    if(outfp)
        fclose(outfp);
    if(bsf_ctx)
        av_bsf_free(&bsf_ctx);
    if(pkt)
        av_packet_free(&pkt);
    if(ifmt_ctx)
        avformat_close_input(&ifmt_ctx);
    printf("finish\n");

    return 0;
}
