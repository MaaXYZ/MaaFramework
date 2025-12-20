package com.maa.framework.nativectrl

import android.graphics.Bitmap

object NativeBridge {
    
    // Screencap methods - matches MaaDef.h
    const val SCREENCAP_METHOD_NONE = 0L
    const val SCREENCAP_METHOD_ACCESSIBILITY = 1L
    const val SCREENCAP_METHOD_MEDIA_PROJECTION = 1L shl 1
    
    // Input methods - matches MaaDef.h
    const val INPUT_METHOD_NONE = 0L
    const val INPUT_METHOD_ACCESSIBILITY = 1L

    @Volatile
    private var accessibilityService: MaaAccessibilityService? = null
    
    @Volatile
    private var mediaProjectionHolder: MediaProjectionHolder? = null
    
    @Volatile
    private var screencapMethods: Long = SCREENCAP_METHOD_NONE
    
    @Volatile
    private var inputMethods: Long = INPUT_METHOD_NONE

    @JvmStatic
    fun attachAccessibilityService(service: MaaAccessibilityService) {
        this.accessibilityService = service
    }

    @JvmStatic
    fun detachAccessibilityService(service: MaaAccessibilityService) {
        if (this.accessibilityService == service) {
            this.accessibilityService = null
        }
    }
    
    @JvmStatic
    fun attachMediaProjection(holder: MediaProjectionHolder) {
        this.mediaProjectionHolder = holder
    }
    
    @JvmStatic
    fun detachMediaProjection() {
        mediaProjectionHolder?.release()
        mediaProjectionHolder = null
    }
    
    // Called from JNI to initialize with methods
    @JvmStatic
    fun init(screencapMethods: Long, inputMethods: Long): Boolean {
        this.screencapMethods = screencapMethods
        this.inputMethods = inputMethods
        return true
    }

    @JvmStatic
    fun connect(): Boolean {
        // Check if accessibility service is available when needed
        val needAccessibility = (screencapMethods and SCREENCAP_METHOD_ACCESSIBILITY) != 0L ||
                               (inputMethods and INPUT_METHOD_ACCESSIBILITY) != 0L
        if (needAccessibility && accessibilityService == null) {
            return false
        }
        
        // MediaProjection will be requested on demand
        return true
    }

    @JvmStatic
    fun requestUuid(): String = accessibilityService?.uuid().orEmpty()

    @JvmStatic
    fun startApp(intent: String): Boolean = accessibilityService?.startApp(intent) ?: false

    @JvmStatic
    fun stopApp(intent: String): Boolean = accessibilityService?.stopApp(intent) ?: false

    @JvmStatic
    fun tap(x: Int, y: Int): Boolean {
        if ((inputMethods and INPUT_METHOD_ACCESSIBILITY) == 0L) return false
        return accessibilityService?.tap(x, y) ?: false
    }

    @JvmStatic
    fun swipe(x1: Int, y1: Int, x2: Int, y2: Int, duration: Int): Boolean {
        if ((inputMethods and INPUT_METHOD_ACCESSIBILITY) == 0L) return false
        return accessibilityService?.swipe(x1, y1, x2, y2, duration) ?: false
    }

    @JvmStatic
    fun scroll(dx: Int, dy: Int): Boolean {
        if ((inputMethods and INPUT_METHOD_ACCESSIBILITY) == 0L) return false
        return accessibilityService?.scroll(dx, dy) ?: false
    }

    @JvmStatic
    fun keyEvent(key: Int, down: Boolean): Boolean {
        if ((inputMethods and INPUT_METHOD_ACCESSIBILITY) == 0L) return false
        return accessibilityService?.keyEvent(key, down) ?: false
    }

    @JvmStatic
    fun inputText(text: String): Boolean {
        if ((inputMethods and INPUT_METHOD_ACCESSIBILITY) == 0L) return false
        return accessibilityService?.inputText(text) ?: false
    }

    @JvmStatic
    fun screencap(): Bitmap? {
        // Try MediaProjection first if enabled (faster and more reliable)
        if ((screencapMethods and SCREENCAP_METHOD_MEDIA_PROJECTION) != 0L) {
            mediaProjectionHolder?.capture()?.let { return it }
        }
        
        // Fall back to Accessibility screenshot
        if ((screencapMethods and SCREENCAP_METHOD_ACCESSIBILITY) != 0L) {
            accessibilityService?.screencap()?.let { return it }
        }
        
        return null
    }
    
    @JvmStatic
    fun getScreenSize(): IntArray {
        return accessibilityService?.getScreenSize() ?: intArrayOf(0, 0)
    }
}
