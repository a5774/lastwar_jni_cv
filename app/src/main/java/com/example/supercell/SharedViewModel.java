package com.example.supercell;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

public class SharedViewModel extends ViewModel {

    private final MutableLiveData<Boolean> isActivityReady = new MutableLiveData<>(false);
    private final MutableLiveData<Boolean> jmBooted = new MutableLiveData<>(false);
    private final MutableLiveData<String> command = new MutableLiveData<>();
    private final MutableLiveData<FrameBufferInfo> frameBufferInfo = new MutableLiveData<>();

    public void setActivityReady(boolean ready) {
        isActivityReady.setValue(ready);
    }

    public boolean getActivityReadyState() {
        return Boolean.TRUE.equals(isActivityReady.getValue());
    }

    public void setJmBooted(boolean booted) {
//        任意线程
//        jmBooted.postValue(booted);
//        仅主线程
        jmBooted.setValue(booted);
    }

    public boolean getJmBooted() {
        return Boolean.TRUE.equals(jmBooted.getValue());
    }

    public LiveData<String> getCommand() {
        return command;
    }

    public void setFrameBufferInfo(FrameBufferInfo info) {
//        ImageReader 回调我使用了MainLooper,可使用setValue（毕竟跨组件了,那就用下postValue吧，芜湖~）
        frameBufferInfo.postValue(info);

    }

    public LiveData<FrameBufferInfo> getFrameBufferInfo() {
        return frameBufferInfo;

    }

    public void sendCommand(String cmd) {
        command.postValue(cmd);
    }


}
