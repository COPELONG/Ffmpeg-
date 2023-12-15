#ifndef FLVPARSER_H
#define FLVPARSER_H

#include <iostream>
#include <vector>
#include <stdint.h>
#include "Videojj.h"

using namespace std;

typedef unsigned long long uint64_t;

class CFlvParser
{
public:
    CFlvParser();
    virtual ~CFlvParser();

    int Parse(uint8_t *pBuf, int nBufSize, int &nUsedLen);
    int PrintInfo();
    int DumpH264(const std::string &path);
    int DumpAAC(const std::string &path);
    int DumpFlv(const std::string &path);

private:
    // FLV头
    typedef struct FlvHeader_s
    {
        int nVersion; // 版本
        int bHaveVideo; // 是否包含视频
        int bHaveAudio; // 是否包含音频
        int nHeadSize;  // FLV头部长度
        /*
        ** 指向存放FLV头部的buffer
        ** 上面的三个成员指明了FLV头部的信息，是从FLV的头部中“翻译”得到的，
        ** 真实的FLV头部是一个二进制比特串，放在一个buffer中，由pFlvHeader成员指明
        */
        uint8_t *pFlvHeader;
    } FlvHeader;

    // Tag头部
    struct TagHeader
    {
        int nType;      // 类型
        int nDataSize;  // 标签body的大小
        int nTimeStamp; // 时间戳
        int nTSEx;      // 时间戳的扩展字节
        int nStreamID;  // 流的ID，总是0

        uint32_t nTotalTS;  // 完整的时间戳nTimeStamp和nTSEx拼装

        TagHeader() : nType(0), nDataSize(0), nTimeStamp(0), nTSEx(0), nStreamID(0), nTotalTS(0) {}
        ~TagHeader() {}
    };

    class Tag
    {
    public:
        Tag() : _pTagHeader(NULL), _pTagData(NULL), _pMedia(NULL), _nMediaLen(0) {}
        void Init(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen);

        TagHeader _header;
        uint8_t *_pTagHeader;   // 指向标签头部
        uint8_t *_pTagData;     // 指向标签body，原始的tag data数据
        uint8_t *_pMedia;       // 指向标签的元数据，改造后的数据
        int _nMediaLen;         // 数据长度
    };

    class CVideoTag : public Tag
    {
    public:
        /**
         * @brief CVideoTag
         * @param pHeader
         * @param pBuf 整个tag的起始地址
         * @param nLeftLen
         * @param pParser
         */
        CVideoTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        int _nFrameType;    // 帧类型
        int _nCodecID;      // 视频编解码类型
        int ParseH264Tag(CFlvParser *pParser);
        int ParseH264Configuration(CFlvParser *pParser, uint8_t *pTagData);
        int ParseNalu(CFlvParser *pParser, uint8_t *pTagData);
    };

    class CAudioTag : public Tag
    {
    public:
        CAudioTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        int _nSoundFormat;  // 音频编码类型
        int _nSoundRate;    // 采样率
        int _nSoundSize;    // 精度
        int _nSoundType;    // 类型

        // aac
        static int _aacProfile;     // 对应AAC profile
        static int _sampleRateIndex;    // 采样率索引
        static int _channelConfig;      // 通道设置

        int ParseAACTag(CFlvParser *pParser);
        int ParseAudioSpecificConfig(CFlvParser *pParser, uint8_t *pTagData);
        int ParseRawAAC(CFlvParser *pParser, uint8_t *pTagData);
    };

    class CMetaDataTag : public Tag
    {
    public:
        CMetaDataTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        double hexStr2double(const unsigned char* hex, const unsigned int length);
        int parseMeta(CFlvParser *pParser);
        void printMeta();

        uint8_t m_amf1_type;
        uint32_t m_amf1_size;
        uint8_t m_amf2_type;
        unsigned char *m_meta;
        unsigned int m_length;

        double m_duration;
        double m_width;
        double m_height;
        double m_videodatarate;
        double m_framerate;
        double m_videocodecid;

        double m_audiodatarate;
        double m_audiosamplerate;
        double m_audiosamplesize;
        bool m_stereo;
        double m_audiocodecid;

        string m_major_brand;
        string m_minor_version;
        string m_compatible_brands;
        string m_encoder;

        double m_filesize;
    };


    struct FlvStat
    {
        int nMetaNum, nVideoNum, nAudioNum;
        int nMaxTimeStamp;
        int nLengthSize;

        FlvStat() : nMetaNum(0), nVideoNum(0), nAudioNum(0), nMaxTimeStamp(0), nLengthSize(0){}
        ~FlvStat() {}
    };



    static uint32_t ShowU32(uint8_t *pBuf) { return (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3]; }
    static uint32_t ShowU24(uint8_t *pBuf) { return (pBuf[0] << 16) | (pBuf[1] << 8) | (pBuf[2]); }
    static uint32_t ShowU16(uint8_t *pBuf) { return (pBuf[0] << 8) | (pBuf[1]); }
    static uint32_t ShowU8(uint8_t *pBuf) { return (pBuf[0]); }
    static void WriteU64(uint64_t & x, int length, int value)
    {
        uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - length);
        x = (x << length) | ((uint64_t)value & mask);
    }
    static uint32_t WriteU32(uint32_t n)
    {
        uint32_t nn = 0;
        uint8_t *p = (uint8_t *)&n;
        uint8_t *pp = (uint8_t *)&nn;
        pp[0] = p[3];
        pp[1] = p[2];
        pp[2] = p[1];
        pp[3] = p[0];
        return nn;
    }

    friend class Tag;

private:

    FlvHeader *CreateFlvHeader(uint8_t *pBuf);
    int DestroyFlvHeader(FlvHeader *pHeader);
    Tag *CreateTag(uint8_t *pBuf, int nLeftLen);
    int DestroyTag(Tag *pTag);
    int Stat();
    int StatVideo(Tag *pTag);
    int IsUserDataTag(Tag *pTag);

private:

    FlvHeader* _pFlvHeader;
    vector<Tag *> _vpTag;
    FlvStat _sStat;
    CVideojj *_vjj;

    // H.264
    int _nNalUnitLength;

};

#endif // FLVPARSER_H
