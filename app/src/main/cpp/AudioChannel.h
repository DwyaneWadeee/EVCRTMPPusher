//
// Created by Evan on 2022/8/23.
//

#ifndef EVCPUSHER_AUDIOCHANNEL_H
#define EVCPUSHER_AUDIOCHANNEL_H


#include <cstdio>
#include <jni.h>
#include "librtmp/rtmp.h"
#include "libfaac/include/faac.h"
#include "util.h"

class AudioChannel {

public:
    AudioChannel();
    ~AudioChannel();

    typedef void (*AudioCallback)(RTMPPacket *packet);

    void setAudioCallback(void (*param)(RTMPPacket *));

    void initAudioEncoder(int i, int i1);

    int getInputSamples();

//    void encodeData(signed char* data);
    void encodeData(int8_t *data);
    RTMPPacket * getAudioSeqHeader();

private:
    u_long inputSamples; // faac输出的样本数
    u_long maxOutputBytes; // faac 编码器 最大能输出的字节数
    int mChannels; // 通道数
    faacEncHandle audioEncoder = 0; // 音频编码器
    u_char *buffer = 0; // 后面要用到的 缓冲区
    AudioCallback audioCallback;
};




#endif //EVCPUSHER_AUDIOCHANNEL_H
