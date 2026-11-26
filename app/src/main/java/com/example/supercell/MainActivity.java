package com.example.supercell;


import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentManager;

import android.accessibilityservice.AccessibilityService;
import android.annotation.SuppressLint;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.graphics.Color;
import android.graphics.Point;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

import android.provider.MediaStore;
import android.provider.Settings;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.content.Context;
import android.graphics.PixelFormat;
import android.view.Gravity;
import android.content.Intent;
import android.widget.LinearLayout;

import android.widget.Toast;

import com.example.supercell.databinding.ActivityMainBinding;

import android.graphics.Bitmap;
import android.view.Surface;

import com.example.supercell.databinding.FloatSupercellFunctionAreaBinding;
import com.example.supercell.databinding.FloatLayoutImagePreviewBinding;
import com.example.supercell.databinding.FloatLayoutOverlayBinding;
import com.example.supercell.databinding.FloatLayoutTerminalBinding;
import com.example.supercell.databinding.FloatMainFunctionAreaBinding;
import com.example.supercell.databinding.FloatSupervellFuncViceAreaBinding;
import com.example.supercell.databinding.FloatViceFunctionAreaBinding;
import com.google.android.flexbox.FlexboxLayout;


public class MainActivity extends AppCompatActivity {
    // Used to load the 'supercell' library on application startup.
    static {
        System.loadLibrary("supercell");
//        System.loadLibrary("EGL");
//        System.loadLibrary("GLESv2");
//        System.loadLibrary("GLESv3");
    }

    private ActivityMainBinding binding;
    private FloatLayoutTerminalBinding floatBinding;
    private FloatLayoutOverlayBinding overlayBinding;
    private FloatLayoutImagePreviewBinding imagePreviewBinding;
    protected FloatMainFunctionAreaBinding floatMainFunctionBinding;
    private FloatViceFunctionAreaBinding floatViceFunctionBinding;
    private FloatSupercellFunctionAreaBinding floatSupercellFunctionBinding;
    private FloatSupervellFuncViceAreaBinding floatSupervellFuncViceAreaBinding;
    private FloatLayoutTerminal floatImagePreview;
    private FloatLayoutTerminal floatLayoutTerminal;
    private FlexboxLayout floatMainFunction;
    private FlexboxLayout floatViceFunction;
    private FlexboxLayout floatSupercellFunction;
    private FlexboxLayout floatSupercellFuncVice;
    private WindowManager windowManager;
    private WindowManager.LayoutParams layoutParams;
    private WindowManager.LayoutParams overlayLayoutParams;
    private WindowManager.LayoutParams imagePreviewLayoutParams;
    private OverlaySelectCoordinate overlay;
    private MediaProjection mediaProjection;
    private PermissionHelper permissionHelper;
    //  主线程（UI线程）句柄可用于切换到UI线程更新UI
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ExecutorService executorService = Executors.newSingleThreadExecutor();
    private final FragmentManager fragmentManager = getSupportFragmentManager();
    private ServiceConnection serviceConnection;
    private Surface surface;
    private ImageReader imageReader;
    private VirtualDisplay virtualDisplay;
    private ByteBuffer rgbaBuffer;
    private volatile FrameBufferInfo frameBufferInfo;
    public byte[] framePixelRow;
    private String ssaid;
    private SharedViewModel viewModel;
    private BlockingQueue<Command> commandQueue;
    private Boolean isRoot = false;
    private Boolean jniBooted = false;
    private final AtomicBoolean secretaryEnable = new AtomicBoolean(false);
    private final AtomicBoolean expeditionEnable = new AtomicBoolean(false);
    private final AtomicBoolean uniqueThread = new AtomicBoolean(false);
    private boolean keepAlive = false;
    private boolean assemblyFilter = false;
    private boolean armamentsSingle = false;
    private boolean win2Stars = false;
    protected boolean bootGame = false;

    private CheckConfig preCheck;
    private long gMainLoop = 1000;


    public enum Command {
        SONAR, STOP_SONAR, CARVE_ATTR, CLANS, STOP_CLANS, BEACH, STOP_BEACH, STOP_EGGS, EGGS, STOP_ASSEMBLY, ASSEMBLY, STOP_TREASURE, TREASURE, STOP_ARMAMENTS, ARMAMENTS, STOP_SECRETARY, SECRETARY, TRACKED_KEEP_ALIVE, SAVE_FRAME_RAW, SKIP_PLACARD, ADJUST_PARAMS, EXIT, INIT, MAIN
    }

    public enum Coordinate {
        ENTER, APPLY, CANCEL
    }

    public enum Secretary {
        SCIENCE, CONSTRUCT
    }

    public enum Mode {
        NORMAL(0x1), ZOMBIE(0x2), NONE(0x0);
        private final int value;

