package com.example.supercell;


import android.content.Context;
import android.util.AttributeSet;

import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.WindowManager;

import android.widget.LinearLayout;

import androidx.annotation.NonNull;


public class FloatLayoutTerminal extends LinearLayout {
    private WindowManager windowManager;
    private WindowManager.LayoutParams layoutParams;
    private int initialX, initialY;
    private float initialTouchX, initialTouchY;
    private Context applicationContext;
    private int layoutWidth, layoutHeight;
    private int screenWidth, screenHeight;
//    private TextView viewTerminal;

    private OrientationEventListener orientationListener;

    public FloatLayoutTerminal(@NonNull Context context) {
        super(context);
        this.initLayoutOuter(context);
    }

    public FloatLayoutTerminal(@NonNull Context context, @NonNull AttributeSet attrs) {
        super(context, attrs);
        this.initLayoutOuter(context);
    }

    public FloatLayoutTerminal(@NonNull Context context, @NonNull AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        this.initLayoutOuter(context);
    }

    public void initLayoutOuter(Context context) {
        this.applicationContext = context.getApplicationContext();
        this.windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
//        在构造函数中访问 layoutParams 尚未初始化,使用post异步调用(已经被添加到父组件)
        post(() -> {
//          viewTerminal = view;
            setClickable(true);
            FloatLayoutTerminal.this.layoutParams = (WindowManager.LayoutParams) FloatLayoutTerminal.this.getLayoutParams();
            FloatLayoutTerminal.this.layoutWidth = getWidth();
            FloatLayoutTerminal.this.layoutHeight = getHeight();
            FloatLayoutTerminal.this.screenWidth = ScreenUtils.getFullMaximumWindowWidth();
            FloatLayoutTerminal.this.screenHeight = ScreenUtils.getFullMaximumWindowHeight();
//            InitOrientationListener();
        });
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        return super.dispatchTouchEvent(ev);
//                onTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (layoutParams == null) return false;
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
//                orientationListener.enable();
                updateMoveLimit();
                initialX = layoutParams.x;
                initialY = layoutParams.y;
                initialTouchX = event.getRawX();
                initialTouchY = event.getRawY();
                return true;
            case MotionEvent.ACTION_MOVE:
                layoutParams.x = Math.max(Math.min(initialX + (int) (event.getRawX() - initialTouchX), screenWidth - layoutWidth), 0);
                layoutParams.y = Math.max(Math.min(initialY + (int) (event.getRawY() - initialTouchY), screenHeight - layoutHeight), 0);
                windowManager.updateViewLayout(this, layoutParams);
                return true;
            case MotionEvent.ACTION_UP:
                performClick();
                return true;
        }
//        此处返回值由自身Clickable决定,Clickable不会因为否设置Listener改变,相反Listener的触发依赖Clickable
//        Clickable为true返回true
        return super.onTouchEvent(event);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean performClick() {
        return super.performClick();
    }

    public void attachToWindow(WindowManager.LayoutParams params) {
        if (windowManager != null) {
            windowManager.addView(this, params);
        }
    }

    public void detachFromWindow() {
        if (windowManager != null && this.isAttachedToWindow() && this.getWindowToken() != null) {
            windowManager.removeView(this);
        }
    }
    public void updateMoveLimit() {
        ScreenUtils.realSizeMaximumWindow(applicationContext);
        this.screenWidth = ScreenUtils.getFullMaximumWindowWidth();
        this.screenHeight = ScreenUtils.getFullMaximumWindowHeight();
    }

}
