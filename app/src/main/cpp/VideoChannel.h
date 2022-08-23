//
// Created by Evan on 2022/8/17.
//

#ifndef EVCPUSHER_VIDEOCHANNEL_H
#define EVCPUSHER_VIDEOCHANNEL_H

#include <pthread.h>
#include <x264.h>

class VideoChannel {
public:
    void initVideoEncoder(int width, int height, int fps, int bitrate);
    VideoChannel();
    ~VideoChannel();
    typedef void (*VideoCallback)(RTMPPacket *packet);

    void encodeData(signed char *string);

    void sendSpsPps(uint8_t sps[100], uint8_t pps[100], int len, int len1);
    void sendFrame(int type, int payload, uint8_t *payload1);

    void setVideoCallback(void (*param)(RTMPPacket *));

private:
    pthread_mutex_t mutex;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    int y_len; // Y分量的长度
    int uv_len; // uv分量的长度
    x264_t *videoEncoder = 0; // x264编码器
    x264_picture_t *pic_in = 0; // 先理解是每一张图片 pic
    VideoCallback videoCallback;
};



#endif //EVCPUSHER_VIDEOCHANNEL_H
