#ifndef EDGE_DETECTOR_H
#define EDGE_DETECTOR_H

#include "detectors/DetectorBase.h"
#include <opencv2/opencv.hpp>

namespace inspection {

/**
 * @brief エッジ検出による欠陥検出器
 *
 * OpenCVのCanny、Sobel、Laplacianエッジ検出を使用して、
 * 画像内のエッジ（輪郭、境界線）を検出します。
 * 傷、クラック、欠け、バリ、境界不良などの欠陥を自動的に識別します。
 */
class EdgeDetector : public DetectorBase {
public:
    /**
     * @brief エッジ検出モード
     */
    enum class EdgeDetectionMode {
        Canny,      ///< Cannyエッジ検出（デフォルト）
        Sobel,      ///< Sobelエッジ検出
        Laplacian,  ///< Laplacianエッジ検出
        Combined    ///< 複数手法の組み合わせ
    };

    /**
     * @brief Cannyエッジ検出パラメータ
     */
    struct CannyParams {
        double lowThreshold = 50.0;      ///< 低閾値（弱いエッジ）
        double highThreshold = 150.0;    ///< 高閾値（強いエッジ）
        int apertureSize = 3;            ///< Sobelオペレータのサイズ（3, 5, 7）
        bool L2gradient = true;          ///< L2ノルムを使用
    };

    /**
     * @brief Sobelエッジ検出パラメータ
     */
    struct SobelParams {
        int kernelSize = 3;       ///< カーネルサイズ（1, 3, 5, 7）
        double scale = 1.0;       ///< 微分値のスケール
        double delta = 0.0;       ///< オフセット値
        double threshold = 50.0;  ///< 2値化閾値
    };

    /**
     * @brief Laplacianエッジ検出パラメータ
     */
    struct LaplacianParams {
        int kernelSize = 3;       ///< カーネルサイズ（1, 3, 5, 7）
        double scale = 1.0;       ///< 微分値のスケール
        double delta = 0.0;       ///< オフセット値
        double threshold = 30.0;  ///< 2値化閾値
    };

    /**
     * @brief デフォルトコンストラクタ
     */
    EdgeDetector();

    /**
     * @brief モード指定コンストラクタ
     * @param mode エッジ検出モード
     */
    explicit EdgeDetector(EdgeDetectionMode mode);

    /**
     * @brief デストラクタ
     */
    ~EdgeDetector() override = default;

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
        return "EdgeDetector";
    }

