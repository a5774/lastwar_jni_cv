package com.example.supercell;

import android.content.Context;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Display;

import android.view.Surface;

public class RotationManager {
    private HandlerThread SensorThread;
    private int sRotation = Surface.ROTATION_0;
    private DisplayManager.DisplayListener listener;
    private DisplayManager displayManager;

    //    static 成员生命周期跟随进程
    private static final class InstanceHolder {
        private static final RotationManager instance = new RotationManager();
    }

    //    延迟加载单例模式
    public static RotationManager getInstance() {
        return InstanceHolder.instance;
    }

    public void registerOrientationListener(Context context) {
        Context applicationContext = context.getApplicationContext();
        if (SensorThread != null) return;
        displayManager = (DisplayManager) applicationContext.getSystemService(Context.DISPLAY_SERVICE);
        Display display = displayManager.getDisplay(Display.DEFAULT_DISPLAY);
        if (display != null) {
            SensorThread = new HandlerThread("OrientationThread");
            SensorThread.start();
            sRotation = display.getRotation();
//            Log.d("SUPPERCELL", "sRotation: " + sRotation);
            syncOrientation(sRotation);
            // 注册监听器
            listener = new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {
                }
                @Override
                public void onDisplayRemoved(int displayId) {
                }
                @Override
                public void onDisplayChanged(int displayId) {
                    if (displayId != Display.DEFAULT_DISPLAY) {
                        return;
                    }
                    int rotation = display.getRotation();
                    if (rotation != sRotation) {
                        sRotation = rotation;
                        syncOrientation(sRotation);
                    }
                }
            };
            displayManager.registerDisplayListener(listener, new Handler(SensorThread.getLooper()));
        }
    }

    public void unregisterOrientationListener() {
        Log.d("SUPPERCELL", "unregisterOrientationListener: ");
        if (listener != null) {
            displayManager.unregisterDisplayListener(listener);
            listener = null;
        }
        if (SensorThread != null) {
//            线程退出了但是 SensorThread引用HandlerThread 对象不会重
            SensorThread.quit();
//            需要显示重置
            SensorThread = null;
//            SensorThread.quitSafely();
        }
    }

    public native void syncOrientation(int rotation);
}
