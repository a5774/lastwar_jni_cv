package com.example.supercell;

import android.app.Application;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.media.AudioAttributes;
import android.media.RingtoneManager;
import android.net.Uri;
import android.provider.Settings;


import androidx.annotation.NonNull;
import androidx.lifecycle.ViewModelProvider;


public class CowApplication extends Application {
    protected static CowApplication instance;
    protected static String channel_foreground_projection_id = "channel_id_0x1";
    protected static String channel_foreground_accessibility_id = "channel_id_0x3";
    protected static String channel_extra_common_id = "channel_id_0x2";
    private NotificationManager notificationManager;
    private SharedViewModel sharedViewModel;
    protected static SharedPreferences preferencesConfig;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        preferencesConfig = getSharedPreferences(PreferenceHelper.PREF_XML_CONFIG, Context.MODE_PRIVATE);
        sharedViewModel = new ViewModelProvider.AndroidViewModelFactory(this).create(SharedViewModel.class);
        createNotificationManager();
        createNotificationChannel(channel_foreground_projection_id, getString(R.string.notification_channel_id_0x1_name), NotificationManager.IMPORTANCE_DEFAULT);
        createNotificationChannel(channel_foreground_accessibility_id, getString(R.string.notification_channel_id_0x2_name), NotificationManager.IMPORTANCE_DEFAULT);
        createNotificationChannel(channel_extra_common_id, getString(R.string.notification_channel_id_0x3_name), NotificationManager.IMPORTANCE_HIGH);
    }

    private void createNotificationManager() {
        notificationManager = getSystemService(NotificationManager.class);
    }

    private void createNotificationChannel(String ChannelID, @NonNull String name, int importance) {
        NotificationChannel channel = new NotificationChannel(ChannelID, name, importance);
        AudioAttributes attributes = new AudioAttributes.Builder()
//                解释用途,例如是否受勿扰模式影响
                .setUsage(AudioAttributes.USAGE_NOTIFICATION)
//                内容影响系统策略
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
//        貌似setSound在MIUI中设置无效（MiPush）
        channel.setSound(Settings.System.DEFAULT_NOTIFICATION_URI, attributes);
        channel.setSound(null, null);
        channel.setDescription(getString(R.string.notification_channel_description));
        notificationManager.createNotificationChannel(channel);
    }

    public SharedViewModel getSharedViewModel() {
        return sharedViewModel;
    }
}
