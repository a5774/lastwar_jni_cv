//
// Created by admin on 2026/11/27.
//

#ifndef SUPERCELL_TESS_OCR_HPP
#define SUPERCELL_TESS_OCR_HPP

#pragma once

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>

#include <string>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cstdio>

#include <android/log.h>

enum class TessPreprocessMode {
    InRange,
    BinaryThreshold,
    OtsuThreshold
};

struct TessPreprocessOptions {
    TessPreprocessMode mode = TessPreprocessMode::InRange;
    cv::Scalar lowerBound = cv::Scalar(0, 0, 0);
    cv::Scalar upperBound = cv::Scalar(255, 255, 255);
    double threshold = 128.0;
    bool invert = false;
};

static void tess_error_handler(
        const char *message,
        va_list args
) {
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), message, args);
    __android_log_print(
            ANDROID_LOG_ERROR,
            "TAG",
            "%s",
            buffer
    );
}

class Tess_OCR {
private:
    tesseract::TessBaseAPI api;
    bool initialized = false;

public:
    Tess_OCR() = default;
    ~Tess_OCR() {
        release();
    }

    // 禁止拷贝，避免 Tesseract 实例被意外复制
    Tess_OCR(const Tess_OCR &) = delete;

    Tess_OCR &operator=(const Tess_OCR &) = delete;

    // 初始化 OCR
    // language:
    // chi_sim       简体中文
    // chi_tra       繁体中文
    // eng           英文
    // chi_sim+eng   简体中文 + 英文
    bool init(
            const std::string &tessdata_path,
            const std::string &language = "eng"
    ) {
        release();
        int ret = api.Init(
                tessdata_path.c_str(),
                language.c_str(),
                tesseract::OEM_LSTM_ONLY
        );
        if (ret != 0) {
            return false;
        }
        initialized = true;
        return true;
    }

    // 设置字符白名单
    void setWhitelist(const std::string &chars) {
        if (!initialized) return;
        api.SetVariable(
                "tessedit_char_whitelist",
                chars.c_str()
        );
    }

    // 设置页面分割模式
    void setPageSegMode(tesseract::PageSegMode mode) {
        if (!initialized) return;
        api.SetPageSegMode(mode);
    }

    std::string recognize(
            const cv::Mat &src,
            tesseract::PageSegMode pageSegMode = tesseract::PSM_SINGLE_BLOCK,
            const TessPreprocessOptions &options = TessPreprocessOptions()
    ) {
        if (!initialized || src.empty()) {
            return {};
        }
        if (src.depth() != CV_8U) {
            __android_log_print(
                    ANDROID_LOG_ERROR,
                    "TAG",
                    "OCR expects 8-bit image data; got depth %d",
                    src.depth()
            );
            return {};
        }
        if (src.channels() != 3) {
            __android_log_print(
                    ANDROID_LOG_ERROR,
                    "TAG",
                    "OCR expects BGR 8U 3-channel image data; got channels %d",
                    src.channels()
            );
            return {};
        }

        cv::Mat processed;
        if (options.mode == TessPreprocessMode::InRange) {
            cv::inRange(src, options.lowerBound, options.upperBound, processed);
        } else {
            cv::Mat gray;
            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            int thresholdType = options.invert
                                ? cv::THRESH_BINARY_INV
                                : cv::THRESH_BINARY;
            if (options.mode == TessPreprocessMode::OtsuThreshold) {
                thresholdType |= cv::THRESH_OTSU;
            }
            cv::threshold(gray, processed, options.threshold, 255, thresholdType);
        }
        api.SetPageSegMode(pageSegMode);
        api.SetImage(
                processed.data,
                processed.cols,
                processed.rows,
                1,
                static_cast<int>(processed.step)
        );
        std::unique_ptr<char[]> raw(api.GetUTF8Text());
        if (!raw) { return {}; }
        return {raw.get()};
    }

    // 仅提取数字
    std::string recognizeNumber(
            const cv::Mat &src,
            tesseract::PageSegMode pageSegMode = tesseract::PSM_SINGLE_LINE,
            const TessPreprocessOptions &options = TessPreprocessOptions()
    ) {
        if (!initialized) return {};
        setWhitelist("0123456789");
        std::string result = recognize(src, pageSegMode, options);
        result.erase(
                std::remove_if(
                        result.begin(),
                        result.end(),
                        [](unsigned char c) {
                            return !std::isdigit(c);
                        }
                ), result.end()
        );
        return result;
    }

    void release() {
        if (initialized) {
            api.End();
            initialized = false;
        }
    }
};

#endif //SUPERCELL_TESS_OCR_HPP
