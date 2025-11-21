#include "detectors/TemplateMatcher.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace inspection {

/**
 * @brief コンストラクタ
 */
TemplateMatcher::TemplateMatcher(
    double diffThreshold,
    double minDefectArea,
    double maxDefectArea
)
    : diffThreshold_(diffThreshold),
      minDefectArea_(minDefectArea),
      maxDefectArea_(maxDefectArea),
      blurKernelSize_(5),
      morphologyKernelSize_(3)
{
    // デフォルトの信頼度閾値を設定
    confidenceThreshold_ = 0.5;
}

/**
 * @brief 画像から欠陥を検出
 */
Defects TemplateMatcher::detect(const cv::Mat& image) {
    auto startTime = cv::getTickCount();
    Defects defects;

    // 入力チェック
    if (image.empty()) {
        return defects;
    }

    // リファレンス画像のチェック
    if (!hasReferenceImage()) {
        return defects;
    }

    // 検出器が無効な場合
    if (!isEnabled()) {
        return defects;
    }

    // グレースケール変換（必要な場合）
    cv::Mat grayImage;
    if (image.channels() == 3) {
        cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
    } else {
        grayImage = image.clone();
    }

    cv::Mat grayReference;
    if (referenceImage_.channels() == 3) {
        cv::cvtColor(referenceImage_, grayReference, cv::COLOR_BGR2GRAY);
    } else {
        grayReference = referenceImage_.clone();
    }

    // サイズチェック
    if (grayImage.size() != grayReference.size()) {
        // リファレンス画像を入力画像のサイズにリサイズ
        cv::resize(grayReference, grayReference, grayImage.size());
    }

    // 画像のアライメント（位置合わせ）
    cv::Mat alignedImage = alignImage(grayImage, grayReference);

    // 差分計算
    cv::Mat diff = computeDifference(alignedImage, grayReference);
    diffImage_ = diff.clone();

    // 欠陥領域の検出
    defects = findDefectRegions(diff);

    // 処理時間の記録
    auto endTime = cv::getTickCount();
    double processingTime = (endTime - startTime) * 1000.0 / cv::getTickFrequency();
    recordStatistics(defects.size(), processingTime);

    return defects;
}

/**
 * @brief 画像をアライメント（位置合わせ）する
 */
cv::Mat TemplateMatcher::alignImage(const cv::Mat& image, const cv::Mat& reference) {
    // 簡易版: アライメントなしで画像をそのまま返す
    // 本格的な実装では、特徴点マッチング（ORB、SIFT等）を使用して
    // ホモグラフィ変換を行うことができます
    return image.clone();
}

/**
 * @brief 差分画像を計算
 */
cv::Mat TemplateMatcher::computeDifference(const cv::Mat& image, const cv::Mat& reference) {
    // ノイズ除去のためのブラー
    cv::Mat blurredImage, blurredReference;
    if (blurKernelSize_ > 1) {
        cv::GaussianBlur(image, blurredImage, cv::Size(blurKernelSize_, blurKernelSize_), 0);
        cv::GaussianBlur(reference, blurredReference, cv::Size(blurKernelSize_, blurKernelSize_), 0);
    } else {
        blurredImage = image.clone();
        blurredReference = reference.clone();
    }

    // 絶対差分を計算
    cv::Mat diff;
    cv::absdiff(blurredImage, blurredReference, diff);

    return diff;
}

/**
 * @brief 差分画像から欠陥領域を検出
 */
