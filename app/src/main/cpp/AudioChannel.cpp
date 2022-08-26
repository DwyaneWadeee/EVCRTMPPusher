//
// Created by Evan on 2022/8/23.
//

#include <cstring>
#include "AudioChannel.h"

AudioChannel::AudioChannel() {}

AudioChannel::~AudioChannel() {
    DELETE(buffer);
    if (audioEncoder){
        faacEncClose(audioEncoder);
        audioEncoder = nullptr;
    }
}
void AudioChannel::initAudioEncoder(int sample_rate, int num_channels) {
    this->mChannels = num_channels;//通道数量 2

    /**
     * 第一步 打开faac编码器
     */
    audioEncoder = faacEncOpen(sample_rate,num_channels,&inputSamples,&maxOutputBytes);
    if (!audioEncoder) {
        LOGE("打开音频编码器失败");
        return;
    }


     /**
      * 2、配置编码参数
      */
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioEncoder);
    config->mpegVersion = MPEG4; // mpeg4标准 acc音频标准
    config->aacObjectType = LOW; // LC标准： https://zhidao.baidu.com/question/1948794313899470708.html
    config->inputFormat = FAAC_INPUT_16BIT; // 16bit

    // 比特流输出格式为：Raw
    config->outputFormat = 0;

    // 1发送的时候，就消除 最好的，   2结束后消除回音（复杂）
    // 工作中：最麻烦的就是，（开启降噪, 噪声控制）
    config->useTns = 1;
    config->useLfe = 0;

    /**
     * 3、把上面的配置参数，传入进去给faac编码器，audioEncoder==faac编码器
     */
    /**
     * 第三步：把三面的配置参数，传入进去给faac编码器,  audioEncoder==faac编码器 真正的编码器，可以用的
     */
    int ret = faacEncSetConfiguration(audioEncoder, config);
    if (!ret) { // ret == 0 失败 和 x264 设计 一样
        LOGE("音频编码器参数配置失败");
        return;
    }

    LOGE("FAAC编码器初始化成功...");

    // 输出缓冲区定义
    buffer = new u_char(maxOutputBytes);
}

// 发送aac序列头
int AudioChannel::getInputSamples() {
    return inputSamples;
}

RTMPPacket * AudioChannel::getAudioSeqHeader(){
    u_char *ppBuffer;
    u_long len;

    // 获取编码器的解码配置信息
    faacEncGetDecoderSpecificInfo(audioEncoder, &ppBuffer, &len);

    //看图表，拼数据
    RTMPPacket *packet = new RTMPPacket;

    // int body_size = 2 + 2; // 后面的2：有16bit描述 头数据的长度，就是16bit
    int body_size = 2 + len;

    RTMPPacket_Alloc(packet, body_size);

    packet->m_body[0] = 0xAF; // 双声道®
    if (mChannels == 1) {
        packet->m_body[0] = 0xAE; // 单声道
    }
    // 序列/头参数==0
    packet->m_body[1] = 0x00;

    // 序列头数据 Copy橙图的 解码数据
    memcpy(&packet->m_body[2], ppBuffer, 2); // 16bit == 2个字节 可以写死

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO; // 包类型，音频
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 11; // 通道ID，随便写一个，注意：不要写的和rtmp.c(里面的m_nChannel有冲突 4301行)
    packet->m_nTimeStamp = 0; // 头一般都是没有时间搓的
    packet->m_hasAbsTimestamp = 0; // 一般都不用
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE; // 大包的类型，如果是头信息，可以给一个小包

}

//signed char 有符号（在所有音视频里面，最好用 无符号  uint8_t）
void AudioChannel::encodeData(int8_t *data) {
    int byteLen = faacEncEncode(audioEncoder, reinterpret_cast<int32_t *>(data), inputSamples, buffer, maxOutputBytes);
    if (byteLen >0){
        RTMPPacket *packet = new RTMPPacket;

        // AF == AAC编码器，44100采样率，位深16bit，双声道
        // AE == AAC编码器，44100采样率，位深16bit，单声道
        int body_size = 2 + byteLen; // 后面的byteLen：我们实际数据编码后的长度

        RTMPPacket_Alloc(packet, body_size); // 堆区实例化里面的成员 packet

        packet->m_body[0] = 0xAF; // 双声道
        if (mChannels == 1) {
            packet->m_body[0] = 0xAE; // 单声道
        }

        // 这里是编码出来的音频数据，所以都是 01，  非序列/非头参数
        packet->m_body[1] = 0x01;

        // 音频数据 橙色Copy进去
        memcpy(&packet->m_body[2], buffer, byteLen);

        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO; // 包类型，音频
        packet->m_nBodySize = body_size;
        packet->m_nChannel = 11; // 通道ID，随便写一个，注意：不要写的和rtmp.c(里面的m_nChannel有冲突 4301行)
        packet->m_nTimeStamp = -1; // 帧数据有时间戳
        packet->m_hasAbsTimestamp = 0; // 一般都不用
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE; // 大包的类型，如果是头信息，可以给一个小包

        // 把数据包放入队列
        audioCallback(packet);
    }
}

void AudioChannel::setAudioCallback(AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}

