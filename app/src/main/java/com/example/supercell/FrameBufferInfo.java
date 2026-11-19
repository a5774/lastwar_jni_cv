package com.example.supercell;

import androidx.annotation.NonNull;

public class FrameBufferInfo {
    public int width;
    public int height;
    public int capacity;
    public int rowValid;
    public int rowStride;
    public int pixelStride;

    public FrameBufferInfo(int width, int height, int rowStride, int pixelStride) {
        this.width = width;
        this.height = height;
        this.rowStride = rowStride;
        this.pixelStride = pixelStride;
        this.capacity = width * height * pixelStride;
        this.rowValid = width * pixelStride;
    }

    @NonNull
    @Override
    public String toString() {
        return "FrameBufferInfo{" +
                "width=" + width +
                ", height=" + height +
                ", capacity=" + capacity +
                ", rowValid=" + rowValid +
                ", rowStride=" + rowStride +
                ", pixelStride=" + pixelStride +
                '}';
    }
}