Defects TemplateMatcher::findDefectRegions(const cv::Mat& diffImage) {
    Defects defects;

    // 二値化
    cv::Mat binary;
    cv::threshold(diffImage, binary, diffThreshold_, 255, cv::THRESH_BINARY);
    thresholdImage_ = binary.clone();

    // モルフォロジー演算でノイズ除去と領域の整形
    if (morphologyKernelSize_ > 0) {
        cv::Mat kernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE,
            cv::Size(morphologyKernelSize_, morphologyKernelSize_)
        );

        // オープニング（小さなノイズ除去）
        cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);

        // クロージング（穴埋め）
        cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    }

    // 輪郭検出
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 各輪郭を処理
    for (const auto& contour : contours) {
        // 面積計算
        double area = cv::contourArea(contour);

        // 面積フィルタリング
        if (area < minDefectArea_ || area > maxDefectArea_) {
            continue;
        }

        // バウンディングボックス
        cv::Rect bbox = cv::boundingRect(contour);

        // 欠陥タイプの分類
        DefectType type = classifyDefect(contour, bbox);

        // 信頼度の計算（簡易版: 差分の平均値から算出）
        cv::Mat roi = diffImage(bbox);
        cv::Scalar meanDiff = cv::mean(roi);
        double confidence = std::min(1.0, meanDiff[0] / 255.0);

        // 信頼度閾値でフィルタリング
        if (confidence < confidenceThreshold_) {
            continue;
        }

        // Defectオブジェクトを作成
        Defect defect(type, bbox, confidence);
        defect.area = area;
        defect.contour = contour;
        defect.circularity = calculateCircularity(contour);

        defects.push_back(defect);
    }

    return defects;
}

/**
 * @brief 輪郭から欠陥の種類を推定
 */
DefectType TemplateMatcher::classifyDefect(
    const std::vector<cv::Point>& contour,
    const cv::Rect& bbox
) {
    // 円形度を計算
    double circularity = calculateCircularity(contour);

    // アスペクト比を計算
    double aspectRatio = static_cast<double>(bbox.width) / static_cast<double>(bbox.height);
    if (aspectRatio < 1.0) {
        aspectRatio = 1.0 / aspectRatio;
    }

    // 簡易的な分類ルール
    if (circularity > 0.8) {
        // 円形に近い → 汚れ（Stain）
        return DefectType::Stain;
    } else if (aspectRatio > 3.0) {
        // 細長い → 傷（Scratch）
        return DefectType::Scratch;
    } else if (circularity < 0.5) {
        // 不定形 → 変色（Discoloration）
        return DefectType::Discoloration;
    } else {
        // その他 → 形状不良（Deformation）
        return DefectType::Deformation;
    }
}

/**
 * @brief 円形度を計算
 */
double TemplateMatcher::calculateCircularity(const std::vector<cv::Point>& contour) {
    if (contour.size() < 3) {
        return 0.0;
    }

    double area = cv::contourArea(contour);
    double perimeter = cv::arcLength(contour, true);

    if (perimeter <= 0.0) {
        return 0.0;
    }

    // 円形度 = 4π * 面積 / 周囲長^2
    // 完全な円の場合は 1.0
    double circularity = (4.0 * M_PI * area) / (perimeter * perimeter);

    return std::min(1.0, circularity);
}

/**
 * @brief パラメータを設定
 */
void TemplateMatcher::setParameters(const json& params) {
    if (params.contains("diff_threshold")) {
        setDiffThreshold(params["diff_threshold"].get<double>());
    }

    if (params.contains("min_area")) {
        setMinDefectArea(params["min_area"].get<double>());
    }

    if (params.contains("max_area")) {
        setMaxDefectArea(params["max_area"].get<double>());
    }

    if (params.contains("confidence_threshold")) {
        setConfidenceThreshold(params["confidence_threshold"].get<double>());
    }

    if (params.contains("blur_kernel_size")) {
        setBlurKernelSize(params["blur_kernel_size"].get<int>());
    }

    if (params.contains("morphology_kernel_size")) {
        setMorphologyKernelSize(params["morphology_kernel_size"].get<int>());
    }
}

/**
 * @brief 現在のパラメータを取得
 */
json TemplateMatcher::getParameters() const {
    json params;
    params["diff_threshold"] = diffThreshold_;
    params["min_area"] = minDefectArea_;
    params["max_area"] = maxDefectArea_;
    params["confidence_threshold"] = confidenceThreshold_;
    params["blur_kernel_size"] = blurKernelSize_;
    params["morphology_kernel_size"] = morphologyKernelSize_;
    return params;
}

/**
 * @brief 検出器のクローンを作成
 */
std::unique_ptr<DetectorBase> TemplateMatcher::clone() const {
    auto cloned = std::make_unique<TemplateMatcher>(*this);
    return cloned;
}

} // namespace inspection
