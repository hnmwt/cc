#ifndef BLOB_DETECTOR_H
#define BLOB_DETECTOR_H

#include "detectors/DetectorBase.h"
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>

namespace inspection {

/**
 * @brief ブロブ検出による欠陥検出器
 *
 * OpenCVのSimpleBlobDetectorを使用して、画像内の塊（blob）状の領域を検出します。
 * 傷、汚れ、異物、形状不良などの欠陥を自動的に識別します。
 */
class BlobDetector : public DetectorBase {
public:
    /**
     * @brief デフォルトコンストラクタ
     */
    BlobDetector();

    /**
     * @brief パラメータ付きコンストラクタ
     * @param params SimpleBlobDetectorのパラメータ
     */
    explicit BlobDetector(const cv::SimpleBlobDetector::Params& params);

    /**
     * @brief デストラクタ
     */
    ~BlobDetector() override = default;

    /**
     * @brief 画像から欠陥を検出
     * @param image 入力画像（グレースケールまたはカラー）
     * @return 検出された欠陥リスト
     */
    Defects detect(const cv::Mat& image) override;

    /**
     * @brief 検出器の名前を取得
     * @return 検出器名
     */
    std::string getName() const override {
        return "BlobDetector";
    }

    /**
     * @brief 検出器のタイプを取得
     * @return タイプ名
     */
    std::string getType() const override {
        return "blob";
    }

    /**
     * @brief パラメータを設定
     * @param params JSON形式のパラメータ
     */
    void setParameters(const json& params) override;

    /**
     * @brief 現在のパラメータを取得
     * @return JSON形式のパラメータ
     */
    json getParameters() const override;

    /**
     * @brief 検出器のクローンを作成
     * @return クローンされた検出器
     */
    std::unique_ptr<DetectorBase> clone() const override;

    // ===== BlobDetector固有のメソッド =====

    /**
     * @brief SimpleBlobDetectorのパラメータを設定
     * @param params パラメータ
     */
    void setBlobParams(const cv::SimpleBlobDetector::Params& params);

    /**
     * @brief SimpleBlobDetectorのパラメータを取得
     * @return パラメータ
     */
    cv::SimpleBlobDetector::Params getBlobParams() const {
        return params_;
    }

    /**
     * @brief 検出する色（明度）を設定
     * @param blobColor 0=暗いブロブ, 255=明るいブロブ
     */
    void setColorThreshold(int blobColor);

    /**
     * @brief 面積の閾値を設定
     * @param minArea 最小面積（ピクセル²）
     * @param maxArea 最大面積（ピクセル²）
     */
    void setAreaThreshold(double minArea, double maxArea);

    /**
     * @brief 円形度の閾値を設定
     * @param minCirc 最小円形度（0.0-1.0）
     * @param maxCirc 最大円形度（0.0-1.0）
     */
    void setCircularityThreshold(double minCirc, double maxCirc);

    /**
     * @brief 凸性の閾値を設定
     * @param minConv 最小凸性（0.0-1.0）
     * @param maxConv 最大凸性（0.0-1.0）
     */
    void setConvexityThreshold(double minConv, double maxConv);

    /**
     * @brief 慣性比の閾値を設定
     * @param minInertia 最小慣性比（0.0-1.0）
     * @param maxInertia 最大慣性比（0.0-1.0）
     */
    void setInertiaThreshold(double minInertia, double maxInertia);

    /**
     * @brief 最後に検出されたキーポイントを取得（デバッグ用）
     * @return キーポイントのリスト
     */
    std::vector<cv::KeyPoint> getLastKeyPoints() const {
        return lastKeyPoints_;
    }

    /**
     * @brief 検出されたブロブの数を取得
     * @return ブロブの数
     */
    size_t getBlobCount() const {
        return lastKeyPoints_.size();
    }

private:
    /**
     * @brief キーポイントをDefectに変換
     * @param kp キーポイント
     * @param image 元画像
     * @return Defectオブジェクト
     */
    Defect keyPointToDefect(const cv::KeyPoint& kp, const cv::Mat& image);

    /**
     * @brief ブロブから欠陥タイプを分類
     * @param kp キーポイント
     * @param image 元画像
     * @return 欠陥タイプ
     */
    DefectType classifyBlob(const cv::KeyPoint& kp, const cv::Mat& image);

    /**
     * @brief ブロブの信頼度を計算
     * @param kp キーポイント
     * @param image 元画像
     * @return 信頼度（0.0-1.0）
     */
    double calculateConfidence(const cv::KeyPoint& kp, const cv::Mat& image);

    /**
     * @brief ブロブの特徴量を計算
     * @param kp キーポイント
     * @param image 元画像
     * @return 特徴量（円形度、凸性、慣性比など）
     */
    struct BlobFeatures {
        double circularity;
        double convexity;
        double inertia_ratio;
        double area;
        double perimeter;
        cv::Rect boundingBox;
    };

    BlobFeatures calculateBlobFeatures(const cv::KeyPoint& kp, const cv::Mat& image);

    /**
     * @brief ブロブ検出器を初期化
     */
    void initializeDetector();

    cv::Ptr<cv::SimpleBlobDetector> detector_;        ///< SimpleBlobDetector
    cv::SimpleBlobDetector::Params params_;           ///< 検出パラメータ
    std::vector<cv::KeyPoint> lastKeyPoints_;         ///< 最後に検出されたキーポイント
};

/**
 * @brief デフォルトのブロブ検出パラメータを作成
 * @return デフォルトパラメータ
 */
cv::SimpleBlobDetector::Params createDefaultBlobParams();

} // namespace inspection

#endif // BLOB_DETECTOR_H
