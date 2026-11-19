package com.example.supercell;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class AssetCopier {
    public static String privateStorage;
    public static String[] carvePaths;

    // 调用此方法来复制 assets 到 internal storage
    public static void copyAssetsToFiles(Context context) {
        privateStorage = MiscUtils.getAbsoluteFiles(context);
        long version = MiscUtils.getAppVersionCode(context);
        File checkFile = new File(privateStorage, "check");
        long lastVersion = CowApplication.preferencesConfig.getLong("last_version", 0);
        carveAttrFragment(context);
        if (version >= 0 && (version > lastVersion) || lastVersion == 0) {
            CowApplication.preferencesConfig.edit().putLong("last_version", version).apply();
            if (checkFile.exists()) checkFile.delete();
        }
        if (checkFile.exists()) return;
        try {
            copyAssets(context);
            SuShellUtils.execSuCommand(String.format(Locale.US, "chmod 777 %s/*", new File(privateStorage, "bin").getAbsoluteFile()));
            checkFile.createNewFile();
        } catch (IOException e) {
            Log.d("TAG", e.toString());
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
    public static void release_assert(Context ctx){
        try {
            SuShellUtils.execSuCommand(String.format(Locale.US, "rm -rf %s", new File(privateStorage, "bin").getAbsoluteFile()));
            copyAssets(ctx);
            SuShellUtils.execSuCommand(String.format(Locale.US, "chmod 777 %s/*", new File(privateStorage, "bin").getAbsoluteFile()));
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
    private static void copyAssets(Context context) throws IOException {
        AssetManager assetManager = context.getAssets();
        String[] files = assetManager.list("");
        if (files != null) {
            for (String file : files) {
                copyAsset(context, file);
            }
        }
    }

    private static void copyAsset(Context context, String asset) throws IOException {
        AssetManager assetManager = context.getAssets();
        File osf = new File(privateStorage, asset);
//        Log.d("TAG", "privateStorage:" + osf);
        String[] files = assetManager.list(asset);
        if (files == null) return;
        if (files.length == 0) {
            if (!osf.exists()) {
                osf.createNewFile();
            }
            try (InputStream in = assetManager.open(asset);
                 OutputStream out = new FileOutputStream(osf)) {
                byte[] buffer = new byte[4096];
                int read;
                while ((read = in.read(buffer)) != -1) {
                    out.write(buffer, 0, read);
                }
                out.flush();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            if (!osf.exists()) {
                osf.mkdir();
            }
            for (String file : files) {
                copyAsset(context, asset + "/" + file);
            }
        }
    }
    private static void carveAttrFragment(Context context) {
        File d_camera = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM);
        File carve = new File(d_camera, "Carve");
        File screenshots = new File(d_camera, "Screenshots");
        File sonar = new File(d_camera, "Sonar");
        if(!sonar.exists())sonar.mkdir();
        List<String> list = new ArrayList<>();
        if (!screenshots.exists()) screenshots.mkdirs();
        if (!carve.exists()) carve.mkdirs();
        for (String name : MiscUtils.carve_fragments) {
            File fragment = new File(carve, name);
            if (!fragment.exists()) fragment.mkdirs();
            list.add(fragment.getAbsolutePath());
        }
        list.add(screenshots.getAbsolutePath());
        carvePaths = list.toArray(new String[0]);
    }

}
