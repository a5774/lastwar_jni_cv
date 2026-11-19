package com.example.supercell;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import com.google.android.material.button.MaterialButton;

public class OverlaySelectCoordinate extends FrameLayout {
    private float clickedX, clickedY;
    private final WindowManager windowManager;
    private WindowManager.LayoutParams layoutParams;
    private ImageView marker;
    private MainActivity.Coordinate coordinate;


    public interface OnCoordinateSelectedListener {
        void onSelected(float x, float y, MainActivity.Coordinate coordinate);
    }

    private OnCoordinateSelectedListener listener;

    public void setOnCoordinateSelectedListener(OnCoordinateSelectedListener listener) {
        this.listener = listener;
    }

    public OverlaySelectCoordinate(@NonNull Context context, WindowManager windowManager) {
        super(context);
        this.windowManager = windowManager;
        this.initOverlay(context);
    }

    public OverlaySelectCoordinate(@NonNull Context context, @Nullable AttributeSet attrs, WindowManager windowManager) {
        super(context, attrs);
        this.windowManager = windowManager;
        this.initOverlay(context);
    }

    public OverlaySelectCoordinate(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, WindowManager windowManager) {
        super(context, attrs, defStyleAttr);
        this.windowManager = windowManager;
        this.initOverlay(context);
    }

    private void initOverlay(Context context) {
//        ActivityMainBinding binding =  ActivityMainBinding.inflate(LayoutInflater.from(context));
        setBackgroundColor(ContextCompat.getColor(context, R.color.gray_translucent));
        layoutParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT
        );
        layoutParams.gravity = Gravity.TOP | Gravity.START;
        createConfirm(context);
        createMarker(context);
    }

    private void createConfirm(Context context) {
        Button confirm = new MaterialButton(context);
        confirm.setText(R.string.confirm_coordinate);
        FrameLayout.LayoutParams confirmLayoutParams = new FrameLayout.LayoutParams(
                LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT
        );
        confirmLayoutParams.gravity = Gravity.TOP | Gravity.END;
        confirmLayoutParams.setMargins(0, 50, 50, 0); // 适当偏移
        addView(confirm, confirmLayoutParams);
        confirm.setOnClickListener(v -> {
            if (listener != null) {
                listener.onSelected(clickedX, clickedY, coordinate);
                if (marker != null) {
                    showClickMarker(0, 0);
                }
            }
            destroyOverlay();
        });
    }

    private void createMarker(Context context) {
        marker = new ImageView(context);
        marker.setImageResource(R.drawable.ic_marker_coordinate);
        marker.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));
    }

    private void showClickMarker(float x, float y) {
        if (marker.getParent() == null) {
            this.addView(marker);
        }
        marker.post(() -> {
            marker.setX(x - marker.getDrawable().getIntrinsicWidth() / 2f);
            marker.setY(y - marker.getDrawable().getIntrinsicHeight() / 2f);
        });
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        performClick();
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            clickedX = event.getRawX();
            clickedY = event.getRawY();
            showClickMarker(clickedX, clickedY);
        }
        return super.onTouchEvent(event);
    }


    @Override
    public boolean performClick() {
        return super.performClick();
    }

    public void assignCoordinatesByOverlay(MainActivity.Coordinate coordinate) {
        if (windowManager != null && layoutParams != null) {
            this.coordinate = coordinate;
            windowManager.addView(this, layoutParams);
        }
    }

    public void destroyOverlay() {
        if (windowManager != null && layoutParams != null) {
            windowManager.removeView(this);
        }
    }
}



