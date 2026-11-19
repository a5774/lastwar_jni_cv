package com.example.supercell;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.GestureDescription;
import android.graphics.Path;

import android.util.Log;
import android.view.accessibility.AccessibilityEvent;

import androidx.core.app.NotificationCompat;

public class ClickAccessibilityService extends AccessibilityService {
    public static volatile ClickAccessibilityService instance;
    public static volatile String cForegroundPackage;
    int NOTIFICATION_ID_ACCESSIBILITY = 1003;

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
        if (event != null) {
            if (event.getEventType() == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED) {
                CharSequence pkg = event.getPackageName();
                if (pkg != null) {
                    cForegroundPackage = pkg.toString();
                }
            }
        }
//        Log.d("TAG", "Event type: " + event.getEventType());
    }

    @Override
    public void onInterrupt() {
//        Log.d("TAG", "Service Interrupted");
    }

    @Override
    protected void onServiceConnected() {
        super.onServiceConnected();
        Log.d("TAG", "onServiceConnected:");
//        Log.d("TAG", "Service onServiceConnected");
//        ((CowApplication) getApplicationContext()).getSharedViewModel().sendCommand("backMainActivity");
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CowApplication.channel_foreground_accessibility_id).setContentTitle("Accessibility Services").setContentText("Maintaining persistent services").setSmallIcon(R.drawable.ic_launcher_foreground).setOngoing(true);
//        提升为（Foreground Service），显示持续通知，提升优先级
        startForeground(NOTIFICATION_ID_ACCESSIBILITY, builder.build());
        instance = this;
    }


    public void performClick(float x, float y) {
        Path path = new Path();
        path.moveTo(x, y);
        GestureDescription gestureDescription = new GestureDescription.Builder().addStroke(new GestureDescription.StrokeDescription(path, 0, 10)).build();
        dispatchGesture(gestureDescription, null, null);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        instance = null;
    }

    public static void stop() {
        if (instance != null) {
            instance.stopSelf();
            instance.stopForeground(true);
            instance = null;
        }
    }
}


//settings get secure enabled_accessibility_services
//settings put secure enabled_accessibility_services com.example.supercell/com.example.supercell.ClickAccessibilityService