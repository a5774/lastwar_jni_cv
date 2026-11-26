#include <jni.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <vector>
#include <sys/file.h>
#include <thread>
#include <random>
#include <sstream>
#include <chrono>
#include <linux/input.h>
#include <dirent.h>
#include <getopt.h>
#include <opencv2/opencv.hpp>
#include <iostream>
//#include <android/native_window_jni.h>
//#include <android/native_window.h>
//#include <android/bitmap.h>
#include <android/log.h>
#include <atomic>
#include <cstdio>
#include <sys/wait.h>
#include <cmath>
#include <spawn.h>
#include "Resources.hpp"
#include "Tess_OCR.hpp"
// #include "android_native_app_glue.h"
#define LOG_TAG "SUPERCELL"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

JavaVM *g_vm = nullptr;
jclass g_android_media_image;
jclass g_android_media_image$plane;
jmethodID g_android_image_close;
jmethodID g_android_image_getPlanes;
jmethodID g_android_plane_getBuffer;


//callback
JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_vm = vm;
    JNIEnv *env;
//  JNI_OnLoad：仅缓存通用的 Android 类
    if (g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass android_media_image = env->FindClass("android/media/Image");
    g_android_media_image = (jclass) env->NewGlobalRef(android_media_image);
    jclass android_media_image$plane = env->FindClass("android/media/Image$Plane");
    g_android_media_image$plane = (jclass) env->NewGlobalRef(android_media_image$plane);
    g_android_image_getPlanes = env->GetMethodID(g_android_media_image, "getPlanes",
                                                 "()[Landroid/media/Image$Plane;");
    g_android_image_close = env->GetMethodID(g_android_media_image, "close", "()V");
    g_android_plane_getBuffer = env->GetMethodID(g_android_media_image$plane, "getBuffer",
                                                 "()Ljava/nio/ByteBuffer;");
    env->DeleteLocalRef(android_media_image);
    env->DeleteLocalRef(android_media_image$plane);
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {

}

JNIEnv *getEnv() {
    JNIEnv *env = nullptr;
    if (g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        // 当前线程未附加到 JVM，尝试附加
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            return nullptr; // 附加失败，返回空指针
        }
    }
    return env;
}

//匿名命名空间(无需手动static)
namespace {
    using namespace resources::paths;
    using namespace resources::mats;
    using namespace resources::points;
    using namespace resources::regions;
    using namespace resources::binary;
    using namespace resources::hsv;
}
namespace fs = std::filesystem;
static bool g_isRoot = false;
static std::atomic<bool> assembly_enable(false);
static std::atomic<bool> treasure_enable(false);
static std::atomic<bool> armaments_enable(false);
static std::atomic<bool> eggs_enable(false);
static std::atomic<bool> unique_thread(false);
static std::atomic<bool> keep_alive_enable(false);
static std::atomic<bool> alive_thread_lock(false);
static std::atomic<int> g_rotation(0);
static std::atomic<long> g_main_msec(1000);
static int g_row_stride;
static int g_cols;
static int g_rows;
static cv::Mat g_empty_mat;
static std::string g_input_device;
static std::string private_storage;
static std::vector<std::string> carve_paths;
static std::atomic<bool> pause_thread(false);
static std::mutex pause_mutex;
static std::condition_variable pause_cv;


jobject g_thiz = nullptr;
jclass g_clazz = nullptr;
jmethodID captureByteID;
jmethodID captureBufferID;
jmethodID captureImageID;
jmethodID updateID;
jmethodID performID;
jmethodID actionBackID;
jmethodID playAudioID;
jmethodID cForegroundID;
jmethodID bootGameID;
jmethodID rejectNetworkID;
jmethodID resumeNetworkID;
//static uint g_count = 0;
std::stringstream g_format;


std::string getAbsolutePath(const std::string &type,
                            const std::string &user = "",
                            const std::string &filename = "") {
    fs::path path(private_storage);
    if (!type.empty())
        path /= type;
    if (!user.empty())
        path /= user;
    if (!filename.empty())
        path /= filename;
    path = path.lexically_normal();
    return path.string();
}

static const char *INPUT_TAP_CMD = "input tap";
static const char *INPUT_SWIPE_CMD = "input swipe";
static const char *KEYCODE_BACK_CMD = "input keyevent KEYCODE_BACK";
static const std::string package = "com.fun.supercell.gp";
static const std::string activity = "com.im30.aps.debug.UnityPlayerActivityCustom";
static const std::string component = package + "/" + activity;
static const std::string pid_of_pkg = "su -c pidof " + package;
static const char *pid_of_pkg_c = pid_of_pkg.c_str();
static const std::string beach_package = "com.tencent.tmgp.supercell.boombeach";


