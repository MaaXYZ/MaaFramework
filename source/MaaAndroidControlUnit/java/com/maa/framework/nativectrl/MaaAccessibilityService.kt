package com.maa.framework.nativectrl

import android.accessibilityservice.AccessibilityService
import android.accessibilityservice.GestureDescription
import android.accessibilityservice.GestureDescription.StrokeDescription
import android.content.Context
import android.graphics.Bitmap
import android.graphics.Path
import android.os.Build
import android.os.Bundle
import android.util.DisplayMetrics
import android.view.Display
import android.view.WindowManager
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityNodeInfo
import android.view.KeyEvent
import java.util.UUID
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit

class MaaAccessibilityService : AccessibilityService() {

    private val uuidCache: String by lazy { loadUuid() }
    
    private var cachedScreenWidth = 0
    private var cachedScreenHeight = 0
    
    private val windowMgr: WindowManager by lazy {
        getSystemService(Context.WINDOW_SERVICE) as WindowManager
    }

    override fun onServiceConnected() {
        super.onServiceConnected()
        updateScreenSize()
        NativeBridge.attachAccessibilityService(this)
    }

    override fun onUnbind(intent: android.content.Intent?): Boolean {
        NativeBridge.detachAccessibilityService(this)
        return super.onUnbind(intent)
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {
        // No-op
    }

    override fun onInterrupt() {
        // No-op
    }
    
    private fun updateScreenSize() {
        val metrics = DisplayMetrics()
        @Suppress("DEPRECATION")
        windowMgr.defaultDisplay.getRealMetrics(metrics)
        cachedScreenWidth = metrics.widthPixels
        cachedScreenHeight = metrics.heightPixels
    }
    
    fun getScreenSize(): IntArray {
        if (cachedScreenWidth == 0 || cachedScreenHeight == 0) {
            updateScreenSize()
        }
        return intArrayOf(cachedScreenWidth, cachedScreenHeight)
    }

    fun uuid(): String = uuidCache

    fun startApp(intentUri: String): Boolean {
        return try {
            val intent = android.content.Intent.parseUri(intentUri, 0).apply {
                addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            startActivity(intent)
            true
        } catch (_: Exception) {
            false
        }
    }

    fun stopApp(intentUri: String): Boolean {
        // Best effort: bring home
        performGlobalAction(GLOBAL_ACTION_HOME)
        return true
    }

    fun tap(x: Int, y: Int): Boolean = dispatchGestureOnce(
        gesturePath { moveTo(x.toFloat(), y.toFloat()); lineTo(x.toFloat(), y.toFloat()) },
        100L
    )

    fun swipe(x1: Int, y1: Int, x2: Int, y2: Int, duration: Int): Boolean =
        dispatchGestureOnce(
            gesturePath(duration.toLong().coerceAtLeast(50L)) { 
                moveTo(x1.toFloat(), y1.toFloat())
                lineTo(x2.toFloat(), y2.toFloat()) 
            }, 
            duration.toLong().coerceAtLeast(50L)
        )

    fun scroll(dx: Int, dy: Int): Boolean {
        val metrics = DisplayMetrics()
        @Suppress("DEPRECATION")
        windowMgr.defaultDisplay.getRealMetrics(metrics)
        val startX = (metrics.widthPixels / 2f + dx * 0.25f)
        val startY = (metrics.heightPixels / 2f + dy * 0.25f)
        val endX = startX - dx
        val endY = startY - dy
        return dispatchGestureOnce(
            gesturePath(150L) { moveTo(startX, startY); lineTo(endX, endY) },
            150L
        )
    }

    fun keyEvent(key: Int, down: Boolean): Boolean {
        // Only trigger on key down to avoid double actions
        if (!down) return true
        return when (key) {
            KeyEvent.KEYCODE_BACK -> performGlobalAction(GLOBAL_ACTION_BACK)
            KeyEvent.KEYCODE_HOME -> performGlobalAction(GLOBAL_ACTION_HOME)
            KeyEvent.KEYCODE_APP_SWITCH -> performGlobalAction(GLOBAL_ACTION_RECENTS)
            else -> false
        }
    }

    fun inputText(text: String): Boolean {
        val node = rootInActiveWindow?.findFocus(AccessibilityNodeInfo.FOCUS_INPUT) ?: return false
        val args = Bundle()
        args.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, text)
        return node.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, args)
    }

    fun screencap(): Bitmap? {
        if (Build.VERSION.SDK_INT < 33) return null
        val future = CompletableFuture<Bitmap?>()
        takeScreenshot(Display.DEFAULT_DISPLAY, mainExecutor, 
            object : TakeScreenshotCallback {
                override fun onSuccess(result: ScreenshotResult) {
                    val hw = result.hardwareBuffer
                    val bmp = Bitmap.wrapHardwareBuffer(hw, result.colorSpace)
                    val copy = bmp?.copy(Bitmap.Config.ARGB_8888, false)
                    bmp?.recycle()
                    hw?.close()
                    future.complete(copy)
                }
                override fun onFailure(errorCode: Int) {
                    future.complete(null)
                }
            }
        )
        return try {
            future.get(1200, TimeUnit.MILLISECONDS)
        } catch (_: Exception) {
            null
        }
    }

    private fun gesturePath(durationMs: Long = 100L, builder: Path.() -> Unit): GestureDescription {
        val path = Path().apply(builder)
        val stroke = StrokeDescription(path, 0, durationMs)
        return GestureDescription.Builder().addStroke(stroke).build()
    }

    private fun dispatchGestureOnce(gesture: GestureDescription, durationMs: Long): Boolean {
        val latch = CompletableFuture<Boolean>()
        dispatchGesture(
            gesture,
            object : GestureResultCallback() {
                override fun onCompleted(gestureDescription: GestureDescription) {
                    latch.complete(true)
                }

                override fun onCancelled(gestureDescription: GestureDescription) {
                    latch.complete(false)
                }
            },
            null
        )
        return try {
            latch.get(durationMs + 500, TimeUnit.MILLISECONDS)
        } catch (_: Exception) {
            false
        }
    }

    private fun loadUuid(): String {
        val prefs = getSharedPreferences("maa_ctrl", MODE_PRIVATE)
        val cached = prefs.getString("uuid", null)
        if (cached != null) return cached
        val generated = UUID.randomUUID().toString()
        prefs.edit().putString("uuid", generated).apply()
        return generated
    }
}
