#include <stdio.h>

#include "libavutil/log.h"
#include "libavformat/avformat.h"

#define ERROR_STRING_SIZE 1024

#define ADTS_HEADER_LEN  7;

const int sampling_frequencies[] = {
    96000,  // 0x0
    88200,  // 0x1
    64000,  // 0x2
    48000,  // 0x3
    44100,  // 0x4
    32000,  // 0x5
    24000,  // 0x6
    22050,  // 0x7
    16000,  // 0x8
    12000,  // 0x9
    11025,  // 0xa
    8000   // 0xb
    // 0xc d e f是保留的
};

int adts_header(char * const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels)
{

    int sampling_frequency_index = 3; // 默认使用48000hz
    int adtsLen = data_length + 7;

    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for(i = 0; i < frequencies_size; i++)
    {
        if(sampling_frequencies[i] == samplerate)
        {
            sampling_frequency_index = i;
            break;
        }
    }
    if(i >= frequencies_size)
    {
        printf("unsupport samplerate:%d\n", samplerate);
        return -1;
    }

    p_adts_header[0] = 0xff;         //syncword:0xfff                          高8bits
    p_adts_header[1] = 0xf0;         //syncword:0xfff                          低4bits
    p_adts_header[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    p_adts_header[1] |= (0 << 1);    //Layer:0                                 2bits
    p_adts_header[1] |= 1;           //protection absent:1                     1bit

    p_adts_header[2] = (profile)<<6;            //profile:profile               2bits
    p_adts_header[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    p_adts_header[2] |= (0 << 1);             //private bit:0                   1bit
    p_adts_header[2] |= (channels & 0x04)>>2; //channel configuration:channels  高1bit

    p_adts_header[3] = (channels & 0x03)<<6; //channel configuration:channels 低2bits
    p_adts_header[3] |= (0 << 5);               //original：0                1bit
    p_adts_header[3] |= (0 << 4);               //home：0                    1bit
    p_adts_header[3] |= (0 << 3);               //copyright id bit：0        1bit
    p_adts_header[3] |= (0 << 2);               //copyright id start：0      1bit
    p_adts_header[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

    p_adts_header[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    p_adts_header[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    p_adts_header[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    p_adts_header[6] = 0xfc;      //‭11111100‬       //buffer fullness:0x7ff 低6bits
    // number_of_raw_data_blocks_in_frame：
    //    表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。

    return 0;
}



// 程序本身 input.mp4  out.h264 out.aac
int main(int argc, char **argv)
{
    // 判断参数
    if(argc != 4) {
        printf("usage app input.mp4  out.h264 out.aac");
        return -1;
    }

    char *in_filename = argv[1];
    char *h264_filename = argv[2];
    char *aac_filename = argv[3];
    FILE *aac_fd = NULL;
    FILE *h264_fd = NULL;

    h264_fd = fopen(h264_filename, "wb");
    if(!h264_fd) {
        printf("fopen %s failed\n", h264_filename);
        return -1;
    }

    aac_fd = fopen(aac_filename, "wb");
    if(!aac_fd) {
        printf("fopen %s failed\n", aac_filename);
        return -1;
    }

    AVFormatContext *ifmt_ctx = NULL;
    int video_index = -1;
    int audio_index = -1;
    AVPacket *pkt = NULL;
    int ret = 0;
    char errors[ERROR_STRING_SIZE+1];  // 主要是用来缓存解析FFmpeg api返回值的错误string

    ifmt_ctx = avformat_alloc_context();
    if(!ifmt_ctx) {
        printf("avformat_alloc_context failed\n");
        //        fclose(aac_fd);
        return -1;
    }

    ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL);
    if(ret < 0) {
        av_strerror(ret, errors, ERROR_STRING_SIZE);
        printf("avformat_open_input failed:%d\n", ret);
        printf("avformat_open_input failed:%s\n", errors);
        avformat_close_input(&ifmt_ctx);
        //        go failed;
        return -1;
    }

    video_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_index == -1) {
        printf("av_find_best_stream video_index failed\n");
        avformat_close_input(&ifmt_ctx);
        return -1;
    }

    audio_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_index == -1) {
        printf("av_find_best_stream audio_index failed\n");
        avformat_close_input(&ifmt_ctx);
        return -1;
    }

    // h264_mp4toannexb
    const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");      // 对应面向对象的方法
    if(!bsfilter) {
        avformat_close_input(&ifmt_ctx);
        printf("av_bsf_get_by_name h264_mp4toannexb failed\n");
        return -1;
    }
    AVBSFContext *bsf_ctx = NULL;        // 对应面向对象的变量
    ret = av_bsf_alloc(bsfilter, &bsf_ctx);
    if(ret < 0) {
        av_strerror(ret, errors, ERROR_STRING_SIZE);
        printf("av_bsf_alloc failed:%s\n", errors);
        avformat_close_input(&ifmt_ctx);
        return -1;
    }
    ret = avcodec_parameters_copy(bsf_ctx->par_in, ifmt_ctx->streams[video_index]->codecpar);
    if(ret < 0) {
        av_strerror(ret, errors, ERROR_STRING_SIZE);
        printf("avcodec_parameters_copy failed:%s\n", errors);
        avformat_close_input(&ifmt_ctx);
        av_bsf_free(&bsf_ctx);
        return -1;
    }
    ret = av_bsf_init(bsf_ctx);
    if(ret < 0) {
        av_strerror(ret, errors, ERROR_STRING_SIZE);
        printf("av_bsf_init failed:%s\n", errors);
        avformat_close_input(&ifmt_ctx);
        av_bsf_free(&bsf_ctx);
        return -1;
    }

    pkt = av_packet_alloc();
    av_init_packet(pkt);
    while (1) {
        ret = av_read_frame(ifmt_ctx, pkt);     // 不会去释放pkt的buf，如果我们外部不去释放，就会出现内存泄露
        if(ret < 0 ) {
            av_strerror(ret, errors, ERROR_STRING_SIZE);
            printf("av_read_frame failed:%s\n", errors);
            break;
        }
        // av_read_frame 成功读取到packet，则外部需要进行buf释放
        if(pkt->stream_index == video_index) {
            // 处理视频
            ret = av_bsf_send_packet(bsf_ctx, pkt); // 内部把我们传入的buf转移到自己bsf内部
            if(ret < 0) {       // 基本不会进入该逻辑
                av_strerror(ret, errors, ERROR_STRING_SIZE);
                printf("av_bsf_send_packet failed:%s\n", errors);
                av_packet_unref(pkt);
                continue;
            }
//            av_packet_unref(pkt); // 这里不需要去释放内存
            while (1) {
                ret = av_bsf_receive_packet(bsf_ctx, pkt);
                if(ret != 0) {
                    break;
                }
                size_t size = fwrite(pkt->data, 1, pkt->size, h264_fd);
                if(size != pkt->size)
                {
                    av_log(NULL, AV_LOG_DEBUG, "h264 warning, length of writed data isn't equal pkt->size(%d, %d)\n",
                           size,
                           pkt->size);
                }
                av_packet_unref(pkt);
            }
        } else if(pkt->stream_index == audio_index) {
            // 处理音频
            char adts_header_buf[7] = {0};
            adts_header(adts_header_buf, pkt->size,
                        ifmt_ctx->streams[audio_index]->codecpar->profile,
                        ifmt_ctx->streams[audio_index]->codecpar->sample_rate,
                        ifmt_ctx->streams[audio_index]->codecpar->channels);
            fwrite(adts_header_buf, 1, 7, aac_fd);  // 写adts header , ts流不适用，ts流分离出来的packet带了adts header
            size_t size = fwrite( pkt->data, 1, pkt->size, aac_fd);   // 写adts data
            if(size != pkt->size)
            {
                av_log(NULL, AV_LOG_DEBUG, "aac warning, length of writed data isn't equal pkt->size(%d, %d)\n",
                       size,
                       pkt->size);
            }
            av_packet_unref(pkt);
        } else {
            av_packet_unref(pkt);       // 释放buffer
        }
    }

    printf("while finish\n");
failed:
    if(h264_fd) {
        fclose(h264_fd);
    }
    if(aac_fd) {
        fclose(aac_fd);
    }
    if(pkt)
        av_packet_free(&pkt);
    if(ifmt_ctx)
        avformat_close_input(&ifmt_ctx);


    printf("Hello World!\n");
    return 0;
}