int execSuCommand(const char *cmdArgs[]) {
    std::ostringstream oss;
    for (int i = 0; cmdArgs[i] != nullptr; ++i) {
        oss << cmdArgs[i];
        if (cmdArgs[i + 1]) oss << " ";
    }
    std::string fullCmd = oss.str();
    char *argv[] = {
            (char *) "su",
            (char *) "-c",
            (char *) fullCmd.c_str(),
            nullptr
    };
    pid_t pid;
    int status = posix_spawnp(&pid, "su", nullptr, nullptr, argv, environ);
    if (status != 0) return -1;
    if (waitpid(pid, &status, 0) == -1) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string pipeCommand(const char *cmd) {
    char buffer[128];
    std::string result;
    snprintf(buffer, sizeof(buffer), "su -c %s", cmd);
    FILE *pipe = popen(buffer, "r");
    if (!pipe) {
        return "popen failed!";
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

//内联优化频繁调用
inline cv::Point threadSafeRandomOffsetPoint(const cv::Point &center, int offset) {
    static thread_local std::mt19937
            generator(std::random_device{}() ^ std::hash<std::thread::id>{}(
            std::this_thread::get_id()));
//    offset需要有符号类型
    std::uniform_int_distribution<int> distribution(-offset, offset);
    int dx = distribution(generator);
    int dy = distribution(generator);
    return center + cv::Point(dx, dy);
}

std::string
genRDString(const std::string &prefix = "find_", const std::string &extension = ".PNG") {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(10000, 99999);
    std::ostringstream oss;
    oss << prefix << dist(gen) << extension;
    return (fs::path(FINED_RP) /= oss.str()).string();
}

//FNV-1a 64-bit
uint64_t fnv1a_64(const std::string &s) {
    uint64_t hash = 14695981039346656037ULL; // offset basis
    for (unsigned char c: s) {
        hash ^= c;
//      64bit素数
        hash *= 1099511628211ULL; // FNV prime
    }
    return hash;
}

std::string
fnv1a_resource_prefix(size_t index, const std::string &root = FINED_RP,
                      const std::string &extension = ".jpg") {
    using namespace std::chrono;
    std::string name = cv::toLowerCase(fs::path(root).filename().string());
    static thread_local std::mt19937_64
            rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    uint32_t rand_id = dist(rng);
    std::ostringstream oss;
    oss << name << std::setw(3) << std::setfill('0') << index << "_";
    oss << std::hex << std::setw(8) << std::setfill('0') << rand_id;//取低32位
    oss << extension;
    return (fs::path(root) /= oss.str()).string();
}

void updateMessage(JNIEnv *env, const char *message) {
    jstring jMessage = env->NewStringUTF(message);
    if (updateID != nullptr) {
        env->CallVoidMethod(g_thiz, updateID, jMessage);
    }
    env->DeleteLocalRef(jMessage);
}


cv::Mat acquireNextMat_byte(JNIEnv *env) {
    if (captureByteID != nullptr) {
        auto rgbaArray = (jbyteArray) env->CallObjectMethod(g_thiz, captureByteID);
        if (rgbaArray != nullptr) {
            jbyte *data = env->GetByteArrayElements(rgbaArray, nullptr);
            if (data != nullptr) {
                cv::Mat bgra_mat;
                cv::Mat rgba_mat(g_cols, g_rows, CV_8UC4, (uchar *) data);
                cv::cvtColor(rgba_mat, bgra_mat, cv::COLOR_RGBA2BGR);
                // JNI_ABORT不同步修改jbyteArray
                env->ReleaseByteArrayElements(rgbaArray, data, JNI_ABORT);
                // 必须删除jvm引用
                env->DeleteLocalRef(rgbaArray);
//      Vec4b内存布局与 uchar[4] 一致，因此你可以安全地将 uchar* 强转为 Vec4b* 来访问Vec(const Vec<_Tp, cn>& v);
//     cv::Vec4b *v4 = bgra_mat.ptr<cv::Vec4b>(1);
                return bgra_mat;
            }
// 引用不为nullptr时务必DeleteLocalRef
            env->DeleteLocalRef(rgbaArray);
        }
    }
    return g_empty_mat;
}

cv::Mat acquireNextMat_buffer(JNIEnv *env) {
    if (captureBufferID != nullptr) {
        auto rgbaBuffer = (jobject) env->CallObjectMethod(g_thiz, captureBufferID);
        if (rgbaBuffer != nullptr) {
            auto data = static_cast<uchar *>(env->GetDirectBufferAddress(rgbaBuffer));
            if (data != nullptr) {
                cv::Mat bgra_mat;
                cv::Mat rgba_mat(g_cols, g_rows, CV_8UC4, data, g_row_stride);
//   cv::cvtColor会复制内存而非引用data
                cv::cvtColor(rgba_mat, bgra_mat, cv::COLOR_RGBA2BGR);
                // 必须删除jvm引用
                env->DeleteLocalRef(rgbaBuffer);
                return bgra_mat;
            }
            env->DeleteLocalRef(rgbaBuffer);
        }
    }
    return g_empty_mat;
}

cv::Mat acquireNextMat(JNIEnv *env, int code = cv::COLOR_RGBA2BGR) {
    if (!captureImageID) return g_empty_mat;
    auto image = (jobject) env->CallObjectMethod(g_thiz, captureImageID);
    if (!image) return g_empty_mat;
    auto planes = (jobjectArray) env->CallObjectMethod(image, g_android_image_getPlanes);
    jobject plane = env->GetObjectArrayElement(planes, 0);
    jobject buffer = env->CallObjectMethod(plane, g_android_plane_getBuffer);
    auto *data = static_cast<uchar *>(env->GetDirectBufferAddress(buffer));
    cv::Mat mat;
    if (data) {
        cv::Mat rgba_mat(g_cols, g_rows, CV_8UC4, data, g_row_stride);
        cv::cvtColor(rgba_mat, mat, code);
    }
    env->DeleteLocalRef(buffer);
    env->DeleteLocalRef(plane);
    env->DeleteLocalRef(planes);
    env->CallVoidMethod(image, g_android_image_close);
    env->DeleteLocalRef(image);
    return mat;
}

jmethodID requestJavaMethodsID(JNIEnv *env, const char *name, const char *sig) {
    if (g_clazz != nullptr) {
//      non-static
        jmethodID methodID = env->GetMethodID(g_clazz, name, sig);
        if (methodID != nullptr) {
            return methodID;
        }
    }
    return nullptr;
}

std::string jstringTostring(JNIEnv *env, jstring jstr) {
    const char *utfChars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(utfChars);
    env->ReleaseStringUTFChars(jstr, utfChars);
    return result;
}

std::vector<std::string> jstringArrayTovector(JNIEnv *env, jobjectArray jary) {
    std::vector<std::string> result;
    if (jary == nullptr) return result;
    jsize arrayLength = env->GetArrayLength(jary);
    for (jsize i = 0; i < arrayLength; ++i) {
        auto jstr = (jstring) env->GetObjectArrayElement(jary, i);
        if (jstr != nullptr) {
            const char *cstr = env->GetStringUTFChars(jstr, nullptr);
            if (cstr != nullptr) {
                result.emplace_back(cstr);
                env->ReleaseStringUTFChars(jstr, cstr);
            }
            env->DeleteLocalRef(jstr);
        }
    }
    return result;
}

cv::Mat cropROI(const cv::Mat &src, cv::Rect roi) {
    roi &= cv::Rect(0, 0, src.cols, src.rows);
    if (roi.empty()) {
        return src;
    }
    //   src(roi) 指向原始图像部分区域的视图，仍共享原始mat数据
    return src(roi);
}


int clickViaRoot(const cv::Point &center) {
//   指针悬空
//    Object backing the pointer will be destroyed at the end of the full-expression
    std::string xStr = std::to_string(center.x);
    std::string yStr = std::to_string(center.y);
    return execSuCommand((const char *[]) {INPUT_TAP_CMD, xStr.c_str(), yStr.c_str(), nullptr});
}

int clickViaEvent(const cv::Point &center) {
    std::string xStr = std::to_string(center.x);
    std::string yStr = std::to_string(center.y);
    std::string rStr = std::to_string(g_rotation.load());
    const char *cmd[] = {performance_click.c_str(), g_input_device.c_str(),
                         xStr.c_str(), yStr.c_str(), rStr.c_str(), nullptr};
    return execSuCommand(cmd);

}

int swipeViaRoot(const cv::Point &begin, const cv::Point &end, const size_t duration,
                 const uint64_t e = 0) {
    if (e) {
        std::this_thread::sleep_for(std::chrono::milliseconds(e));
    }
    std::string bxStr = std::to_string(begin.x);
    std::string byStr = std::to_string(begin.y);
    std::string exStr = std::to_string(end.x);
    std::string eyStr = std::to_string(end.y);
    std::string dStr = std::to_string(duration);
    return execSuCommand((const char *[]) {INPUT_SWIPE_CMD, bxStr.c_str(),
                                           byStr.c_str(), exStr.c_str(), eyStr.c_str(),
                                           dStr.c_str(), nullptr});
}


int rejectNetwork(JNIEnv *env) {
    if (bootGameID != nullptr) {
        env->CallObjectMethod(g_thiz, rejectNetworkID);
        return 0;
    }
    return 1;
}

int resolveNetwork(JNIEnv *env) {
    if (bootGameID != nullptr) {
        env->CallObjectMethod(g_thiz, resumeNetworkID);
        return 0;
    }
    return 1;
}

int bootGameViaActivity(JNIEnv *env) {
    if (bootGameID != nullptr) {
        env->CallObjectMethod(g_thiz, bootGameID);
        return 0;
    }
    return 1;
}

int bootGameViaRoot() {
    return execSuCommand(
            (const char *[]) {"am", "start", "--user", "0", "-n", component.c_str(), nullptr});
}

int bootGameZero(JNIEnv *env) {
    if (g_isRoot) {
        return bootGameViaRoot();
    }
    return bootGameViaActivity(env);
}

bool isAliveByForeground(JNIEnv *env) {
    if (g_isRoot) {
        return !pipeCommand(pid_of_pkg_c).empty();
    } else {
        if (cForegroundID != nullptr) {
            std::string gm_pkg = jstringTostring(env, (jstring) env->CallObjectMethod(g_thiz,
                                                                                      cForegroundID));
            return gm_pkg == package;
        }
    }
    return false;
}


int clickViaAccessibility(JNIEnv *env, const cv::Point &center, uint64_t ea = 60) {
    if (performID != nullptr) {
        if (ea) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ea));
        }
        env->CallVoidMethod(g_thiz, performID, (float) center.x, (float) center.y);
        return 1;
    }
    return 0;
}

int clickByPoint(JNIEnv *env, const cv::Point &center, uint64_t e = 0) {
    if (e) {
        std::this_thread::sleep_for(std::chrono::milliseconds(e));
    }
    if (g_isRoot) {
//        return clickViaRoot(center);
        return clickViaEvent(center);
    } else {
        return clickViaAccessibility(env, center);
    }
}

int backKeyViaRoot() {
    return execSuCommand((const char *[]) {KEYCODE_BACK_CMD, nullptr});
}


int backKeyViaAccessibility(JNIEnv *env) {
    if (actionBackID != nullptr) {
        env->CallVoidMethod(g_thiz, actionBackID);
        return 1;
    }
    return 0;
}

int playAudio(JNIEnv *env) {
    if (playAudioID != nullptr) {
        env->CallVoidMethod(g_thiz, playAudioID);
        return 1;
    }
    return 0;
}

std::vector<std::pair<cv::Point, float>>
extractMatchPoints(const cv::Mat &result, double threshold) {
    std::vector<std::pair<cv::Point, float>> points;
    if (!result.empty()) {
        for (int y = 0; y < result.rows; ++y) {
            const auto *row = result.ptr<float>(y);
            for (int x = 0; x < result.cols; ++x) {
                float score = row[x];
                if (score >= threshold) {
//         points.push_back(std::move(std::pair<cv::Point, float>())) ==  points.push_back(std::pair<cv::Point, float>())
//         等价points.push_back(std::move(point))调用移动构造
//         等价points.emplace_back(std::move(point))
                    points.emplace_back(cv::Point(x, y), score);

                }
            }
        }
    }
    return points;
}

template<typename... Args>
std::string formatMessage(const Args &...args) {
    g_format.str("");
    (g_format << ... << args);
    return g_format.str();
}


bool d_search_single(JNIEnv *env, const cv::Mat &t_mat, double threshold = 0.8,
                     const cv::Rect &roi = cv::Rect(), const cv::Mat &mat = cv::Mat(),
                     const bool click = true, const bool logs = false,
                     const char *format = "Evaluate:") {
    cv::Mat c_mat = mat.empty() ? acquireNextMat(env) : mat;
    if (c_mat.empty() || t_mat.empty()) {
        updateMessage(env, "Input images are empty!");
        return false;
    }
    if (!roi.empty()) {
        // 当roi为empty时,则无需corp减少开销
        c_mat = cropROI(c_mat, roi);
    }
    cv::Mat result;
    cv::Point min_loc, max_loc;
    double min_val, max_val;
    //    元素图像格式必须相同,同为CV_8UC4或CV_8UC,原始数据帧格式为RGBA,需要转为BGR,模板图像格式为BGR
    cv::matchTemplate(c_mat, t_mat, result, cv::TM_CCOEFF_NORMED);
    cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);
    if (max_val > threshold) {
        if (logs) {
            g_format << format << max_val << "\n";
            updateMessage(env, g_format.str().c_str());
            g_format.str("");
        }
        cv::Point center(
                roi.x + max_loc.x + t_mat.cols / 2,
                roi.y + max_loc.y + t_mat.rows / 2);
        click && clickByPoint(env, center);
        return true;
    }
    return false;
}

static int l_offset_x = 145;
static int l_offset_y = 140;
static int t_space = -14;
static int t_block = -192;
static int t_adjust = -4;
static int t4_offset_x = t_block + t_space + t_adjust;
static int t3_offset_x = t_block * 2 + t_space * 2 + t_adjust;
static int t2_offset_x = t_block * 3 + t_space * 3 + t_adjust;

void postMatchPoints(JNIEnv *env,
                     std::vector<std::pair<cv::Point, float>> &points,
                     const char *msgPrefix = "Evaluate:") {
    std::ostringstream oss;
    oss << msgPrefix << "\n";
    for (const auto &p: points) {
        oss << "X= " << p.first.x << "Y= " << p.first.y << " score: " << p.second << "\n";
    }
    updateMessage(env, oss.str().c_str());
}


