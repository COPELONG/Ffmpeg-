/**
 * @example resampling_audio.c
 * libswresample API use example.
 */

#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include "audioresampler.h"

/**
* 腾讯课堂 零声学院 音视频高级教程
* Darren QQ326873713
* 该代码主要实现音频重采样：
* (1) 进行重采样;
* (2) 将重采样后的samples放入audio fifo;
* (3) 再从fifo读取指定的samples.
*/

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
    { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
    { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
    { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
    { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
    { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
};
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "Sample format %s not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return AVERROR(EINVAL);
}

/**
 * Fill dst buffer with nb_samples, generated starting from t.
 */
static void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t)
{
    int i, j;
    double tincr = 1.0 / sample_rate, *dstp = dst;
    const double c = 2 * M_PI * 440.0;

    /* generate sin tone with 440Hz frequency and duplicated channels */
    for (i = 0; i < nb_samples; i++) {
        *dstp = sin(c * *t);
        for (j = 1; j < nb_channels; j++)
            dstp[j] = dstp[0];
        dstp += nb_channels;
        *t += tincr;
    }
}

int main(int argc, char **argv)
{
    // 输入参数
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO;
    int src_rate = 48000;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_DBL;
    int src_nb_channels = 0;
    uint8_t **src_data = NULL;  // 二级指针
    int src_linesize;
    int src_nb_samples = 1024;


    // 输出参数
    int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;
    int dst_rate = 44100;
    enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;
    int dst_nb_channels = 0;
    uint8_t **dst_data = NULL;  //二级指针
    int dst_linesize;
    int dst_nb_samples = 1152; // 固定为1152

    // 输出文件
    const char *dst_filename = NULL;
    FILE *dst_file;
    int dst_bufsize;
    const char *fmt;

    // 重采样实例
    audio_resampler_params_t resampler_params;
    resampler_params.src_channel_layout = src_ch_layout;
    resampler_params.src_sample_fmt     = src_sample_fmt;
    resampler_params.src_sample_rate    = src_rate;

    resampler_params.dst_channel_layout = dst_ch_layout;
    resampler_params.dst_sample_fmt     = dst_sample_fmt;
    resampler_params.dst_sample_rate    = dst_rate;

    audio_resampler_t *resampler = audio_resampler_alloc(resampler_params);
    if(!resampler) {
        printf("audio_resampler_alloc failed\n");
        goto end;
    }

    double t;
    int ret;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s output_file\n"
                        "API example program to show how to resample an audio stream with libswresample.\n"
                        "This program generates a series of audio frames, resamples them to a specified "
                        "output format and rate and saves them to an output file named output_file.\n",
                argv[0]);
        exit(1);
    }
    dst_filename = argv[1];

    dst_file = fopen(dst_filename, "wb");
    if (!dst_file) {
        fprintf(stderr, "Could not open destination file %s\n", dst_filename);
        exit(1);
    }


    /* allocate source and destination samples buffers */
    // 计算出输入源的通道数量
    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    // 给输入源分配内存空间
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,
                                             src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        goto end;
    }


    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    dst_nb_samples = 1152;      // 指定每次获取1152的数据
    // 分配输出缓存内存
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        goto end;
    }

    t = 0;
    int64_t in_pts = 0;
    int64_t out_pts = 0;
    do {
        /* generate synthetic audio */
        // 生成输入源
        fill_samples((double *)src_data[0], src_nb_samples, src_nb_channels, src_rate, &t);

        int ret_size = audio_resampler_send_frame2(resampler, src_data, src_nb_samples, in_pts);
        in_pts += src_nb_samples;
        ret_size = audio_resampler_receive_frame2(resampler, dst_data, dst_nb_samples, &out_pts);

        if(ret_size > 0) {
            dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                     ret_size, dst_sample_fmt, 1);
            if (dst_bufsize < 0) {
                fprintf(stderr, "Could not get sample buffer size\n");
                goto end;
            }
            printf("t:%f in:%d out:%d, out_pts:%"PRId64"\n", t, src_nb_samples, ret_size, out_pts);
            fwrite(dst_data[0], 1, dst_bufsize, dst_file);
        } else {
            printf("can't get %d samples, ret_size:%d, cur_size:%d\n", dst_nb_samples, ret_size,
                   audio_resampler_get_fifo_size(resampler));
        }
    } while (t < 10);  // 调整t观察内存使用情况

    // flush  只有对数据完整度要求非常高的场景才需要做flush
    audio_resampler_send_frame2(resampler, NULL, 0, 0);
    int fifo_size = audio_resampler_get_fifo_size(resampler);
    int get_size = (fifo_size >dst_nb_samples ? dst_nb_samples : fifo_size);
    int ret_size = audio_resampler_receive_frame2(resampler, dst_data, get_size, &out_pts);
    if(ret_size > 0) {
        printf("flush ret_size:%d\n", ret_size);
        // 不够一帧的时候填充为静音, 这里的目的是补偿最后一帧如果不够采样点数不足1152，用静音数据进行补足
        av_samples_set_silence(dst_data, ret_size, dst_nb_samples - ret_size, dst_nb_channels, dst_sample_fmt);
        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                 dst_nb_samples, dst_sample_fmt, 1);
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            goto end;
        }
        printf("flush in:%d out:%d, out_pts:%"PRId64"\n", 0, ret_size, out_pts);
        fwrite(dst_data[0], 1, dst_bufsize, dst_file);
    }




    if ((ret = get_format_from_sample_fmt(&fmt, dst_sample_fmt)) < 0)
        goto end;
    fprintf(stderr, "Resampling succeeded. Play the output file with the command:\n"
                    "ffplay -f %s -channel_layout %"PRId64" -channels %d -ar %d %s\n",
            fmt, dst_ch_layout, dst_nb_channels, dst_rate, dst_filename);

end:
    fclose(dst_file);

    if (src_data)
        av_freep(&src_data[0]);
    av_freep(&src_data);

    if (dst_data)
        av_freep(&dst_data[0]);
    av_freep(&dst_data);

    if(resampler) {
        audio_resampler_free(resampler);
    }
    printf("finish\n");
    return ret < 0;
}
