# Android arm64-v8a Tesseract / Leptonica 构建说明

本文对应当前项目目录和桌面构建脚本，记录构建依据、依赖关系、工具链、参数含义、实际执行顺序和输出位置。

## 1. 构建结果与边界

目标是在 Windows 上使用 Android NDK 将 Tesseract 5.5.4 和 Leptonica 1.85.0 编译为 Android `arm64-v8a` 共享库：

- `libleptonica.so`
- `libtesseract.so`

应用 CMake 不编译这两个第三方源码。它把已经构建好的 `.so` 声明为 imported targets，链接到应用 native target，并由 Android Gradle 将库打包到 APK。

模型文件（例如 `eng.traineddata`）是运行时资源，不参与 `.so` 的编译或链接。

## 2. 依据与依赖关系

### 上游构建依据

- Tesseract 5.5.4 上游源码和 CMake 配置：[GitHub tag 5.5.4](https://github.com/tesseract-ocr/tesseract/tree/5.5.4)
- Leptonica 1.85.0 上游源码和 CMake 配置：[GitHub tag 1.85.0](https://github.com/DanBloomberg/leptonica/tree/1.85.0)
- Google `cpu_features` 0.9.0：[GitHub tag v0.9.0](https://github.com/google/cpu_features/tree/v0.9.0)
- Android 官方 NDK CMake 构建说明：[NDK CMake guide](https://developer.android.com/ndk/guides/cmake)

上游 Tesseract CMake 指定 Leptonica 最低版本为 1.80，并通过 `find_package(Leptonica ... CONFIG)` 查找它。Android 配置还执行 `find_package(CpuFeaturesNdkCompat REQUIRED)`。所以构建顺序是：

```text
Android NDK toolchain
  ├─ cpu_features（提供 cpu_features 和 NDK compatibility 包）
  ├─ Leptonica（生成共享库和 CMake package）
  └─ Tesseract（通过 CMake package 找到 Leptonica 与 CpuFeaturesNdkCompat）
```

`cpu_features` 和 `ndk_compat` 是构建 Tesseract 所需的静态辅助库，不单独作为 Android 运行时 `.so` 打包。

### 本机构建依赖

| 依赖 | 当前使用位置 | 用途 |
|---|---|---|
| Windows 10 | 构建主机 | 运行 CMake、Ninja 和 PowerShell |
| Android NDK r27（27.0.12077973） | `E:\android-sdk\ndk\27.0.12077973` | Clang 18.0.1、Android sysroot、链接器和 Android CMake toolchain |
| CMake 3.22.1 | `E:\android-sdk\cmake\3.22.1\bin\cmake.exe` | 配置项目、生成构建文件、调用 Ninja |
| Ninja | `E:\android-sdk\cmake\3.22.1\bin\ninja.exe` | 按 CMake 生成的规则编译和链接 |
| Tesseract 5.5.4 源码 | `cpp\third_party\tesseract-5.5.4` | OCR 引擎源码 |
| Leptonica 1.85.0 源码 | `cpp\third_party\leptonica-1.85.0` | 图像处理库源码 |
| cpu_features 0.9.0 源码 | `cpp\third_party\cpu_features-0.9.0` | Tesseract Android 构建要求的辅助依赖 |

不需要在 Windows 主机上安装用于 Android 的 Visual Studio 编译器；实际 C/C++ 编译由 NDK Clang 交叉编译完成。编译期不需要 Java、Gradle 或 Android Studio。它们只在后续构建 APK 时使用。

## 3. 路径布局

```text
E:\lastwar_jni_cv\app\src\main\cpp\third_party\
  cpu_features-0.9.0\
  leptonica-1.85.0\
  tesseract-5.5.4\

E:\android-sdk\ocr-build\arm64-v8a\
  cpu_features\       # cpu_features 的 CMake/Ninja build tree
  leptonica\          # Leptonica 的 CMake/Ninja build tree
  tesseract\          # Tesseract 的 CMake/Ninja build tree
  prefix\             # 前两者 install 到这里，供后续 CMake find_package 使用

E:\lastwar_jni_cv\app\src\main\cpp\libs\arm64-v8a\
  libleptonica.so     # app CMake 导入的 Android 共享库
  libtesseract.so     # app CMake 导入的 Android 共享库
```

`-S` 指定源码目录；`-B` 指定独立构建目录。构建目录里的 `CMakeCache.txt`、`build.ninja`、配置头文件等由本机 CMake 生成，不是上游源码文件。

## 4. 配置参数

### 三个库共用的 Android 参数

| 参数 | 值 | 含义 |
|---|---|---|
| `-G Ninja` | `Ninja` | 使用 Ninja 生成器 |
| `-DCMAKE_MAKE_PROGRAM=...` | NDK SDK 内的 `ninja.exe` 路径 | 明确指定构建程序 |
| `-DCMAKE_TOOLCHAIN_FILE=...` | NDK 的 `android.toolchain.cmake` | 启用 Android 交叉编译器和 sysroot |
| `-DANDROID_ABI=arm64-v8a` | `arm64-v8a` | 只生成 64 位 ARM Android 库 |
| `-DANDROID_PLATFORM=android-28` | API 28 | 设置最低 Android native API level |
| `-DANDROID_STL=c++_shared` | `c++_shared` | 使用 NDK 的共享 libc++ runtime；应用打包时也需包含 `libc++_shared.so` |
| `-DCMAKE_BUILD_TYPE=Release` | `Release` | 优化构建，不生成 Debug 配置 |
| `-DCMAKE_INSTALL_PREFIX=...` | `...\prefix` | `cmake --install` 的统一安装根目录 |

更换 ABI 或 API 时，建议同时使用新的 `-B` 构建目录，避免复用已有目录中不匹配的 CMake cache。

### cpu_features 参数

| 参数 | 值 | 含义 |
|---|---|---|
| `BUILD_SHARED_LIBS=OFF` | OFF | 将辅助依赖构建为静态库，而不是额外的共享库 |
| `BUILD_EXECUTABLE=OFF` | OFF | 不构建 `list_cpu_features` 命令行程序 |
| `BUILD_TESTING=OFF` | OFF | 不构建测试 |
| `ENABLE_INSTALL=ON` | ON | 允许安装库、头文件和 CMake package |

### Leptonica 参数

| 参数 | 值 | 含义 |
|---|---|---|
| `BUILD_SHARED_LIBS=ON` | ON | 生成 `libleptonica.so` |
| `BUILD_PROG=OFF` | OFF | 不构建 Leptonica 附带的工具程序 |
| `SW_BUILD=OFF` | OFF | 不使用 Software Network 构建服务 |
| `ENABLE_ZLIB/PNG/GIF/JPEG/TIFF/WEBP/OPENJPEG=OFF` | 全部 OFF | 禁用可选压缩图片格式依赖，减少依赖和构建项 |

禁用这些图片编解码器意味着不能假设库能直接解码 PNG、JPEG、TIFF 等文件。若应用将图像解码为原始像素后通过 Tesseract/Leptonica API 传入，则不依赖这些文件编解码器；若需要 `pixRead` 等读取压缩图片文件的功能，需按需求重新启用对应 codec 并提供其 Android 依赖。

### Tesseract 参数

| 参数 | 值 | 含义 |
|---|---|---|
| `CMAKE_PREFIX_PATH=...\prefix` | install 前缀 | 让 CMake 在统一前缀中查找依赖 package |
| `Leptonica_DIR=...\prefix\lib\cmake\leptonica` | Leptonica package 目录 | 明确指定刚安装的 Leptonica CMake 配置 |
| `BUILD_SHARED_LIBS=ON` | ON | 生成 `libtesseract.so` |
| `BUILD_TESTS=OFF` | OFF | 不构建测试 |
| `BUILD_TRAINING_TOOLS=OFF` | OFF | 不构建训练工具 |
| `DISABLED_LEGACY_ENGINE=ON` | ON | 只保留现代 LSTM OCR 引擎 |
| `DISABLE_ARCHIVE/CURL/TIFF=ON` | ON | 禁用 archive、curl、TIFF 可选支持 |
| `GRAPHICS_DISABLED=ON` | ON | 禁用 ScrollView 图形调试功能 |
| `OPENMP_BUILD=OFF` | OFF | 不依赖 OpenMP |
| `USE_SYSTEM_ICU=OFF` | OFF | 不查找并链接系统 ICU |
| `ENABLE_PRECOMPILED_HEADERS=OFF` | OFF | 关闭预编译头，避免跨平台构建中的额外复杂性 |
| `ENABLE_CCACHE=OFF` | OFF | 不依赖 ccache |

Tesseract 的 CMake 会为 Android 查找 `CpuFeaturesNdkCompat`；cpu_features 安装步骤把这个 package 一起放进 `prefix`。

## 5. 逐阶段构建流程

以下是脚本中实际执行的流程。脚本已固化相同路径和参数，日常重建直接运行脚本即可。
为便于直接复制以下命令，先在 PowerShell 定义路径变量：

```powershell
$ProjectCpp = 'E:\lastwar_jni_cv\app\src\main\cpp'
$ThirdParty = Join-Path $ProjectCpp 'third_party'
$SdkRoot = 'E:\android-sdk'
$NdkRoot = Join-Path $SdkRoot 'ndk\27.0.12077973'
$CMakeRoot = Join-Path $SdkRoot 'cmake\3.22.1\bin'
$CMakeExe = Join-Path $CMakeRoot 'cmake.exe'
$NinjaExe = Join-Path $CMakeRoot 'ninja.exe'
$Toolchain = Join-Path $NdkRoot 'build\cmake\android.toolchain.cmake'
$BuildRoot = Join-Path $SdkRoot 'ocr-build\arm64-v8a'
$Prefix = Join-Path $BuildRoot 'prefix'
$CpuSource = Join-Path $ThirdParty 'cpu_features-0.9.0'
$LeptonicaSource = Join-Path $ThirdParty 'leptonica-1.85.0'
$TesseractSource = Join-Path $ThirdParty 'tesseract-5.5.4'
$CpuBuild = Join-Path $BuildRoot 'cpu_features'
$LeptonicaBuild = Join-Path $BuildRoot 'leptonica'
$TesseractBuild = Join-Path $BuildRoot 'tesseract'
$LeptonicaPackage = Join-Path $Prefix 'lib\cmake\leptonica'
```

### 阶段 A：配置、编译并安装 cpu_features

```powershell
cmake -S "$CpuSource" -B "$CpuBuild" -G Ninja `
  "-DCMAKE_MAKE_PROGRAM=$NinjaExe" `
  "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-28 `
  -DANDROID_STL=c++_shared `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_INSTALL_PREFIX=$Prefix" `
  -DBUILD_SHARED_LIBS=OFF `
  -DBUILD_EXECUTABLE=OFF `
  -DBUILD_TESTING=OFF `
  -DENABLE_INSTALL=ON

cmake --build "$CpuBuild" --parallel 4
cmake --install "$CpuBuild"
```

这一步在 `prefix` 安装 `libcpu_features.a`、`libndk_compat.a`、头文件和对应的 CMake package 配置。

### 阶段 B：配置、编译并安装 Leptonica

```powershell
cmake -S "$LeptonicaSource" -B "$LeptonicaBuild" -G Ninja `
  "-DCMAKE_MAKE_PROGRAM=$NinjaExe" `
  "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-28 `
  -DANDROID_STL=c++_shared `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_INSTALL_PREFIX=$Prefix" `
  -DBUILD_SHARED_LIBS=ON `
  -DBUILD_PROG=OFF `
  -DSW_BUILD=OFF `
  -DENABLE_ZLIB=OFF `
  -DENABLE_PNG=OFF `
  -DENABLE_GIF=OFF `
  -DENABLE_JPEG=OFF `
  -DENABLE_TIFF=OFF `
  -DENABLE_WEBP=OFF `
  -DENABLE_OPENJPEG=OFF

cmake --build "$LeptonicaBuild" --target leptonica --parallel 4
cmake --install "$LeptonicaBuild"
```

`cmake --build` 编译并链接 Leptonica target；`cmake --install` 再将 `.so`、头文件和 CMake package 安装到 `prefix`。Tesseract 随后通过这个已安装 package 查找 Leptonica。

### 阶段 C：配置并编译 Tesseract

```powershell
cmake -S "$TesseractSource" -B "$TesseractBuild" -G Ninja `
  "-DCMAKE_MAKE_PROGRAM=$NinjaExe" `
  "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-28 `
  -DANDROID_STL=c++_shared `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_INSTALL_PREFIX=$Prefix" `
  "-DCMAKE_PREFIX_PATH=$Prefix" `
  "-DLeptonica_DIR=$LeptonicaPackage" `
  -DBUILD_SHARED_LIBS=ON `
  -DBUILD_TESTS=OFF `
  -DBUILD_TRAINING_TOOLS=OFF `
  -DDISABLED_LEGACY_ENGINE=ON `
  -DDISABLE_ARCHIVE=ON `
  -DDISABLE_CURL=ON `
  -DDISABLE_TIFF=ON `
  -DGRAPHICS_DISABLED=ON `
  -DOPENMP_BUILD=OFF `
  -DUSE_SYSTEM_ICU=OFF `
  -DENABLE_PRECOMPILED_HEADERS=OFF `
  -DENABLE_CCACHE=OFF

cmake --build "$TesseractBuild" --target libtesseract --parallel 4
```

这里只构建核心 `libtesseract` target，不构建可选命令行程序和训练工具。脚本不对 Tesseract 执行整体 `cmake --install`：该上游 install 流程还会尝试安装未构建的可选 CLI。脚本直接复制已经构建的核心库。

### 阶段 D：复制到 Android 项目

脚本从以下位置复制 `.so`：

```text
E:\android-sdk\ocr-build\arm64-v8a\prefix\lib\libleptonica.so
E:\android-sdk\ocr-build\arm64-v8a\tesseract\libtesseract.so
```

目标位置：

```text
E:\lastwar_jni_cv\app\src\main\cpp\libs\arm64-v8a\
```

随后 app CMake 使用 `IMPORTED_LOCATION` 引用这些文件。这个导入/链接阶段与前面的源码构建是分开的：前面产生 `.so`，后面消费 `.so`。

## 6. configure、build、install 分别做什么

1. `cmake -S ... -B ... -D...` 是**配置阶段**：读取上游 `CMakeLists.txt`，检测目标平台和依赖，生成 CMake cache 与 Ninja 构建规则。它通常不编译整个库。
2. `cmake --build <build-dir> --target ...` 是**构建阶段**：CMake 调用 Ninja，Ninja 再调用 Android NDK 的 Clang 编译 C/C++ 源文件并链接 `.so`。
3. `cmake --install <build-dir>` 是**安装阶段**：把构建产物、头文件、package 配置文件复制到 `CMAKE_INSTALL_PREFIX`。它不是编译步骤。
4. `Copy-Item` 是**项目集成准备**：将两个库放到 app 的 `libs\arm64-v8a`，供 app CMake import/link。
5. Gradle 构建 APK 时，依据 app 的 native CMake target 和 jniLibs 规则，把 `.so` 与 `libc++_shared.so` 打包进 APK。

## 7. 运行脚本与清理语义

增量构建：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File 'C:\Users\admin\Desktop\OCR\Build-Tesseract-Android.ps1'
```

在现有 CMake/Ninja build tree 中清理目标后重编：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File 'C:\Users\admin\Desktop\OCR\Build-Tesseract-Android.ps1' -Clean
```

`-Clean` 使用 Ninja 的 `--clean-first`，会清理并重建该 build tree 的目标；保留 `CMakeCache.txt` 和配置，不等同于删除整个构建目录或清空 prefix。

## 8. 已知诊断与验证

- 在此环境中 Tesseract 上游执行可选 IPO/LTO 探测时，试探链接器 `gold` 失败并打印诊断；上游配置继续执行，最终关闭 LTO。实际 Tesseract 构建成功，`HAVE_NEON=TRUE`，`CMAKE_INTERPROCEDURAL_OPTIMIZATION=FALSE`。这不是最终 `libtesseract` 链接失败。
- 本次 `-Clean` 运行最终成功生成两份库。验证它们是 AArch64 ELF，输出大小分别约为 Leptonica 9.0 MiB、Tesseract 30.2 MiB。
- 后续可用 NDK `llvm-readelf -h <library.so>` 检查 `Machine: AArch64`；用 `llvm-readelf -d <library.so>` 查看 `NEEDED` 运行时依赖。
- 构建 APK 后还应确认 APK 内存在 `lib/arm64-v8a/libtesseract.so`、`libleptonica.so` 和 `libc++_shared.so`。无 Android 设备时，这只能验证构建和打包，不能代替设备上的模型加载与 OCR 运行测试。