bool
f_search_single(JNIEnv *env, const cv::Mat &t_mat, const cv::Mat &et_mat, double threshold = 0.95,
                const cv::Rect &roi = cv::Rect(), const bool then = false,
                const char *format = "Evaluate:") {
    cv::Mat c_mat;
    cv::Mat result;
    cv::Mat o_mat = acquireNextMat(env);
    if (o_mat.empty() || t_mat.empty() || et_mat.empty()) {
        updateMessage(env, "Input images are empty!");
        return false;
    }
    c_mat = cropROI(o_mat, roi);
    cv::matchTemplate(c_mat, t_mat, result, cv::TM_CCOEFF_NORMED);
    std::vector<std::pair<cv::Point, float>> points = extractMatchPoints(result, threshold);
//    postMatchPoints(env,points,"lv75");
    std::pair<cv::Point, float> *t_pt = nullptr;
    if (!points.empty()) {
//        引用
        for (std::pair<cv::Point, float> &pt: points) {
            CORP_GF_LV.x = roi.x + pt.first.x + t_mat.cols + l_offset_x;
            CORP_GF_LV.y = roi.y + pt.first.y + l_offset_y;
            CORP_GF_THEN_N.x = roi.x + pt.first.x + t2_offset_x;
            CORP_GF_THEN_N.y = roi.y + pt.first.y + t_mat.rows + t_adjust;
//        cv::imwrite(genRDString(), cropROI(o_mat, CORP_GF_LV));
            if (d_search_single(env, m_GF_75, 0.85, CORP_GF_LV, o_mat, false)) {
                if (d_search_single(env, m_GF_THEN_SLOT, 0.85, CORP_GF_THEN_N, o_mat, false)) {
                    continue;
                }
            }
            t_pt = &pt;
            break;
        }
    } else {
        cv::matchTemplate(c_mat, et_mat, result, cv::TM_CCOEFF_NORMED);
        std::vector<std::pair<cv::Point, float>> points_e = extractMatchPoints(result, threshold);
//        postMatchPoints(env,points_e,"lv80");
        if (!points_e.empty()) {
            for (std::pair<cv::Point, float> &pt: points_e) {
                CORP_GF_THEN_N.x = roi.x + pt.first.x + t3_offset_x;
                CORP_GF_THEN_N.y = roi.y + pt.first.y + t_adjust;
                if (d_search_single(env, m_GF_THEN_SLOT, 0.85, CORP_GF_THEN_N, o_mat, false)) {
                    continue;
                }
                t_pt = &pt;
                break;
            }
        }
    }
    if (t_pt != nullptr) {
        g_format << format << t_pt->second << "\n";
        updateMessage(env, g_format.str().c_str());
        g_format.str("");
        clickByPoint(env, cv::Point(
                roi.x + t_pt->first.x + et_mat.cols / 4,
                roi.y + t_pt->first.y + et_mat.rows / 2)
        );
        return true;
    }
    return false;
}

double m_search_single(JNIEnv *env, const cv::Mat &t_mat, const cv::Rect &roi = cv::Rect(),
                       const cv::Mat &mat = cv::Mat(), const char *format = "Evaluate:") {
    cv::Mat c_mat = mat.empty() ? acquireNextMat(env) : mat;
    if (c_mat.empty() || t_mat.empty()) {
        updateMessage(env, "Input images are empty!");
        return 0;
    }
    if (!roi.empty()) {
        c_mat = cropROI(c_mat, roi);
    }
    cv::Mat result;
    cv::Point min_loc, max_loc;
    double min_val, max_val;
    cv::matchTemplate(c_mat, t_mat, result, cv::TM_CCOEFF_NORMED);
    cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);
    g_format << format << max_val << "\n";
    updateMessage(env, g_format.str().c_str());
    g_format.str("");
    return max_val;
}


/*
cv::Mat hsv, patch;
patch = cv::imread("C:/Users/Administrator/Desktop/0/m10.jpg", cv::IMREAD_COLOR);

//cv::cvtColor(patch(cv::Rect(2300,1165,10,10)), hsv, cv::COLOR_BGR2HSV);
cv::cvtColor(patch(cv::Rect(1485,1520,10,10)), hsv, cv::COLOR_BGR2HSV);


//ΔH = 6
cv::Vec3b v = hsv.at<cv::Vec3b>(0, 0);
std::cout << "H=" << (int)v[0]
<< " S=" << (int)v[1]
<< " V=" << (int)v[2] << std::endl;


bool hit = cv::countNonZero(mask) > mask.total() * 0.9;

std::cout << cv::countNonZero(mask) << std::endl;
*/
cv::Scalar cv_8uc3ToHsv(const cv::Vec3b &rgb) {
    cv::Mat hsv;
    cv::Mat rgb_mat(1, 1, CV_8UC3);
    rgb_mat.at<cv::Vec3b>(0, 0) = rgb;
    cv::cvtColor(rgb_mat, hsv, cv::COLOR_RGB2HSV);
    cv::Vec3b hsv_v3b = hsv.at<cv::Vec3b>(0, 0);
    return cv::Scalar{static_cast<double>(hsv_v3b[0]),
                      static_cast<double>(hsv_v3b[1]),
                      static_cast<double>(hsv_v3b[2])};
}

bool hsvMaskRatio(JNIEnv *env, const cv::Mat &mat, const cv::Rect &roi, const cv::Scalar &lower,
                  const cv::Scalar &upper) {
    if (mat.empty()) {
        updateMessage(env, "[hsvMaskRatio:] Input images are empty!");
        return false;
    }
    cv::Mat hsv, mask;
    cv::Mat c_mat = cropROI(mat, roi);
    cv::cvtColor(c_mat, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(
            hsv,
            lower, upper,
            mask
    );
//    cv::Vec3b hsvPixel = hsv.at<cv::Vec3b>(4, 4);
//    g_format << cv::countNonZero(mask) << "\n";
//    g_format << "H="
//             << (int) hsvPixel[0]
//             << " S="
//             << (int) hsvPixel[1]
//             << " V="
//             << (int) hsvPixel[2];
//    updateMessage(env, g_format.str().c_str());
//    g_format.str("");
    return cv::countNonZero(mask) * 100 > (mask.total() * 90);
}

bool isHsvRange(JNIEnv *env, const cv::Rect &roi, const cv::Scalar &lower,
                const cv::Scalar &upper) {
    cv::Mat mat = acquireNextMat(env);
    return hsvMaskRatio(env, mat, roi, lower, upper);
}

bool isHsvRange(JNIEnv *env, const cv::Mat &mat, const cv::Rect &roi, const cv::Scalar &lower,
                const cv::Scalar &upper) {
    return hsvMaskRatio(env, mat, roi, lower, upper);
}


bool maskMatchPoints(JNIEnv *env, const cv::Mat &t_mat, const cv::Mat &mask,
                     const cv::Mat &mat,
                     double threshold = 0.95,
                     const cv::Rect &roi = cv::Rect(),
                     std::vector<std::pair<cv::Point, float>> *out_points = nullptr,
                     const bool clickable = true,
                     int methods = cv::TM_CCOEFF_NORMED,
                     const char *format = "Evaluate:") {
    cv::Mat c_mat, result;
    if (mat.empty() || t_mat.empty()) {
        updateMessage(env, "Input images are empty!");
        return false;
    }
    c_mat = cropROI(mat, roi);
//    cv::imwrite(genRDString(), c_mat);
    cv::Point min_loc, max_loc;
    double min_val, max_val;
//    TM_CCOEFF_NORMED  光照抵抗,整体亮度偏移不敏感,性能较差
    cv::matchTemplate(c_mat, t_mat, result, methods, mask);
    cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);
    g_format << format << max_val << "\n";
    updateMessage(env, g_format.str().c_str());
    g_format.str("");
    if ((max_val > threshold) && !std::isinf(max_val)) {
        if (out_points != nullptr) {
//          extractMatchPoints表达式自身是prvlaue,tmp自身没有接管资源，没有发生拷贝
//          auto tmp = std::move( extractMatchPoints(result, threshold));
            auto tmp = extractMatchPoints(result, threshold);
//          postMatchPoints(env, tmp);
//          out_points->swap(tmp);
            *out_points = std::move(tmp);
            return true;
        }
        if (clickable) {
            clickByPoint(env, cv::Point(
                    roi.x + max_loc.x + t_mat.cols / 2,
                    roi.y + max_loc.y + t_mat.rows / 2)
            );
        }
        return true;
    }
    return false;
}

bool
t_search_single(JNIEnv *env, const cv::Mat &t_mat, const cv::Mat &mask,
                double threshold = 0.95,
                const cv::Rect &roi = cv::Rect(),
                std::vector<std::pair<cv::Point, float>> *out_points = nullptr,
                int methods = cv::TM_CCOEFF_NORMED,
                const char *format = "Evaluate:"
) {
    cv::Mat mat = acquireNextMat(env);
    return maskMatchPoints(env, t_mat, mask, mat, threshold, roi, out_points, true, methods,
                           format);
}

bool
t_search_single(JNIEnv *env, const cv::Mat &t_mat, const cv::Mat &mask,
                const cv::Rect &roi = cv::Rect(),
                double threshold = 0.95
) {
    cv::Mat mat = acquireNextMat(env);
    return maskMatchPoints(env, t_mat, mask, mat, threshold, roi, nullptr, false);
}

bool
t_search_single(JNIEnv *env, const cv::Mat &t_mat, const cv::Mat &mask,
                const cv::Mat &mat,
                double threshold = 0.95,
                const cv::Rect &roi = cv::Rect(),
                std::vector<std::pair<cv::Point, float>> *out_points = nullptr,
                int methods = cv::TM_CCOEFF_NORMED,
                const char *format = "Evaluate:"
) {
    return maskMatchPoints(env, t_mat, mask, mat, threshold, roi, out_points, true, methods,
                           format);
}


