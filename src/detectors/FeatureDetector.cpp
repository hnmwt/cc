#include "detectors/FeatureDetector.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace inspection {

/**
 * @brief コンストラクタ
 */
FeatureDetector::FeatureDetector(
    DetectionMode mode,
    double minArea,
    double maxArea
)
    : mode_(mode),
      minArea_(minArea),
      maxArea_(maxArea),
      minCircularity_(0.0),
      maxCircularity_(1.0),
      cannyLowThreshold_(50.0),
      cannyHighThreshold_(150.0),
      adaptiveBlockSize_(11),
      adaptiveC_(2.0)
{
    confidenceThreshold_ = 0.5;
}

/**
 * @brief 画像から欠陥を検出
 */
Defects FeatureDetector::detect(const cv::Mat& image) {
    auto startTime = cv::getTickCount();
    Defects defects;

    if (image.empty() || !isEnabled()) {
        return defects;
    }

    // 検出モードに応じて処理
    switch (mode_) {
        case DetectionMode::Edge:
            defects = detectByEdge(image);
            break;
        case DetectionMode::Threshold:
            defects = detectByThreshold(image);
            break;
        case DetectionMode::Adaptive:
            defects = detectByAdaptive(image);
            break;
        case DetectionMode::Combined:
            defects = detectByCombined(image);
            break;
    }

    // 処理時間の記録
    auto endTime = cv::getTickCount();
    double processingTime = (endTime - startTime) * 1000.0 / cv::getTickFrequency();
    recordStatistics(defects.size(), processingTime);

    return defects;
}

/**
 * @brief エッジベースの検出
 */
Defects FeatureDetector::detectByEdge(const cv::Mat& image) {
    // グレースケール変換
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }

    // ノイズ除去
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 1.5);

    // Cannyエッジ検出
    cv::Mat edges;
    cv::Canny(blurred, edges, cannyLowThreshold_, cannyHighThreshold_);

    // モルフォロジー演算でエッジを太くする
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(edges, edges, kernel);

    processedImage_ = edges.clone();

    // 輪郭検出
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    return extractDefectsFromContours(contours, gray);
}

/**
 * @brief 閾値ベースの検出
 */
Defects FeatureDetector::detectByThreshold(const cv::Mat& image) {
    // グレースケール変換
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }

    // Otsuの自動閾値処理
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    processedImage_ = binary.clone();

    // 輪郭検出
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    return extractDefectsFromContours(contours, gray);
}

/**
 * @brief 適応的閾値ベースの検出
 */
Defects FeatureDetector::detectByAdaptive(const cv::Mat& image) {
    // グレースケール変換
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }

    // ノイズ除去
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    // 適応的閾値処理
    cv::Mat binary;
    cv::adaptiveThreshold(
        blurred,
        binary,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        adaptiveBlockSize_,
        adaptiveC_
    );

    // モルフォロジー演算でノイズ除去
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);

    processedImage_ = binary.clone();

    // 輪郭検出
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    return extractDefectsFromContours(contours, gray);
}

/**
 * @brief 複合モードの検出
 */
Defects FeatureDetector::detectByCombined(const cv::Mat& image) {
    // エッジとアダプティブの結果を組み合わせる
    Defects edgeDefects = detectByEdge(image);
    Defects adaptiveDefects = detectByAdaptive(image);

    // 重複を除いてマージ（簡易版: 単純に連結）
    Defects combined = edgeDefects;
    combined.insert(combined.end(), adaptiveDefects.begin(), adaptiveDefects.end());

    return combined;
}

/**
 * @brief 輪郭から欠陥を抽出
 */
Defects FeatureDetector::extractDefectsFromContours(
    const std::vector<std::vector<cv::Point>>& contours,
    const cv::Mat& image
) {
    Defects defects;

    for (const auto& contour : contours) {
        // 面積計算
        double area = cv::contourArea(contour);

        // 面積フィルタリング
        if (area < minArea_ || area > maxArea_) {
            continue;
        }

        // バウンディングボックス
        cv::Rect bbox = cv::boundingRect(contour);

        // 円形度計算
        double circularity = calculateCircularity(contour);

        // 円形度フィルタリング
        if (circularity < minCircularity_ || circularity > maxCircularity_) {
            continue;
        }

        // 平均輝度計算
        cv::Mat roi = image(bbox);
        cv::Scalar meanIntensity = cv::mean(roi);

        // 信頼度計算
        double confidence = calculateConfidence(contour, bbox, image);

        // 信頼度フィルタリング
        if (confidence < confidenceThreshold_) {
            continue;
        }

        // 欠陥タイプの分類
        DefectType type = classifyDefect(contour, bbox, circularity, meanIntensity[0]);

        // Defectオブジェクトを作成
        Defect defect(type, bbox, confidence);
        defect.area = area;
        defect.contour = contour;
        defect.circularity = circularity;

        defects.push_back(defect);
    }

    return defects;
}

/**
 * @brief 輪郭から欠陥タイプを分類
 */
