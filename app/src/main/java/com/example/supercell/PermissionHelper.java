package com.example.supercell;

import android.Manifest;
import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.AccessibilityServiceInfo;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.projection.MediaProjectionManager;
import android.net.Uri;

import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.List;

public class PermissionHelper {
    private static final int REQUEST_CODE_OVERLAY = 1001;
    private static final int REQUEST_CODE_MEDIA_PROJECTION = 1002;
    private static final int REQUEST_CODE_ACCESSIBILITY_SERVICE = 1003;
    private static final int REQUEST_CODE_MANAGE_ALL_STORAGE = 1004;
    public static final int REQUEST_RUNTIME_CODE_POST_NOTIFICATIONS = 101001;

    private final MainActivity activity;
    private final MediaProjectionManager projectionManager;
    private final PermissionCallback callback;

    public interface PermissionCallback {
        void onOverlayGranted();

        void onProjectionGranted(int resultCode, Intent data);

        void onAccessibilityServiceGranted(int resultCode, Intent data);

        void onManageAllStorageGranted(int resultCode, Intent data);

        void onPermissionDenied(String reason);
    }

    public PermissionHelper(Context context, PermissionCallback callback) {
        this.callback = callback;
        this.activity = (MainActivity) context;
        this.projectionManager = (MediaProjectionManager) activity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
    }

    public void requestOverlayPermission() {
        if (!Settings.canDrawOverlays(activity)) {
            Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + activity.getPackageName()));
            activity.startActivityForResult(intent, REQUEST_CODE_OVERLAY);
        } else {
            callback.onOverlayGranted();
        }
    }

    public void requestMediaProjectionPermission() {
        Intent captureIntent = projectionManager.createScreenCaptureIntent();
        activity.startActivityForResult(captureIntent, REQUEST_CODE_MEDIA_PROJECTION);
    }

    public void requestAccessibilityServicePermission() {
        if (!isAccessibilityServiceEnabled(activity, ClickAccessibilityService.class)) {
//           在非Activity上下中启动,在新的任务栈中启动
            Intent AccessibilityIntent = new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS);
//            禁用 FLAG_ACTIVITY_NEW_TASK 与MainActivity在同一个Task栈
//            AccessibilityIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//            registerForActivityResult
            activity.startActivityForResult(AccessibilityIntent, REQUEST_CODE_ACCESSIBILITY_SERVICE);
        }
    }


    public void requestManageAllStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + activity.getPackageName()));
                activity.startActivityForResult(intent, REQUEST_CODE_MANAGE_ALL_STORAGE);

            }
        }
    }

    public void requestRuntimePostNotifications() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(activity, android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.POST_NOTIFICATIONS}, REQUEST_RUNTIME_CODE_POST_NOTIFICATIONS);
            }
        }
    }

    public boolean isExternalStorageManagerEnable() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        }
        return false;
    }

    public boolean isAccessibilityServiceEnabled(Context context, Class<? extends AccessibilityService> service) {
        // 获取 AccessibilityManager 实例
        AccessibilityManager am = (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
        if (am != null) {
            // 获取当前已启用的无障碍服务列表
            List<AccessibilityServiceInfo> enabledServices = am.getEnabledAccessibilityServiceList(AccessibilityEvent.TYPES_ALL_MASK);
            for (AccessibilityServiceInfo info : enabledServices) {
                ComponentName cn = new ComponentName(context, service);
                if (info.getId().toLowerCase().contains(cn.getShortClassName().toLowerCase())) {
                    return true;
                }
            }
        }
//        Compatible with some devices
        String enabledService = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        if (enabledService != null) {
            ComponentName serviceComponent = new ComponentName(context, service);
            return enabledService.toLowerCase().contains(serviceComponent.flattenToString().toLowerCase());
        }
        return false;
    }

    public void handleActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_CODE_OVERLAY:
                if (Settings.canDrawOverlays(activity)) {
                    callback.onOverlayGranted();
                } else {
                    callback.onPermissionDenied(activity.getString(R.string.system_alert_window_denied));
                }
                break;
            case REQUEST_CODE_MEDIA_PROJECTION:
                if (resultCode == Activity.RESULT_OK && data != null) {
                    callback.onProjectionGranted(resultCode, data);
                } else {
                    callback.onPermissionDenied(activity.getString(R.string.projection_media_denied));
                }
                break;
            case REQUEST_CODE_ACCESSIBILITY_SERVICE:
                if (isAccessibilityServiceEnabled(activity, ClickAccessibilityService.class)) {
                    callback.onAccessibilityServiceGranted(resultCode, data);
                } else {
                    callback.onPermissionDenied(activity.getString(R.string.accessibility_service_guide));
                }
                break;
            case REQUEST_CODE_MANAGE_ALL_STORAGE:
                if (isExternalStorageManagerEnable()) {
                    callback.onManageAllStorageGranted(resultCode, data);
                } else {
                    callback.onPermissionDenied(activity.getString(R.string.manage_external_storage_pending));
                }
                break;
        }
    }

}