    /**
     * @brief 検出器のタイプを取得
     * @return タイプ名
     */
    std::string getType() const override {
        return "edge";
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

    // ===== EdgeDetector固有のメソッド =====

    /**
     * @brief 検出モードを設定
     * @param mode エッジ検出モード
     */
    void setDetectionMode(EdgeDetectionMode mode);

    /**
     * @brief 検出モードを取得
     * @return エッジ検出モード
     */
    EdgeDetectionMode getDetectionMode() const {
        return mode_;
    }

    /**
     * @brief Cannyパラメータを設定
     * @param params Cannyパラメータ
     */
    void setCannyParams(const CannyParams& params);

    /**
     * @brief Cannyパラメータを取得
     * @return Cannyパラメータ
     */
    CannyParams getCannyParams() const {
        return cannyParams_;
    }

    /**
     * @brief Sobelパラメータを設定
     * @param params Sobelパラメータ
     */
    void setSobelParams(const SobelParams& params);

    /**
     * @brief Sobelパラメータを取得
     * @return Sobelパラメータ
     */
    SobelParams getSobelParams() const {
        return sobelParams_;
    }

    /**
     * @brief Laplacianパラメータを設定
     * @param params Laplacianパラメータ
     */
    void setLaplacianParams(const LaplacianParams& params);

    /**
     * @brief Laplacianパラメータを取得
     * @return Laplacianパラメータ
     */
    LaplacianParams getLaplacianParams() const {
        return laplacianParams_;
    }

    /**
     * @brief エッジ長さフィルタを設定
     * @param minLength 最小エッジ長さ（ピクセル）
     * @param maxLength 最大エッジ長さ（ピクセル）
     */
    void setEdgeLengthFilter(double minLength, double maxLength);

    /**
     * @brief エッジ角度フィルタを設定
     * @param minAngle 最小角度（度）
     * @param maxAngle 最大角度（度）
     */
    void setEdgeAngleFilter(double minAngle, double maxAngle);

    /**
     * @brief 角度フィルタの有効/無効を設定
     * @param enabled 有効にする場合true
     */
    void setAngleFilterEnabled(bool enabled) {
        angleFilterEnabled_ = enabled;
    }

    /**
     * @brief 最後に検出されたエッジ画像を取得（デバッグ用）
     * @return エッジ画像（2値画像）
     */
    cv::Mat getLastEdgeImage() const {
        return lastEdgeImage_.clone();
    }

private:
    /**
     * @brief エッジ特徴量
     */
    struct EdgeFeatures {
        double length;              ///< エッジの長さ
        double angle;               ///< エッジの平均角度（度）
        double strength;            ///< エッジの強度
        double straightness;        ///< 直線性（0.0〜1.0）
        double curvature;           ///< 曲率
        bool isOnBoundary;          ///< 画像境界上にあるか
        int gaps;                   ///< 途切れの数
        cv::Rect boundingBox;       ///< バウンディングボックス
        std::vector<cv::Point> points;  ///< エッジのポイント列
    };

    /**
     * @brief Cannyエッジ検出を実行
     * @param image 入力画像（グレースケール）
     * @return エッジ画像（2値画像）
     */
    cv::Mat detectCannyEdges(const cv::Mat& image);

    /**
     * @brief Sobelエッジ検出を実行
     * @param image 入力画像（グレースケール）
     * @return エッジ画像（2値画像）
     */
    cv::Mat detectSobelEdges(const cv::Mat& image);

    /**
     * @brief Laplacianエッジ検出を実行
     * @param image 入力画像（グレースケール）
     * @return エッジ画像（2値画像）
     */
    cv::Mat detectLaplacianEdges(const cv::Mat& image);

    /**
     * @brief エッジ特徴量を計算
     * @param contour 輪郭点列
     * @param imageSize 画像サイズ
     * @return エッジ特徴量
     */
    EdgeFeatures calculateEdgeFeatures(
        const std::vector<cv::Point>& contour,
        const cv::Size& imageSize
    );

    /**
     * @brief エッジから欠陥タイプを分類
     * @param features エッジ特徴量
     * @return 欠陥タイプ
     */
    DefectType classifyEdge(const EdgeFeatures& features);

    /**
     * @brief エッジの信頼度を計算
     * @param features エッジ特徴量
     * @return 信頼度（0.0〜1.0）
     */
    double calculateConfidence(const EdgeFeatures& features);

    /**
     * @brief エッジ特徴量をDefectに変換
     * @param features エッジ特徴量
     * @return Defectオブジェクト
     */
    Defect edgeToDefect(const EdgeFeatures& features);

    /**
     * @brief エッジが長さフィルタを通過するか
     * @param length エッジの長さ
     * @return 通過する場合true
     */
    bool passLengthFilter(double length) const;

    /**
     * @brief エッジが角度フィルタを通過するか
     * @param angle エッジの角度
     * @return 通過する場合true
     */
    bool passAngleFilter(double angle) const;

    EdgeDetectionMode mode_;         ///< 検出モード
    CannyParams cannyParams_;        ///< Cannyパラメータ
    SobelParams sobelParams_;        ///< Sobelパラメータ
    LaplacianParams laplacianParams_; ///< Laplacianパラメータ

    double minEdgeLength_ = 10.0;    ///< 最小エッジ長さ
    double maxEdgeLength_ = 1000.0;  ///< 最大エッジ長さ
    double minEdgeAngle_ = 0.0;      ///< 最小エッジ角度（度）
    double maxEdgeAngle_ = 180.0;    ///< 最大エッジ角度（度）
    bool angleFilterEnabled_ = false; ///< 角度フィルタ有効フラグ

    cv::Mat lastEdgeImage_;          ///< 最後に検出されたエッジ画像
};

/**
 * @brief EdgeDetectionModeを文字列に変換
 * @param mode エッジ検出モード
 * @return モード名
 */
inline std::string edgeModeToString(EdgeDetector::EdgeDetectionMode mode) {
    switch (mode) {
        case EdgeDetector::EdgeDetectionMode::Canny: return "canny";
        case EdgeDetector::EdgeDetectionMode::Sobel: return "sobel";
        case EdgeDetector::EdgeDetectionMode::Laplacian: return "laplacian";
        case EdgeDetector::EdgeDetectionMode::Combined: return "combined";
        default: return "unknown";
    }
}

/**
 * @brief 文字列をEdgeDetectionModeに変換
 * @param str モード名
 * @return エッジ検出モード
 */
inline EdgeDetector::EdgeDetectionMode stringToEdgeMode(const std::string& str) {
    if (str == "canny") return EdgeDetector::EdgeDetectionMode::Canny;
    if (str == "sobel") return EdgeDetector::EdgeDetectionMode::Sobel;
    if (str == "laplacian") return EdgeDetector::EdgeDetectionMode::Laplacian;
    if (str == "combined") return EdgeDetector::EdgeDetectionMode::Combined;
    return EdgeDetector::EdgeDetectionMode::Canny;  // デフォルト
}

} // namespace inspection

#endif // EDGE_DETECTOR_H