DefectType FeatureDetector::classifyDefect(
    const std::vector<cv::Point>& contour,
    const cv::Rect& bbox,
    double circularity,
    double intensity
) {
    // アスペクト比を計算
    double aspectRatio = static_cast<double>(bbox.width) / static_cast<double>(bbox.height);
    if (aspectRatio < 1.0) {
        aspectRatio = 1.0 / aspectRatio;
    }

    // 分類ルール
    if (circularity > 0.85) {
        // 非常に円形 → 汚れ
        return DefectType::Stain;
    } else if (aspectRatio > 4.0) {
        // 非常に細長い → 傷
        return DefectType::Scratch;
    } else if (intensity < 100) {
        // 暗い領域 → 変色
        return DefectType::Discoloration;
    } else if (circularity < 0.4) {
        // 不定形 → 形状不良
        return DefectType::Deformation;
    } else {
        // その他 → 汚れ
        return DefectType::Stain;
    }
}

/**
 * @brief 円形度を計算
 */
double FeatureDetector::calculateCircularity(const std::vector<cv::Point>& contour) {
    if (contour.size() < 3) {
        return 0.0;
    }

    double area = cv::contourArea(contour);
    double perimeter = cv::arcLength(contour, true);

    if (perimeter <= 0.0) {
        return 0.0;
    }

    // 円形度 = 4π * 面積 / 周囲長^2
    double circularity = (4.0 * M_PI * area) / (perimeter * perimeter);

    return std::min(1.0, circularity);
}

/**
 * @brief 信頼度を計算
 */
double FeatureDetector::calculateConfidence(
    const std::vector<cv::Point>& contour,
    const cv::Rect& bbox,
    const cv::Mat& image
) {
    // 面積と輪郭の整合性
    double contourArea = cv::contourArea(contour);
    double bboxArea = bbox.width * bbox.height;

    if (bboxArea <= 0.0) {
        return 0.0;
    }

    double areaRatio = contourArea / bboxArea;

    // 円形度も考慮
    double circularity = calculateCircularity(contour);

    // 信頼度 = 面積比率と円形度の加重平均
    double confidence = 0.6 * areaRatio + 0.4 * circularity;

    return std::min(1.0, std::max(0.0, confidence));
}

/**
 * @brief パラメータを設定
 */
void FeatureDetector::setParameters(const json& params) {
    if (params.contains("min_area")) {
        setMinArea(params["min_area"].get<double>());
    }

    if (params.contains("max_area")) {
        setMaxArea(params["max_area"].get<double>());
    }

    if (params.contains("min_circularity")) {
        minCircularity_ = params["min_circularity"].get<double>();
    }

    if (params.contains("max_circularity")) {
        maxCircularity_ = params["max_circularity"].get<double>();
    }

    if (params.contains("confidence_threshold")) {
        setConfidenceThreshold(params["confidence_threshold"].get<double>());
    }

    if (params.contains("canny_low")) {
        cannyLowThreshold_ = params["canny_low"].get<double>();
    }

    if (params.contains("canny_high")) {
        cannyHighThreshold_ = params["canny_high"].get<double>();
    }

    if (params.contains("adaptive_block_size")) {
        adaptiveBlockSize_ = params["adaptive_block_size"].get<int>();
    }

    if (params.contains("adaptive_c")) {
        adaptiveC_ = params["adaptive_c"].get<double>();
    }

    if (params.contains("mode")) {
        std::string modeStr = params["mode"].get<std::string>();
        if (modeStr == "edge") {
            mode_ = DetectionMode::Edge;
        } else if (modeStr == "threshold") {
            mode_ = DetectionMode::Threshold;
        } else if (modeStr == "adaptive") {
            mode_ = DetectionMode::Adaptive;
        } else if (modeStr == "combined") {
            mode_ = DetectionMode::Combined;
        }
    }
}

/**
 * @brief 現在のパラメータを取得
 */
json FeatureDetector::getParameters() const {
    json params;
    params["min_area"] = minArea_;
    params["max_area"] = maxArea_;
    params["min_circularity"] = minCircularity_;
    params["max_circularity"] = maxCircularity_;
    params["confidence_threshold"] = confidenceThreshold_;
    params["canny_low"] = cannyLowThreshold_;
    params["canny_high"] = cannyHighThreshold_;
    params["adaptive_block_size"] = adaptiveBlockSize_;
    params["adaptive_c"] = adaptiveC_;

    // モードを文字列に変換
    std::string modeStr;
    switch (mode_) {
        case DetectionMode::Edge:
            modeStr = "edge";
            break;
        case DetectionMode::Threshold:
            modeStr = "threshold";
            break;
        case DetectionMode::Adaptive:
            modeStr = "adaptive";
            break;
        case DetectionMode::Combined:
            modeStr = "combined";
            break;
    }
    params["mode"] = modeStr;

    return params;
}

/**
 * @brief 検出器のクローンを作成
 */
std::unique_ptr<DetectorBase> FeatureDetector::clone() const {
    return std::make_unique<FeatureDetector>(*this);
}

} // namespace inspection
