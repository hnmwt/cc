#include "detectors/BlobDetector.h"
#include "utils/Logger.h"
#include <cmath>

namespace inspection {

// ========================================
// デフォルトパラメータ作成
// ========================================

cv::SimpleBlobDetector::Params createDefaultBlobParams() {
    cv::SimpleBlobDetector::Params params;

    // 閾値設定
    params.minThreshold = 10;
    params.maxThreshold = 220;
    params.thresholdStep = 10;

    // 色フィルタ
    params.filterByColor = true;
    params.blobColor = 0;  // 0=暗いブロブ, 255=明るいブロブ

    // 面積フィルタ
    params.filterByArea = true;
    params.minArea = 50.0f;
    params.maxArea = 50000.0f;

    // 円形度フィルタ
    params.filterByCircularity = true;
    params.minCircularity = 0.1f;
    params.maxCircularity = 1.0f;

    // 凸性フィルタ
    params.filterByConvexity = true;
    params.minConvexity = 0.5f;
    params.maxConvexity = 1.0f;

    // 慣性比フィルタ
    params.filterByInertia = true;
    params.minInertiaRatio = 0.1f;
    params.maxInertiaRatio = 1.0f;

    // その他
    params.minDistBetweenBlobs = 10.0f;
    params.minRepeatability = 2;

    return params;
}

// ========================================
// BlobDetector 実装
// ========================================

BlobDetector::BlobDetector()
    : params_(createDefaultBlobParams())
{
    initializeDetector();
}

BlobDetector::BlobDetector(const cv::SimpleBlobDetector::Params& params)
    : params_(params)
{
    initializeDetector();
}

void BlobDetector::initializeDetector() {
    detector_ = cv::SimpleBlobDetector::create(params_);
    LOG_DEBUG("BlobDetector initialized");
}

Defects BlobDetector::detect(const cv::Mat& image) {
    Defects defects;

    if (image.empty()) {
        LOG_ERROR("BlobDetector: Empty image");
        return defects;
    }

    if (!enabled_) {
        LOG_DEBUG("BlobDetector is disabled");
        return defects;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // グレースケール変換（カラー画像の場合）
        cv::Mat grayImage;
        if (image.channels() == 3) {
            cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
        } else {
            grayImage = image;
        }

        // ブロブ検出
        lastKeyPoints_.clear();
        detector_->detect(grayImage, lastKeyPoints_);

        LOG_DEBUG("BlobDetector detected {} blobs", lastKeyPoints_.size());

        // キーポイントをDefectに変換
        for (const auto& kp : lastKeyPoints_) {
            Defect defect = keyPointToDefect(kp, grayImage);

            // 信頼度フィルタリング
            if (defect.confidence >= confidenceThreshold_) {
                defects.push_back(defect);
            }
        }

        // 統計情報を記録
        auto endTime = std::chrono::high_resolution_clock::now();
        double processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        recordStatistics(defects.size(), processingTime);

        LOG_DEBUG("BlobDetector: {} defects detected (threshold={}, time={}ms)",
                  defects.size(), confidenceThreshold_, processingTime);

    } catch (const cv::Exception& e) {
        LOG_ERROR("BlobDetector OpenCV error: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("BlobDetector error: {}", e.what());
    }

    return defects;
}

Defect BlobDetector::keyPointToDefect(const cv::KeyPoint& kp, const cv::Mat& image) {
    Defect defect;

    // 中心座標
    defect.center = kp.pt;

    // バウンディングボックス
    float radius = kp.size / 2.0f;
    int x = static_cast<int>(kp.pt.x - radius);
    int y = static_cast<int>(kp.pt.y - radius);
    int width = static_cast<int>(kp.size);
    int height = static_cast<int>(kp.size);

    // 画像範囲内にクリップ
    x = std::max(0, std::min(x, image.cols - 1));
    y = std::max(0, std::min(y, image.rows - 1));
    width = std::min(width, image.cols - x);
    height = std::min(height, image.rows - y);

    defect.bbox = cv::Rect(x, y, width, height);

    // 面積
    defect.area = kp.size * kp.size;

    // 欠陥タイプの自動分類
    defect.type = classifyBlob(kp, image);

    // 信頼度の計算
    defect.confidence = calculateConfidence(kp, image);

    // 特徴量の計算
    auto features = calculateBlobFeatures(kp, image);
    defect.circularity = features.circularity;

    return defect;
}

DefectType BlobDetector::classifyBlob(const cv::KeyPoint& kp, const cv::Mat& image) {
    auto features = calculateBlobFeatures(kp, image);

    // 傷: 細長い、低円形度
    if (features.inertia_ratio < 0.3 && features.circularity < 0.5) {
        return DefectType::Scratch;
    }

    // 異物/汚れ: 円形、小〜中面積
    if (features.circularity > 0.7 && features.area < 1000) {
        return DefectType::Stain;
    }

    // 形状不良: 大面積、低凸性
    if (features.area > 5000 && features.convexity < 0.7) {
        return DefectType::Deformation;
    }

    // 変色: その他
    return DefectType::Discoloration;
}

double BlobDetector::calculateConfidence(const cv::KeyPoint& kp, const cv::Mat& image) {
    // 基本信頼度（キーポイントの応答強度に基づく）
    double baseConfidence = std::min(1.0, kp.response / 100.0);

    // サイズに基づく調整
    double sizeScore = 1.0;
    if (kp.size < params_.minArea || kp.size > params_.maxArea) {
        sizeScore = 0.5;
    }

    // 最終信頼度
    double confidence = baseConfidence * sizeScore;

    // 0.0-1.0の範囲に正規化
    confidence = std::max(0.0, std::min(1.0, confidence));

    return confidence;
}

BlobDetector::BlobFeatures BlobDetector::calculateBlobFeatures(
    const cv::KeyPoint& kp,
    const cv::Mat& image
) {
    BlobFeatures features;

    // 基本的な特徴量（SimpleBlobDetectorから）
    features.area = kp.size * kp.size;

    // バウンディングボックス
    float radius = kp.size / 2.0f;
    int x = std::max(0, static_cast<int>(kp.pt.x - radius));
    int y = std::max(0, static_cast<int>(kp.pt.y - radius));
    int width = std::min(static_cast<int>(kp.size), image.cols - x);
    int height = std::min(static_cast<int>(kp.size), image.rows - y);
    features.boundingBox = cv::Rect(x, y, width, height);

    // 円形度の推定（SimpleBlobDetectorの結果から）
    // 完全な円形の場合、面積とサイズの関係から推定
    double estimatedRadius = kp.size / 2.0;
    double estimatedCircleArea = M_PI * estimatedRadius * estimatedRadius;
    features.circularity = std::min(1.0, features.area / estimatedCircleArea);

    // 凸性と慣性比はSimpleBlobDetectorの設定値から推定
    // （詳細な計算には輪郭情報が必要だが、ここでは簡易版）
    features.convexity = 0.8;  // デフォルト値
    features.inertia_ratio = 0.5;  // デフォルト値

    // ROIが有効な場合、より詳細な計算を試みる
    if (features.boundingBox.width > 0 && features.boundingBox.height > 0) {
        try {
            cv::Mat roi = image(features.boundingBox);

            // 二値化
            cv::Mat binary;
            double threshold = cv::mean(roi)[0];
            cv::threshold(roi, binary, threshold, 255, cv::THRESH_BINARY);

            // 輪郭検出
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            if (!contours.empty()) {
                // 最大の輪郭を使用
                auto maxContour = *std::max_element(
                    contours.begin(),
                    contours.end(),
                    [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                        return cv::contourArea(a) < cv::contourArea(b);
                    }
                );

                // 周囲長
                features.perimeter = cv::arcLength(maxContour, true);

                // 円形度の再計算
                if (features.perimeter > 0) {
                    double contourArea = cv::contourArea(maxContour);
                    features.circularity = 4 * M_PI * contourArea / (features.perimeter * features.perimeter);
                    features.circularity = std::min(1.0, features.circularity);
                }

                // 凸包を計算
                std::vector<cv::Point> hull;
                cv::convexHull(maxContour, hull);
                double hullArea = cv::contourArea(hull);

                // 凸性
                if (hullArea > 0) {
                    features.convexity = cv::contourArea(maxContour) / hullArea;
                }

                // モーメントから慣性比を計算
                cv::Moments m = cv::moments(maxContour);
                if (m.mu20 + m.mu02 > 0) {
                    double denominator = m.mu20 + m.mu02;
                    double numerator = std::sqrt((m.mu20 - m.mu02) * (m.mu20 - m.mu02) + 4 * m.mu11 * m.mu11);
                    features.inertia_ratio = (denominator - numerator) / (denominator + numerator);
                }
            }
        } catch (const cv::Exception& e) {
            LOG_WARN("Failed to calculate detailed blob features: {}", e.what());
        }
    }

    return features;
}

void BlobDetector::setParameters(const json& params) {
    try {
        // 閾値
        if (params.contains("min_threshold")) {
            params_.minThreshold = params["min_threshold"].get<float>();
        }
        if (params.contains("max_threshold")) {
            params_.maxThreshold = params["max_threshold"].get<float>();
        }
        if (params.contains("threshold_step")) {
            params_.thresholdStep = params["threshold_step"].get<float>();
        }

        // 色
        if (params.contains("blob_color")) {
            params_.blobColor = params["blob_color"].get<unsigned char>();
            params_.filterByColor = true;
        }

        // 面積
        if (params.contains("min_area")) {
            params_.minArea = params["min_area"].get<float>();
            params_.filterByArea = true;
        }
        if (params.contains("max_area")) {
            params_.maxArea = params["max_area"].get<float>();
            params_.filterByArea = true;
        }

        // 円形度
        if (params.contains("min_circularity")) {
            params_.minCircularity = params["min_circularity"].get<float>();
            params_.filterByCircularity = true;
        }
        if (params.contains("max_circularity")) {
            params_.maxCircularity = params["max_circularity"].get<float>();
            params_.filterByCircularity = true;
        }

        // 凸性
        if (params.contains("min_convexity")) {
            params_.minConvexity = params["min_convexity"].get<float>();
            params_.filterByConvexity = true;
        }
        if (params.contains("max_convexity")) {
            params_.maxConvexity = params["max_convexity"].get<float>();
            params_.filterByConvexity = true;
        }

        // 慣性比
        if (params.contains("min_inertia_ratio")) {
            params_.minInertiaRatio = params["min_inertia_ratio"].get<float>();
            params_.filterByInertia = true;
        }
        if (params.contains("max_inertia_ratio")) {
            params_.maxInertiaRatio = params["max_inertia_ratio"].get<float>();
            params_.filterByInertia = true;
        }

        // 信頼度閾値
        if (params.contains("confidence_threshold")) {
            setConfidenceThreshold(params["confidence_threshold"].get<double>());
        }

        // 検出器を再初期化
        initializeDetector();

        LOG_INFO("BlobDetector parameters updated");

    } catch (const json::exception& e) {
        LOG_ERROR("Failed to set BlobDetector parameters: {}", e.what());
    }
}

json BlobDetector::getParameters() const {
    json params;

    params["type"] = getType();
    params["name"] = getName();
    params["enabled"] = isEnabled();

    // 閾値
    params["min_threshold"] = params_.minThreshold;
    params["max_threshold"] = params_.maxThreshold;
    params["threshold_step"] = params_.thresholdStep;

    // 色
    params["filter_by_color"] = params_.filterByColor;
    params["blob_color"] = params_.blobColor;

    // 面積
    params["filter_by_area"] = params_.filterByArea;
    params["min_area"] = params_.minArea;
    params["max_area"] = params_.maxArea;

    // 円形度
    params["filter_by_circularity"] = params_.filterByCircularity;
    params["min_circularity"] = params_.minCircularity;
    params["max_circularity"] = params_.maxCircularity;

    // 凸性
    params["filter_by_convexity"] = params_.filterByConvexity;
    params["min_convexity"] = params_.minConvexity;
    params["max_convexity"] = params_.maxConvexity;

    // 慣性比
    params["filter_by_inertia"] = params_.filterByInertia;
    params["min_inertia_ratio"] = params_.minInertiaRatio;
    params["max_inertia_ratio"] = params_.maxInertiaRatio;

    // その他
    params["min_distance_between_blobs"] = params_.minDistBetweenBlobs;
    params["min_repeatability"] = params_.minRepeatability;

    // 信頼度
    params["confidence_threshold"] = getConfidenceThreshold();

    return params;
}

std::unique_ptr<DetectorBase> BlobDetector::clone() const {
    auto cloned = std::make_unique<BlobDetector>(params_);
    cloned->setEnabled(isEnabled());
    cloned->setConfidenceThreshold(getConfidenceThreshold());
    return cloned;
}

void BlobDetector::setBlobParams(const cv::SimpleBlobDetector::Params& params) {
    params_ = params;
    initializeDetector();
}

void BlobDetector::setColorThreshold(int blobColor) {
    params_.blobColor = static_cast<unsigned char>(blobColor);
    params_.filterByColor = true;
    initializeDetector();
}

void BlobDetector::setAreaThreshold(double minArea, double maxArea) {
    params_.minArea = static_cast<float>(minArea);
    params_.maxArea = static_cast<float>(maxArea);
    params_.filterByArea = true;
    initializeDetector();
}

void BlobDetector::setCircularityThreshold(double minCirc, double maxCirc) {
    params_.minCircularity = static_cast<float>(minCirc);
    params_.maxCircularity = static_cast<float>(maxCirc);
    params_.filterByCircularity = true;
    initializeDetector();
}

void BlobDetector::setConvexityThreshold(double minConv, double maxConv) {
    params_.minConvexity = static_cast<float>(minConv);
    params_.maxConvexity = static_cast<float>(maxConv);
    params_.filterByConvexity = true;
    initializeDetector();
}

void BlobDetector::setInertiaThreshold(double minInertia, double maxInertia) {
    params_.minInertiaRatio = static_cast<float>(minInertia);
    params_.maxInertiaRatio = static_cast<float>(maxInertia);
    params_.filterByInertia = true;
    initializeDetector();
}

} // namespace inspection
