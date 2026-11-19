D:\\Android\\Sdk\\cmake\\3.22.1\\bin\\cmake.exe ^
  "-HD:\\lastwar_jni\\app\\src\\main\\cpp\\tools" ^
  "-DCMAKE_SYSTEM_NAME=Android" ^
  "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" ^
  "-DCMAKE_SYSTEM_VERSION=28" ^
  "-DANDROID_PLATFORM=android-28" ^
  "-DANDROID_ABI=arm64-v8a" ^
  "-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a" ^
  "-DANDROID_NDK=D:\\Android\\Sdk\\ndk\\27.0.12077973" ^
  "-DCMAKE_ANDROID_NDK=D:\\Android\\Sdk\\ndk\\27.0.12077973" ^
  "-DCMAKE_TOOLCHAIN_FILE=D:\\Android\\Sdk\\ndk\\27.0.12077973\\build\\cmake\\android.toolchain.cmake" ^
  "-DCMAKE_MAKE_PROGRAM=D:\\Android\\Sdk\\cmake\\3.22.1\\bin\\ninja.exe" ^
  "-DCMAKE_CXX_FLAGS=-std=c++17" ^
  "-DANDROID_STL=c++_static" ^
  "-DCMAKE_VERBOSE_MAKEFILE=ON" ^
  "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=D:\\lastwar_jni\\app\\src\\main\\assets\\bin" ^
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=D:\\lastwar_jni\\app\\src\\main\\assets\\bin" ^
  "-DCMAKE_BUILD_TYPE=Release" ^
  "-BD:\\lastwar_jni\\app\\.cxx\\event\\arm64-v8a" ^
  -GNinja

 D:\Android\Sdk\cmake\3.22.1\bin\ninja -C  D:\lastwar_jni\app\.cxx\event\arm64-v8a

@REM  adb push D:\lastwar_jni\app\src\main\assets\bin\frp_scrcpy /sdcard/
 adb push D:\lastwar_jni\app\src\main\assets\bin\zoom_events /sdcard/
 adb shell su -c mv /sdcard/zoom_events /data/lost+found
 adb shell su -c chmod 777 /data/lost+found/zoom_events
 adb shell su -c /data/lost+found/zoom_events 


@REM  mv /sdcard/frp_scrcpy /data/lost+found/ && chmod 777 frp_scrcpy
@REM adb push D:\lastwar_jni\app\src\main\assets\bin\frp_scrcpy /sdcard/