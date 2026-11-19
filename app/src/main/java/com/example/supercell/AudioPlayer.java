package com.example.supercell;

import android.content.Context;
import android.media.MediaPlayer;
import android.util.Log;

import java.io.File;
import java.io.IOException;

public class AudioPlayer {
    private final MediaPlayer mediaPlayer;
    private OnAudioCompletionListener onAudioCompletionListener;
    private final File audioFile;
    private boolean isPreparing = false; // 标志，避免多次调用 prepareAsync()

    private int playCount = 0;
    private int maxPlayCount = 3;

    public interface OnAudioCompletionListener {
        void onCompletion();
    }

    public AudioPlayer(Context context) {
        this.audioFile = new File(context.getFilesDir(), MiscUtils.audioFile);
        this.mediaPlayer = new MediaPlayer();
        // 预先设置MediaPlayer相关的配置
        mediaPlayer.setOnCompletionListener(mp -> {
            playCount++;
            if (onAudioCompletionListener != null) {
                onAudioCompletionListener.onCompletion();
            }
            if (playCount < maxPlayCount) {
                mp.seekTo(0);
                mp.start();
            } else {
                playCount = 0;
                isPreparing = false;
            }
        });
    }

    public void setOnAudioCompletionListener(OnAudioCompletionListener listener) {
        this.onAudioCompletionListener = listener;
    }

    public void setMaxPlayCount(int count) {
        this.maxPlayCount = count;
    }

    public void playAudioAsync() {
        if (isPreparing) {
            return;
        }

        new Thread(() -> {
            try {
                isPreparing = true;
                mediaPlayer.reset();
                mediaPlayer.setDataSource(audioFile.toString());
                mediaPlayer.setOnPreparedListener(mp -> {
                    mp.setVolume(1.0f, 1.0f);
                    mp.start();
                });
                mediaPlayer.prepareAsync();
            } catch (IOException e) {
                isPreparing = false;
                Log.d("TAG", e.toString());
            }
        }).start();
    }

    public void stop() {
        if (mediaPlayer.isPlaying()) {
            mediaPlayer.stop();
        }
        mediaPlayer.reset();
        isPreparing = false;
    }
}
