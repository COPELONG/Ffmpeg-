/**
* @projectName   08-01-encode_audio
* @brief         音频编码
*               从本地读取PCM数据进行AAC编码
*           1. 输入PCM格式问题，通过AVCodec的sample_fmts参数获取具体的格式支持
*           （1）默认的aac编码器输入的PCM格式为:AV_SAMPLE_FMT_FLTP
*           （2）libfdk_aac编码器输入的PCM格式为AV_SAMPLE_FMT_S16.
*           2. 支持的采样率，通过AVCodec的supported_samplerates可以获取
* @author        Liao Qingfu
* @date          2020-04-15
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>

/* 检测该编码器是否支持该采样格式 */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) { // 通过AV_SAMPLE_FMT_NONE作为结束符
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* 检测该编码器是否支持该采样率 */
static int check_sample_rate(const AVCodec *codec, const int sample_rate)
{
    const int *p = codec->supported_samplerates;
    while (*p != 0)  {// 0作为退出条件，比如libfdk-aacenc.c的aac_sample_rates
        printf("%s support %dhz\n", codec->name, *p);
        if (*p == sample_rate)
            return 1;
        p++;
    }
    return 0;
}

/* 检测该编码器是否支持该采样率, 该函数只是作参考 */
static int check_channel_layout(const AVCodec *codec, const uint64_t channel_layout)
{
    // 不是每个codec都给出支持的channel_layout
    const uint64_t *p = codec->channel_layouts;
    if(!p) {
        printf("the codec %s no set channel_layouts\n", codec->name);
        return 1;
    }
    while (*p != 0) { // 0作为退出条件，比如libfdk-aacenc.c的aac_channel_layout
        printf("%s support channel_layout %d\n", codec->name, *p);
        if (*p == channel_layout)
            return 1;
        p++;
    }
    return 0;
}


static void get_adts_header(AVCodecContext *ctx, uint8_t *adts_header, int aac_length)
{
    uint8_t freq_idx = 0;    //0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    switch (ctx->sample_rate) {
        case 96000: freq_idx = 0; break;
        case 88200: freq_idx = 1; break;
        case 64000: freq_idx = 2; break;
        case 48000: freq_idx = 3; break;
        case 44100: freq_idx = 4; break;
        case 32000: freq_idx = 5; break;
        case 24000: freq_idx = 6; break;
        case 22050: freq_idx = 7; break;
        case 16000: freq_idx = 8; break;
        case 12000: freq_idx = 9; break;
        case 11025: freq_idx = 10; break;
        case 8000: freq_idx = 11; break;
        case 7350: freq_idx = 12; break;
        default: freq_idx = 4; break;
    }
    uint8_t chanCfg = ctx->channels;
    uint32_t frame_length = aac_length + 7;
    adts_header[0] = 0xFF;
    adts_header[1] = 0xF1;
    adts_header[2] = ((ctx->profile) << 6) + (freq_idx << 2) + (chanCfg >> 2);
    adts_header[3] = (((chanCfg & 3) << 6) + (frame_length  >> 11));
    adts_header[4] = ((frame_length & 0x7FF) >> 3);
    adts_header[5] = (((frame_length & 7) << 5) + 0x1F);
    adts_header[6] = 0xFC;
}
/*
*
*/
static int encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output)
{
    int ret;

    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
        return -1;
    }

    /* read all the available output packets (in general there may be any number of them */
    // 编码和解码都是一样的，都是send 1次，然后receive多次, 直到AVERROR(EAGAIN)或者AVERROR_EOF
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            return -1;
        }

        size_t len = 0;
        printf("ctx->flags:0x%x & AV_CODEC_FLAG_GLOBAL_HEADER:0x%x, name:%s\n",ctx->flags, ctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER, ctx->codec->name);
        if((ctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER)) {
            // 需要额外的adts header写入
            uint8_t aac_header[7];
            get_adts_header(ctx, aac_header, pkt->size);
            len = fwrite(aac_header, 1, 7, output);
            if(len != 7) {
                fprintf(stderr, "fwrite aac_header failed\n");
                return -1;
            }
        }
        len = fwrite(pkt->data, 1, pkt->size, output);
        if(len != pkt->size) {
            fprintf(stderr, "fwrite aac data failed\n");
            return -1;
        }
        /* 是否需要释放数据? avcodec_receive_packet第一个调用的就是 av_packet_unref
        * 所以我们不用手动去释放，这里有个问题，不能将pkt直接插入到队列，因为编码器会释放数据
        * 可以新分配一个pkt, 然后使用av_packet_move_ref转移pkt对应的buffer
        */
        // av_packet_unref(pkt);
    }
    return -1;
}

