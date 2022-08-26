package com.chan.evcpusher

import android.app.Activity
import android.hardware.Camera
import android.view.SurfaceHolder
import com.chan.evcpusher.util.CameraHelper

/**
 * 推流中转站
 */
class EvcPush(activity: Activity, cameraId: Int, width: Int, height: Int, fps: Int, bitrate: Int) {

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }
    var mActivity = activity
    var cameraId = cameraId
    var width = width
    var height = height
    var fps = fps
    var bitrate = bitrate

    lateinit var videoChannel:VideoChannel
    lateinit var audioChannel:AudioChannel
    lateinit var cameraHelper:CameraHelper

    init {
        cameraHelper = CameraHelper(activity, Camera.CameraInfo.CAMERA_FACING_FRONT, 640, 480)

        native_init()
        videoChannel = VideoChannel(
            this,
            mActivity,
            Camera.CameraInfo.CAMERA_FACING_FRONT,
            640,
            480,
            25,
            800000
        )

        audioChannel = AudioChannel(this)
    }


    fun setPreviewDisplay(holder: SurfaceHolder) {
        videoChannel.setPreviewDisplay(holder)
    }

    fun switchCamera() {
        videoChannel.switchCamera()
    }

    fun startLive(path:String) {
        native_start(path)
        videoChannel.startLive()
        audioChannel.startLive()
    }

    fun stopLive() {
        videoChannel.stopLive()
        audioChannel.stopLive()
        native_stop()
    }

    fun release() {
        videoChannel.release()
        audioChannel.release()
        native_release()
    }

    // 音频通道需要样本数（faac的编码器，输出样本 的样本数，才是标准）
    fun getInputSamples(): Int {
        return native_getInputSamples() // native层-->从faacEncOpen中获取到的样本数
    }


    external fun native_init()
    external fun native_start(path: String)
    external fun native_stop()
    external fun native_release()

    //视频↓
    external fun native_pushVideo(data: ByteArray)
    external fun native_initVideoEncoder(width: Int, height: Int, mFps: Int, bitrate: Int)
    //音频↓
    external fun native_initAudioEncoder(sampleRate: Int, numChannels: Int)
    external fun native_getInputSamples(): Int
    external fun native_pushAudio(bytes: ByteArray?)
}