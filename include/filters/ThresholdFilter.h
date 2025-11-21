#ifndef THRESHOLD_FILTER_H
#define THRESHOLD_FILTER_H

#include "filters/FilterBase.h"
#include <opencv2/opencv.hpp>

namespace inspection {

/**
 * @brief 二値化フィルタ
 *
 * 画像を二値化（白黒）に変換するフィルタ
 */
class ThresholdFilter : public FilterBase {
public:
    /**
     * @brief 閾値処理の方法
     */
    enum class ThresholdMethod {
        Binary,          ///< 固定閾値（二値化）
        BinaryInv,       ///< 固定閾値（反転）
        Truncate,        ///< 切り捨て
        ToZero,          ///< ゼロへ
        ToZeroInv,       ///< ゼロへ（反転）
        Otsu,            ///< Otsuの自動閾値
        Adaptive         ///< 適応的閾値
    };

    /**
     * @brief コンストラクタ
     * @param threshold 閾値（0-255）
     * @param method 閾値処理の方法
     * @param maxValue 最大値（デフォルト: 255）
     */
    ThresholdFilter(
        double threshold = 128.0,
        ThresholdMethod method = ThresholdMethod::Binary,
        double maxValue = 255.0
    );

    /**
     * @brief 画像を処理
     */
    cv::Mat process(const cv::Mat& input) override;

    /**
     * @brief フィルタ名を取得
     */
    std::string getName() const override {
        return "Threshold Filter";
    }

    /**
     * @brief フィルタタイプを取得
     */
    std::string getType() const override {
        return "threshold";
    }

    /**
     * @brief パラメータを設定
     */
    void setParameters(const json& params) override;

    /**
     * @brief パラメータを取得
     */
    json getParameters() const override;

    /**
     * @brief クローンを作成
     */
    std::unique_ptr<FilterBase> clone() const override;

    /**
     * @brief 閾値を設定
     */
    void setThreshold(double threshold) {
        if (threshold >= 0.0 && threshold <= 255.0) {
            threshold_ = threshold;
        }
    }

    /**
     * @brief 閾値を取得
     */
    double getThreshold() const {
        return threshold_;
    }

    /**
     * @brief 最大値を設定
     */
    void setMaxValue(double maxValue) {
        if (maxValue >= 0.0 && maxValue <= 255.0) {
            maxValue_ = maxValue;
        }
    }

    /**
     * @brief 最大値を取得
     */
    double getMaxValue() const {
        return maxValue_;
    }

    /**
     * @brief 閾値処理の方法を設定
     */
    void setMethod(ThresholdMethod method) {
        method_ = method;
    }

    /**
     * @brief 閾値処理の方法を取得
     */
    ThresholdMethod getMethod() const {
        return method_;
    }

    /**
     * @brief 適応的閾値のブロックサイズを設定
     */
    void setAdaptiveBlockSize(int blockSize) {
        if (blockSize >= 3 && blockSize % 2 == 1) {
            adaptiveBlockSize_ = blockSize;
        }
    }

    /**
     * @brief 適応的閾値のブロックサイズを取得
     */
    int getAdaptiveBlockSize() const {
        return adaptiveBlockSize_;
    }

    /**
     * @brief 適応的閾値のC値を設定
     */
    void setAdaptiveC(double c) {
        adaptiveC_ = c;
    }

    /**
     * @brief 適応的閾値のC値を取得
     */
    double getAdaptiveC() const {
        return adaptiveC_;
    }

private:
    double threshold_;           ///< 閾値
    double maxValue_;            ///< 最大値
    ThresholdMethod method_;     ///< 閾値処理の方法

    // 適応的閾値用パラメータ
    int adaptiveBlockSize_;      ///< ブロックサイズ
    double adaptiveC_;           ///< 定数C
};

} // namespace inspection

#endif // THRESHOLD_FILTER_H