        Mode(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    public enum Crystal {
        LIFE(0x0), ICE(0x1), LAVA(0x2), DARK(0x3);
        private final int offsetRatio;

        Crystal(int ratio) {
            this.offsetRatio = ratio;
        }

        public int getRatio() {
            return offsetRatio;
        }
    }


    private static final Mode mode = Mode.ZOMBIE;
    private static Crystal crystal = Crystal.LIFE;
    private static Secretary secretary = Secretary.CONSTRUCT;
    //    LinkedHashMap保证插入顺序
    private final Map<Coordinate, float[]> coordinates = new LinkedHashMap<>();
    private AudioPlayer audioPlayer;
    private int widthWindow;
    private int heightWindow;
    private int widthDisplay;
    private int heightDisplay;
    private ContentObserver accessibilityObserver;

    private final int samplePoint = 5;

    private int ability_value;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ScreenUtils.realSizeCurrentWindow(this);
        ScreenUtils.realSizeMaximumWindow(this);
//        需重构
        ScreenUtils.template(this);
        AssetCopier.copyAssetsToFiles(this);
//        AssetCopier.release_assert(this);
        RotationManager.getInstance().registerOrientationListener(MainActivity.this);
        viewModel = ((CowApplication) getApplication()).getSharedViewModel();
        widthWindow = ScreenUtils.getFullCurrentWindowWidth();
        heightWindow = ScreenUtils.getFullCurrentWindowHeight();
        widthDisplay = ScreenUtils.getFullMaximumWindowWidth();
        heightDisplay = ScreenUtils.getFullMaximumWindowHeight();
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        floatBinding = FloatLayoutTerminalBinding.inflate(getLayoutInflater());
        floatLayoutTerminal = floatBinding.getRoot();
        overlayBinding = FloatLayoutOverlayBinding.inflate(getLayoutInflater());
        imagePreviewBinding = FloatLayoutImagePreviewBinding.inflate(getLayoutInflater());
        floatImagePreview = imagePreviewBinding.getRoot();
        floatMainFunctionBinding = FloatMainFunctionAreaBinding.inflate(getLayoutInflater());
        floatViceFunctionBinding = FloatViceFunctionAreaBinding.inflate(getLayoutInflater());
        floatSupercellFunctionBinding = FloatSupercellFunctionAreaBinding.inflate(getLayoutInflater());
        floatSupervellFuncViceAreaBinding = FloatSupervellFuncViceAreaBinding.inflate(getLayoutInflater());
        floatMainFunction = floatMainFunctionBinding.getRoot();
        floatViceFunction = floatViceFunctionBinding.getRoot();
        floatSupercellFunction = floatSupercellFunctionBinding.getRoot();
        floatSupercellFuncVice = floatSupervellFuncViceAreaBinding.getRoot();
        commandQueue = new LinkedBlockingQueue<>();
        floatLayoutTerminal.addView(floatSupercellFunction);
//        setContentView(binding.getRoot());
//        系统UI可见性
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(MiscUtils.UIOptions);
        initUsageState();
        initUniqueSSAID();
        initUserEnvironment();
        initPermissionHelper();
        initPermissionsStatus();
        initRequiredComponents();
        initLoadPreferences();
        executorService.execute(() -> {
            while (true) {
                try {
                    Command cmd = commandQueue.take();
                    if (!checkPermissionStatus() && !Command.EXIT.equals(cmd)) {
                        updateMessage(getString(R.string.insufficient_permissions_overlay_attention));
                        continue;
                    }
                    switch (cmd) {
                        case INIT:
                            int w = imageReader.getWidth();
                            int h = imageReader.getHeight();
//                            if (isRoot || checkConfigLoaded()) {
                            initJNIThread(w, h, isRoot, ssaid, AssetCopier.privateStorage, AssetCopier.carvePaths);
//                            }
                            break;
                        case MAIN:
                            mainOpenCvLastWar(mode.getValue(), assemblyFilter);
                            break;
                        case EGGS:
                            bootAutoEggs();
                            break;
                        case ASSEMBLY:
                            bootAutoAssembly();
                            break;
                        case TREASURE:
                            bootTreasureDigs();
                            break;
                        case ARMAMENTS:
                            bootAutoArmaments(armamentsSingle);
                            break;
                        case SAVE_FRAME_RAW:
//                            saveFramePicture();
                            saveSonarTable();
                            break;
                        case STOP_ASSEMBLY:
                            stopAutoAssembly();
                            break;
                        case STOP_TREASURE:
                            stopTreasureDigs();
                            break;
                        case STOP_ARMAMENTS:
                            stopAutoArmaments();
                            break;
                        case STOP_EGGS:
                            stopAutoEggs();
                            break;
                        case SKIP_PLACARD:
                            skipGamePlacard();
                            break;
                        case ADJUST_PARAMS:
                            if (frameBufferInfo != null) {
                                syncJNIArgs(gMainLoop, frameBufferInfo.rowStride);
                            }
                            break;
                        case TRACKED_KEEP_ALIVE:
                            trackedKeepAlive();
                            break;
                        case CLANS:
                            tessOcrRecognize(ability_value);
//                            bootClansLoop(win2Stars);
                            break;
                        case STOP_CLANS:
                            tessOcrDetach();
//                            stopClansLoop();
                            break;
                        case BEACH:
                            bootBeachLoop(crystal.getRatio());
                            break;
                        case STOP_BEACH:
                            MiscUtils.backMainActivity(this);
                            stopBeachLoop();
                            break;
                        case CARVE_ATTR:
                            exportCarveAttr();
                            break;
                        case SONAR:
                            bootSonarLoop(samplePoint);
                            break;
                        case STOP_SONAR:
                            stopSonarLoop();
                            break;
                        case EXIT:
                            resetThreadStatus();
                            break;
                    }
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        });
        bindingListenerSetting();
        viewModel.getCommand().observe(this, data -> {
            switch (data) {
                case "jmBooted":
                    this.processHandler();
                    floatMainFunctionBinding.floatSwitchDigs.setChecked(true);
                    break;
                default:
                    break;
            }
        });
        viewModel.getFrameBufferInfo().observe(this, info -> {
            if (info != null) {
                frameBufferInfo = info;
                Log.d("TAG", frameBufferInfo.toString());
                binding.launchProcess.setEnabled(true);
            }
        });

    }

    private void registerAccessibilityObserver() {
        if (accessibilityObserver != null) return;
        accessibilityObserver = new ContentObserver(handler) {
            @Override
            public void onChange(boolean selfChange) {
                if (permissionHelper.isAccessibilityServiceEnabled(MainActivity.this, ClickAccessibilityService.class)) {
//                    解决在Setting页activity被pause,无法回到MainActivity
                    Intent intent = new Intent(MainActivity.this, MainActivity.class);
                    MainActivity.this.startActivity(intent);
                    unregisterAccessibilityObserver();
                }
            }
        };
        getContentResolver().registerContentObserver(Settings.Secure.getUriFor(Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES), false, accessibilityObserver);
    }

    private void unregisterAccessibilityObserver() {
        if (accessibilityObserver != null) {
            getContentResolver().unregisterContentObserver(accessibilityObserver);
            accessibilityObserver = null;
        }
    }


    public void InitCaptureService(int resultCode, Intent data) {
        this.startForegroundService(new Intent(this, ScreenCaptureService.class));
        serviceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder binder) {
//              binder获取Service
                ScreenCaptureService screenCaptureService = ((ScreenCaptureService.CaptureBinder) binder).getService();
                screenCaptureService.setFrameBufferCallback(info -> {
                    viewModel.setFrameBufferInfo(info);
                });
                screenCaptureService.InitProjection(resultCode, data);
                screenCaptureService.InitVirtualDisplay(MainActivity.this, handler);
                // 访问 imageReader、surface、virtualDisplay
                surface = screenCaptureService.getSurface();
                imageReader = screenCaptureService.getImageReader();
                virtualDisplay = screenCaptureService.getVirtualDisplay();
                mediaProjection = screenCaptureService.getMediaProjection();
//                闪退点
//                InitFrameBuffer();
                if (virtualDisplay != null) {
                    commandQueue.offer(Command.INIT);
                    authorizationPendingMediaProjection(R.string.status_authorized_0x3, Color.GREEN, true);
                    RotationManager.getInstance().registerOrientationListener(MainActivity.this);
                    Toast.makeText(MainActivity.this, getString(R.string.virtual_display_init), Toast.LENGTH_SHORT).show();
                }
            }

            //unbindService不会触发onServiceDisconnected
            @Override
            public void onServiceDisconnected(ComponentName name) {
                Log.d("TAG", "Service Disconnected by Exception");
            }
        };
    }

