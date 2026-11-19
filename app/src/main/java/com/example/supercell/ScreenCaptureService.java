package com.example.supercell;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.icu.text.SimpleDateFormat;
import android.icu.util.Calendar;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;


import java.util.Locale;


public class ScreenCaptureService extends Service {
    int NOTIFICATION_ID_FOREGROUND = 1001;
    public static int NOTIFICATION_ID_EXTRA = 1002;
    private final IBinder binder = new CaptureBinder();
    private ImageReader imageReader;
    private Surface surface;
    private VirtualDisplay virtualDisplay;
    private MediaProjection mediaProjection;
    private FrameBufferInfo frameBufferInfo;
    private FrameBufferCallback frameBufferCallback;


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CowApplication.channel_foreground_projection_id)
                .setContentTitle(getString(R.string.projection_media_notify_title))
                .setContentText(getString(R.string.projection_media_notify_content))
                .setSmallIcon(R.drawable.ic_launcher_foreground)
                .setOngoing(true);
        // 显示前台通知
        startForeground(NOTIFICATION_ID_FOREGROUND, builder.build());
        // 计划任务
//        scheduleDailyAlarm(this);
        //如果系统资源不足，可能会杀掉一些后台应用或服务,自动重启服务
        return Service.START_STICKY;
    }

    public void InitProjection(int resultCode, Intent data) {
        MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) getSystemService(MEDIA_PROJECTION_SERVICE);
        // 实现Serializable,Serializable接口才可使用putExtra传递
        mediaProjection = mediaProjectionManager.getMediaProjection(resultCode, data);
    }


    public void InitVirtualDisplay(Context context, Handler handler) {
        int fw = ScreenUtils.getPhysicalWidth();
        int fh = ScreenUtils.getPhysicalHeight();
        int dp = ScreenUtils.getMetricsDensityDpi(context);
        imageReader = ImageReader.newInstance(fh, fw, PixelFormat.RGBA_8888, 2);
        InitFrameBuffer(handler);
        surface = imageReader.getSurface();
        if (mediaProjection != null) {
//            mediaProjection.VirtualDisplay(输出)和imageReader(输入) 本质上共享同一个缓冲区队列
            virtualDisplay = mediaProjection.createVirtualDisplay(getString(R.string.virtual_display_name_frame), fh, fw, dp, DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION | DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC, surface, null, null);
            return;
        }
        MiscUtils.sendNotification(getString(R.string.virtual_display_error));
    }

    private void InitFrameBuffer(Handler handler) {
        if (imageReader != null) {
//            提前注册监听
            imageReader.setOnImageAvailableListener(reader -> {
                try (Image image = reader.acquireLatestImage()) {
                    if (image != null) {
                        int width = image.getWidth();
                        int height = image.getHeight();
                        int pixelStride = image.getPlanes()[0].getPixelStride();
                        int rowStride = image.getPlanes()[0].getRowStride();
                        frameBufferInfo = new FrameBufferInfo(width, height, rowStride, pixelStride);
                        if (frameBufferCallback != null) {
                            frameBufferCallback.onFrameBufferReady(frameBufferInfo);
                        }
                        imageReader.setOnImageAvailableListener(null, null);
                    }
                } catch (Exception e) {
                    Log.d("TAG", e.toString());
                }
            }, handler);
        } else {
            Log.d("TAG", "imageReader is null ");
        }
    }


    public void scheduleDailyAlarm(Context context) {
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        Intent intent = new Intent(context, ScheduleTimerBootReceiver.class);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(System.currentTimeMillis());
        calendar.set(Calendar.HOUR_OF_DAY, 6);
        calendar.set(Calendar.MINUTE, 0);
        calendar.set(Calendar.SECOND, 0);
        calendar.set(Calendar.MILLISECOND, 0);

        if (calendar.getTimeInMillis() <= System.currentTimeMillis()) {
            calendar.add(Calendar.DAY_OF_MONTH, 1);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (alarmManager.canScheduleExactAlarms()) {
                alarmManager.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, calendar.getTimeInMillis(), pendingIntent);
            }
        } else {
            alarmManager.setExact(AlarmManager.RTC_WAKEUP, calendar.getTimeInMillis(), pendingIntent);
        }
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
        MiscUtils.sendNotification("定时任务", sdf.format(calendar.getTime()));

    }

    public MediaProjection getMediaProjection() {
        return mediaProjection;
    }

    public ImageReader getImageReader() {
        return imageReader;
    }


    public Surface getSurface() {
        return surface;
    }

    public VirtualDisplay getVirtualDisplay() {
        return virtualDisplay;
    }

    @Override
    @Nullable
    public IBinder onBind(Intent intent) {
        return binder;
    }

    public class CaptureBinder extends Binder {
        ScreenCaptureService getService() {
            return ScreenCaptureService.this;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (imageReader != null) {
            imageReader.close();
        }
        if (virtualDisplay != null) {
            virtualDisplay.release();
        }
        if (mediaProjection != null) {
            mediaProjection.stop();
        }
    }

    public interface FrameBufferCallback {
        void onFrameBufferReady(FrameBufferInfo info);

    }

    public void setFrameBufferCallback(FrameBufferCallback callback) {
        this.frameBufferCallback = callback;
//        防止activity端调用在InitVirtualDisplay之后,保证frameBufferInfo绑定
        if (frameBufferInfo != null) {
            callback.onFrameBufferReady(frameBufferInfo);
        }
    }
}
