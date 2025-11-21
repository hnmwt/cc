#ifndef FEATURE_DETECTOR_H
#define FEATURE_DETECTOR_H

#include "detectors/DetectorBase.h"
#include <opencv2/opencv.hpp>

namespace inspection {

/**
 * @brief 特徴量ベースの欠陥検出器
 *
 * リファレンス画像を必要とせず、画像内の特徴量（輪郭、面積、円形度など）を
 * 解析して欠陥を検出します。
 */
class FeatureDetector : public DetectorBase {
public:
    /**
     * @brief 検出モード
     */
    enum class DetectionMode {
        Edge,           ///< エッジ検出ベース
        Threshold,      ///< 閾値処理ベース
        Adaptive,       ///< 適応的閾値処理ベース
        Combined        ///< 複合モード
    };

    /**
     * @brief コンストラクタ
     * @param mode 検出モード
     * @param minArea 最小面積（ピクセル数）
     * @param maxArea 最大面積（ピクセル数）
     */
    FeatureDetector(
        DetectionMode mode = DetectionMode::Adaptive,
        double minArea = 100.0,
        double maxArea = 50000.0
    );

    /**
     * @brief 画像から欠陥を検出
     */
    Defects detect(const cv::Mat& image) override;

    /**
     * @brief 検出器の名前を取得
     */
    std::string getName() const override {
        return "FeatureDetector";
    }

    /**
     * @brief 検出器のタイプを取得
     */
    std::string getType() const override {
        return "feature";
    }

    /**
     * @brief パラメータを設定
     */
    void setParameters(const json& params) override;

    /**
     * @brief 現在のパラメータを取得
     */
    json getParameters() const override;

    /**
     * @brief 検出器のクローンを作成
     */
    std::unique_ptr<DetectorBase> clone() const override;

    /**
     * @brief 検出モードを設定
     */
    void setDetectionMode(DetectionMode mode) {
        mode_ = mode;
    }

    /**
     * @brief 検出モードを取得
     */
    DetectionMode getDetectionMode() const {
        return mode_;
    }

    /**
     * @brief 最小面積を設定
     */
    void setMinArea(double area) {
        if (area >= 0.0) {
            minArea_ = area;
        }
    }

    /**
     * @brief 最小面積を取得
     */
    double getMinArea() const {
        return minArea_;
    }

    /**
     * @brief 最大面積を設定
     */
    void setMaxArea(double area) {
        if (area >= 0.0) {
            maxArea_ = area;
        }
    }

    /**
     * @brief 最大面積を取得
     */
    double getMaxArea() const {
        return maxArea_;
    }

    /**
     * @brief 円形度の範囲を設定
     * @param minCirc 最小円形度（0.0 - 1.0）
     * @param maxCirc 最大円形度（0.0 - 1.0）
     */
    void setCircularityRange(double minCirc, double maxCirc) {
        if (minCirc >= 0.0 && minCirc <= 1.0) {
            minCircularity_ = minCirc;
        }
        if (maxCirc >= 0.0 && maxCirc <= 1.0 && maxCirc >= minCirc) {
            maxCircularity_ = maxCirc;
        }
    }

    /**
     * @brief Cannyエッジ検出の閾値を設定
     */
    void setCannyThresholds(double low, double high) {
        if (low >= 0.0 && high >= low) {
            cannyLowThreshold_ = low;
            cannyHighThreshold_ = high;
        }
    }

    /**
     * @brief 適応的閾値のパラメータを設定
     */
    void setAdaptiveThresholdParams(int blockSize, double C) {
        if (blockSize >= 3 && blockSize % 2 == 1) {
            adaptiveBlockSize_ = blockSize;
        }
        adaptiveC_ = C;
    }

    /**
     * @brief 処理済み画像を取得（デバッグ用）
     */
    cv::Mat getProcessedImage() const {
        return processedImage_;
    }

private:
    /**
     * @brief エッジベースの検出
     */
    Defects detectByEdge(const cv::Mat& image);

    /**
     * @brief 閾値ベースの検出
     */
    Defects detectByThreshold(const cv::Mat& image);

    /**
     * @brief 適応的閾値ベースの検出
     */
    Defects detectByAdaptive(const cv::Mat& image);

    /**
     * @brief 複合モードの検出
     */
    Defects detectByCombined(const cv::Mat& image);

    /**
     * @brief 輪郭から欠陥を抽出
     */
    Defects extractDefectsFromContours(
        const std::vector<std::vector<cv::Point>>& contours,
        const cv::Mat& image
    );

    /**
     * @brief 輪郭から欠陥タイプを分類
     */
    DefectType classifyDefect(
        const std::vector<cv::Point>& contour,
        const cv::Rect& bbox,
        double circularity,
        double intensity
    );

    /**
     * @brief 円形度を計算
     */
    double calculateCircularity(const std::vector<cv::Point>& contour);

    /**
     * @brief 信頼度を計算
     */
    double calculateConfidence(
        const std::vector<cv::Point>& contour,
        const cv::Rect& bbox,
        const cv::Mat& image
    );

    DetectionMode mode_;             ///< 検出モード
    double minArea_;                 ///< 最小面積
    double maxArea_;                 ///< 最大面積
    double minCircularity_;          ///< 最小円形度
    double maxCircularity_;          ///< 最大円形度

    // エッジ検出パラメータ
    double cannyLowThreshold_;       ///< Canny低閾値
    double cannyHighThreshold_;      ///< Canny高閾値

    // 適応的閾値パラメータ
    int adaptiveBlockSize_;          ///< ブロックサイズ
    double adaptiveC_;               ///< 定数C

    // デバッグ用
    cv::Mat processedImage_;         ///< 処理済み画像
};

} // namespace inspection

#endif // FEATURE_DETECTOR_H
