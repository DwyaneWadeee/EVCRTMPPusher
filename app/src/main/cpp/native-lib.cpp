#include <jni.h>
#include <string>

#include <rtmp.h>

//extern "C"{
    #include <x264.h>
#include "VideoChannel.h"
#include "util.h"
#include "safe_queue.h"

//}

extern "C" JNIEXPORT jstring JNICALL
Java_com_chan_evcpusher_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    char version[30];
    sprintf(version,"librtmp version : %d",RTMP_LibVersion());

    x264_picture_t * picture = new x264_picture_t;

    return env->NewStringUTF(version);
}

VideoChannel *videoChannel = nullptr;
bool isStart =0;
pthread_t pid_start;
bool readyPushing;
SafeQueue<RTMPPacket *> packets; // 不区分音频和视频，（音频 & 视频）一直存储，  start直接拿出去发送给流媒体服务器（添加到队列中的是压缩包）
uint32_t start_time; // 记录时间搓，为了计算下，时间搓


// 释放RTMPPacket * 包的函数指针实现
// T无法释放， 让外界释放
void releasePackets(RTMPPacket **packet) {
    if (packet) {
        RTMPPacket_Free(*packet);
//        delete packet;
        packet = nullptr;
    }
}

// videoCallback 函数指针的实现（存放packet到队列）
void callback(RTMPPacket *packet) {
    if (packet) {
        if (packet->m_nTimeStamp == -1) {
            packet->m_nTimeStamp = RTMP_GetTime() - start_time; // 如果是sps+pps 没有时间搓，如果是I帧就需要有时间搓
        }
        packets.push(packet); // 存入队列里面  把刚刚的音频Packet丢进去
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_chan_evcpusher_EvcPush_native_1init(JNIEnv *env, jobject thiz) {
    videoChannel = new VideoChannel;

    // 存入队列的 关联
    videoChannel->setVideoCallback(callback);

    // 存入队列的 关联
//    audioChannel->setAudioCallback(callback);

    // 队列的释放工作 关联
    packets.setReleaseCallback(releasePackets);
}

void *task_start(void *args) {
//   char *url  = static_cast<char *>(args);
    char *url = "rtmp://sendtc3a.douyu.com/live/11075556rKZ8i7Vb?wsSecret=3c92696413b21b80b87558a8b4ccdefa&wsTime=62fef6da&wsSeek=off&wm=0&tw=0&roirecognition=0&record=flv&origin=tct";
//   RTMPDump API 九步曲
    RTMP *rtmp = nullptr;
    int ret;

    do {//使用do while方便break，方便重试
        // 1.1，rtmp 初始化
        rtmp = RTMP_Alloc();
        if (!rtmp){
            LOGE("rtmp初始化失败");
            break;
        }

        // 1.2，rtmp 初始化
        RTMP_Init(rtmp);
        rtmp->Link.timeout = 5; // 设置连接的超时时间（以秒为单位的连接超时）

        // 2，rtmp 设置流媒体地址
        ret = RTMP_SetupURL(rtmp, url);
        if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
            LOGE("rtmp 设置流媒体地址失败");
            break;
        }

        // 3，开启输出模式
        RTMP_EnableWrite(rtmp);

        // 4，建立连接
        ret = RTMP_Connect(rtmp, nullptr);
        if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
            LOGE("rtmp 建立连接失败:%d, url: %s", ret, url);
            break;
        }

        LOGE("rtmp建立连接成功");

        // 5，连接流
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
            LOGE("rtmp 连接流失败");
            break;
        }

        LOGE("rtmp连接流成功");


        start_time = RTMP_GetTime();

        // 准备好了，可以开始向服务器推流了
        readyPushing = true;

        //我从队列里获取包，直接发给服务器
        packets.setWork(1);        // 队列开始工作
        RTMPPacket * packet = nullptr;

        while (readyPushing){
            packets.pop(packet);

            if (!readyPushing) {
                break;
            }

            if (!packet) {
                continue;
            }

            // 给rtmp的流id
            packet->m_nInfoField2 = rtmp->m_stream_id;
            // 成功取出数据包，发送
            ret = RTMP_SendPacket(rtmp, packet, 1); // 1==true 开启内部缓冲

            // packet 你都发给服务器了，可以大胆释放
            releasePackets(&packet);

            if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
                LOGE("rtmp 失败 自动断开服务器");
                break;
            }
        }
        releasePackets(&packet);// 只要跳出循环，就释放
    } while (false);

    // 本次一系列释放工作
    isStart = false;
    readyPushing = false;
    packets.setWork(0);
    packets.clear();

    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete url;

    return nullptr;
}
/**
 * 开始直播
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_chan_evcpusher_EvcPush_native_1start(JNIEnv *env, jobject thiz, jstring path_) {

    //子线程
    // 1、连接流媒体服务器
    // 2、发包
    if (isStart) {
        return;
    }
    isStart = true;
    const char *path = env->GetStringUTFChars(path_, nullptr);

    // 深拷贝
    // 以前我们在做player播放器的时候，是用Flag来控制（isPlaying）
    char *url = new char(strlen(path) + 1); // C++的堆区开辟 new -- delete
    strcpy(url, path);

    //如果不做深拷贝，直接把path_丢进去，函数一执行完就会释放
    // 创建线程来进行直播
    pthread_create(&pid_start, nullptr, task_start, url);

    env->ReleaseStringUTFChars(path_, path); // 你随意释放，我已经深拷贝了
}

extern "C"
JNIEXPORT void JNICALL
Java_com_chan_evcpusher_EvcPush_native_1stop(JNIEnv *env, jobject thiz) {
}

extern "C"
JNIEXPORT void JNICALL
Java_com_chan_evcpusher_EvcPush_native_1release(JNIEnv *env, jobject thiz) {
}


/**
 * 往队列里面加入RTMPACKET （x264编码后的RTMPPkt）
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_chan_evcpusher_EvcPush_native_1pushVideo(JNIEnv *env, jobject thiz, jbyteArray data_) {
    // data == nv21数据  编码  加入队列
    if (!videoChannel || !readyPushing) { return; }

    // 把jni ---> C语言的
    jbyte *data = env->GetByteArrayElements(data_, nullptr);
    videoChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0); // 释放byte[]


}


/**
 * 初始化x264编码器
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_chan_evcpusher_EvcPush_native_1initVideoEncoder(JNIEnv *env, jobject thiz, jint width,
                                                         jint height, jint m_fps, jint bitrate) {

    if (videoChannel){
        videoChannel->initVideoEncoder(width,height,m_fps,bitrate);
    }
}

