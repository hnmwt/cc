#ifndef DETECTOR_BASE_H
#define DETECTOR_BASE_H

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include "detectors/Defect.h"

namespace inspection {

using json = nlohmann::json;

/**
 * @brief 欠陥検出器の抽象基底クラス
 *
 * すべての欠陥検出アルゴリズムはこのクラスを継承して実装します。
 * Strategy パターンを採用し、異なる検出アルゴリズムを統一的に扱えます。
 */
class DetectorBase {
public:
    virtual ~DetectorBase() = default;

    /**
     * @brief 画像から欠陥を検出する
     * @param image 入力画像（前処理済み）
     * @return 検出された欠陥のリスト
     */
    virtual Defects detect(const cv::Mat& image) = 0;

    /**
     * @brief 検出器の名前を取得
     * @return 検出器の名前（例: "TemplateMatcher", "FeatureDetector"）
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 検出器のタイプを取得
     * @return 検出器のタイプ（例: "template", "feature", "ml"）
     */
    virtual std::string getType() const = 0;

    /**
     * @brief パラメータを設定する
     * @param params JSON形式のパラメータ
     */
    virtual void setParameters(const json& params) = 0;

    /**
     * @brief 現在のパラメータを取得する
     * @return JSON形式のパラメータ
     */
    virtual json getParameters() const = 0;

    /**
     * @brief 検出器が有効かどうか
     * @return 有効な場合true
     */
    virtual bool isEnabled() const {
        return enabled_;
    }

    /**
     * @brief 検出器の有効/無効を設定
     * @param enabled 有効にする場合true
     */
    virtual void setEnabled(bool enabled) {
        enabled_ = enabled;
    }

    /**
     * @brief 検出器の信頼度閾値を取得
     * @return 信頼度閾値（0.0 - 1.0）
     */
    virtual double getConfidenceThreshold() const {
        return confidenceThreshold_;
    }

    /**
     * @brief 検出器の信頼度閾値を設定
     * @param threshold 信頼度閾値（0.0 - 1.0）
     */
    virtual void setConfidenceThreshold(double threshold) {
        if (threshold >= 0.0 && threshold <= 1.0) {
            confidenceThreshold_ = threshold;
        }
    }

    /**
     * @brief リファレンス画像を設定する（テンプレートマッチング用）
     * @param referenceImage リファレンス画像
     */
    virtual void setReferenceImage(const cv::Mat& referenceImage) {
        referenceImage_ = referenceImage.clone();
    }

    /**
     * @brief リファレンス画像を取得する
     * @return リファレンス画像
     */
    virtual cv::Mat getReferenceImage() const {
        return referenceImage_;
    }

    /**
     * @brief リファレンス画像が設定されているか
     * @return 設定されている場合true
     */
    virtual bool hasReferenceImage() const {
        return !referenceImage_.empty();
    }

    /**
     * @brief 検出器のクローンを作成
     * @return クローンされた検出器（Prototype パターン）
     */
    virtual std::unique_ptr<DetectorBase> clone() const = 0;

    /**
     * @brief 検出結果を可視化した画像を生成
     * @param image 元画像
     * @param defects 検出された欠陥リスト
     * @param drawContour 輪郭を描画するか
     * @param drawBbox バウンディングボックスを描画するか
     * @param drawLabel ラベルを描画するか
     * @return 可視化された画像
     */
    static cv::Mat visualizeDefects(
        const cv::Mat& image,
        const Defects& defects,
        bool drawContour = true,
        bool drawBbox = true,
        bool drawLabel = true
    );

    /**
     * @brief 検出統計情報を取得
     * @return 統計情報のJSON
     */
    virtual json getStatistics() const {
        json stats;
        stats["name"] = getName();
        stats["type"] = getType();
        stats["enabled"] = isEnabled();
        stats["confidence_threshold"] = getConfidenceThreshold();
        stats["has_reference"] = hasReferenceImage();
        stats["total_detections"] = totalDetections_;
        stats["total_processing_time_ms"] = totalProcessingTime_;
        return stats;
    }

    /**
     * @brief 統計情報をリセット
     */
    virtual void resetStatistics() {
        totalDetections_ = 0;
        totalProcessingTime_ = 0.0;
    }

protected:
    /**
     * @brief 検出数と処理時間を記録
     * @param numDefects 検出数
     * @param processingTime 処理時間（ミリ秒）
     */
    void recordStatistics(size_t numDefects, double processingTime) {
        totalDetections_ += numDefects;
        totalProcessingTime_ += processingTime;
    }

    bool enabled_ = true;                    ///< 検出器が有効かどうか
    double confidenceThreshold_ = 0.5;       ///< 信頼度閾値（デフォルト: 0.5）
    cv::Mat referenceImage_;                 ///< リファレンス画像（良品画像など）

    // 統計情報
    size_t totalDetections_ = 0;             ///< 合計検出数
    double totalProcessingTime_ = 0.0;       ///< 合計処理時間（ミリ秒）
};

/**
 * @brief 複数の検出器を保持するコンテナ型
 */
using Detectors = std::vector<std::unique_ptr<DetectorBase>>;

} // namespace inspection

#endif // DETECTOR_BASE_H
