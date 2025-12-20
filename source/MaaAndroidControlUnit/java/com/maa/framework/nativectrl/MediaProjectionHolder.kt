package com.maa.framework.nativectrl

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Bitmap
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.Image
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.os.Handler
import android.os.HandlerThread
import android.util.DisplayMetrics
import android.view.WindowManager
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

class MediaProjectionHolder(
    private val context: Context,
    private val mediaProjection: MediaProjection
) {
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null
    private var handlerThread: HandlerThread? = null
    private var handler: Handler? = null
    
    private var screenWidth = 0
    private var screenHeight = 0
    private var screenDensity = 0

    init {
        initScreen()
        setupImageReader()
    }

    @SuppressLint("WrongConstant")
    private fun initScreen() {
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        val metrics = DisplayMetrics()
        @Suppress("DEPRECATION")
        wm.defaultDisplay.getRealMetrics(metrics)
        screenWidth = metrics.widthPixels
        screenHeight = metrics.heightPixels
        screenDensity = metrics.densityDpi
    }

    @SuppressLint("WrongConstant")
    private fun setupImageReader() {
        handlerThread = HandlerThread("MediaProjectionHandler").apply { start() }
        handler = Handler(handlerThread!!.looper)

        imageReader = ImageReader.newInstance(
            screenWidth,
            screenHeight,
            PixelFormat.RGBA_8888,
            2
        )

        virtualDisplay = mediaProjection.createVirtualDisplay(
            "MaaScreenCapture",
            screenWidth,
            screenHeight,
            screenDensity,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            imageReader!!.surface,
            null,
            handler
        )
    }

    fun capture(): Bitmap? {
        val reader = imageReader ?: return null
        
        // 首先尝试获取已有的图像
        var image: Image? = try {
            reader.acquireLatestImage()
        } catch (_: Exception) {
            null
        }
        
        // 如果没有现成的图像，等待下一帧
        if (image == null) {
            val imageRef = AtomicReference<Image?>()
            val latch = CountDownLatch(1)

            reader.setOnImageAvailableListener({ ir ->
                try {
                    imageRef.set(ir.acquireLatestImage())
                } catch (_: Exception) {
                } finally {
                    latch.countDown()
                }
            }, handler)

            if (!latch.await(500, TimeUnit.MILLISECONDS)) {
                reader.setOnImageAvailableListener(null, null)
                return null
            }
            reader.setOnImageAvailableListener(null, null)
            
            image = imageRef.get()
        }
        
        if (image == null) return null
        
        return try {
            imageToBitmap(image)
        } finally {
            image.close()
        }
    }

    private fun imageToBitmap(image: Image): Bitmap? {
        val planes = image.planes
        val buffer = planes[0].buffer
        val pixelStride = planes[0].pixelStride
        val rowStride = planes[0].rowStride
        val rowPadding = rowStride - pixelStride * image.width

        val bitmap = Bitmap.createBitmap(
            image.width + rowPadding / pixelStride,
            image.height,
            Bitmap.Config.ARGB_8888
        )
        bitmap.copyPixelsFromBuffer(buffer)

        // Crop if needed
        return if (rowPadding > 0) {
            val cropped = Bitmap.createBitmap(bitmap, 0, 0, image.width, image.height)
            bitmap.recycle() // 回收原始 bitmap 避免内存泄漏
            cropped
        } else {
            bitmap
        }
    }

    fun release() {
        virtualDisplay?.release()
        virtualDisplay = null

        imageReader?.close()
        imageReader = null

        handlerThread?.quitSafely()
        handlerThread = null
        handler = null

        mediaProjection.stop()
    }
}
