package com.example.supercell;

import android.util.Log;
import androidx.annotation.NonNull;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import com.google.gson.Gson;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

public class CheckConfigFetcher {
    private final OkHttpClient client;
    public interface OnConfigFetchedListener {
        void onSuccess(CheckConfig config);

        void onFailure(Exception e);
    }

    public CheckConfigFetcher() {
        this.client = new OkHttpClient.Builder()
                .connectTimeout(5, TimeUnit.SECONDS) // 连接超时
                .readTimeout(5, TimeUnit.SECONDS)    // 读取超时
                .writeTimeout(5, TimeUnit.SECONDS)   // 写入超时
                .build();
    }
    public void fetchConfig(String url, OnConfigFetchedListener listener) {
        Request request = new Request.Builder()
                .url(url)
                .get()
                .build();
        client.newCall(request).enqueue(new okhttp3.Callback() {
            @Override
            public void onFailure(@NonNull okhttp3.Call call, @NonNull IOException e) {
                if (listener != null) {
                    Log.d("TAG", e.toString());
                    listener.onFailure(e);
                }
            }
            @Override
            public void onResponse(@NonNull okhttp3.Call call, @NonNull okhttp3.Response response) {
                if (response.isSuccessful() && response.body() != null) {
                    try {
                        String json = response.body().string();
                        CheckConfig config = new Gson().fromJson(json, CheckConfig.class);
                        if (listener != null) {
                            listener.onSuccess(config);
                        }
                    } catch (Exception e) {
                        listener.onFailure(e);
                        Log.d("TAG", e.toString());
                    }
                } else {
                    listener.onFailure(new Exception("HTTP error: " + response.code()));
                    Log.d("TAG", String.valueOf(response.code()));
                }
            }
        });
    }
}