    public void initUsageState() {
//        静态工厂方法加载
//        LayoutInflater.from(context).inflate(R.layout.xxx, parent, false);
//        仅在activity
//        AlertDialog loadDialog = MiscUtils.infoDialog(this, getLayoutInflater().inflate(R.layout.loading_config_dialog, null));
        setContentView(binding.getRoot());
        CheckConfigFetcher fetcher = new CheckConfigFetcher();
        fetcher.fetchConfig(getString(R.string.net_config_url), new CheckConfigFetcher.OnConfigFetchedListener() {
            @Override
            public void onSuccess(CheckConfig config) {
                handler.post(() -> {
//                    loadDialog.dismiss();
//                    setContentView(binding.getRoot());
                    if (config != null) {
                        preCheck = config;
                        gMainLoop = config.MainLoop;
                        CheckConfig.Dialog dconfig = config.dialog;
//                        if (dconfig.show)
//                            showDialog(dconfig.title, dconfig.message, dconfig.positiveButton, dconfig.negativeButton, null, null);
                    }
                    checkUsageState();
                });
            }

            @Override
            public void onFailure(Exception e) {
//                loadDialog.dismiss();
                /*showDialog(
                        getString(R.string.alert_dialog_check_network),
                        getString(R.string.alert_dialog_message_network),
                        getString(R.string.alert_dialog_positive_network),
                        getString(R.string.alert_dialog_negative_network),
                        new CheckDialogFragment.DialogCallback() {
                            @Override
                            public void onConfirm() {
                                initUsageState();
                            }

                            @Override
                            public void onCancel() {
                                terminate();
                            }
                        }, null);*/
            }
        });
    }

    public boolean checkAvailable() {
        if (preCheck != null && preCheck.UsageStatus != 1) {
            showDialog(
                    getString(R.string.alert_dialog_check_maintenance),
                    getString(R.string.alert_dialog_message_maintenance),
                    getString(R.string.alert_dialog_positive_maintenance),
                    getString(R.string.alert_dialog_negative_maintenance),
                    new CheckDialogFragment.DialogCallback() {
                        @Override
                        public void onConfirm() {
                            terminate();
                        }
                    }, null);
            return false;
        }
        return true;
    }

    public boolean checkUpdate() {
        long versionCode = MiscUtils.getAppVersionCode(this);
        if (preCheck != null && preCheck.VersionCode > versionCode) {
            showDialog(
                    getString(R.string.alert_dialog_check_upgrade),
                    getString(R.string.alert_dialog_message_upgrade),
                    getString(R.string.alert_dialog_positive_upgrade),
                    getString(R.string.alert_dialog_negative_upgrade),
                    new CheckDialogFragment.DialogCallback() {
                        @Override
                        public void onConfirm() {
                            MiscUtils.openURLToWebView(MainActivity.this, preCheck.UpdateURL);
                        }
                    }, null);
            return true;
        }
        return false;
    }

    public boolean checkConfigLoaded() {
        if (preCheck == null) {
            showDialog(
                    getString(R.string.alert_dialog_check_verify),
                    getString(R.string.alert_dialog_message_verify),
                    getString(R.string.alert_dialog_positive_verify),
                    getString(R.string.alert_dialog_negative_verify),
                    new CheckDialogFragment.DialogCallback() {
                        @Override
                        public void onConfirm() {
                            initUsageState();
                        }
                    }, null);
            return false;
        }
        return true;
    }

    public void showDialog(String check, String message, String positive, String negative, CheckDialogFragment.DialogCallback callback, String tag) {
//        tag 参数:防止重复show时创建多次Dialog,被依赖的额activity销毁或重构时重新创建Fragment
        CheckDialogFragment.newInstance(check, message, positive, negative, callback).show(fragmentManager, tag);
    }

    public void showDialog(String check, String message, String positive, String negative) {
        CheckDialogFragment.newInstance(check, message, positive, negative, null).show(fragmentManager, null);
    }

    public boolean checkUsageState() {
        return isRoot || (checkConfigLoaded() && !checkUpdate() && checkAvailable());
    }


    public void createImagePreviewLayoutParams(int width, int height, int offsetX, int offsetY) {
        imagePreviewLayoutParams = new WindowManager.LayoutParams(width, height, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, PixelFormat.TRANSPARENT);
        imagePreviewLayoutParams.gravity = Gravity.TOP | Gravity.START;
        imagePreviewLayoutParams.x = offsetX;
        imagePreviewLayoutParams.y = offsetY;
    }

    public void createOverlayLayoutParams(int width, int height, int offsetX, int offsetY) {
        overlayLayoutParams = new WindowManager.LayoutParams(width, height, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, PixelFormat.TRANSPARENT);
        overlayLayoutParams.gravity = Gravity.TOP | Gravity.START;
        overlayLayoutParams.x = offsetX;
        overlayLayoutParams.y = offsetY;
    }

    private void createFloatWindow(int width, int height, int offsetX, int offsetY) {
        windowManager = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
        layoutParams = new WindowManager.LayoutParams(
//                容器大小由组件自身决定
//                WindowManager.LayoutParams.WRAP_CONTENT,
//                WindowManager.LayoutParams.WRAP_CONTENT,
                width, height,
//                窗口优先级
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
//                额外的行为
//                | WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
//                窗口的透明度
                PixelFormat.TRANSLUCENT);
        // 基于父容器对齐方式
        layoutParams.gravity = Gravity.TOP | Gravity.START;
        //偏移
        layoutParams.x = offsetX;
        layoutParams.y = offsetY;
    }

    @SuppressLint("ClickableViewAccessibility")
    public void setOverlayWindow(Coordinate coordinate) {
        if (overlayBinding.getRoot().getParent() == null) {
            MiscUtils.bootGameWithUser(this, 0, isRoot);
            AtomicBoolean isClick = new AtomicBoolean(false);
            windowManager.addView(overlayBinding.getRoot(), overlayLayoutParams);
            overlayBinding.floatOverlayImage.setTag(coordinate);
            AtomicReference<Float> interceptDownX = new AtomicReference<>(0f);
            AtomicReference<Float> interceptDownY = new AtomicReference<>(0f);
            overlayBinding.floatOverlayImage.setOnTouchListener((v, event) -> {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        isClick.set(true);
                        interceptDownX.set(event.getX());
                        interceptDownY.set(event.getY());
//                        单独初始化父控件起始坐标
                        ((LinearLayout) v.getParent()).onTouchEvent(event);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        float dx = Math.abs(event.getX() - interceptDownX.get());
                        float dy = Math.abs(event.getY() - interceptDownY.get());
//                        Log.d("TAG", String.valueOf(event.getX()));
//                        同步移动
                        ((LinearLayout) v.getParent()).onTouchEvent(event);
                        if (dx > MiscUtils.TOUCH_SLOP || dy > MiscUtils.TOUCH_SLOP) {
                            isClick.set(false);
                        }
                        break;
                    case MotionEvent.ACTION_UP:
                        if (isClick.get()) {
                            windowManager.removeView(overlayBinding.getRoot());
                            overlay.assignCoordinatesByOverlay(coordinate);
                            v.setOnTouchListener(null);
                        }
                        break;
                }
//                如果子view不处理OnTouchEvent则传递给父级OnTouchEvent,而不是父级dispatchTouchEvent
                return true;
            });
        }
    }

