#ifndef FLVMETADATA_H
#define FLVMETADATA_H

#include <cstdint>

class FlvMetaData
{
    public:
    FlvMetaData(uint8_t *meta, uint32_t length);
    ~FlvMetaData();

    FlvMetaData(const FlvMetaData&);
    FlvMetaData& operator=(const FlvMetaData&);

    double getDuration();
    double getWidth();
    double getHeight();
    double getFramerate();
    double getVideoDatarate();
    double getAudioDatarate();
    double getVideoCodecId();
    double getAudioCodecId();
    double getAudioSamplerate();
    double getAudioSamplesize();
    bool getStereo();

private:
    //convert HEX to double
    double hexStr2double(const uint8_t* hex, const uint32_t length);
    void parseMeta();

private:
    uint8_t *m_meta;
    uint32_t m_length;

    double m_duration;
    double m_width;
    double m_height;
    double m_framerate;
    double m_videodatarate;
    double m_audiodatarate;
    double m_videocodecid;
    double m_audiocodecid;
    double m_audiosamplerate;
    double m_audiosamplesize;

    bool m_stereo;
};

#endif // FLVMETADATA_H
