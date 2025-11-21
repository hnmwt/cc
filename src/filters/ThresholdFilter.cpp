#include "filters/ThresholdFilter.h"
#include <opencv2/imgproc.hpp>

namespace inspection {

/**
 * @brief コンストラクタ
 */
ThresholdFilter::ThresholdFilter(
    double threshold,
    ThresholdMethod method,
    double maxValue
)
    : threshold_(threshold),
      method_(method),
      maxValue_(maxValue),
      adaptiveBlockSize_(11),
      adaptiveC_(2.0)
{
}

/**
 * @brief 画像を処理
 */
cv::Mat ThresholdFilter::process(const cv::Mat& input) {
    if (input.empty()) {
        return input;
    }

    cv::Mat output;
    cv::Mat gray;

    // グレースケール変換（必要な場合）
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }

    // 閾値処理の方法に応じて処理
    switch (method_) {
        case ThresholdMethod::Binary:
            cv::threshold(gray, output, threshold_, maxValue_, cv::THRESH_BINARY);
            break;

        case ThresholdMethod::BinaryInv:
            cv::threshold(gray, output, threshold_, maxValue_, cv::THRESH_BINARY_INV);
            break;

        case ThresholdMethod::Truncate:
            cv::threshold(gray, output, threshold_, maxValue_, cv::THRESH_TRUNC);
            break;

        case ThresholdMethod::ToZero:
            cv::threshold(gray, output, threshold_, maxValue_, cv::THRESH_TOZERO);
            break;

        case ThresholdMethod::ToZeroInv:
            cv::threshold(gray, output, threshold_, maxValue_, cv::THRESH_TOZERO_INV);
            break;

        case ThresholdMethod::Otsu:
            // Otsuの自動閾値
            cv::threshold(gray, output, 0, maxValue_, cv::THRESH_BINARY | cv::THRESH_OTSU);
            break;

        case ThresholdMethod::Adaptive:
            // 適応的閾値
            cv::adaptiveThreshold(
                gray,
                output,
                maxValue_,
                cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                cv::THRESH_BINARY,
                adaptiveBlockSize_,
                adaptiveC_
            );
            break;

        default:
            output = gray.clone();
            break;
    }

    return output;
}

/**
 * @brief パラメータを設定
 */
void ThresholdFilter::setParameters(const json& params) {
    if (params.contains("threshold")) {
        setThreshold(params["threshold"].get<double>());
    }

    if (params.contains("max_value")) {
        setMaxValue(params["max_value"].get<double>());
    }

    if (params.contains("adaptive_block_size")) {
        setAdaptiveBlockSize(params["adaptive_block_size"].get<int>());
    }

    if (params.contains("adaptive_c")) {
        setAdaptiveC(params["adaptive_c"].get<double>());
    }

    if (params.contains("method")) {
        std::string methodStr = params["method"].get<std::string>();
        if (methodStr == "binary") {
            method_ = ThresholdMethod::Binary;
        } else if (methodStr == "binary_inv") {
            method_ = ThresholdMethod::BinaryInv;
        } else if (methodStr == "truncate") {
            method_ = ThresholdMethod::Truncate;
        } else if (methodStr == "tozero") {
            method_ = ThresholdMethod::ToZero;
        } else if (methodStr == "tozero_inv") {
            method_ = ThresholdMethod::ToZeroInv;
        } else if (methodStr == "otsu") {
            method_ = ThresholdMethod::Otsu;
        } else if (methodStr == "adaptive") {
            method_ = ThresholdMethod::Adaptive;
        }
    }
}

/**
 * @brief パラメータを取得
 */
json ThresholdFilter::getParameters() const {
    json params;
    params["threshold"] = threshold_;
    params["max_value"] = maxValue_;
    params["adaptive_block_size"] = adaptiveBlockSize_;
    params["adaptive_c"] = adaptiveC_;

    // メソッドを文字列に変換
    std::string methodStr;
    switch (method_) {
        case ThresholdMethod::Binary:
            methodStr = "binary";
            break;
        case ThresholdMethod::BinaryInv:
            methodStr = "binary_inv";
            break;
        case ThresholdMethod::Truncate:
            methodStr = "truncate";
            break;
        case ThresholdMethod::ToZero:
            methodStr = "tozero";
            break;
        case ThresholdMethod::ToZeroInv:
            methodStr = "tozero_inv";
            break;
        case ThresholdMethod::Otsu:
            methodStr = "otsu";
            break;
        case ThresholdMethod::Adaptive:
            methodStr = "adaptive";
            break;
    }
    params["method"] = methodStr;

    return params;
}

/**
 * @brief クローンを作成
 */
std::unique_ptr<FilterBase> ThresholdFilter::clone() const {
    return std::make_unique<ThresholdFilter>(*this);
}

} // namespace inspection