    public void bootSecretaryApply() {
        switch (secretary) {
            case CONSTRUCT:
                updateMessage(getString(R.string.secretary_science));
                break;
            case SCIENCE:
                updateMessage(getString(R.string.secretary_construct));
                break;
        }
        if (secretaryEnable.get()) return;
        secretaryEnable.set(true);
        Thread workerThread = new Thread(() -> {
            if (coordinates.size() != 3) {
                updateMessage(getString(R.string.coordinate_configuration_overlay_attention));
                return;
            }
            if (uniqueThread.get()) {
                updateMessage(getString(R.string.single_thread_overlay_attention));
                return;
            }
            uniqueThread.set(true);
            while (secretaryEnable.get()) {
                for (float[] point : coordinates.values()) {
                    clickByPoint(point[0], point[1]);
                    MiscUtils.safeSleep(400);
                }
            }
            uniqueThread.set(false);
        });
        workerThread.start();
    }

    public void stopSecretaryApply() {
//        方案二:判断线程是否完全停止
        if (!secretaryEnable.get()) return;
        secretaryEnable.set(false);
//        if (workerThread != null) {
//            workerThread.interrupt();
//        }
    }

    public void bootUnmountEquipment() {
        if (expeditionEnable.get()) return;
        expeditionEnable.set(true);
        Thread workerThread = new Thread(() -> {
            if (uniqueThread.get()) {
                updateMessage(getString(R.string.single_thread_overlay_attention));
                return;
            }
            uniqueThread.set(true);
            while (expeditionEnable.get()) {
                for (Point equipmentSlot : ExpeditionPoints.EquipmentSlots) {
                    clickByPoint(equipmentSlot);
                    MiscUtils.safeSleep(MiscUtils.UE_delay);
                    clickByPoint(ExpeditionPoints.unmount);
                    MiscUtils.safeSleep(MiscUtils.UE_delay);
                }
                clickByPoint(ExpeditionPoints.nextHero);
                MiscUtils.safeSleep(MiscUtils.UE_delay);
            }
            uniqueThread.set(false);
        });
        workerThread.start();
    }

    public void stopUnmountEquipment() {
        if (!expeditionEnable.get()) return;
        expeditionEnable.set(false);
    }


    public void initLoadPreferences() {
        for (Coordinate coordinate : Coordinate.values()) {
            String value = CowApplication.preferencesConfig.getString(secretary.name() + coordinate.name(), null);
            if (value != null) {
                switch (coordinate) {
                    case ENTER:
                        binding.secretaryEnter.setText(value);
                        break;
                    case APPLY:
                        binding.secretaryApply.setText(value);
                        break;
                    case CANCEL:
                        binding.secretaryCancel.setText(value);
                        break;
                }
                coordinates.put(coordinate, MiscUtils.toFloatArray(value.split(",")));
            }
        }
    }

    public void initRequiredComponents() {
        createOverlayLayoutParams(dpToPx(48), dpToPx(48), 100, 200);
        createFloatWindow(dpToPx(200), dpToPx(200), widthWindow / 2, 20);
        createImagePreviewLayoutParams(dpToPx(300), dpToPx(300), 0, 0);
        audioPlayer = new AudioPlayer(this);
        overlay = new OverlaySelectCoordinate(this, windowManager);
        overlay.setOnCoordinateSelectedListener((x, y, coordinate) -> {
            SharedPreferences.Editor editor = CowApplication.preferencesConfig.edit();
            StringBuilder sb = new StringBuilder();
            sb.append(x).append(",").append(y);
            float[] overlay_coordinate = new float[]{x, y};
            switch (coordinate) {
                case ENTER:
                    binding.secretaryEnter.setText(sb.toString());
                    break;
                case APPLY:
                    binding.secretaryApply.setText(sb.toString());
                    break;
                case CANCEL:
                    binding.secretaryCancel.setText(sb.toString());
                    break;
            }
            coordinates.put(coordinate, overlay_coordinate);
            editor.putString(secretary.name() + coordinate.name(), sb.toString());
            editor.apply();
            MiscUtils.backMainActivity(MainActivity.this);
        });
    }


