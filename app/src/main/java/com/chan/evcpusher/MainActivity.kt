package com.chan.evcpusher

import android.hardware.Camera
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.chan.evcpusher.databinding.ActivityMainBinding
import com.chan.evcpusher.util.CameraHelper

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var pusher // 中转站（C++层打交道） 视频 和 音频
            : EvcPush? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        var cameraHelper = CameraHelper(this, Camera.CameraInfo.CAMERA_FACING_FRONT, 640, 480)

        pusher = EvcPush(this, Camera.CameraInfo.CAMERA_FACING_FRONT, 640, 480,25,800000)
        pusher?.setPreviewDisplay(binding.surfaceView.holder)

    }


    companion object {
//        init {
//            System.loadLibrary("native-lib")
//        }
    }

    external fun stringFromJNI(): String


    fun switchCamera(view: View) {
        pusher?.switchCamera()
    }


    fun startLive(view: View) {
        var baseurl = "rtmp://sendtc3a.douyu.com/live/"
//        var sub = "11075556rEF1ck0Z?wsSecret=6cf1d6b7b19e2931c301b2640fec77ec&wsTime=62fde4da&wsSeek=off&wm=0&tw=0&roirecognition=0&record=flv&origin=tct"
        var sub = "11075556rcD1wLHt?wsSecret=eb626ef666f080379705e28de60833ff&wsTime=62fded48&wsSeek=off&wm=0&tw=0&roirecognition=0&record=flv&origin=tct"
        pusher?.startLive(baseurl+sub)
    }

    fun stopLive(view: View) {
        pusher?.stopLive()

    }

    override fun onDestroy() {
        super.onDestroy()
        pusher?.release()
    }

}