cv::Mat
retry_until_success_mat(const std::function<cv::Mat()> &func, size_t max_attempts,
                        std::chrono::milliseconds interval_ms = std::chrono::milliseconds(0)) {
    cv::Mat t_mat;
    while (max_attempts-- > 0) {
        t_mat = func();
        if (!t_mat.empty()) return t_mat;
        if (interval_ms.count() > 0)std::this_thread::sleep_for(interval_ms);
    }
    return t_mat;
}

bool
retry_until_success(const std::function<bool()> &func, size_t max_attempts,
                    std::chrono::milliseconds interval_ms = std::chrono::milliseconds(0)) {
    cv::Mat t_mat;
    while (max_attempts-- > 0) {
        if (func()) return true;
        if (interval_ms.count() > 0)
            std::this_thread::sleep_for(interval_ms);
    }
    return false;
}

bool
retry_until_success_val(const std::function<double()> &func, size_t max_attempts,
                        double not_less_than,
                        double success_threshold,
                        std::chrono::milliseconds interval_ms = std::chrono::milliseconds(0)) {
    int consecutive_failures = 0;
    while (consecutive_failures < max_attempts) {
        double val = func();
        if (val > success_threshold) return true;
        if (val >= not_less_than) {
            consecutive_failures = 0;
        } else {
            ++consecutive_failures;
        }
        if (interval_ms.count() > 0)
            std::this_thread::sleep_for(interval_ms);
    }
    return false;
}

void defaultBehavior(JNIEnv *env) {
    if (!g_isRoot)return;
    std::string pid;
//    c_str返回const char* ,pipeCommand参数接受char* &cmd,传递的参数必须非临时的,额外的指针变量分配
    std::string appids = pipeCommand(pid_of_pkg_c);
    std::istringstream iss(appids);
    while (iss >> pid) {
        execSuCommand((const char *[]) {"renice", "-n", "-20", "-p", pid.c_str(), nullptr});
        execSuCommand(
                (const char *[]) {"ionice", "-n", "1", "-n", "0", "-p", pid.c_str(), nullptr});
    }
    updateMessage(env, appids.c_str());
}

void backKeyEvent(JNIEnv *env) {
    if (g_isRoot) {
        backKeyViaRoot();
    } else {
        backKeyViaAccessibility(env);
    }
}

bool backKeyMatch(JNIEnv *env,
                  const std::function<bool()> &func = nullptr,
                  int max_attempts = 1,
                  std::chrono::milliseconds wait_render = std::chrono::milliseconds(0)
) {
    bool matched = false;
    if (func) {
        for (int i = 0; i < max_attempts; ++i) {
            if (wait_render.count() > 0) {
                std::this_thread::sleep_for(wait_render);
            }
            if (func()) {
                matched = true;
                break;
            } else {
                updateMessage(env, "请确保处于世界地图中...");
                backKeyEvent(env);
                matched = false;
            }
        }
    }
    return matched;
}


void resetToWordLocation(JNIEnv *env, uint_t e = 0) {
    if (e > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(e));
    }
    if (d_search_single(env, m_WD, 0.85, CORP_HW, g_empty_mat)) {
        return;
    }
    if (retry_until_success([env]() {
        return !d_search_single(env, m_HD, 0.70, CORP_HW, g_empty_mat);
    }, 5, std::chrono::milliseconds(1500))) {
        clickByPoint(env, TownBase, 1500);
        return;
    }
}

