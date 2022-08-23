package com.chan.evcpusher.util

import android.app.Activity
import android.graphics.ImageFormat
import android.hardware.Camera
import android.hardware.Camera.PreviewCallback
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder

class CameraHelper(activity: Activity, cameraId: Int, width: Int, height: Int) :
    Camera.PreviewCallback, SurfaceHolder.Callback {
    var mActivity = activity
    var mCameraId = cameraId
    var mWidth = width
    var mHeight = height

    private var mCamera // Camera1 预览采集图像数据
            : Camera? = null
    private var mRotation // 旋转画面相关的标识
            = 0
    private lateinit var buffer // 数据
            : ByteArray
    private var mPreviewCallback // 后面预览的画面，把此预览的画面 的数据回调出现 --->DerryPush ---> C++层
            : PreviewCallback? = null
    private var mSurfaceHolder // Surface画面的帮助
            : SurfaceHolder? = null
    private var mOnChangedSizeListener // 你的宽和高发生改变，就会回调此接口
            : OnChangedSizeListener? = null

    private val TAG = "CameraHelper"



    private fun startPreview(){
        try {
            // 获得camera对象
            mCamera = Camera.open(mCameraId)
            //配置参数
            var parameters = mCamera?.parameters
            // 设置预览数据格式为nv21
            parameters?.previewFormat = ImageFormat.NV21
            // 这是摄像头宽、高
            if (parameters != null) {
                setPreviewSize(parameters)
            }
            // 设置摄像头 图像传感器的角度、方向
            setPreviewOrientation(parameters!!)
            mCamera?.setParameters(parameters)
            buffer = ByteArray(mWidth * mHeight * 3 / 2) // 上面的细节
            // 数据缓存区
            mCamera?.addCallbackBuffer(buffer)
            mCamera?.setPreviewCallbackWithBuffer(this)
            // 设置预览画面
            mCamera?.setPreviewDisplay(mSurfaceHolder) // SurfaceView 和 Camera绑定
            if (mOnChangedSizeListener != null) { // 你的宽和高发生改变，就会回调此接口
                mOnChangedSizeListener?.onChanged(mWidth, mHeight)
            }
            // 开启预览
            mCamera?.startPreview()
        }catch (e:Exception){
            e.printStackTrace()
        }
    }

    /**
     * 停止预览
     */
    fun stopPreview() {
        if (mCamera != null) {
            // 预览数据回调接口
            mCamera!!.setPreviewCallback(null)
            // 停止预览
            mCamera!!.stopPreview()
            // 释放摄像头
            mCamera!!.release()
            mCamera = null
        }
    }

    fun switchCamera() {
        mCameraId = if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            Camera.CameraInfo.CAMERA_FACING_FRONT
        } else {
            Camera.CameraInfo.CAMERA_FACING_BACK
        }
        stopPreview() // 先停止预览
        startPreview() // 在开启预览
    }


    private fun setPreviewOrientation(parameters:Camera.Parameters){
        var info = Camera.CameraInfo()
        Camera.getCameraInfo(mCameraId,info)
        mRotation = mActivity.windowManager.defaultDisplay.rotation
        var degrees = 0
        when (mRotation) {
            Surface.ROTATION_0 -> degrees = 0
            Surface.ROTATION_90 -> degrees = 90
            Surface.ROTATION_180 -> degrees = 180
            Surface.ROTATION_270 -> degrees = 270
        }
        var result =0
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360
            result = (360 - result) % 360 // compensate the mirror
        } else { // back-facing
            result = (info.orientation - degrees + 360) % 360
        }
        // 设置角度, 参考源码注释，从源码里面copy出来的，Google给出旋转的解释
        mCamera!!.setDisplayOrientation(result)
    }

    private fun setPreviewSize(parameters:Camera.Parameters){
        //获取支持的宽高
        var supportedPreviewSizes = parameters.supportedPreviewSizes
        var size = supportedPreviewSizes[0]
        Log.d(TAG, "Camera支持: " + size.width + "x" + size.height)

        // 选择一个与设置的差距最小的支持分辨率
        // 选择一个与设置的差距最小的支持分辨率
        var m = Math.abs(size.height * size.width - mWidth * mHeight)
        supportedPreviewSizes.removeAt(0)
        var iterator: Iterator<Camera.Size> = supportedPreviewSizes.iterator()

        //遍历
        for (item in supportedPreviewSizes){
            val n: Int = Math.abs(item.height * item.width - mWidth * mHeight)
            if (n < m) {
                m = n
                size = item
            }
        }
        mWidth = size.width
        mHeight = size.height
        parameters.setPreviewSize(mWidth, mHeight)
        Log.d(TAG, "预览分辨率 width:" + size.width + " height:" + size.height)

    }

    override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
        if (mPreviewCallback!=null){
            mPreviewCallback!!.onPreviewFrame(
                data,
                camera
            ) // byte[] data == nv21 ===> C++层 ---> 流媒体服务器

            camera?.addCallbackBuffer(buffer)
        }
    }

    fun setPreviewCallback(previewCallback: PreviewCallback) {
        mPreviewCallback = previewCallback
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        mSurfaceHolder = surfaceHolder
        mSurfaceHolder?.addCallback(this)
    }

    override fun surfaceCreated(p0: SurfaceHolder) {

    }

    override fun surfaceChanged(p0: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
        // 释放摄像头
        stopPreview()
        // 开启摄像头
        startPreview()
    }

    override fun surfaceDestroyed(p0: SurfaceHolder) {
        stopPreview() // 只要画面不可见，就必须释放，因为预览耗电 耗资源
    }

    fun setOnChangedSizeListener(listener: OnChangedSizeListener) {
        mOnChangedSizeListener = listener
    }

    interface OnChangedSizeListener {
        fun onChanged(width: Int, height: Int)
    }
}