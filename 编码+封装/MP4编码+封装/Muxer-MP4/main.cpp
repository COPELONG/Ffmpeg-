#include <iostream>

#include "audioencoder.h"
#include "audioresampler.h"
#include "videoencoder.h"
#include "muxer.h"
using namespace std;

#define YUV_WIDTH 720
#define YUV_HEIGHT 576
#define YUV_FPS  25

#define VIDEO_BIT_RATE 500*1024

#define PCM_SAMPLE_FORMAT AV_SAMPLE_FMT_S16
#define PCM_SAMPLE_RATE 44100
#define PCM_CHANNELS 2

#define AUDIO_BIT_RATE 128*1024

#define AUDIO_TIME_BASE 1000000
#define VIDEO_TIME_BASE 1000000
//ffmpeg -i sound_in_sync_test.mp4 -pix_fmt yuv420p 720x576_yuv420p.yuv
//ffmpeg -i sound_in_sync_test.mp4 -vn -ar 44100 -ac 2 -f s16le 44100_2_s16le.pcm
// 执行文件  yuv文件 pcm文件 输出mp4文件
int main(int argc, char **argv)
{
    if(argc != 4) {
        printf("usage -> exe in.yuv in.pcm out.mp4");
        return -1;
    }
    // 1. 打开yuv pcm文件
    char *in_yuv_name = argv[1];
    char *in_pcm_name = argv[2];
    char *out_mp4_name = argv[3];
    FILE *in_yuv_fd = NULL;
    FILE *in_pcm_fd = NULL;
    //1. 打开测试文件
    // 打开YUV文件
    in_yuv_fd = fopen(in_yuv_name, "rb");
    if( !in_yuv_fd )
    {
        printf("Failed to open %s file\n", in_yuv_name);
        return -1;
    }

    // 打开PCM文件
    in_pcm_fd = fopen(in_pcm_name, "rb");
    if( !in_pcm_fd )
    {
        printf("Failed to open %s file\n", in_pcm_name);
        return -1;
    }

    int ret = 0;
    // 2. 初始化编码器，包括视频、音频编码器, 分配yuv、pcm的帧buffer
    // 2.1 初始化video
    // 初始化编码器
    int yuv_width = YUV_WIDTH;
    int yuv_height = YUV_HEIGHT;
    int yuv_fps = YUV_FPS;
    int video_bit_rate = VIDEO_BIT_RATE;
    VideoEncoder video_encoder;
    ret = video_encoder.InitH264(yuv_width, yuv_height, yuv_fps, video_bit_rate);
    if(ret < 0)
    {
        printf("video_encoder.InitH264 failed\n");
        return -1;
    }
    // 分配yuv buf
    int y_frame_size = yuv_width * yuv_height;
    int u_frame_size = yuv_width * yuv_height /4;
    int v_frame_size = yuv_width * yuv_height /4;
    int yuv_frame_size = y_frame_size + u_frame_size + v_frame_size;
    uint8_t *yuv_frame_buf = (uint8_t *)malloc(yuv_frame_size);
    if(!yuv_frame_buf)
    {
        printf("malloc(yuv_frame_size)\n");
        return -1;
    }

    // 2.2 初始化audio
    // 初始化音频编码器
    int pcm_channels= PCM_CHANNELS;
    int pcm_sample_rate = PCM_SAMPLE_RATE;
    int pcm_sample_format = PCM_SAMPLE_FORMAT;
    int audio_bit_rate = AUDIO_BIT_RATE;
    AudioEncoder audio_encoder;
    ret = audio_encoder.InitAAC(pcm_channels, pcm_sample_rate, audio_bit_rate);
    if(ret < 0)
    {
        printf("audio_encoder.InitAAC failed\n");
        return -1;
    }
    // 分配pcm buf
    // pcm_frame_size  = 单个采样点占用的字节 * 通道数量 * 每个通道有多少给采用点
    int pcm_frame_size = av_get_bytes_per_sample((AVSampleFormat)pcm_sample_format)
            *pcm_channels * audio_encoder.GetFrameSize();
    if(pcm_frame_size <= 0) {
        printf("pcm_frame_size <= 0\n");
        return -1;
    }
    uint8_t *pcm_frame_buf = (uint8_t *)malloc(pcm_frame_size);
    if(!pcm_frame_buf)
    {
        printf("malloc(pcm_frame_size)\n");
        return -1;
    }

    // 初始化重采样
    AudioResampler audio_resampler;
    ret = audio_resampler.InitFromS16ToFLTP(pcm_channels, pcm_sample_rate,
                                      audio_encoder.GetChannels(), audio_encoder.GetSampleRate());
    if(ret < 0)
    {
        printf("audio_resampler.InitFromS16ToFLTP failed\n");
        return -1;
    }

    // 3. mp4初始化 包括新建流，open io, send header
    Muxer mp4_muxer;
    ret = mp4_muxer.Init(out_mp4_name);
    if(ret < 0)
    {
        printf("mp4_muxer.Init failed\n");
        return -1;
    }

    ret = mp4_muxer.AddStream(video_encoder.GetCodecContext());
    if(ret < 0)
    {
        printf("mp4_muxer.AddStream video failed\n");
        return -1;
    }

    ret = mp4_muxer.AddStream(audio_encoder.GetCodecContext());
    if(ret < 0)
    {
        printf("mp4_muxer.AddStream audio failed\n");
        return -1;
    }

    ret = mp4_muxer.Open();
    if(ret < 0)
    {
        printf("mp4_muxer.Open failed\n");
        return -1;
    }

    ret = mp4_muxer.SendHeader();
    if(ret < 0)
    {
        printf("mp4_muxer.SendHeader failed\n");
        return -1;
    }
    // 4. 在while循环读取yuv、pcm进行编码然后发送给MP4 muxer
    // 4.1 时间戳相关
    int64_t audio_time_base = AUDIO_TIME_BASE;
    int64_t video_time_base = VIDEO_TIME_BASE;
    double audio_pts = 0;
    double video_pts = 0;
    double audio_frame_duration = 1.0 * audio_encoder.GetFrameSize()/pcm_sample_rate*audio_time_base;
    double video_frame_duration = 1.0/yuv_fps * video_time_base;

    int audio_finish = 0;   // 两者都为0的时候才结束while循环
    int video_finish = 0;

    size_t read_len = 0;
    AVPacket *packet =  NULL;
    int audio_index = mp4_muxer.GetAudioStreamIndex();
    int video_index = mp4_muxer.GetVideoStreamIndex();
    while (1) {
        if(audio_finish && video_finish) {
            break;
        }
        printf("apts:%0.0lf vpts:%0.0lf\n", audio_pts/1000, video_pts/1000);
        if((video_finish != 1 && audio_pts > video_pts)   // audio和vidoe都还有数据，优先audio（audio_pts > video_pts）
              ||  (video_finish != 1 && audio_finish == 1)) {
            read_len = fread(yuv_frame_buf, 1, yuv_frame_size, in_yuv_fd);
            if(read_len < yuv_frame_size) {
                video_finish = 1;
                printf("fread yuv_frame_buf finish\n");
            }

            if(video_finish != 1) {
                packet = video_encoder.Encode(yuv_frame_buf, yuv_frame_size, video_index,
                                              video_pts, video_time_base);
            }else {
                packet = video_encoder.Encode(NULL, 0, video_index,
                                              video_pts, video_time_base);
            }
            video_pts += video_frame_duration;  // 叠加pts
            if(packet) {
                mp4_muxer.SendPacket(packet);
            }
        } else if(audio_finish != 1) {
            read_len = fread(pcm_frame_buf, 1, pcm_frame_size, in_pcm_fd);
            if(read_len < pcm_frame_size) {
                audio_finish = 1;
                printf("fread pcm_frame_buf finish\n");
            }

            if(audio_finish != 1) {
                AVFrame *fltp_frame = AllocFltpPcmFrame(pcm_channels, audio_encoder.GetFrameSize());
                ret = audio_resampler.ResampleFromS16ToFLTP(pcm_frame_buf, fltp_frame);
                if(ret < 0)
                    printf("ResampleFromS16ToFLTP error\n");
                packet = audio_encoder.Encode(fltp_frame, audio_index,
                                              audio_pts, audio_time_base);
                FreePcmFrame(fltp_frame);
            }else {
                packet = audio_encoder.Encode(NULL,video_index,
                                              audio_pts, audio_time_base);
            }
            audio_pts += audio_frame_duration;  // 叠加pts
            if(packet) {
                mp4_muxer.SendPacket(packet);
            }
        }
    }
    ret = mp4_muxer.SendTrailer();
    if(ret < 0)
    {
        printf("mp4_muxer.SendTrailer failed\n");
    }

    printf("write mp4 finish\n");

    if(yuv_frame_buf)
        free(yuv_frame_buf);
    if(pcm_frame_buf)
        free(pcm_frame_buf);
    if(in_yuv_fd)
        fclose(in_yuv_fd);
    if(in_pcm_fd)
        fclose(in_pcm_fd);

    return 0;
}