void resources::init(JNIEnv *env, const std::string &_user_) {
    ARMAMENTS_FREE_RP = getAbsolutePath("image", _user_, "free.PNG");
//    ARMAMENTS_TEAM_RP = getAbsolutePath("image", _user_, "team.PNG");
    DIG_TIP_RP = getAbsolutePath("image", _user_, "small.PNG");
    DIG_DRONES_RP = getAbsolutePath("image", _user_, "drones.PNG");
    DIG_TREASURE_RP = getAbsolutePath("image", _user_, "treasure.PNG");
    DIG_AWARD_DRONES_RP = getAbsolutePath("image", _user_, "award_drones.PNG");
    DIG_AWARD_TREASURE_RP = getAbsolutePath("image", _user_, "award_treasure.PNG");
    DIG_GIFT_BOX_RP = getAbsolutePath("image", _user_, "gift_box.PNG");
    DIG_DONE_RP = getAbsolutePath("image", _user_, "treasure_done.PNG");
    EGG_TIP_RP = getAbsolutePath("image", _user_, "eggs_tips.PNG");
    POST_TIP_RP = getAbsolutePath("image", _user_, "post.PNG");
    EGG_PACKAGE_RP = getAbsolutePath("image", _user_, "eggs.PNG");
    GENE_JOIN_RP = getAbsolutePath("image", _user_, "join.PNG");
    GOLD_JOIN_RP = getAbsolutePath("image", _user_, "join_gold.PNG");
    GOLD_JOIN_THEN80_RP = getAbsolutePath("image", _user_, "join_gold_then80.PNG");
//    GOLD_JOIN_SCORPION_RP = getAbsolutePath("image", _user_, "join_scorpion.PNG");
    GOLD_TARGET_RP = getAbsolutePath("image", _user_, "target.PNG");
    GOLD_FILTER_70_RP = getAbsolutePath("image", _user_, "lv_70_75.PNG");
    GOLD_FILTER_75_RP = getAbsolutePath("image", _user_, "lv_75.PNG");
    GOLD_FILTER_THEN1_RP = getAbsolutePath("image", _user_, "then1.PNG");
    GOLD_FILTER_THEN4_RP = getAbsolutePath("image", _user_, "free_slot.PNG");
//    GOLD_TARGET_RP_REL = getAbsolutePath("image", _user_, "target_rel.PNG");
    HOME_DETECT_RP = getAbsolutePath("image", _user_, "home.PNG");
    WORD_DETECT_RP = getAbsolutePath("image", _user_, "word.PNG");
    CANCEL_FLAG_RP = getAbsolutePath("image", _user_, "cancel.PNG");
    FINED_RP = getAbsolutePath("find");


    attack_axe_rp = getAbsolutePath("image", _user_, "attack_axe.PNG");
    arrow_switch_rp = getAbsolutePath("image", _user_, "arrow_switch.PNG");
    win_stars_rp = getAbsolutePath("image", _user_, "win_stars.PNG");
    carve_next_rp = getAbsolutePath("image", _user_, "carve_next.PNG");
    car_holy_rp = getAbsolutePath("image", _user_, "car_holy.PNG");
    daily_event_rp = getAbsolutePath("image", _user_, "daily_event.PNG");
    sonar_event_rp = getAbsolutePath("image", _user_, "sonar_event.PNG");

    mask_axe_rp = getAbsolutePath("image", _user_, "mask_axe.PNG");
    mask_switch_rp = getAbsolutePath("image", _user_, "mask_switch.PNG");
    mask_stars_rp = getAbsolutePath("image", _user_, "mask_stars.PNG");
    mask_holy_rp = getAbsolutePath("image", _user_, "mask_holy.PNG");
    mask_daily_event_rp = getAbsolutePath("image", _user_, "mask_daily_event.PNG");

    events = getAbsolutePath("bin", "events");
    performance_click = getAbsolutePath("bin", "performance_click");
    zoom_events = getAbsolutePath("bin", "zoom_events");


    t_attack_axe = cv::imread(attack_axe_rp, cv::IMREAD_COLOR);
    t_arrow_switch = cv::imread(arrow_switch_rp, cv::IMREAD_COLOR);
    t_win_stars = cv::imread(win_stars_rp, cv::IMREAD_COLOR);
    t_carve_next = cv::imread(carve_next_rp, cv::IMREAD_COLOR);
    t_car_holy = cv::imread(car_holy_rp, cv::IMREAD_COLOR);
    t_daily_event = cv::imread(daily_event_rp, cv::IMREAD_COLOR);

    t_sonar_event = cv::imread(sonar_event_rp, cv::IMREAD_COLOR);


    m_attack_axe = cv::imread(mask_axe_rp, cv::IMREAD_GRAYSCALE);
    m_arrow_switch = cv::imread(mask_switch_rp, cv::IMREAD_GRAYSCALE);
    m_win_stars = cv::imread(mask_stars_rp, cv::IMREAD_GRAYSCALE);
    m_car_holy = cv::imread(mask_holy_rp, cv::IMREAD_GRAYSCALE);
    m_daily_event = cv::imread(mask_daily_event_rp, cv::IMREAD_GRAYSCALE);


    m_GT = cv::imread(GOLD_TARGET_RP, cv::IMREAD_COLOR);
//    m_GT = cv::imread(GOLD_TARGET_RP, cv::IMREAD_GRAYSCALE);
//    m_GT = cv::imread(GOLD_TARGET_RP, cv::IMREAD_GRAYSCALE);
//    m_GT_REL = cv::imread(GOLD_TARGET_RP_REL, cv::IMREAD_COLOR);

    m_GJ = cv::imread(GENE_JOIN_RP, cv::IMREAD_COLOR);
    m_GJ_G = cv::imread(GOLD_JOIN_RP, cv::IMREAD_COLOR);
    m_GJ_T80 = cv::imread(GOLD_JOIN_THEN80_RP, cv::IMREAD_COLOR);
//    m_GJ_s = cv::imread(GOLD_JOIN_SCORPION_RP, cv::IMREAD_COLOR);
    m_GF_70 = cv::imread(GOLD_FILTER_70_RP, cv::IMREAD_COLOR);
    m_GF_75 = cv::imread(GOLD_FILTER_75_RP, cv::IMREAD_COLOR);
    m_GF_THEN1 = cv::imread(GOLD_FILTER_THEN1_RP, cv::IMREAD_COLOR);
    m_GF_THEN4 = cv::imread(GOLD_FILTER_THEN4_RP, cv::IMREAD_COLOR);

    m_AF = cv::imread(ARMAMENTS_FREE_RP, cv::IMREAD_COLOR);
//    m_AT = cv::imread(ARMAMENTS_TEAM_RP, cv::IMREAD_COLOR);

    m_HD = cv::imread(HOME_DETECT_RP, cv::IMREAD_COLOR);
    m_WD = cv::imread(WORD_DETECT_RP, cv::IMREAD_COLOR);
    m_DP = cv::imread(DIG_TIP_RP, cv::IMREAD_COLOR);
    m_DD = cv::imread(DIG_DRONES_RP, cv::IMREAD_COLOR);
    m_DT = cv::imread(DIG_TREASURE_RP, cv::IMREAD_COLOR);
    m_DA_D = cv::imread(DIG_AWARD_DRONES_RP, cv::IMREAD_COLOR);
    m_DA_T = cv::imread(DIG_AWARD_TREASURE_RP, cv::IMREAD_COLOR);
    m_GB = cv::imread(DIG_GIFT_BOX_RP, cv::IMREAD_COLOR);
    m_CF = cv::imread(CANCEL_FLAG_RP, cv::IMREAD_COLOR);
    m_DO = cv::imread(DIG_DONE_RP, cv::IMREAD_COLOR);

    m_ET = cv::imread(EGG_TIP_RP, cv::IMREAD_COLOR);
    m_PT = cv::imread(POST_TIP_RP, cv::IMREAD_COLOR);
    m_EP = cv::imread(EGG_PACKAGE_RP, cv::IMREAD_COLOR);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_initJNIThread(JNIEnv *env, jobject thiz, jint width,
                                                      jint height, jboolean is_root, jstring ssaid,
                                                      jstring external, jobjectArray jary
) {
    if (g_vm == nullptr) {
        if (env->GetJavaVM(&g_vm) != JNI_OK) {
//          Failed to get JavaVM
            return;
        }
    }
    if (g_thiz != nullptr) {
        env->DeleteGlobalRef(g_thiz);
        g_thiz = nullptr;
    }
    g_thiz = env->NewGlobalRef(thiz);
    jclass clazz = env->GetObjectClass(g_thiz);
    g_clazz = (jclass) env->NewGlobalRef(clazz);
    env->DeleteLocalRef(clazz);
    g_rows = width;
    g_cols = height;
    g_isRoot = is_root;
    private_storage = std::move(jstringTostring(env, external));
    carve_paths = std::move(jstringArrayTovector(env, jary));
    captureByteID = requestJavaMethodsID(env, "captureOnceSync_byte", "()[B");
    captureBufferID = requestJavaMethodsID(env, "captureOnceSync_buffer",
                                           "()Ljava/nio/ByteBuffer;");
    captureImageID = requestJavaMethodsID(env, "captureOnceSync_image", "()Landroid/media/Image;");
    updateID = requestJavaMethodsID(env, "updateMessage", "(Ljava/lang/String;)V");
    performID = requestJavaMethodsID(env, "performClickPoint", "(FF)V");
    actionBackID = requestJavaMethodsID(env, "actionBackAccess", "()V");
    playAudioID = requestJavaMethodsID(env, "playAudioInActivity", "()V");
    cForegroundID = requestJavaMethodsID(env, "foregroundApplication", "()Ljava/lang/String;");
    bootGameID = requestJavaMethodsID(env, "bootGameWithZero", "()V");
    rejectNetworkID = requestJavaMethodsID(env, "rejectNetworkBeach", "()V");
    resumeNetworkID = requestJavaMethodsID(env, "resumeNetworkBeach", "()V");
    const std::string _user_ = jstringTostring(env, ssaid);
    resources::init(env, _user_);
    g_input_device = pipeCommand(events.c_str());
    if (_user_ == "[hash]") {
//        override
    }
}

void clickWithRandomOffset(JNIEnv *env,
                           const cv::Point &basePoint,
                           size_t count = 70,
                           int radius = 10,
                           uint_t delay = 50,
                           const std::function<bool()> &func = nullptr,
                           size_t maxRetries = 10,
                           size_t retries = 0
) {
    bool needRepeat = true;
    while (needRepeat && retries++ < maxRetries) {
        needRepeat = false;
        for (size_t i = 0; i < count; ++i) {
            clickByPoint(env, threadSafeRandomOffsetPoint(basePoint, radius), delay);
        }
        if (func && func()) {
            updateMessage(env, "匹配误差超时");
            needRepeat = true;
        }
    }
}


cv::Mat toGray(const cv::Mat &src) {
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }
    return src;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_mainOpenCvLastWar(JNIEnv *env_, jobject thiz_, jint mode,
                                                          jboolean assembly_filter
) {
//    线程启动,函数已经结束,参数变量被销毁,引用被销毁,必须拷贝
    std::thread([&, mode, assembly_filter]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        if (unique_thread.load()) {
            updateMessage(tEnv, "单例线程,请勿同时打开多个开关");
            g_vm->DetachCurrentThread();
            return;
        }
        unique_thread.store(true);
        if (g_isRoot) {
            defaultBehavior(tEnv);
            updateMessage(tEnv, "当前用户模式:ROOT");
        } else {
            updateMessage(tEnv, "当前用户模式:NORMAL");
        }
        if (m_GT.empty()) {
            updateMessage(tEnv, "GT image empty...");
        }
        if (m_GJ.empty()) {
            updateMessage(tEnv, "GJ image empty...");
        }
        if (m_GJ_T80.empty()) {
            updateMessage(tEnv, "m_GJ_T80 image empty...");
        }
        if (m_GF_70.empty() && m_GF_75.empty()) {
            updateMessage(tEnv, "m_GF_70_75 is empty");
        }
        if (m_HD.empty()) {
            updateMessage(tEnv, "HD image empty...");
        }
        if (m_ET.empty()) {
            updateMessage(tEnv, "ET image empty...");
        }
        if (m_PT.empty()) {
            updateMessage(tEnv, "PT image empty...");
        }
        if (m_GB.empty()) {
            updateMessage(tEnv, "GB image empty...");
        }
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "屏幕数据帧捕获正常");
        }
        switch (mode) {
            case 0x1:
                updateMessage(tEnv, "通用模式");
                break;
            case 0x2:
                updateMessage(tEnv, "金币丧尸");
                break;
            default:
                updateMessage(tEnv, "可把你能的,居然卡到bug了");
        }
        if (assembly_filter) {
            updateMessage(tEnv, "规则:过滤lv75|组队人数不为(1/5)lv80以上|队列末尾");
        }
        static bool join;
        while (treasure_enable.load() || assembly_enable.load() || eggs_enable.load()) {
            {
                std::unique_lock<std::mutex> lock(pause_mutex);
                pause_cv.wait(lock, [] {
                    return !pause_thread.load();
                });
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(g_main_msec.load()));
            cv::Mat c_mat = acquireNextMat(tEnv);
            if (keep_alive_enable.load()) {
                d_search_single(tEnv, m_CF, 0.75, CORP_CANCEL_LIMIT, c_mat);
            }
            if (c_mat.empty()) {
                updateMessage(tEnv, "c_mat is empty!");
                continue;
            }
            if (assembly_enable.load()) {
                if (d_search_single(tEnv, m_GT, 0.75, CORP_GT, c_mat)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    switch (mode) {
                        case 0x1:
                            join = d_search_single(tEnv, m_GJ, 0.85, CORP_GJ_LIMIT,
                                                   g_empty_mat);
                            break;
                        case 0x2:
                            join = f_search_single(tEnv, m_GJ_G, m_GJ_T80, 0.95, CORP_GJ_LIMIT,
                                                   "金币丧尸:");
                            break;
                        default:
                            break;
                    }
                    if (join) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(600));
                        clickByPoint(tEnv, Dispatch);
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    } else {
                        clickByPoint(tEnv, Back);
                    }
                } else {
                    backKeyMatch(tEnv, [&tEnv, &c_mat]() {
                        return d_search_single(tEnv, m_HD, 0.9, CORP_HW, c_mat, false);
                    });
                }
            }
            if (treasure_enable.load()) {
                if (d_search_single(tEnv, m_PT, 0.70, CORP_DP, c_mat)) {
                    backKeyMatch(tEnv, [&tEnv]() {
                        return d_search_single(tEnv, m_HD, 0.9, CORP_HW, g_empty_mat, false);
                    }, 5, std::chrono::milliseconds(500));
                }
                if (d_search_single(tEnv, m_DP, 0.70, CORP_DP, c_mat)) {
                    updateMessage(tEnv, "发现挖掘机");
                    playAudio(tEnv);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    cv::Mat t_mat_ = retry_until_success_mat([&tEnv]() {
                        cv::Mat d_mat = acquireNextMat(tEnv);
                        if (d_search_single(tEnv, m_DT, 0.85, CORP_TD_LIMIT, d_mat)) {
                            return m_DA_T;
                        }
                        if (d_search_single(tEnv, m_DD, 0.85, CORP_TD_LIMIT, d_mat)) {
                            return m_DA_D;
                        }
                        return g_empty_mat;
                    }, 15, std::chrono::milliseconds(300));
                    if (!t_mat_.empty()) {
                        updateMessage(tEnv, "发现挖掘坐标");
                        clickByPoint(tEnv, PrepDig, 1500);
                        clickByPoint(tEnv, Digg, 1500);
                        clickByPoint(tEnv, Dispatch, 1500);
                        bool dig_complete = retry_until_success_val(
                                [&tEnv, &t_mat_]() {
                                    return m_search_single(tEnv, t_mat_, CORP_DA, g_empty_mat,
                                                           "挖掘进度:");
                                },
                                30,
                                0.5,
                                0.95,
                                std::chrono::milliseconds(500)
                        );
                        if (dig_complete) {
                            updateMessage(tEnv, "开始点击...");
                            clickWithRandomOffset(tEnv, PrepDig, 60, 10, 50, [&tEnv, &t_mat_]() {
                                return d_search_single(tEnv, t_mat_, 0.75, CORP_DA, g_empty_mat);
                            });
                            updateMessage(tEnv, "超时复查...");
                            retry_until_success([&tEnv]() {
                                d_search_single(tEnv, m_GB, 0.65, CORP_GB_LIMIT, g_empty_mat);
                                return false;
                            }, 10, std::chrono::milliseconds(500));
                            updateMessage(tEnv, "不知道有没有抢到>_<");
                            backKeyMatch(tEnv,
                                         [&tEnv]() {
                                             return !d_search_single(tEnv, m_DO, 0.85);
                                         }, 5, std::chrono::milliseconds(500));
                        }
                        backKeyMatch(tEnv,
                                     [&tEnv]() {
                                         return d_search_single(tEnv, m_HD, 0.85, CORP_HW,
                                                                g_empty_mat, false);
                                     }, 5, std::chrono::milliseconds(500));
                        resetToWordLocation(tEnv, 1000);
                    }
                }
            }
            if (eggs_enable.load()) {
                if (d_search_single(tEnv, m_ET, 0.70, CORP_DP, c_mat)) {
                    updateMessage(tEnv, "发现金蛋...");
                    bool success = retry_until_success([&tEnv]() {
                        return d_search_single(tEnv, m_EP, 0.85, CORP_EGG_LIMIT, g_empty_mat);
                    }, 15, std::chrono::milliseconds(500));
                    if (success) {
                        updateMessage(tEnv, "开始点击...");
                        for (int i = 0; i < 10; ++i) {
                            clickByPoint(tEnv, OpenEgg, 100);
                        }
                        updateMessage(tEnv, "也许抢到了...");
                        backKeyMatch(tEnv,
                                     [&tEnv]() {
                                         return d_search_single(tEnv, m_HD, 0.9, CORP_HW,
                                                                g_empty_mat, false);
                                     }, 10, std::chrono::milliseconds(500));
                    }
                }
            }
        }
        updateMessage(tEnv, "进程暂停中..");
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootAutoAssembly(JNIEnv *env, jobject thiz) {
    assembly_enable.store(true);
    updateMessage(env, "监听组队...");
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopAutoAssembly(JNIEnv *env, jobject thiz) {
    assembly_enable.store(false);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootTreasureDigs(JNIEnv
                                                         *env, jobject thiz) {
    treasure_enable.store(true);
    updateMessage(env, "挖机机监听...");
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopTreasureDigs(JNIEnv *env, jobject thiz) {
    treasure_enable.store(false);
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootAutoEggs(JNIEnv *env, jobject thiz) {
    eggs_enable.store(true);
    updateMessage(env, "金蛋监听...");
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopAutoEggs(JNIEnv *env, jobject thiz) {
    eggs_enable.store(false);
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_saveFramePicture(JNIEnv *env, jobject thiz) {
    updateMessage(env, "保存数据帧...");
    std::thread([&]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        if (unique_thread.load()) {
            updateMessage(tEnv, "单例线程,请勿同时打开多个开关");
            g_vm->DetachCurrentThread();
            return;
        }
        unique_thread.store(true);
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "屏幕数据帧捕获正常");
        }
        clickWithRandomOffset(tEnv, PrepDig, 60, 10, 50);
        /*for (int i = 0; i < 5; ++i) {
            string s = genRDString();
            cv::imwrite(s, acquireNextMat(tEnv));
            updateMessage(tEnv, s.c_str());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }*/
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).detach();
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootAutoArmaments(JNIEnv *env, jobject thiz,
                                                          jboolean single) {
//UI线程频繁调用boot/stop导致的重复的thread执行？
//当armaments_enable被设置为false时上次的while (armaments_enable.load())循环并没有立即退出
//新的boot函数又继续启动且armaments_enable满足启动条件新的线程启动，旧的线程while读取armaments_enable为ture没有退出导致两个线程同时运行
    if (!armaments_enable.load()) {
        armaments_enable.store(true);
        updateMessage(env, "自动军备...");
        std::thread([&, single]() -> void {
            JNIEnv *tEnv = getEnv();
            if (!tEnv) return;
            if (unique_thread.load()) {
                updateMessage(tEnv, "单例线程,请勿同时打开多个开关");
                g_vm->DetachCurrentThread();
                return;
            }
            unique_thread.store(true);
            if (m_AF.empty()) {
                updateMessage(tEnv, "cat AF image empty...");
            }
//            if (m_AT.empty()) {
//                updateMessage(tEnv, "cat AT image empty...");
//            }
            if (!acquireNextMat(tEnv).empty()) {
                updateMessage(tEnv, "屏幕数据帧捕获正常");
            }
            if (single) {
                updateMessage(tEnv, "仅莫妮卡");
            }
            while (armaments_enable.load()) {
                {
                    std::unique_lock<std::mutex> lock(pause_mutex);
                    pause_cv.wait(lock, [] {
                        return !pause_thread.load();
                    });
                }
                cv::Mat c_mat = acquireNextMat(tEnv);
                if (backKeyMatch(tEnv, [&tEnv, &c_mat]() {
                    return d_search_single(tEnv, m_HD, 0.9, CORP_HW, c_mat, false);
                })) {
                    if (!single || d_search_single(tEnv, m_AF, 0.85, CORP_AF, c_mat, false)) {
                        clickByPoint(tEnv, Magnifier, 1000);
                        clickByPoint(tEnv, Search, 1000);
                        clickByPoint(tEnv, TeamUp, 1000);
                        clickByPoint(tEnv, Dispatch, 1000);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
            updateMessage(tEnv, "进程暂停中..");
//            解决方案是在线程启动时unique_thread锁定，在线程完全结束时释放锁定
            g_vm->DetachCurrentThread();
            unique_thread.store(false);
        }).detach();
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopAutoArmaments(JNIEnv *env, jobject thiz) {
    armaments_enable.store(false);
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_syncJNIArgs(JNIEnv *env, jobject thiz, jlong main_msec,
                                                    jint row_stride) {
    g_main_msec.store(main_msec);
    g_row_stride = row_stride;
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_skipGamePlacard(JNIEnv *env, jobject thiz) {
    std::thread([&]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        unique_thread.store(true);
        if (m_CF.empty()) {
            updateMessage(tEnv, "cat SP image empty...");
        }
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "屏幕数据帧捕获正常");
        }
        updateMessage(tEnv, "等待跳过前置公告!!!");
        bool alive = isAliveByForeground(tEnv);
        if (!alive) {
            retry_until_success([&tEnv]() {
                d_search_single(tEnv, m_CF, 0.7, cv::Rect());
                return false;
            }, 15, std::chrono::milliseconds(1000));
        }
        resetToWordLocation(tEnv);
        updateMessage(tEnv, "跳过完成!目前在主基地...");
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).join();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_trackedKeepAlive(JNIEnv *env, jobject thiz) {
    keep_alive_enable.store(true);
    std::thread([&]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        if (alive_thread_lock.load()) {
            updateMessage(tEnv, "旁路保护运行中!");
            g_vm->DetachCurrentThread();
            return;
        }
        alive_thread_lock.store(true);
        static std::chrono::milliseconds cycle(g_main_msec.load() * 30);
        updateMessage(tEnv, ("轮询周期:" + std::to_string(cycle.count())).c_str());
        while (keep_alive_enable.load()) {
            updateMessage(tEnv, "bypass cycle");
            bool alive = isAliveByForeground(tEnv);
            if (!alive) {
//     Java_com_example_supercell_MainActivity_skipGamePlacard(env, thiz);
                updateMessage(tEnv, "尝试重启...");
                pause_thread.store(true);
                pause_cv.notify_all();
                int exit = bootGameZero(tEnv);
                if (exit == EXIT_SUCCESS) {
                    updateMessage(tEnv, "主线程暂停");
                    retry_until_success([&tEnv]() {
                        d_search_single(tEnv, m_CF, 0.7, cv::Rect());
                        return false;
                    }, 15, std::chrono::milliseconds(1000));
                    pause_thread.store(false);
                    pause_cv.notify_all();
                    updateMessage(tEnv, "主线程恢复");
                }
                resetToWordLocation(tEnv);
            }
            std::this_thread::sleep_for(cycle);
        }
        updateMessage(tEnv, "旁路保护结束!");
        g_vm->DetachCurrentThread();

//        default:memory_order_seq_cst 保证所有原子操作的全局时序一致性,任意线程在store函数之前的内存访问都不会在store函数之后
//        线程中load会自旋等待其他线程中的store完成,轮询而非挂起
        alive_thread_lock.store(false);
    }).detach();

}



//adb connect 172.168.1.7:5555


static std::atomic<bool> clans_enable(false);
static std::atomic<bool> beach_enable(false);
constexpr int hero_deploy_offset = 200;
constexpr int win2_stars_offset = 73;

bool deployTroopLocation(JNIEnv *env, std::vector<std::pair<cv::Point, float >> &points) {
    for (const cv::Point &dp: DEPLOY_BASES) {
        clickByPoint(env, cv::Point(
                points[0].first.x + corp_switch_limit.x - hero_deploy_offset,
                points[0].first.y + corp_switch_limit.y - t_arrow_switch.cols));
        clickByPoint(env, dp, 100);
        for (std::pair<cv::Point, float> &pt: points) {
            cv::Point dest(
                    corp_switch_limit.x + pt.first.x,
                    corp_switch_limit.y + pt.first.y - t_arrow_switch.cols);
            clickByPoint(env, dest, 100);
            clickByPoint(env, dp, 100);
            clickByPoint(env, dest, 100);
        }
        if (!t_search_single(env, t_arrow_switch, m_arrow_switch, corp_switch_limit, 0.96)) {
            return true;
        }
    }
    return false;
}


void assert_meanStdDev(cv::Mat &mat, cv::Mat &mask) {
    cv::Mat masked_temp;
    mat.copyTo(masked_temp, mask);
    cv::Scalar mean, stddev;
    cv::meanStdDev(masked_temp, mean, stddev, mask);
    LOGD("%s", std::to_string(stddev[0]).c_str());
}

void assert_countNonZero(cv::Mat &mask) {
    if (mask.type() == CV_8U) {
        LOGD("%s", std::to_string(cv::countNonZero(mask)).c_str());
    }
}


static size_t g_count;
extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootClansLoop(JNIEnv *env, jobject thiz,
                                                      jboolean win2_stars) {
//73px  X_offset
    clans_enable.store(true);
    cv::Rect corp_stars(2436, 1387, 70, 70);
    if (win2_stars) {
//        lambda捕获值拷贝默认const,mutable解除
        corp_stars.x += win2_stars_offset;
    }
    std::thread([&, corp_stars]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        if (unique_thread.load()) {
            updateMessage(tEnv, "点你吗,请勿同时打开多个开关");
            g_vm->DetachCurrentThread();
            return;
        }
        unique_thread.store(true);
        if (t_attack_axe.empty() ||
            t_arrow_switch.empty() ||
            t_win_stars.empty() ||
            m_attack_axe.empty() ||
            m_arrow_switch.empty() ||
            m_win_stars.empty()) {
            updateMessage(tEnv, "模板缺失!线程终止");
            g_vm->DetachCurrentThread();
        }
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "屏幕数据帧捕获正常");
        }
        while (clans_enable.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(g_main_msec.load()));
            if (g_count >= 2) {
                bool home = retry_until_success([&tEnv]() {
                    return t_search_single(tEnv, t_attack_axe, m_attack_axe, corp_attack, 0.96);
                }, 5, std::chrono::milliseconds(1000));
                if (home) {
                    std::string rStr = std::to_string(g_rotation.load());
                    const char *cmd[] = {zoom_events.c_str(), g_input_device.c_str(), rStr.c_str(),
                                         nullptr};
                    execSuCommand(cmd);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    if (t_search_single(tEnv, t_car_holy, m_car_holy, 0.98, car_holy_limit, nullptr,
                                        cv::TM_CCORR_NORMED)) {
                        clickByPoint(tEnv, COLLECT_BTN, 500);
                    }
                    g_count = 0;
                    continue;
                }
            }
            if (t_search_single(tEnv, t_attack_axe, m_attack_axe, 0.85, corp_attack)) {
                bool in = retry_until_success([&tEnv]() {
                    return isHsvRange(tEnv, begin_attack_sample, confirm_lower, confirm_upper);
                }, 5, std::chrono::milliseconds(250));
                if (in) {
                    std::vector<std::pair<cv::Point, float>> points;
                    clickByPoint(tEnv, cv::Point(begin_attack_sample.x, begin_attack_sample.y));
                    bool success = retry_until_success([&tEnv, &points]() {
                        return t_search_single(tEnv, t_arrow_switch, m_arrow_switch, 0.96,
                                               corp_switch_limit, &points);
                    }, 10, std::chrono::milliseconds(1000));
                    if (success && !points.empty()) {
                        postMatchPoints(tEnv, points, "points: ");
                        if (!deployTroopLocation(tEnv, points)) {
                            clickByPoint(tEnv, OVER_ATTACK);
                            clickByPoint(tEnv, OVER_ATTACK_ENTER, 300);
                            break;
                        }
                        bool success_win = retry_until_success([&tEnv, &corp_stars]() {
                            cv::Mat shard = acquireNextMat(tEnv);
                            return t_search_single(tEnv, t_win_stars, m_win_stars, shard, 0.95,
                                                   corp_stars) ||
                                   isHsvRange(tEnv, shard, back_home_sample, confirm_lower,
                                              confirm_upper);
                        }, 60, std::chrono::milliseconds(1000));
                        if (success_win) {
                            clickByPoint(tEnv, OVER_ATTACK);
                            clickByPoint(tEnv, OVER_ATTACK_ENTER, 300);
                            in = retry_until_success([&tEnv]() {
                                return isHsvRange(tEnv, back_home_sample, confirm_lower,
                                                  confirm_upper);
                            }, 5, std::chrono::milliseconds(250));
                            if (in) {
                                clickByPoint(tEnv,
                                             cv::Point(back_home_sample.x, back_home_sample.y));
                                g_count++;
                            }
                        }
                    }
//                    防止外层检测
                    continue;
                }
//                防止其它UI遮挡进攻按钮
                backKeyEvent(tEnv);
                continue;
            }
//          通用去除遮挡
            backKeyEvent(tEnv);
        }
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).detach();

}

constexpr int crystal_base_offset_x = 384;
constexpr int crystal_width = 482;
constexpr int crystal_space = 51;
constexpr int crystal_step = crystal_width + crystal_space;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootBeachLoop(JNIEnv *env, jobject thiz, jint crystal) {
    beach_enable.store(true);
    cv::Point base_btn(crystal_base_offset_x + (crystal_width >> 1), 1350);
    cv::Rect attr_carve_area(crystal_base_offset_x, 425, crystal_width, 90);
    cv::Rect corp_carve_next(crystal_base_offset_x, 1290, crystal_width, 140);
    if (crystal >= 1 && crystal <= 3) {
        int crystal_total = crystal * crystal_step;
        attr_carve_area.x += crystal_total;
        base_btn.x += crystal_total;
        corp_carve_next.x += crystal_total;
    }
    std::thread([&, crystal, base_btn, attr_carve_area, corp_carve_next]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        unique_thread.store(true);
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "屏幕数据帧捕获正常");
        }
        std::string name = std::to_string(crystal);
        updateMessage(tEnv, name.c_str()
        );
        size_t index = 0;
        size_t max = 12;
        while (beach_enable.load() && (index < max)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(g_main_msec.load()));
            clickByPoint(tEnv, base_btn, 300);
            clickByPoint(tEnv, base_btn, 300);
            clickByPoint(tEnv, DIAM_SPEED, 300);
            clickByPoint(tEnv, DIAM_SPENT, 300);
            clickByPoint(tEnv, base_btn, 500);
            clickByPoint(tEnv, base_btn, 800);
            retry_until_success([&tEnv, &index, &crystal, &corp_carve_next, &attr_carve_area]() {
                cv::Mat mat = acquireNextMat(tEnv);
                bool in = t_search_single(tEnv, t_carve_next, g_empty_mat, mat, 0.85,
                                          corp_carve_next);
                return in && cv::imwrite(fnv1a_resource_prefix(index, carve_paths.at(crystal)),
                                         cropROI(mat, attr_carve_area));
            }, 5, std::chrono::milliseconds(250));
            index++;
        }
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).detach();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopClansLoop(JNIEnv *env, jobject thiz) {
    clans_enable.store(false);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopBeachLoop(JNIEnv *env, jobject thiz) {
    beach_enable.store(false);
}


static std::atomic<bool> sonar_enable(false);
const char *svc_wifi_enable[] = {"svc", "wifi", "enable", nullptr};
const char *svc_wifi_disable[] = {"svc", "wifi", "disable", nullptr};
const static fs::path sonar_fragment_path("/sdcard/DCIM/Sonar/");
cv::Point network_retry(800, 990);//网络重试
cv::Point daily_event(2780, 640);//日常事件
cv::Point sonar_enter(2680, 1605);//声呐入口
cv::Point red_bomb(2720, 1650);//集束炸弹
cv::Point blue_bomb(2485, 1650);//普通炸弹
static std::vector<cv::Point> sonar_samples =
        {
                cv::Point(990, 720),
                cv::Point(2010, 720),
                cv::Point(1490, 400),
                cv::Point(1500, 650),
                cv::Point(1500, 1110)
        };
/*  {
  cv::Point(570, 720),
  cv::Point(990, 720),
  cv::Point(2420, 720),
  cv::Point(2010, 720),

  cv::Point(1490, 270),
  cv::Point(1480, 650),
  cv::Point(1480, 1100),
  cv::Point(1480, 1470),

  cv::Point(1010, 455),
  cv::Point(965, 1025),
  cv::Point(1880, 395),
  cv::Point(1800, 1080)
};*/

extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_bootSonarLoop(JNIEnv *env, jobject thiz, jint sample) {
    sonar_enable.store(true);
    std::thread([&]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        if (unique_thread.load()) {
            updateMessage(tEnv, "单例线程,请勿同时打开多个开关");
            g_vm->DetachCurrentThread();
            return;
        }
        unique_thread.store(true);
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "屏幕数据帧捕获正常");
        }
        if (t_daily_event.empty() ||
            m_daily_event.empty() ||
            t_sonar_event.empty()
                ) {
            updateMessage(tEnv, "模板缺失!线程终止");
            g_vm->DetachCurrentThread();
        }
        int sample_index = 0;
        while (sonar_enable.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (isHsvRange(tEnv, retry_network_sample, retry_network_lower, retry_network_upper)) {
                updateMessage(tEnv, "重新连接中");
                execSuCommand(svc_wifi_enable);
                clickByPoint(tEnv, network_retry, 1000);
            }
            if (t_search_single(tEnv, t_daily_event, m_daily_event, daily_event_btn, 0.85)) {
                g_format << "当前采样点:" << "X: " << sonar_samples[sample_index].x << "Y: "
                         << sonar_samples[sample_index].y;
                updateMessage(tEnv, g_format.str().c_str());
                g_format.str("");
                updateMessage(tEnv, "开启丢包");
                rejectNetwork(tEnv);
                updateMessage(tEnv, "发现日常活动");
                clickByPoint(tEnv, daily_event, 200);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                bool enter = retry_until_success([&tEnv]() {
                    if (!t_search_single(tEnv, t_sonar_event, g_empty_mat, sonar_event_btn, 0.85)) {
                        return swipeViaRoot(SWIPE_SONAR_BEGIN, SWIPE_SONAR_END, 50, 1000);
                    }
                    return 1;
                }, 5, std::chrono::milliseconds(1000));
                if (enter) {
                    updateMessage(tEnv, "发现声呐入口");
                    clickByPoint(tEnv, sonar_enter);
                    bool select = retry_until_success([&tEnv] {
                        return isHsvRange(tEnv, red_bomb_sample, red_bomb_lower,
                                          red_bomb_upper);
                    }, 5, std::chrono::milliseconds(500));
                    if (select) {
                        clickByPoint(tEnv, red_bomb);
//                        clickByPoint(tEnv, sonar_samples[sample_index], 500);
                        updateMessage(tEnv, "请在3秒内选择采样点");
                        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                        backKeyViaRoot();
                        bool home = retry_until_success([&tEnv]() {
                            return t_search_single(tEnv, t_daily_event, m_daily_event,
                                                   daily_event_btn, 0.85);
                        }, 10, std::chrono::milliseconds(500));
                        if (home) {
                            clickByPoint(tEnv, daily_event, 200);
                            bool enter_sec = retry_until_success([&tEnv]() {
                                if (!t_search_single(tEnv, t_sonar_event, g_empty_mat,
                                                     sonar_event_btn, 0.85)) {
                                    return swipeViaRoot(SWIPE_SONAR_BEGIN, SWIPE_SONAR_END, 50,
                                                        1000);
                                }
                                return 1;
                            }, 5, std::chrono::milliseconds(1000));
                            if (enter_sec) {
                                updateMessage(tEnv, "二次发现声呐入口");
                                clickByPoint(tEnv, sonar_enter);
                                std::this_thread::sleep_for(std::chrono::milliseconds(1300));
                                cv::Mat mat = acquireNextMat(tEnv);
                                cv::imwrite(
                                        fnv1a_resource_prefix(2026, sonar_fragment_path.string(),
                                                              ".PNG"), mat);
                                updateMessage(tEnv, "图片已经保存");
                                execSuCommand(svc_wifi_disable);
                                updateMessage(tEnv, "wifi断开");
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                resolveNetwork(tEnv);
                                updateMessage(tEnv, "关闭丢包");
//                              sample_index++;
//                              if (sample_index >= sonar_samples.size())break;
                            }
                        }
                    }
                }
            }
        }
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).detach();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_stopSonarLoop(JNIEnv *env, jobject thiz) {
    sonar_enable.store(false);
}



extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_saveSonarTable(JNIEnv *env, jobject thiz) {
    cv::Mat mat = acquireNextMat(env);
    cv::imwrite(fnv1a_resource_prefix(2026, sonar_fragment_path.string(), ".PNG"), mat);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_exportCarveAttr(JNIEnv *env, jobject thiz) {
    std::string res = std::accumulate(carve_paths.begin(), carve_paths.end(), std::string(""),
                                      [](const std::string &a, const std::string &b) {
                                          return a + b + std::string("\n");
                                      });
    updateMessage(env, res.c_str());
    if (!carve_paths.empty()) {
        updateMessage(env, "雕刻预览图合成");
        std::vector<cv::Mat> results;
        results.reserve(carve_paths.size());
        for (size_t i = 0; i < 4 && i < carve_paths.size(); i++) {
            auto &cp = carve_paths.at(i);
            std::vector<cv::Mat> mats;
            std::vector<std::string> paths;
            paths.reserve(12);
            mats.reserve(paths.capacity());
            for (auto &p: fs::directory_iterator(cp)) {
                if (p.is_regular_file()) {
                    std::string fragment_path = p.path().string();
//                    updateMessage(env, fragment_path.c_str());
                    paths.push_back(fragment_path);
                }
            }
            std::sort(paths.begin(), paths.end()
            );
            for (auto &p: paths) {
//                std::move 资源转移fragment销毁
                cv::Mat fragment = cv::imread(p, cv::IMREAD_COLOR);
                if (!fragment.empty()) {
                    mats.push_back(std::move(fragment));
                }
            }
            if (!mats.empty()) {
                cv::Mat result;
                cv::vconcat(mats, result);
                results.push_back(std::move(result));
            }
        }
        if (!results.empty()) {
            cv::Mat large;
            cv::hconcat(results, large);
            cv::imwrite(fnv1a_resource_prefix(0, carve_paths.at(4)), large);
        }
        for (int idx = 0; idx < carve_paths.size() - 1; idx++) {
            std::string path = carve_paths.at(idx);
            updateMessage(env, path.c_str());
            path += "/*";
            const char *cmd[] = {"rm", "-rf", path.c_str(), nullptr};
            execSuCommand(cmd);
        }
    }
}


void transparentByMask(cv::Mat &mat, const cv::Mat &mask) {
    CV_Assert(mat.type() == CV_8UC4);
    CV_Assert(mask.type() == CV_8UC1);
    CV_Assert(mat.size() == mask.size());
    mat.setTo(cv::Scalar(0, 0, 0, 255), mask);
//    std::vector<cv::Mat> channels;
//    cv::split(mat, channels);
//    channels[3].setTo(0, mask);
//    cv::merge(channels, mat);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_mergeSonarTable(JNIEnv *env, jobject thiz) {
    if (fs::exists(sonar_fragment_path) && fs::is_directory(sonar_fragment_path)) {
        updateMessage(env, "声呐透视图合成");
        int count = 0;
        cv::Mat canvas;
        for (const auto &entry: fs::directory_iterator(sonar_fragment_path)) {
            if (entry.is_regular_file()) {
//                auto ext = entry.path().extension().string();
                cv::Mat sonar_fragment = cv::imread(entry.path().string(), cv::IMREAD_COLOR);
                cv::Mat mask;
                cv::inRange(
                        sonar_fragment,
                        sonar_table_lower,
                        sonar_table_upper,
                        mask
                );
                sonar_fragment.setTo(
                        cv::Scalar(0, 0, 0, 255),
                        mask
                );
                if (sonar_fragment.empty())continue;
                if (canvas.empty()) {
                    canvas = sonar_fragment.clone();
                    count = 1;
                    continue;
                }
                double alpha = 1.0 / (count + 1);
                cv::addWeighted(canvas, 1.0 - alpha, sonar_fragment, alpha, 0.0, canvas);
                count++;
            }
        }
        if (count) {
            cv::imwrite(fnv1a_resource_prefix(2026, carve_paths.at(4)), canvas);
            std::string path = sonar_fragment_path.string();
            path += "/*";
            const char *cmd[] = {"rm", "-rf", path.c_str(), nullptr};
            execSuCommand(cmd);
        }
    }
}


Tess_OCR Tess;
static std::atomic<bool> tess_enable(false);
static cv::Rect GAIN_AREA(1465, 510, 1585 - 1465, 640 - 510);
static cv::Rect RECYCLE_AREA(1940, 1120, 2220 - 1940, 1215 - 1120);
extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_tessOcrRecognize(JNIEnv *env, jobject thiz,
                                                         jint ability_value) {
    tess_enable.store(true);
    std::thread([&,ability_value]() -> void {
        JNIEnv *tEnv = getEnv();
        if (!tEnv) return;
        if (unique_thread.load()) {
            updateMessage(tEnv, "single thread!");
            g_vm->DetachCurrentThread();
            return;
        }
        if (!acquireNextMat(tEnv).empty()) {
            updateMessage(tEnv, "frame capture");
        }
        if (!Tess.init(fs::path(private_storage) /= "ocr/tessdata/", "chi_sim")) {
            updateMessage(tEnv, "Tess init failed");
            g_vm->DetachCurrentThread();
            return;
        }
        unique_thread.store(true);
        updateMessage(tEnv, "Tess init success");
        updateMessage(tEnv, std::to_string(ability_value).c_str());
        TessPreprocessOptions gainPreprocess;
        gainPreprocess.lowerBound = cv::Scalar(96, 230, 177);
        gainPreprocess.upperBound = cv::Scalar(116, 255, 197);
        TessPreprocessOptions recyclePreprocess;
        recyclePreprocess.lowerBound = cv::Scalar(200, 200, 200);
        recyclePreprocess.upperBound = cv::Scalar(255, 255, 255);
        while (tess_enable.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            cv::Mat mat = acquireNextMat(tEnv);
            updateMessage(tEnv, "recognize");
            std::string attribute = Tess.recognize(
                    cropROI(mat, GAIN_AREA),
                    tesseract::PSM_SINGLE_LINE,
                    gainPreprocess
            );
            std::string recycle = Tess.recognize(
                    cropROI(mat, RECYCLE_AREA),
                    tesseract::PSM_SINGLE_BLOCK,
                    recyclePreprocess
            );

            updateMessage(tEnv, attribute.c_str());
            updateMessage(tEnv, recycle.c_str());


        }
        g_vm->DetachCurrentThread();
        unique_thread.store(false);
    }).detach();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_tessOcrDetach(JNIEnv *env, jobject thiz) {
    tess_enable.store(false);
}


extern "C" JNIEXPORT void JNICALL
Java_com_example_supercell_MainActivity_resetThreadStatus(JNIEnv *env, jobject thiz
) {
    eggs_enable.store(false);
    unique_thread.store(false);
    treasure_enable.store(false);
    assembly_enable.store(false);
    armaments_enable.store(false);
    keep_alive_enable.store(false);
    beach_enable.store(false);
    clans_enable.store(false);
    sonar_enable.store(false);
    tess_enable.store(false);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_supercell_RotationManager_syncOrientation(JNIEnv *env, jobject thiz,
                                                           jint rotation) {
    g_rotation.store(rotation);
    updateMessage(env, std::to_string(rotation).c_str());
}
