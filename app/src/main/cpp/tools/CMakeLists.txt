# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).


#cmake 启动
#│
#├── 读取 -D 参数
#│
#├── 立刻加载 Toolchain 文件  ← 这里！
#│
#├── 执行 project()
#│
#└── 执行 CMakeLists.txt

project("tools")


message(STATUS "Android Platform: ${ANDROID_PLATFORM}")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(ANDROID_NATIVE_APP_GLUE ${ANDROID_NDK}/sources/android/native_app_glue)



#add_executable(events evt_node.cpp)

add_executable(zoom_events zoom_events.c)

#add_executable(performance_click performance_click.c)

#add_executable(daemon_dev daemon_dev.c)

#add_executable(frp_scrcpy frp_scrcpy.cpp)
#
#单独生效,静态库已经被合并,但运行时依旧会提示libc++_shard.so缺失尽管已经使用了静态编译,貌似toolchain固定注入了-lc++_shared
#编译参数
#target_compile_options(events PRIVATE
#        -stdlib=libc++
#)
#target_link_libraries(events PRIVATE
#
#)

#get_target_property(CXX_OPTS events COMPILE_OPTIONS)
#message(STATUS "events COMPILE_OPTIONS = ${CXX_OPTS}")
message(STATUS "链接标准c++: ${CMAKE_ANDROID_STL_TYPE}")


#adb push ./events /sdcard

#readelf -d events