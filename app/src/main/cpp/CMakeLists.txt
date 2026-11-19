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

project("supercell")

#toolchain.cmake 优先设置,构建阶段无法修改,使用cmake 命令行 -D
#set(CMAKE_ANDROID_STL_TYPE c++_shared)
#导致jni.h找不到
#set(triple aarch64-linux-android)
#set(CMAKE_CXX_COMPILER_TARGET ${triple})
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fPIC -frtti")

#set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
#set(OpenCVSDK ${CMAKE_CURRENT_SOURCE_DIR}/OpenCV-android-sdk/sdk/native)
#MESSAGE(STATUS "This is OPENCV_PATH dir " ${OpenCVSDK})

#set(CMAKE_PREFIX_PATH ${OpenCVSDK}/jni/abi-arm64-v8a)
#find_package(OpenCV REQUIRED)

#include_directories(
#        ${OpenCVSDK}/jni/include
#)
#
#link_directories(
#        ${OpenCVSDK}/staticlibs/arm64-v8a
#        ${OpenCVSDK}/3rdparty/libs/arm64-v8a
##       ${FFmpeg}/arm64/lib
#        # ${OpenCVSDK}/libs/arm64-v8a
#)
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
set(OPENCV_LIBS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(OPENCV_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/libs)
#set(ANDROID_NATIVE_APP_GLUE ${ANDROID_NDK}/sources/android/native_app_glue)

include_directories(
        ${OPENCV_LIBS_INCLUDE}
        #${ANDROID_NATIVE_APP_GLUE}
)
add_library(libopencv_java4
        SHARED
        IMPORTED
)
set_target_properties(libopencv_java4 PROPERTIES IMPORTED_LOCATION
        ${OPENCV_LIBS}/${ANDROID_ABI}/libopencv_java4.so
)

#此处需与系统 dlopen 加载的so名称相同
#System.loadLibrary("supercell");
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -frtti)
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        libopencv_java4
        #toolchain.cmake 已完成导入
        android
        log
)

message(STATUS " CMAKE_ANDROID_STL_TYPE: ${CMAKE_ANDROID_STL_TYPE}")
add_subdirectory(tools)