    public void initPermissionHelper() {
        MiscUtils.openAppForLauncher(this, MiscUtils.su_manager);
        if (isRoot) MiscUtils.opsAuthorization();
        permissionHelper = new PermissionHelper(this, new PermissionHelper.PermissionCallback() {
            @Override
            public void onOverlayGranted() {
                authorizationPendingOverlays(R.string.status_authorized_0x1, Color.GREEN, true);
                Toast.makeText(MainActivity.this, R.string.system_alert_window_allowed, Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onProjectionGranted(int resultCode, Intent data) {
                Toast.makeText(MainActivity.this, R.string.projection_media_allowed, Toast.LENGTH_SHORT).show();
                //启动前台服务和创建serviceConnect监听
                InitCaptureService(resultCode, data);
                //异步绑定
                //必须InitProjection之前启动前台服务和通知
                //此处会触发onServiceConnected
                bindService(new Intent(MainActivity.this, ScreenCaptureService.class), serviceConnection, Context.BIND_AUTO_CREATE);
            }

            @Override
            public void onAccessibilityServiceGranted(int resultCode, Intent data) {
                Toast.makeText(MainActivity.this, R.string.accessibility_service_enable, Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onManageAllStorageGranted(int resultCode, Intent data) {
                Toast.makeText(MainActivity.this, R.string.manage_external_storage_allowed, Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onPermissionDenied(String reason) {
                checkPermissionStatus();
                Toast.makeText(MainActivity.this, reason, Toast.LENGTH_SHORT).show();
                binding.launchProcess.setEnabled(true);
            }
        });
        permissionHelper.requestRuntimePostNotifications();
        permissionHelper.requestManageAllStoragePermission();
    }

    @SuppressLint("HardwareIds")
    public void initUniqueSSAID() {
        ssaid = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);
        String format = String.format("(%s)", ssaid);
        binding.userUniqueId.setText(format);
        binding.userUniqueId.setOnClickListener(v -> {
            ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
            ClipData clip = ClipData.newPlainText("label", format);
            clipboard.setPrimaryClip(clip);
            Toast.makeText(this, R.string.already_copy, Toast.LENGTH_SHORT).show();
        });
        if (!new File(new File(AssetCopier.privateStorage, "image"), ssaid).exists()) {
            ssaid = "Normal";
        }
    }

    public void bindingListenerSetting() {
        floatBinding.floatFunctionArea.setOnClickListener(v -> {
            if (floatLayoutTerminal.indexOfChild(floatSupercellFunction) != -1) {
                floatLayoutTerminal.removeView(floatSupercellFunction);
                floatLayoutTerminal.addView(floatSupercellFuncVice);
                floatBinding.floatFunctionArea.setText(R.string.float_area_main);
            } else if (floatLayoutTerminal.indexOfChild(floatSupercellFuncVice) != -1) {
                floatLayoutTerminal.removeView(floatSupercellFuncVice);
                floatLayoutTerminal.addView(floatSupercellFunction);
                floatBinding.floatFunctionArea.setText(R.string.float_area_vice);
            }
        });
        JNIBootBind(floatMainFunctionBinding.floatSwitchTeam, Command.ASSEMBLY, Command.STOP_ASSEMBLY);
        JNIBootBind(floatMainFunctionBinding.floatSwitchDigs, Command.TREASURE, Command.STOP_TREASURE);
        JNIBootBind(floatViceFunctionBinding.floatSwitchEggs, Command.EGGS, Command.STOP_EGGS);
        floatBinding.floatExitAll.setOnClickListener((v) -> processExit());
        floatMainFunctionBinding.floatSwitchSecretary.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                bootSecretaryApply();
            } else {
                stopSecretaryApply();
            }
        });
        floatViceFunctionBinding.floatSwitchExpedition.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                bootUnmountEquipment();
            } else {
                stopUnmountEquipment();
            }
        });
        floatMainFunctionBinding.floatSwitchArms.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                commandQueue.offer(Command.ARMAMENTS);
            } else {
                commandQueue.offer(Command.STOP_ARMAMENTS);
            }
        });
        floatViceFunctionBinding.floatSwitchFrame.setOnCheckedChangeListener((checkView, enable) -> {
            if (permissionHelper.isExternalStorageManagerEnable()) {
                if (enable) {
                    commandQueue.offer(Command.SAVE_FRAME_RAW);
                    checkView.setChecked(false);
                    return;
                }
            }
            Toast.makeText(MainActivity.this, R.string.manage_external_storage_denied, Toast.LENGTH_LONG).show();
        });
        floatViceFunctionBinding.floatSwitchHide.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                floatLayoutTerminal.detachFromWindow();
                handler.postDelayed(() -> {
                    floatLayoutTerminal.attachToWindow(layoutParams);
                    checkView.setChecked(false);
                }, 3000);
            }
        });

        floatSupercellFunctionBinding.floatSwitchBeachBlock.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                MiscUtils.iptables_drop_packet(MiscUtils.boom_beach_pkg);
            } else {
                MiscUtils.iptables_clear_table();
            }
        });
        floatSupercellFunctionBinding.floatSwitchBeachReboot.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
               /* Thread wokerThread = new Thread(() -> {
                    Image image = imageReader.acquireNextImage();
                    Uri Rp = extractAndSaveToPictures(this, image);
                    handler.post(() -> {
                        imagePreviewBinding.floatImageView.setImageURI(Rp);
                        floatImagePreview.attachToWindow(imagePreviewLayoutParams);
                    });
                });
                wokerThread.start();*/
                MiscUtils.userSpaceGameBoot(0);
//                return;
            }
//            floatImagePreview.detachFromWindow();
            checkView.setChecked(false);
        });
        floatSupercellFunctionBinding.floatSwitchClansLoop.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
//                tessOcrRecognize();
                commandQueue.offer(Command.CLANS);
            } else {
//                tessOcrDetach();
                commandQueue.offer(Command.STOP_CLANS);
            }
        });
        floatSupercellFunctionBinding.floatSwitchBeachLoop.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                MiscUtils.iptables_drop_packet(MiscUtils.boom_beach_pkg);
                commandQueue.offer(Command.BEACH);
            } else {
                commandQueue.offer(Command.STOP_BEACH);
            }
//            MiscUtils.resolveNetwork();
        });
        floatSupervellFuncViceAreaBinding.floatSwitchBeachSonarSimulation.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                commandQueue.offer(Command.SONAR);
            } else {
                commandQueue.offer(Command.STOP_SONAR);
            }
        });
        floatSupercellFunctionBinding.floatSwitchBeachSvcWifi.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                checkView.setChecked(false);
//                Thread内异步
                MiscUtils.reset_sockets_by_foreground_packages(this).thenRunAsync(() -> {
                    updateMessage("ss -tunK Done!");
                    floatSupercellFunctionBinding.floatSwitchBeachBlock.setChecked(false);
                }, getMainExecutor());
            }
        });
        floatSupervellFuncViceAreaBinding.floatSwitchBeachSvcWifi.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                checkView.setChecked(false);
                MiscUtils.reset_sockets_by_foreground_packages(this);
            }
        });
        floatSupervellFuncViceAreaBinding.floatSwitchBeachBlockCopy.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                floatSupercellFunctionBinding.floatSwitchBeachBlock.setChecked(true);
            } else {
                floatSupercellFunctionBinding.floatSwitchBeachBlock.setChecked(false);
                floatSupervellFuncViceAreaBinding.floatSwitchBeachSvcWifi.setChecked(false);

            }
        });
        floatSupervellFuncViceAreaBinding.floatSwitchSonarFrame.setOnCheckedChangeListener((checkView, enable) -> {
            commandQueue.offer(Command.SAVE_FRAME_RAW);
            checkView.setChecked(false);
            Toast.makeText(this, getString(R.string.save_frame_complete), Toast.LENGTH_SHORT).show();

        });
        floatSupervellFuncViceAreaBinding.floatSwitchBeachSonarMerge.setOnCheckedChangeListener((checkView, enable) -> {
            mergeSonarTable();
            checkView.setChecked(false);
            Toast.makeText(this, getString(R.string.merge_frame_complete), Toast.LENGTH_SHORT).show();
        });


        binding.secretaryEnter.setOnClickListener(v -> {
            setOverlayWindow(Coordinate.ENTER);
        });
        binding.secretaryApply.setOnClickListener(v -> {
            setOverlayWindow(Coordinate.APPLY);
        });
        binding.secretaryCancel.setOnClickListener(v -> {
            setOverlayWindow(Coordinate.CANCEL);
        });
        binding.checkboxBeachGame.setOnCheckedChangeListener((compound, checked) -> {
            if (checked) {

            }
            floatLayoutTerminal.removeView(floatSupercellFunction);
            floatLayoutTerminal.addView(floatMainFunction);
            processExit();
        });
        binding.checkboxSingleArmaments.setOnCheckedChangeListener((compound, checked) -> {
            armamentsSingle = checked;
        });
        binding.checkboxBootBehavior.setOnCheckedChangeListener((compound, checked) -> {
//            bootGame = checked;
        });