/*
 * 这里只支持2通道的转换
*/
void f32le_convert_to_fltp(float *f32le, float *fltp, int nb_samples) {
    float *fltp_l = fltp;   // 左通道
    float *fltp_r = fltp + nb_samples;   // 右通道
    for(int i = 0; i < nb_samples; i++) {
        fltp_l[i] = f32le[i*2];     // 0 1   - 2 3
        fltp_r[i] = f32le[i*2+1];   // 可以尝试注释左声道或者右声道听听声音
    }
}
/*
 * 提取测试文件：
 * （1）s16格式：ffmpeg -i buweishui.aac -ar 48000 -ac 2 -f s16le 48000_2_s16le.pcm
 * （2）flt格式：ffmpeg -i buweishui.aac -ar 48000 -ac 2 -f f32le 48000_2_f32le.pcm
 *      ffmpeg只能提取packed格式的PCM数据，在编码时候如果输入要为fltp则需要进行转换
 * 测试范例:
 * （1）48000_2_s16le.pcm libfdk_aac.aac libfdk_aac  // 如果编译的时候没有支持fdk aac则提示找不到编码器
 * （2）48000_2_f32le.pcm aac.aac aac // 我们这里只测试aac编码器，不测试fdkaac
*/
int main(int argc, char **argv)
{
    char *in_pcm_file = NULL;
    char *out_aac_file = NULL;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx= NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    int ret = 0;
    int force_codec = 0;     // 强制使用指定的编码
    char *codec_name = NULL;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file out_file [codec_name]>, argc:%d\n",
                argv[0], argc);
        return 0;
    }
    in_pcm_file = argv[1];      // 输入PCM文件
    out_aac_file = argv[2];     // 输出的AAC文件

    enum AVCodecID codec_id = AV_CODEC_ID_AAC;

    if(4 == argc) {
        if(strcmp(argv[3], "libfdk_aac") == 0) {
            force_codec = 1;     // 强制使用 libfdk_aac
            codec_name = "libfdk_aac";
        } else if(strcmp(argv[3], "aac") == 0) {
            force_codec = 1;
            codec_name = "aac";
        }
    }
    if(force_codec)
        printf("force codec name: %s\n", codec_name);
    else
        printf("default codec name: %s\n", "aac");

    if(force_codec == 0) { // 没有强制设置编码器
        codec = avcodec_find_encoder(codec_id); // 按ID查找则缺省的aac encode为aacenc.c
    } else {
        // 按名字查找指定的encode,对应AVCodec的name字段
        codec = avcodec_find_encoder_by_name(codec_name);
    }
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }
    codec_ctx->codec_id = codec_id;
    codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    codec_ctx->bit_rate = 128*1024;
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    codec_ctx->sample_rate    = 48000; //48000;
    codec_ctx->channels       = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
    codec_ctx->profile = FF_PROFILE_AAC_LOW;    //

    if(strcmp(codec->name, "aac") == 0) {
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    } else if(strcmp(codec->name, "libfdk_aac") == 0) {
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    } else {
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    }

    /* 检测支持采样格式支持情况 */
    if (!check_sample_fmt(codec, codec_ctx->sample_fmt)) {
        fprintf(stderr, "Encoder does not support sample format %s",
                av_get_sample_fmt_name(codec_ctx->sample_fmt));
        exit(1);
    }
    if (!check_sample_rate(codec, codec_ctx->sample_rate)) {
        fprintf(stderr, "Encoder does not support sample rate %d", codec_ctx->sample_rate);
        exit(1);
    }
    if (!check_channel_layout(codec, codec_ctx->channel_layout)) {
        fprintf(stderr, "Encoder does not support channel layout %lu", codec_ctx->channel_layout);
        exit(1);
    }

    printf("\n\nAudio encode config\n");
    printf("bit_rate:%ldkbps\n", codec_ctx->bit_rate/1024);
    printf("sample_rate:%d\n", codec_ctx->sample_rate);
    printf("sample_fmt:%s\n", av_get_sample_fmt_name(codec_ctx->sample_fmt));
    printf("channels:%d\n", codec_ctx->channels);
    // frame_size是在avcodec_open2后进行关联
    printf("1 frame_size:%d\n", codec_ctx->frame_size);
    codec_ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;  //ffmpeg默认的aac是不带adts，而fdk_aac默认带adts，这里我们强制不带
    /* 将编码器上下文和编码器进行关联 */
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    printf("2 frame_size:%d\n\n", codec_ctx->frame_size); // 决定每次到底送多少个采样点
    // 打开输入和输出文件
    infile = fopen(in_pcm_file, "rb");
    if (!infile) {
        fprintf(stderr, "Could not open %s\n", in_pcm_file);
        exit(1);
    }
    outfile = fopen(out_aac_file, "wb");
    if (!outfile) {
        fprintf(stderr, "Could not open %s\n", out_aac_file);
        exit(1);
    }

    /* packet for holding encoded output */
    pkt = av_packet_alloc();
    if (!pkt)
    {
        fprintf(stderr, "could not allocate the packet\n");
        exit(1);
    }

    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }
    /* 每次送多少数据给编码器由：
     *  (1)frame_size(每帧单个通道的采样点数);
     *  (2)sample_fmt(采样点格式);
     *  (3)channel_layout(通道布局情况);
     * 3要素决定
     */
    frame->nb_samples     = codec_ctx->frame_size;
    frame->format         = codec_ctx->sample_fmt;
    frame->channel_layout = codec_ctx->channel_layout;
    frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
    printf("frame nb_samples:%d\n", frame->nb_samples);
    printf("frame sample_fmt:%d\n", frame->format);
    printf("frame channel_layout:%lu\n\n", frame->channel_layout);
    /* 为frame分配buffer */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }
    // 计算出每一帧的数据 单个采样点的字节 * 通道数目 * 每帧采样点数量
    int frame_bytes = av_get_bytes_per_sample(frame->format) \
            * frame->channels \
            * frame->nb_samples;
    printf("frame_bytes %d\n", frame_bytes);
    uint8_t *pcm_buf = (uint8_t *)malloc(frame_bytes);
    if(!pcm_buf) {
        printf("pcm_buf malloc failed\n");
        return 1;
    }
    uint8_t *pcm_temp_buf = (uint8_t *)malloc(frame_bytes);
    if(!pcm_temp_buf) {
        printf("pcm_temp_buf malloc failed\n");
        return 1;
    }
    int64_t pts = 0;
    printf("start enode\n");
    for (;;) {
        memset(pcm_buf, 0, frame_bytes);
        size_t read_bytes = fread(pcm_buf, 1, frame_bytes, infile);
        if(read_bytes <= 0) {
            printf("read file finish\n");
            break;
//            fseek(infile,0,SEEK_SET);
//            fflush(outfile);
//            continue;
        }

        /* 确保该frame可写, 如果编码器内部保持了内存参考计数，则需要重新拷贝一个备份
            目的是新写入的数据和编码器保存的数据不能产生冲突
        */
        ret = av_frame_make_writable(frame);
        if(ret != 0)
            printf("av_frame_make_writable failed, ret = %d\n", ret);

        if(AV_SAMPLE_FMT_S16 == frame->format) {
            // 将读取到的PCM数据填充到frame去，但要注意格式的匹配, 是planar还是packed都要区分清楚
            ret = av_samples_fill_arrays(frame->data, frame->linesize,
                                   pcm_buf, frame->channels,
                                   frame->nb_samples, frame->format, 0);
        } else {
            // 将读取到的PCM数据填充到frame去，但要注意格式的匹配, 是planar还是packed都要区分清楚
            // 将本地的f32le packed模式的数据转为float palanar
            memset(pcm_temp_buf, 0, frame_bytes);
            f32le_convert_to_fltp((float *)pcm_buf, (float *)pcm_temp_buf, frame->nb_samples);
            ret = av_samples_fill_arrays(frame->data, frame->linesize,
                                   pcm_temp_buf, frame->channels,
                                   frame->nb_samples, frame->format, 0);
        }

        // 设置pts
        pts += frame->nb_samples;
        frame->pts = pts;       // 使用采样率作为pts的单位，具体换算成秒 pts*1/采样率
        ret = encode(codec_ctx, frame, pkt, outfile);
        if(ret < 0) {
            printf("encode failed\n");
            break;
        }
    }

    /* 冲刷编码器 */
    encode(codec_ctx, NULL, pkt, outfile);

    // 关闭文件
    fclose(infile);
    fclose(outfile);

    // 释放内存
    if(pcm_buf) {
        free(pcm_buf);
    }
    if (pcm_temp_buf) {
        free(pcm_temp_buf);
    }
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    printf("main finish, please enter Enter and exit\n");
    getchar();
    return 0;
}

