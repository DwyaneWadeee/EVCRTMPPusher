package com.chan.evcpusher

import android.app.Activity
import android.hardware.Camera
import android.view.SurfaceHolder
import com.chan.evcpusher.util.CameraHelper

class VideoChannel(
    evcPush: EvcPush,
    mActivity: Activity,
    cameraId: Int,
    width: Int,
    height: Int,
    fps: Int,
    bitrate: Int
) : Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {
    var mPusher = evcPush
    var mFps = fps
    var bitrate = bitrate


    private var isLive  = false// 是否直播：非常重要的标记，开始直播就是true，停止直播就是false，通过此标记控制是否发送数据给C++层

    var cameraHelper= CameraHelper(mActivity,cameraId,width,height)

    init {
//        var cameraHelper
        cameraHelper.setPreviewCallback(this)
        cameraHelper.setOnChangedSizeListener(this)
    }


    // 调用帮助类-->切换摄像头
    fun switchCamera() {
        cameraHelper.switchCamera()
    }

    // 调用帮助类：与Surface绑定 == surfaceView.getHolder()
    fun setPreviewDisplay(holder: SurfaceHolder?) {
        cameraHelper.setPreviewDisplay(holder!!)
    }


    fun startLive() {
        isLive = true
    }

    fun stopLive() {
        isLive = false
    }

    override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
        if (isLive&&data!=null) {
            // 图像数据推送
            mPusher.native_pushVideo(data)
        }
    }

    override fun onChanged(width: Int, height: Int) {
        // 视频编码器的初始化有关：width，height，fps，bitrate
        mPusher.native_initVideoEncoder(width, height, mFps, bitrate) // 初始化x264编码器
    }

    // 调用帮助类-->停止预览
    fun release() {
        cameraHelper.stopPreview()
    }
}