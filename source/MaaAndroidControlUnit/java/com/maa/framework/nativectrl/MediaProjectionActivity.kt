package com.maa.framework.nativectrl

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjectionManager
import android.os.Bundle

/**
 * Transparent activity to request MediaProjection permission.
 * This is launched from native code when MediaProjection is needed.
 */
class MediaProjectionActivity : Activity() {

    companion object {
        private const val REQUEST_CODE_CAPTURE = 1001
        
        @Volatile
        private var pendingCallback: ((Boolean) -> Unit)? = null
        
        fun requestPermission(context: Context, callback: (Boolean) -> Unit) {
            pendingCallback = callback
            val intent = Intent(context, MediaProjectionActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }
        
        // 清理静态回调，防止内存泄漏
        internal fun clearCallback() {
            pendingCallback = null
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // 检查是否有待处理的回调，如果没有则直接关闭
        if (pendingCallback == null) {
            finish()
            return
        }
        
        val projectionManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        @Suppress("DEPRECATION")
        startActivityForResult(projectionManager.createScreenCaptureIntent(), REQUEST_CODE_CAPTURE)
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        
        if (requestCode == REQUEST_CODE_CAPTURE) {
            val callback = pendingCallback
            pendingCallback = null  // 立即清理回调引用
            
            val success = if (resultCode == RESULT_OK && data != null) {
                val projectionManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
                val projection = projectionManager.getMediaProjection(resultCode, data)
                if (projection != null) {
                    val holder = MediaProjectionHolder(applicationContext, projection)
                    NativeBridge.attachMediaProjection(holder)
                    true
                } else {
                    false
                }
            } else {
                false
            }
            
            callback?.invoke(success)
        }
        
        finish()
    }
    
    override fun onDestroy() {
        super.onDestroy()
        // 确保在 Activity 销毁时清理回调
        pendingCallback = null
    }
}
