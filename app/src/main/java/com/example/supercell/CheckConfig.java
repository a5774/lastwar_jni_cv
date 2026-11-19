package com.example.supercell;

import androidx.annotation.NonNull;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

public class CheckConfig {
    public int UsageStatus;
    public long MainLoop;
    public int VersionCode;
    public String UpdateURL;
    public Dialog dialog;

    public static class Dialog {
        public boolean show;
        public String title;
        public String message;
        public String positiveButton;
        public String negativeButton;
    }

    @NonNull
    @Override
    public String toString() {
        Gson gson = new GsonBuilder().setPrettyPrinting().serializeNulls().create();
        return gson.toJson(this);
    }
}
