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
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val projectionManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        startActivityForResult(projectionManager.createScreenCaptureIntent(), REQUEST_CODE_CAPTURE)
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        
        if (requestCode == REQUEST_CODE_CAPTURE) {
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
            
            pendingCallback?.invoke(success)
            pendingCallback = null
        }
        
        finish()
    }
}
