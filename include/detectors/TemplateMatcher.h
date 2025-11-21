#ifndef TEMPLATE_MATCHER_H
#define TEMPLATE_MATCHER_H

#include "detectors/DetectorBase.h"
#include <opencv2/opencv.hpp>

namespace inspection {

/**
 * @brief テンプレートマッチングによる欠陥検出器
 *
 * 良品画像（リファレンス）と検査対象画像を比較し、
 * 差分が大きい領域を欠陥として検出します。
 */
class TemplateMatcher : public DetectorBase {
public:
    /**
     * @brief コンストラクタ
     * @param diffThreshold 差分検出の閾値（0-255、デフォルト: 30）
     * @param minDefectArea 欠陥と判定する最小面積（ピクセル数、デフォルト: 100）
     * @param maxDefectArea 欠陥と判定する最大面積（ピクセル数、デフォルト: 50000）
     */
    TemplateMatcher(
        double diffThreshold = 30.0,
        double minDefectArea = 100.0,
        double maxDefectArea = 50000.0
    );

    /**
     * @brief 画像から欠陥を検出
     * @param image 入力画像（前処理済み、グレースケール推奨）
     * @return 検出された欠陥リスト
     */
    Defects detect(const cv::Mat& image) override;

    /**
     * @brief 検出器の名前を取得
     */
    std::string getName() const override {
        return "TemplateMatcher";
    }

    /**
     * @brief 検出器のタイプを取得
     */
    std::string getType() const override {
        return "template";
    }

    /**
     * @brief パラメータを設定
     * @param params JSON形式のパラメータ
     *               - diff_threshold: 差分閾値
     *               - min_area: 最小面積
     *               - max_area: 最大面積
     *               - confidence_threshold: 信頼度閾値
     *               - blur_kernel_size: ブラーカーネルサイズ
     *               - morphology_kernel_size: モルフォロジーカーネルサイズ
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
     * @brief 差分閾値を設定
     * @param threshold 差分閾値（0-255）
     */
    void setDiffThreshold(double threshold) {
        if (threshold >= 0.0 && threshold <= 255.0) {
            diffThreshold_ = threshold;
        }
    }

    /**
     * @brief 差分閾値を取得
     */
    double getDiffThreshold() const {
        return diffThreshold_;
    }

    /**
     * @brief 最小欠陥面積を設定
     * @param area 最小面積（ピクセル数）
     */
    void setMinDefectArea(double area) {
        if (area >= 0.0) {
            minDefectArea_ = area;
        }
    }

    /**
     * @brief 最小欠陥面積を取得
     */
    double getMinDefectArea() const {
        return minDefectArea_;
    }

    /**
     * @brief 最大欠陥面積を設定
     * @param area 最大面積（ピクセル数）
     */
    void setMaxDefectArea(double area) {
        if (area >= 0.0) {
            maxDefectArea_ = area;
        }
    }

    /**
     * @brief 最大欠陥面積を取得
     */
    double getMaxDefectArea() const {
        return maxDefectArea_;
    }

    /**
     * @brief ブラーカーネルサイズを設定
     * @param size カーネルサイズ（奇数、3以上）
     */
    void setBlurKernelSize(int size) {
        if (size >= 3 && size % 2 == 1) {
            blurKernelSize_ = size;
        }
    }

    /**
     * @brief ブラーカーネルサイズを取得
     */
    int getBlurKernelSize() const {
        return blurKernelSize_;
    }

    /**
     * @brief モルフォロジーカーネルサイズを設定
     * @param size カーネルサイズ（1以上）
     */
    void setMorphologyKernelSize(int size) {
        if (size >= 1) {
            morphologyKernelSize_ = size;
        }
    }

    /**
     * @brief モルフォロジーカーネルサイズを取得
     */
    int getMorphologyKernelSize() const {
        return morphologyKernelSize_;
    }

    /**
     * @brief 差分画像を取得（デバッグ用）
     * @return 最後に計算された差分画像
     */
    cv::Mat getDiffImage() const {
        return diffImage_;
    }

    /**
     * @brief 二値化画像を取得（デバッグ用）
     * @return 最後に計算された二値化画像
     */
    cv::Mat getThresholdImage() const {
        return thresholdImage_;
    }

private:
    /**
     * @brief 画像をアライメント（位置合わせ）する
     * @param image 入力画像
     * @param reference リファレンス画像
     * @return アライメントされた画像
     */
    cv::Mat alignImage(const cv::Mat& image, const cv::Mat& reference);

    /**
     * @brief 差分画像を計算
     * @param image 入力画像
     * @param reference リファレンス画像
     * @return 差分画像
     */
    cv::Mat computeDifference(const cv::Mat& image, const cv::Mat& reference);

    /**
     * @brief 差分画像から欠陥領域を検出
     * @param diffImage 差分画像
     * @return 検出された欠陥リスト
     */
    Defects findDefectRegions(const cv::Mat& diffImage);

    /**
     * @brief 輪郭から欠陥の種類を推定
     * @param contour 輪郭
     * @param bbox バウンディングボックス
     * @return 推定された欠陥タイプ
     */
    DefectType classifyDefect(const std::vector<cv::Point>& contour, const cv::Rect& bbox);

    /**
     * @brief 円形度を計算
     * @param contour 輪郭
     * @return 円形度（0.0 - 1.0）
     */
    double calculateCircularity(const std::vector<cv::Point>& contour);

    double diffThreshold_;           ///< 差分検出の閾値
    double minDefectArea_;           ///< 最小欠陥面積
    double maxDefectArea_;           ///< 最大欠陥面積
    int blurKernelSize_;             ///< ブラーカーネルサイズ
    int morphologyKernelSize_;       ///< モルフォロジーカーネルサイズ

    // デバッグ用の中間画像
    cv::Mat diffImage_;              ///< 差分画像
    cv::Mat thresholdImage_;         ///< 二値化画像
};

} // namespace inspection

#endif // TEMPLATE_MATCHER_H