//        layout_gravity 可以同时控制水平垂直方向对齐方式right属性仅对父级orientation是vertical时生效
        binding.checkboxAssemblyFilter.setOnCheckedChangeListener((compound, checked) -> {
            win2Stars = checked;
            assemblyFilter = checked;
        });
        binding.checkboxKeepAlive.setOnCheckedChangeListener((compound, checked) -> {
            keepAlive = checked;
        });
        binding.radioGroupSecretary.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.radio_construct_secretary) {
                secretary = Secretary.CONSTRUCT;
            }
            if (checkedId == R.id.radio_science_secretary) {
                secretary = Secretary.SCIENCE;
            }
            initLoadPreferences();
        });
        binding.authorize0x1.setOnClickListener(v -> {
            processExit();
            permissionHelper.requestOverlayPermission();
        });
        binding.authorize0x2.setOnClickListener(v -> {
            processExit();
            registerAccessibilityObserver();
            permissionHelper.requestAccessibilityServicePermission();
        });
        binding.authorize0x3.setOnClickListener(v -> {
            processExit();
            binding.launchProcess.setEnabled(false);
            permissionHelper.requestMediaProjectionPermission();
        });
        binding.radioGroupMode.setOnCheckedChangeListener((group, checkedId) -> {
//            switch (checkedId){
//               非常量表达式
//                case R.id.radio_normal_mode:
//
//            }
            if (checkedId == R.id.radio_engraving_life) {
                crystal = Crystal.LIFE;
            }
            if (checkedId == R.id.radio_engraving_ice) {
                crystal = Crystal.ICE;
            }
            if (checkedId == R.id.radio_engraving_lava) {
                crystal = Crystal.LAVA;
            }
            if (checkedId == R.id.radio_engraving_dark) {
                crystal = Crystal.DARK;
            }
            MiscUtils.userSpaceGameBoot(0);
        });

        binding.launchProcess.setOnClickListener(v -> {
            processHandler();
        });
        binding.finishProcess.setOnClickListener(v -> {
            terminate();
        });
        binding.carveMerge.setOnClickListener(v -> {
            commandQueue.offer(Command.CARVE_ATTR);
            MiscUtils.openAppForLauncher(this, MiscUtils.gallery);
        });
        binding.usageDocumentation.setOnClickListener(v -> {
            String url = v.getContext().getString(R.string.usage_documentation_url);
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            v.getContext().startActivity(intent);
        });
        binding.sliderSpeedSelection.addOnChangeListener((slider, speed, fromUser) -> {
           /* if (speed < MiscUtils.minMainLoop) {
                slider.setValue(MiscUtils.minMainLoop);
                ThrottleUtils.runThrottled("slider_speed_dialog", 1500, () -> {
                    showDialog(
                            getString(R.string.alert_dialog_check_slider),
                            getString(R.string.alert_dialog_message_slider),
                            getString(R.string.alert_dialog_positive_slider),
                            getString(R.string.alert_dialog_negative_slider)
                    );
                });
            } else {
                binding.sliderLabel.setText(String.valueOf((long) speed));
                gMainLoop = (long) speed;
            }*/
            binding.sliderLabel.setText(String.valueOf((int) speed));
            ability_value = (int) speed;
        });
    }

    private void JNIBootBind(SwitchCompat switchCompat, Command boot, Command stop) {
        switchCompat.setOnCheckedChangeListener((checkView, enable) -> {
            if (enable) {
                commandQueue.offer(boot);
                if (!jniBooted) {
                    jniBooted = true;
                    commandQueue.offer(Command.MAIN);
                }
                return;
            }
            resetJNIBoot();
            commandQueue.offer(stop);
        });
    }

    private void resetJNIBoot() {
        if (!floatMainFunctionBinding.floatSwitchTeam.isChecked() && !floatMainFunctionBinding.floatSwitchDigs.isChecked() && !floatViceFunctionBinding.floatSwitchEggs.isChecked()) {
            jniBooted = false;
        }
    }

    public void runtimeJNIPreParams() {
        commandQueue.offer(Command.ADJUST_PARAMS);
        if (bootGame) {
            commandQueue.offer(Command.SKIP_PLACARD);
            MiscUtils.safeSleep(1000);
            MiscUtils.bootGameWithUser(this, 0, isRoot);
        }
        if (keepAlive) {
            commandQueue.offer(Command.TRACKED_KEEP_ALIVE);
        }
    }


    public void processExit() {
        if (viewModel.getJmBooted()) {
            viewModel.setJmBooted(false);
            floatMainFunctionBinding.floatSwitchTeam.setChecked(false);
            floatMainFunctionBinding.floatSwitchDigs.setChecked(false);
            floatMainFunctionBinding.floatSwitchSecretary.setChecked(false);
            floatMainFunctionBinding.floatSwitchArms.setChecked(false);
            floatViceFunctionBinding.floatSwitchExpedition.setChecked(false);
            floatViceFunctionBinding.floatSwitchEggs.setChecked(false);
            floatViceFunctionBinding.floatSwitchFrame.setChecked(false);
            floatSupercellFunctionBinding.floatSwitchBeachBlock.setChecked(false);
            floatSupercellFunctionBinding.floatSwitchBeachReboot.setChecked(false);
            floatSupercellFunctionBinding.floatSwitchClansLoop.setChecked(false);
            floatSupercellFunctionBinding.floatSwitchBeachLoop.setChecked(false);
            floatSupervellFuncViceAreaBinding.floatSwitchBeachSonarSimulation.setChecked(false);
            floatSupervellFuncViceAreaBinding.floatSwitchBeachSvcWifi.setChecked(false);
            floatSupervellFuncViceAreaBinding.floatSwitchBeachBlockCopy.setChecked(false);
            floatSupervellFuncViceAreaBinding.floatSwitchSonarFrame.setChecked(false);
            floatSupervellFuncViceAreaBinding.floatSwitchBeachSonarMerge.setChecked(false);
            commandQueue.offer(Command.EXIT);
            MiscUtils.backMainActivity(this);
            floatLayoutTerminal.detachFromWindow();
            launchProcessStatus();
        }
    }

    public void processHandler() {
        if (!viewModel.getJmBooted()) {
            if (checkPermissionStatus()) {
//            if (!checkPermissionStatus() || !checkUsageState()) {
                runtimeJNIPreParams();
            } else {
//                updateMessage(getString(R.string.insufficient_permissions_notify_content));
                MiscUtils.sendNotification(getString(R.string.insufficient_permissions_notify_title), getString(R.string.insufficient_permissions_notify_content));
            }
            viewModel.setJmBooted(true);
            floatLayoutTerminal.attachToWindow(layoutParams);
            updateMessage("X:" + widthWindow + "*" + "Y:" + heightWindow);
            updateMessage("RX:" + widthDisplay + "*" + "RY:" + heightDisplay);
            updateMessage("DPI:" + ScreenUtils.getSystemDensityDpi());
            updateMessage("RDPI:" + ScreenUtils.getMetricsDensityDpi(this));
            updateMessage("FC:" + ScreenUtils.getFontScale(this));
            updateMessage("SPEED:" + gMainLoop);
            launchProcessStatus();
        } else {
            processExit();
        }
    }

    public void terminate() {
        processExit();
        if (serviceConnection != null) {
//          serviceConnection和ScreenCaptureService同步按顺序在授权阶段启动和连接
            unbindService(serviceConnection);
            stopService(new Intent(this, ScreenCaptureService.class));
        }
        executorService.shutdown();
        unregisterAccessibilityObserver();
        RotationManager.getInstance().unregisterOrientationListener();
        ClickAccessibilityService.stop();
        ActivityCompat.finishAffinity(this);
    }

    public void initPermissionsStatus() {
        if (Settings.canDrawOverlays(this)) {
            authorizationPendingOverlays(R.string.status_authorized_0x1, Color.GREEN, true);
        }
        if (permissionHelper.isAccessibilityServiceEnabled(this, ClickAccessibilityService.class)) {
            authorizationPendingAccessibility(R.string.status_authorized_0x2, Color.GREEN, true);
        }
        if (virtualDisplay != null) {
            authorizationPendingMediaProjection(R.string.status_authorized_0x3, Color.GREEN, true);
        }
    }

    public boolean checkPermissionStatus() {
        if (!Settings.canDrawOverlays(this)) {
            authorizationPendingOverlays(R.string.check_permission_status, Color.RED, false);
            return false;
        }
        if (!isRoot && !permissionHelper.isAccessibilityServiceEnabled(this, ClickAccessibilityService.class)) {
            authorizationPendingAccessibility(R.string.check_permission_status, Color.RED, false);
            return false;
        }
        if (virtualDisplay == null) {
            authorizationPendingMediaProjection(R.string.check_permission_status, Color.RED, false);
            return false;
        }
        if (!permissionHelper.isExternalStorageManagerEnable()) {
            return false;
        }
        return true;
    }

    public void initUserEnvironment() {
        if (MiscUtils.checkSuCommand() || MiscUtils.checkSuExists()) {
            binding.userEnvironment.setText(R.string.user_environment_root);
            isRoot = true;
        }
    }

    public void launchProcessStatus() {
        binding.launchProcess.setBackgroundTintList(null);
        if (viewModel.getJmBooted()) {
            binding.launchProcess.setText(R.string.application_end);
            binding.launchProcess.setBackground(Objects.requireNonNull(ContextCompat.getDrawable(this, R.drawable.button_end_rounded)));
            return;
        }
        binding.launchProcess.setText(R.string.application_boot);
        binding.launchProcess.setBackground(Objects.requireNonNull(ContextCompat.getDrawable(this, R.drawable.button_boot_rounded)));
    }

    public void authorizationPendingOverlays(int rid, int color, boolean disable) {
        binding.status0x1.setText(rid);
        binding.status0x1.setTextColor(color);
        binding.authorize0x1.setEnabled(!disable);
    }

    public void authorizationPendingAccessibility(int rid, int color, boolean disable) {
        binding.status0x2.setText(rid);
        binding.status0x2.setTextColor(color);
        binding.authorize0x2.setEnabled(!disable);
    }

    public void authorizationPendingMediaProjection(int rid, int color, boolean disable) {
        binding.status0x3.setText(rid);
        binding.status0x3.setTextColor(color);
        binding.authorize0x3.setEnabled(!disable);
    }

    @Override
    protected void onResume() {
        super.onResume();
        initPermissionsStatus();
        viewModel.setActivityReady(true);
    }

    @Override
    protected void onPause() {
        super.onPause();
        viewModel.setActivityReady(false);
    }
