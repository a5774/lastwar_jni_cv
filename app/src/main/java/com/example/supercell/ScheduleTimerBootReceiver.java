package com.example.supercell;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class ScheduleTimerBootReceiver extends BroadcastReceiver {
    private SharedViewModel viewModel;

    @Override
    public void onReceive(Context context, Intent intent) {
        SharedViewModel viewModel;
        if (context.getApplicationContext() instanceof CowApplication app) {
            viewModel = app.getSharedViewModel();
            if (viewModel.getJmBooted() && viewModel.getActivityReadyState()) {
                viewModel.sendCommand("jmBooted");
                return;
            }
            MiscUtils.sendNotification(context.getString(R.string.no_boot_activity));
        }
    }
}
