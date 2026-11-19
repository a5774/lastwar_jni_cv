package com.example.supercell;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Insets;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.WindowMetrics;

public class ScreenUtils {
    private static WindowMetrics maximumMetrics;
    private static WindowMetrics currentMetrics;
    private static DisplayMetrics displayMetrics;
    private static Display.Mode mode;

    public static void realSizeCurrentWindow(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            currentMetrics = context.getSystemService(WindowManager.class).getCurrentWindowMetrics();
            Insets insets = currentMetrics.getWindowInsets().getInsetsIgnoringVisibility(
                    WindowInsets.Type.statusBars()
                            | WindowInsets.Type.navigationBars()
//                            | WindowInsets.Type.captionBar()
//                            | WindowInsets.Type.ime()
//                            | WindowInsets.Type.systemGestures()
//                            | WindowInsets.Type.mandatorySystemGestures()
//                            | WindowInsets.Type.tappableElement()
//                            | WindowInsets.Type.displayCutout()
            );
//            insets.left;
        }
    }

    public static void realSizeMaximumWindow(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ (API 30+)
            maximumMetrics = context.getSystemService(WindowManager.class).getMaximumWindowMetrics();
        } else {
            // Android 4.2 ~ 10
            WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
            if (wm != null) {
                Display display = wm.getDefaultDisplay();
                displayMetrics = new DisplayMetrics();
                display.getRealMetrics(displayMetrics);
            }
        }
    }

    public static void template(Context context) {
        WindowManager wm =
                (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        mode = display.getMode();
    }

    public static int getPhysicalWidth() {
        //不受设备旋转方向影响,系统固定值
        return mode.getPhysicalWidth();

    }

    public static int getPhysicalHeight() {
        return mode.getPhysicalHeight();
    }


    public static int getFullCurrentWindowWidth() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return ScreenUtils.currentMetrics.getBounds().width();
        }
        return ScreenUtils.getFullWidth();
    }

    public static int getFullCurrentWindowHeight() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return ScreenUtils.currentMetrics.getBounds().height();
        }
        return ScreenUtils.getFullHeight();
    }


    public static int getFullMaximumWindowWidth() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return ScreenUtils.maximumMetrics.getBounds().width();
        }
        return ScreenUtils.displayMetrics.widthPixels;
    }

    public static int getFullMaximumWindowHeight() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return ScreenUtils.maximumMetrics.getBounds().height();
        }
        return ScreenUtils.displayMetrics.heightPixels;
    }

    public static int getFullWidth() {
        return Resources.getSystem().getDisplayMetrics().widthPixels;
    }

    public static int getFullHeight() {
        return Resources.getSystem().getDisplayMetrics().heightPixels;
    }

    public static int getSystemDensityDpi() {
        return Resources.getSystem().getDisplayMetrics().densityDpi;
    }

    public static int getMetricsDensityDpi(Context context) {
//        android14
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            return (int) (maximumMetrics.getDensity() * 160);
        }
//        android11
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            DisplayMetrics metrics = new DisplayMetrics();
            Display display = context.getDisplay();
            display.getRealMetrics(metrics);
            return metrics.densityDpi;
        }
        return getSystemDensityDpi();
    }


    public static float getFontScale(Context context) {
        return context.getResources().getConfiguration().fontScale;

    }

}