//    main重构配置通知
//    public void onConfigurationChanged(Configuration newConfig){
//        super.onConfigurationChanged(newConfig);
//    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        permissionHelper.handleActivityResult(requestCode, resultCode, data);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PermissionHelper.REQUEST_RUNTIME_CODE_POST_NOTIFICATIONS) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(MainActivity.this, R.string.runtime_post_notifications_denied, Toast.LENGTH_SHORT).show();
            } else {
                permissionHelper.requestRuntimePostNotifications();
                Toast.makeText(MainActivity.this, R.string.runtime_post_notifications_allowed, Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Deprecated
    private void InitFrameBuffer() {
//        setOnImageAvailableListener自身提供handler异步,无需Executor
        ExecutorService imageExecutor = Executors.newSingleThreadExecutor();
        if (imageReader != null) {
            imageReader.setOnImageAvailableListener(reader -> {
//                imageExecutor.execute(() -> {
                try (Image image = reader.acquireLatestImage()) {
                    if (image != null) {
                        int width = image.getWidth();
                        int height = image.getHeight();
                        int pixelStride = image.getPlanes()[0].getPixelStride();
                        int rowStride = image.getPlanes()[0].getRowStride();
                        frameBufferInfo = new FrameBufferInfo(width, height, rowStride, pixelStride);
                        framePixelRow = new byte[rowStride];
                        if (rgbaBuffer == null || rgbaBuffer.capacity() != frameBufferInfo.capacity) {
                            rgbaBuffer = ByteBuffer.allocateDirect(frameBufferInfo.capacity);
                        }
                        imageReader.setOnImageAvailableListener(null, null);
                    }
                } catch (Exception e) {
                    Log.d("TAG", e.toString());
                } finally {
                    imageExecutor.shutdown();
                }
//                });
            }, null);
        }
    }

    public Uri extractAndSaveToPictures(Context context, Image image) {
        Uri Rp = null;
        Image.Plane[] planes = image.getPlanes();
        if (planes.length == 1) {
            ByteBuffer buffer = planes[0].getBuffer();
            int width = image.getWidth();
            int height = image.getHeight();
            int pixelStride = planes[0].getPixelStride();
            int rowStride = planes[0].getRowStride();
            int rowPadding = rowStride - pixelStride * width;
            Bitmap bitmap = Bitmap.createBitmap(width + rowPadding / pixelStride, height, Bitmap.Config.ARGB_8888);
            bitmap.copyPixelsFromBuffer(buffer);
            Bitmap croppedBitmap = Bitmap.createBitmap(bitmap, 0, 0, width, height);
            Rp = saveBitmapToPictures(context, croppedBitmap);
            bitmap.recycle();
            croppedBitmap.recycle();
            image.close();
            return Rp;
        }
        return null;
    }

    @Nullable
    private Uri saveBitmapToPictures(Context context, Bitmap bitmap) {
        Uri uri = null;
        try {
            String fileName = "cow_" + System.currentTimeMillis() + ".png";
            String relativePath = Environment.DIRECTORY_PICTURES + "/lastWar";
            String fullPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + relativePath + "/" + fileName;
            ContentValues values = new ContentValues();
            values.put(MediaStore.Images.Media.DISPLAY_NAME, fileName);
            values.put(MediaStore.Images.Media.MIME_TYPE, "image/png");
            values.put(MediaStore.Images.Media.RELATIVE_PATH, relativePath);
            ContentResolver resolver = context.getContentResolver();
            uri = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
            if (uri != null) {
                try (OutputStream out = resolver.openOutputStream(uri)) {
                    if (out != null) {
                        bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
                        out.flush();
                        return uri;
                    }
                }
            }
        } catch (Exception e) {
            Log.d("TAG", e.toString());
        }
        return uri;
    }

    @Deprecated
    public byte[] extractRGBA_byte(Image image) throws RuntimeException {
        if (image != null) {
            Image.Plane[] planes = image.getPlanes();
            if (planes.length == 0) return null;
            ByteBuffer buffer = planes[0].getBuffer();
            if (buffer != null && buffer.remaining() > 0) {
                int width = image.getWidth();
                int height = image.getHeight();
                int pixelStride = planes[0].getPixelStride();   // 每个像素的字节数，通常是 4（RGBA）
                int rowStride = planes[0].getRowStride();
                byte[] rgba = new byte[width * height * pixelStride];
                byte[] row = new byte[rowStride];
                int offset = 0;
                for (int i = 0; i < height; i++) {
                    buffer.get(row, 0, rowStride);
                    System.arraycopy(row, 0, rgba, offset, width * pixelStride);
                    offset += width * 4;
                }
                return rgba;
            }
        }
        return null;
    }

    @Deprecated
    public void extractRGBA_buffer(Image image) throws RuntimeException {
        if (image != null && rgbaBuffer != null) {
            Image.Plane[] planes = image.getPlanes();
            if (planes.length == 0) return;
            ByteBuffer buffer = planes[0].getBuffer();
            if (buffer != null && buffer.remaining() > 0) {
                rgbaBuffer.clear();
                for (int i = 0; i < frameBufferInfo.height; i++) {
//                    完整像素行
                    buffer.get(framePixelRow, 0, frameBufferInfo.rowStride);
//                    有效像素行
                    rgbaBuffer.put(framePixelRow, 0, frameBufferInfo.rowValid);
                }
                rgbaBuffer.flip();
            }
        }
    }

    @Deprecated
    public ByteBuffer extractRGBA_performance(Image image) throws RuntimeException {
        if (image != null) {
            Image.Plane[] planes = image.getPlanes();
            if (planes.length == 0) return null;
            ByteBuffer buffer = planes[0].getBuffer();
//            image.getHardwareBuffer()
            if (buffer != null && buffer.isDirect() && buffer.remaining() > 0) {
                return buffer;
            }
        }
        return null;
    }

    @Deprecated
    public byte[] captureOnceSync_byte() {
        //依赖帧合成帧器刷新,在系统层面任静止的画面都不会填充新的数据帧
        try (Image image = imageReader.acquireNextImage()) {
            if (image != null) {
                return extractRGBA_byte(image);
            }
        } catch (Exception e) {
            Log.d("TAG", e.toString());
        }
        return null;
    }

    @Deprecated
    public ByteBuffer captureOnceSync_buffer() {
        //自动释放
        try (Image image = imageReader.acquireNextImage()) {
            if (image != null) {
//                extractRGBA_buffer(image);
//                image提前close导致buffer内存释放需在JNI立刻拷贝(不安全)
                return extractRGBA_performance(image);
            }
        } catch (Exception e) {
            Log.d("TAG", e.toString());
        }
//        return ByteBuffer.allocateDirect(0);
        return null;
    }

    public Image captureOnceSync_image() {
        return imageReader.acquireNextImage();
    }

    public int dpToPx(float dp) {
        float density = this.getResources().getDisplayMetrics().density;
        return (int) (dp * density);
    }

    public void updateMessage(final String message) {
//        适合跨类、跨线程地组织逻辑，比如在 Service、子线程、JNI
        handler.post(() -> {
            floatBinding.floatScrollText.append(message + "\n");  // 更新 TextView
            floatBinding.floatScrollView.post(() -> floatBinding.floatScrollView.fullScroll(View.FOCUS_DOWN));
            if (floatBinding.floatScrollText.getText().length() > MiscUtils.maxCharCount) {
                floatBinding.floatScrollText.setText(null);
            }
        });
    }

    public void performClickPoint(float x, float y) {
        handler.post(() -> ClickAccessibilityService.instance.performClick(x, y));
    }

    public void inputClickPoint(float x, float y) {
        try {
            String command = String.format(Locale.US, "input tap  %f %f", x, y);
            new ProcessBuilder("su", "-c", command).start().waitFor();
        } catch (IOException | InterruptedException e) {
            Log.d("TAG", e.toString());
        }
    }

    public void clickByPoint(float x, float y) {
        if (isRoot) {
            inputClickPoint(x, y);
            return;
        }
        performClickPoint(x, y);
    }

    public void clickByPoint(Point point) {
        if (isRoot) {
            inputClickPoint(point.x, point.y);
            return;
        }
        performClickPoint(point.x, point.y);
    }

    public String foregroundApplication() {
        return ClickAccessibilityService.cForegroundPackage;
    }

    public void actionBackAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ClickAccessibilityService.instance.performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
        } else {
            onBackPressed();
        }

    }

    public void playAudioInActivity() {
        audioPlayer.setOnAudioCompletionListener(() -> Toast.makeText(this, R.string.play_audio_done, Toast.LENGTH_SHORT).show());
        audioPlayer.setMaxPlayCount(3);
        audioPlayer.playAudioAsync();
    }

    public void bootGameWithZero() {
        MiscUtils.bootGameWithUser(this, 0, isRoot);
    }

    public void rejectNetworkBeach() {
        MiscUtils.iptables_drop_packet(MiscUtils.boom_beach_pkg);
    }

    public void resumeNetworkBeach() {
        MiscUtils.iptables_clear_table();
    }

    public native void mainOpenCvLastWar(int mode, boolean assembly_filter);

    public native void resetThreadStatus();

    public native void bootTreasureDigs();

    public native void stopTreasureDigs();

    public native void bootAutoAssembly();

    public native void stopAutoAssembly();

    public native void bootAutoArmaments(boolean single);

    public native void stopAutoArmaments();

    public native void bootAutoEggs();

    public native void stopAutoEggs();

    public native void saveFramePicture();

    public native void skipGamePlacard();

    public native void trackedKeepAlive();

    public native void syncJNIArgs(long main_msec, int row_stride);

//    public native void syncOrientation(int rotation);

    public native void initJNIThread(int width, int height, boolean isRoot, String ssaid, String external, String[] carve_attr);

    public native void bootClansLoop(boolean win2_stars);

    public native void stopClansLoop();

    public native void bootBeachLoop(int crystal);

    public native void stopBeachLoop();

    public native void bootSonarLoop(int sample);

    public native void stopSonarLoop();

    public native void exportCarveAttr();

    public native void saveSonarTable();

    public native void mergeSonarTable();

    public native void tessOcrRecognize(int ability_value);

    public native void tessOcrDetach();
}