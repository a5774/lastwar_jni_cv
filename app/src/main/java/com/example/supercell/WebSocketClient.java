package com.example.supercell;

import android.content.Context;

import androidx.annotation.NonNull;

import com.google.gson.Gson;

import okhttp3.*;

public class WebSocketClient extends WebSocketListener {

    private WebSocket webSocket;
    private final Gson gson = new Gson();
    private MainActivity activity;

    public static class MessageData {
        String type;
        String content;
    }


    public void start(Context context) {
        activity = (MainActivity) context;
        OkHttpClient client = new OkHttpClient();
        Request request = new Request.Builder()
                .url("wss://knockdoor.top/last")
                .build();
        webSocket = client.newWebSocket(request, this);
    }

    @Override
    public void onMessage(@NonNull WebSocket webSocket, @NonNull String text) {
        MessageData data = gson.fromJson(text, MessageData.class);
        activity.runOnUiThread(() -> {
            switch (data.type) {
                case "SHUTDOWN":
                    activity.terminate();
                    break;
                case "RESTART":
//                activity.reboot();
                    break;
                default:
            }
        });
    }

    public void close() {
        if (webSocket != null) {
            webSocket.close(1000, "Goodbye!");
        }
    }
}
