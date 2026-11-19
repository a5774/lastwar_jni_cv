package com.example.supercell;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.core.app.NotificationCompat;

import com.example.supercell.databinding.FloatSupercellFunctionAreaBinding;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashSet;

import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class MiscUtils {
    private static final Pattern ACTIVITY_RECORD_PATTERN = Pattern.compile("\\{([^}]*)\\}");
    private static final Pattern SOCKET_PID_PATTERN = Pattern.compile("\\bpid=(\\d+)\\b");

    static final long minMainLoop = 500;
    static final int TOUCH_SLOP = 20; // 拖动阈值
    static final int maxCharCount = 3000;
    static final int UE_delay = 500;
    static int UIOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
    static final String pkg = "com.fun.supercell.gp";
    static final String clazz = "com.im30.aps.debug.UnityPlayerActivityCustom";
    static final String boom_beach_pkg = "com.tencent.tmgp.supercell.boombeach";
    static final String clash_clans_pkg = "com.tencent.tmgp.supercell.clashofclans";
    static final String boom_beach_clazz = "com.supercell.titan.tencent.GameAppTencentMidas";
    static final String clash_clans_clazz = "com.supercell.titan.tencent.GameAppTencent";
    static final String audioFile = "sound/DIG.wav";
    static final String[] carve_fragments = {"LIFE", "ICE", "LAVA", "DARK"};
    static final String[] gallery = {"com.google.android.apps.photos", "com.miui.gallery", "com.huawei.photos", "com.samsung.android.gallery3d"};
    static final String[] su_manager = {
            "com.topjohnwu.magisk",
            "io.github.vvb2060.magisk",
            "io.github.huskydg.magisk",
            "me.weishu.kernelsu",
            "com.rifsxd.ksunext",
            "com.sukisu.ultra",
            "com.resukisu.resukisu",
            "me.bmax.apatch",
            "me.garfieldhan.apatch.next",
            "eu.chainfire.supersu",
            "com.koushikdutta.superuser",
            "com.noshufou.android.su",
            "com.noshufou.android.su.elite",
            "me.phh.superuser",
            "kingoroot.supersu"
    };

    public static boolean checkSuCommand() {
        Process process = null;
        try {
            process = Runtime.getRuntime().exec(new String[]{"su", "-c", "id"});
            java.io.BufferedReader in = new java.io.BufferedReader(new java.io.InputStreamReader(process.getInputStream()));
            String output = in.readLine();
            return output != null && output.contains("uid=0");
        } catch (Exception e) {
            return false;
        } finally {
            if (process != null) {
                process.destroy();
            }
        }
    }

    public static boolean checkSuExists() {
        String[] paths = {"/system/bin/su", "/system/xbin/su", "/sbin/su", "/system/sd/xbin/su", "/system/bin/failsafe/su", "/data/local/xbin/su", "/data/local/bin/su", "/data/local/su", "/su/bin/su"};
        for (String path : paths) {
            if (new java.io.File(path).exists()) {
                return true;
            }
        }
        return false;
    }


    public static void clickByPoint(int x, int y, long delay) {
        new Thread(() -> {
            try {
                safeSleep(delay);
                String command = String.format(Locale.US, "input tap  %d %d", x, y);
                new ProcessBuilder("su", "-c", command).start().waitFor();
            } catch (IOException | InterruptedException e) {
                Log.d("TAG", e.toString());
            }
        }).start();
    }

    public static void backMainActivity(Context context) {
        Intent intent = new Intent(context, MainActivity.class);
//        FLAG_ACTIVITY_NEW_TASK 非Activity上下文可用
//        Intent.FLAG_ACTIVITY_NEW_TASK  |
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(intent);
    }

    public static float[] toFloatArray(String[] strings) {
        float[] result = new float[strings.length];
        for (int i = 0; i < strings.length; i++) {
            result[i] = Float.parseFloat(strings[i].trim());
        }
        return result;
    }

    public static String getStringWithDefault(Bundle bundle, String key, String defaultValue) {
        if (bundle != null) {
            String value = bundle.getString(key);
            return value != null ? value : defaultValue;
        }
        return defaultValue;
    }

    public static AlertDialog infoDialog(Context context, View view) {
        ProgressBar progressBar = new ProgressBar(context);
        progressBar.setIndeterminate(true);
        AlertDialog dialog = new AlertDialog.Builder(context).setView(view).create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.setCancelable(false);
        dialog.show();
        return dialog;
    }

    public static long getAppVersionCode(Context context) {
        try {
            PackageManager pm = context.getPackageManager();
            PackageInfo info = pm.getPackageInfo(context.getPackageName(), 0);
            return info.getLongVersionCode();
        } catch (PackageManager.NameNotFoundException e) {
            Log.d("TAG", e.toString());
            return -1;
        }
    }

    public static void openURLToWebView(Context context, String url) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    public static void safeSleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt(); // 恢复中断标志
            Log.d("TAG", e.toString());
        }
    }


    public static void bootGameIntent(Context context, String pkg, String clazz) {
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(pkg, clazz));
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        List<ResolveInfo> activities = context.getPackageManager().queryIntentActivities(intent, 0);
        if (!activities.isEmpty()) {
            context.startActivity(intent);
        } else {
            Toast.makeText(context, "目标应用未安装或组件不存在", Toast.LENGTH_SHORT).show();
        }
    }

    public static void bootGameWithUser(Context context, int users, boolean isRoot) {
        if (!isRoot) {
            MiscUtils.bootGameIntent(context, MiscUtils.pkg, MiscUtils.clazz);
            return;
        }
        try {
            String component = MiscUtils.pkg + "/" + MiscUtils.clazz;
            String command = String.format("am start --user %s -n %s", users, component);
            ProcessBuilder builder = new ProcessBuilder("su", "-c", command);
            int exitCode = builder.start().waitFor();
            if (exitCode != 0) {
                Toast.makeText(context, "该用户空间不存在或应用未安装", Toast.LENGTH_SHORT).show();
            }
        } catch (InterruptedException | IOException e) {
            Log.d("TAG", e.toString());
        }
    }

    public static void iptables_drop_packet(String pkg) {
        new Thread(() -> {
            try {
//                pm list packages --user 999 -U | grep boombeach
                String uid = SuShellUtils.execSuCommand(String.format(Locale.US, "dumpsys package %s | grep userId= | awk -F= '{print $2}'", pkg)).stdout().trim();
//                iptables -I OUTPUT -m owner --uid-owner 10256 -j DROP
//                ip6tables -I OUTPUT -m owner --uid-owner 10256 -j DROP
//                iptables -I INPUT  -m owner --uid-owner 10256 -j DROP
//                ip6tables -I INPUT  -m owner --uid-owner 10256 -j DROP
                SuShellUtils.execSuCommand(String.format(Locale.US, "iptables -I OUTPUT -m owner --uid-owner %s  -j DROP", uid));
                SuShellUtils.execSuCommand(String.format(Locale.US, "ip6tables -I OUTPUT -m owner --uid-owner %s  -j DROP", uid));
//                iptables -I OUTPUT -m owner --uid-owner 10216  -m statistic --mode random --probability 1.0  -j DROP
//                input链不支持owner match
//                SuShellUtils.execSuCommand(String.format(Locale.US, "iptables -I INPUT  -m owner --uid-owner %s -j DROP", uid));
//                SuShellUtils.execSuCommand(String.format(Locale.US, "ip6tables -I INPUT  -m owner --uid-owner %s -j DROP", uid));
            } catch (Exception e) {
                Log.d("TAG", e.toString());
            }
        }).start();
    }

    public static void iptables_clear_table() {
        new Thread(() -> {
            try {
                SuShellUtils.execSuCommand("iptables -F");
                SuShellUtils.execSuCommand("ip6tables -F");
            } catch (Exception e) {
                Log.d("TAG", e.toString());
            }
        }).start();
    }

    public record SocketInfo(String localAddress, int localPort, String remoteAddress,
                             int remotePort) {
    }

    public record ActivityRecord(String record_id, int user_id, String package_name,
                                 String activity_name) {
    }

    public static SocketInfo foramtSocketInfo(String local, String remote) {
        int localColon = local.lastIndexOf(':');
        int remoteColon = remote.lastIndexOf(':');
        if (localColon <= 0 || remoteColon <= 0) {
            return null;
        }
        try {
            String localAddress = local.substring(0, localColon);
            int localPort = Integer.parseInt(local.substring(localColon + 1));
            String remoteAddress = remote.substring(0, remoteColon);
            int remotePort = Integer.parseInt(remote.substring(remoteColon + 1));
            return new SocketInfo(localAddress, localPort, remoteAddress, remotePort);
        } catch (NumberFormatException e) {
            Log.d("TAG", e.toString());
            return null;
        }
    }

    public static CompletableFuture<Void> reset_sockets_by_foreground_packages(Context ctx) {
        CompletableFuture<Void> future = new CompletableFuture<>();
        new Thread(() -> {
            try {
                String activities = SuShellUtils.execSuCommand("dumpsys activity activities | grep 'topResumedActivity='").stdout();
                Matcher activity_record_matcher = ACTIVITY_RECORD_PATTERN.matcher(activities);
                Set<ActivityRecord> foreground_activities = new LinkedHashSet<>();
                while (activity_record_matcher.find()) {
                    String[] record_fields = activity_record_matcher.group(1).trim().split("\\s+", 3);
                    if (record_fields.length != 3) {
                        continue;
                    }
                    String[] component_fields = record_fields[2].split("/", 2);
                    if (component_fields.length != 2) {
                        continue;
                    }
                    ActivityRecord activity_record = new ActivityRecord(
                            record_fields[0],
                            Integer.parseInt(record_fields[1].substring(1)),
                            component_fields[0],
                            component_fields[1]
                    );
                    foreground_activities.add(activity_record);
                }
                Set<String> foreground_packages = new LinkedHashSet<>();
                for (ActivityRecord activity_record : foreground_activities) {
                    foreground_packages.add(activity_record.package_name());
                }
                ((MainActivity) ctx).updateMessage(String.join(",\n", foreground_packages));
                Set<String> foreground_pids = new LinkedHashSet<>();
                for (String foreground_package : foreground_packages) {
                    SuShellUtils.ExecResult pid_result = SuShellUtils.execSuCommand(String.format(Locale.US, "pidof %s", foreground_package));
                    foreground_pids.addAll(Arrays.asList(pid_result.stdout().trim().split("\\s+")));
                }
                SuShellUtils.ExecResult ss_result = SuShellUtils.execSuCommand("ss -tunpH");
                if (ss_result.exitCode() != 0) {
                    ((MainActivity) ctx).updateMessage(ss_result.stdout());
                    return;
                }
                Set<SocketInfo> socket_info_set = new LinkedHashSet<>();
                for (String socket_line : ss_result.stdout().split("\\R")) {
                    Matcher pid_matcher = SOCKET_PID_PATTERN.matcher(socket_line);
                    boolean has_foreground_pid = false;
                    while (pid_matcher.find()) {
                        if (foreground_pids.contains(pid_matcher.group(1))) {
                            has_foreground_pid = true;
                            break;
                        }
                    }
                    if (!has_foreground_pid) {
                        continue;
                    }
                    String[] socket_fields = socket_line.trim().split("\\s+");
                    int state_index = -1;
                    for (int field_index = 0; field_index < socket_fields.length; field_index++) {
                        if ("ESTAB".equalsIgnoreCase(socket_fields[field_index])) {
                            state_index = field_index;
                            break;
                        }
                    }
                    if (state_index < 0 || state_index + 4 >= socket_fields.length) {
                        continue;
                    }
                    SocketInfo socket_info = foramtSocketInfo(socket_fields[state_index + 3], socket_fields[state_index + 4]);
                    if (socket_info != null) {
                        socket_info_set.add(socket_info);
                    }
                }
                // ss without a family selector includes both IPv4 and IPv6 sockets.
                for (SocketInfo socket_info : socket_info_set) {
                    String kill_command = String.format(Locale.US, "ss -tunK src %s sport = :%d dst %s dport = :%d", socket_info.localAddress(), socket_info.localPort(), socket_info.remoteAddress(), socket_info.remotePort());
                    SuShellUtils.execSuCommand(kill_command);
                }
                if (foreground_activities.size() == 1 && boom_beach_pkg.equals(foreground_activities.iterator().next().package_name())) {
                    MiscUtils.clickByPoint(800, 1000, 300);
                }
                future.complete(null);
            } catch (Exception exception) {
                Log.d("TAG", exception.toString());
            }
        }).start();
        return future;
    }

    public static void back_event_input() {
        new Thread(() -> {
            try {
                SuShellUtils.execSuCommand("");
            } catch (Exception e) {
                Log.d("TAG", e.toString());
            }
        }).start();

    }

    public static void userSpaceGameBoot(int user) {
        new Thread(() -> {
            try {
                SuShellUtils.execSuCommand(String.format(Locale.US, "am force-stop %s", boom_beach_pkg));
                safeSleep(1000);
                String activity = boom_beach_pkg + "/" + boom_beach_clazz;
                String command = String.format("am start --user %s -n %s", user, activity);
                SuShellUtils.execSuCommand(command);
                MiscUtils.iptables_clear_table();
            } catch (Exception e) {
                Log.d("TAG", e.toString());
            }
        }).start();
    }


    public static String getAbsoluteFiles(Context context) {
        return Objects.requireNonNull(context.getFilesDir()).getAbsolutePath();
    }

    public static void launchApp(Context context, Intent intent) {
        try {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
//            Toast.makeText(context, "包名或启动入口不存在!", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            Log.d("TAG", e.toString());
        }
    }

    public static void openAppForLauncher(Context context, String[] pkgs) {
        for (String pkm : pkgs) {
            Intent intent = context.getPackageManager().getLaunchIntentForPackage(pkm);
            if (intent != null) {
                launchApp(context, intent);
            }
        }
    }
    public static void sendNotification(String title, String content) {
        NotificationManager manager = CowApplication.instance.getSystemService(NotificationManager.class);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(CowApplication.instance, CowApplication.channel_extra_common_id).setContentTitle(title).setContentText(content).setSmallIcon(R.drawable.ic_launcher_foreground).setPriority(NotificationCompat.PRIORITY_HIGH).setDefaults(NotificationCompat.DEFAULT_ALL).setAutoCancel(true);
        manager.notify(ScreenCaptureService.NOTIFICATION_ID_EXTRA, builder.build());
    }

    public static void sendNotification(String content) {
        NotificationManager manager = CowApplication.instance.getSystemService(NotificationManager.class);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(CowApplication.instance, CowApplication.channel_extra_common_id).setContentText(content).setSmallIcon(R.drawable.ic_launcher_foreground).setPriority(NotificationCompat.PRIORITY_HIGH).setDefaults(NotificationCompat.DEFAULT_ALL).setAutoCancel(true);
        manager.notify(ScreenCaptureService.NOTIFICATION_ID_EXTRA, builder.build());
    }

    public static void opsAuthorization() {
        String cPackage = CowApplication.instance.getPackageName();
        try {
//            SuShellUtils.execSuCommand(String.format(Locale.US, "appops set %s POST_NOTIFICATION allow", cPackage));
            SuShellUtils.execSuCommand(String.format(Locale.US, "appops set %s SYSTEM_ALERT_WINDOW allow", cPackage));
            SuShellUtils.execSuCommand(String.format(Locale.US, "appops set %s MANAGE_EXTERNAL_STORAGE allow", cPackage));
            SuShellUtils.execSuCommand(String.format(Locale.US, "dumpsys deviceidle whitelist +%s", cPackage));
//            SuShellUtils.execSuCommand(String.format(Locale.US, "appops set %s PROJECT_MEDIA allow", cPackage));
//            SuShellUtils.execSuCommand(String.format(Locale.US, "settings put secure enabled_accessibility_services %s/.ClickAccessibilityService ", cPackage));
        } catch (Exception e) {
            Log.d("TAG", e.toString());
        }
    }

}
