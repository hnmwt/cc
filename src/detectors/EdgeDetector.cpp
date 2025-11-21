#include "detectors/EdgeDetector.h"
#include "utils/Logger.h"
#include <cmath>

namespace inspection {

// ========================================
// コンストラクタ
// ========================================

EdgeDetector::EdgeDetector()
    : mode_(EdgeDetectionMode::Canny)
{
    LOG_DEBUG("EdgeDetector initialized (mode: Canny)");
}

EdgeDetector::EdgeDetector(EdgeDetectionMode mode)
    : mode_(mode)
{
    LOG_DEBUG("EdgeDetector initialized (mode: {})", edgeModeToString(mode));
}

// ========================================
// メイン検出処理
// ========================================

Defects EdgeDetector::detect(const cv::Mat& image) {
    Defects defects;

    if (image.empty()) {
        LOG_ERROR("EdgeDetector: Empty image");
        return defects;
    }

    if (!enabled_) {
        LOG_DEBUG("EdgeDetector is disabled");
        return defects;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // グレースケール変換
        cv::Mat grayImage;
        if (image.channels() == 3) {
            cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
        } else {
            grayImage = image;
        }

        // エッジ検出
        cv::Mat edgeImage;
        switch (mode_) {
            case EdgeDetectionMode::Canny:
                edgeImage = detectCannyEdges(grayImage);
                break;
            case EdgeDetectionMode::Sobel:
                edgeImage = detectSobelEdges(grayImage);
                break;
            case EdgeDetectionMode::Laplacian:
                edgeImage = detectLaplacianEdges(grayImage);
                break;
            case EdgeDetectionMode::Combined:
                // 複数手法の組み合わせ
                cv::Mat canny = detectCannyEdges(grayImage);
                cv::Mat sobel = detectSobelEdges(grayImage);
                cv::bitwise_or(canny, sobel, edgeImage);
                break;
        }

        lastEdgeImage_ = edgeImage.clone();

        // 輪郭検出
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edgeImage, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

        LOG_DEBUG("EdgeDetector detected {} contours", contours.size());

        // 各輪郭を処理
        for (const auto& contour : contours) {
            if (contour.size() < 3) {
                continue;  // 点が少なすぎる
            }

            // エッジ特徴量を計算
            EdgeFeatures features = calculateEdgeFeatures(contour, grayImage.size());

            // 長さフィルタ
            if (!passLengthFilter(features.length)) {
                continue;
            }

            // 角度フィルタ
            if (angleFilterEnabled_ && !passAngleFilter(features.angle)) {
                continue;
            }

            // Defectに変換
            Defect defect = edgeToDefect(features);

            // 信頼度フィルタリング
            if (defect.confidence >= confidenceThreshold_) {
                defects.push_back(defect);
            }
        }

        // 統計情報を記録
        auto endTime = std::chrono::high_resolution_clock::now();
        double processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        recordStatistics(defects.size(), processingTime);

        LOG_DEBUG("EdgeDetector: {} defects detected (mode={}, threshold={}, time={}ms)",
                  defects.size(), edgeModeToString(mode_), confidenceThreshold_, processingTime);

    } catch (const cv::Exception& e) {
        LOG_ERROR("EdgeDetector OpenCV error: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("EdgeDetector error: {}", e.what());
    }

    return defects;
}

// ========================================
// エッジ検出アルゴリズム
// ========================================

cv::Mat EdgeDetector::detectCannyEdges(const cv::Mat& image) {
    cv::Mat edges;

    // ガウシアンブラーでノイズ除去
    cv::Mat blurred;
    cv::GaussianBlur(image, blurred, cv::Size(5, 5), 1.0);

    // Cannyエッジ検出
    cv::Canny(blurred,
              edges,
              cannyParams_.lowThreshold,
              cannyParams_.highThreshold,
              cannyParams_.apertureSize,
              cannyParams_.L2gradient);

    return edges;
}

cv::Mat EdgeDetector::detectSobelEdges(const cv::Mat& image) {
    cv::Mat edges;

    // ガウシアンブラーでノイズ除去
    cv::Mat blurred;
    cv::GaussianBlur(image, blurred, cv::Size(5, 5), 1.0);

    // Sobelエッジ検出（X方向）
    cv::Mat sobelX, sobelY;
    cv::Sobel(blurred, sobelX, CV_16S, 1, 0,
              sobelParams_.kernelSize,
              sobelParams_.scale,
              sobelParams_.delta);

    // Sobelエッジ検出（Y方向）
    cv::Sobel(blurred, sobelY, CV_16S, 0, 1,
              sobelParams_.kernelSize,
              sobelParams_.scale,
              sobelParams_.delta);

    // 絶対値に変換
    cv::Mat absX, absY;
    cv::convertScaleAbs(sobelX, absX);
    cv::convertScaleAbs(sobelY, absY);

    // 合成
    cv::Mat gradient;
    cv::addWeighted(absX, 0.5, absY, 0.5, 0, gradient);

    // 2値化
    cv::threshold(gradient, edges, sobelParams_.threshold, 255, cv::THRESH_BINARY);

    return edges;
}

cv::Mat EdgeDetector::detectLaplacianEdges(const cv::Mat& image) {
    cv::Mat edges;

    // ガウシアンブラーでノイズ除去
    cv::Mat blurred;
    cv::GaussianBlur(image, blurred, cv::Size(5, 5), 1.0);

    // Laplacianエッジ検出
    cv::Mat laplacian;
    cv::Laplacian(blurred, laplacian, CV_16S,
                  laplacianParams_.kernelSize,
                  laplacianParams_.scale,
                  laplacianParams_.delta);

    // 絶対値に変換
    cv::Mat absLaplacian;
    cv::convertScaleAbs(laplacian, absLaplacian);

    // 2値化
    cv::threshold(absLaplacian, edges, laplacianParams_.threshold, 255, cv::THRESH_BINARY);

    return edges;
}

// ========================================
// エッジ特徴量の計算
// ========================================

EdgeDetector::EdgeFeatures EdgeDetector::calculateEdgeFeatures(
    const std::vector<cv::Point>& contour,
    const cv::Size& imageSize
) {
    EdgeFeatures features;
    features.points = contour;

    // バウンディングボックス
    features.boundingBox = cv::boundingRect(contour);

    // エッジの長さ
    features.length = cv::arcLength(contour, false);

    // 直線フィッティング
    cv::Vec4f line;
    if (contour.size() >= 2) {
        cv::fitLine(contour, line, cv::DIST_L2, 0, 0.01, 0.01);

        // 角度を計算（度）
        double vx = line[0];
        double vy = line[1];
        features.angle = std::atan2(vy, vx) * 180.0 / M_PI;
        if (features.angle < 0) {
            features.angle += 180.0;
        }

        // 直線性を計算（各点から直線までの距離の平均）
        double sumDistance = 0.0;
        for (const auto& pt : contour) {
            // 点から直線までの距離
            double x0 = line[2];  // 直線上の点のx座標
            double y0 = line[3];  // 直線上の点のy座標
            double distance = std::abs(vy * (pt.x - x0) - vx * (pt.y - y0)) /
                            std::sqrt(vx * vx + vy * vy);
            sumDistance += distance;
        }
        double avgDistance = sumDistance / contour.size();

        // 直線性（距離が小さいほど1に近い）
        features.straightness = 1.0 / (1.0 + avgDistance / 10.0);
        features.straightness = std::max(0.0, std::min(1.0, features.straightness));
    } else {
        features.angle = 0.0;
        features.straightness = 0.0;
    }

    // 曲率（簡易計算：バウンディングボックスのアスペクト比から）
    if (features.boundingBox.width > 0 && features.boundingBox.height > 0) {
        double aspectRatio = static_cast<double>(features.boundingBox.height) /
                           features.boundingBox.width;
        features.curvature = std::abs(aspectRatio - 1.0);
    } else {
        features.curvature = 0.0;
    }

    // 境界上にあるか
    features.isOnBoundary = false;
    for (const auto& pt : contour) {
        if (pt.x <= 1 || pt.y <= 1 ||
            pt.x >= imageSize.width - 2 || pt.y >= imageSize.height - 2) {
            features.isOnBoundary = true;
            break;
        }
    }

    // 途切れの数（簡易計算：隣接点間の距離が大きい箇所）
    features.gaps = 0;
    for (size_t i = 1; i < contour.size(); ++i) {
        double dist = cv::norm(contour[i] - contour[i - 1]);
        if (dist > 10.0) {
            features.gaps++;
        }
    }

    // エッジ強度（仮の値、実際のエッジ画像から計算することも可能）
    features.strength = 100.0;

    return features;
}

// ========================================
// 欠陥分類
// ========================================

DefectType EdgeDetector::classifyEdge(const EdgeFeatures& features) {
    // 傷: 長く（>100px）、直線的（straightness > 0.9）
    if (features.length > 100.0 && features.straightness > 0.9) {
        return DefectType::Scratch;
    }

    // クラック（短く途切れた線）: 傷として分類
    if (features.length < 50.0 && features.gaps > 0) {
        return DefectType::Scratch;
    }

    // 欠け（境界上の急激な変化）: 形状不良として分類
    if (features.isOnBoundary && features.straightness < 0.5) {
        return DefectType::Deformation;
    }

    // 形状不良: 境界上にあり、曲率が異常
    if (features.isOnBoundary && features.curvature > 0.3) {
        return DefectType::Deformation;
    }

    // バリ（境界外への突起）: 形状不良として分類
    if (features.isOnBoundary && features.straightness > 0.8) {
        return DefectType::Deformation;
    }

    // その他
    return DefectType::Unknown;
}

double EdgeDetector::calculateConfidence(const EdgeFeatures& features) {
    // 基本信頼度（長さに基づく）
    double baseConfidence = std::min(1.0, features.length / 200.0);

    // 直線性による調整
    double straightnessScore = features.straightness;

    // 強度による調整
    double strengthScore = std::min(1.0, features.strength / 150.0);

    // 最終信頼度
    double confidence = (baseConfidence * 0.5 +
                        straightnessScore * 0.3 +
                        strengthScore * 0.2);

    // 0.0-1.0の範囲に正規化
    confidence = std::max(0.0, std::min(1.0, confidence));

    return confidence;
}

Defect EdgeDetector::edgeToDefect(const EdgeFeatures& features) {
    Defect defect;

    // 中心座標（バウンディングボックスの中心）
    defect.center = cv::Point2f(
        features.boundingBox.x + features.boundingBox.width / 2.0f,
        features.boundingBox.y + features.boundingBox.height / 2.0f
    );

    // バウンディングボックス
    defect.bbox = features.boundingBox;

    // 面積（エッジの長さ × 平均幅、ここでは簡易的に長さのみ）
    defect.area = features.length;

    // 欠陥タイプの自動分類
    defect.type = classifyEdge(features);

    // 信頼度の計算
    defect.confidence = calculateConfidence(features);

    return defect;
}

// ========================================
// フィルタリング
// ========================================

bool EdgeDetector::passLengthFilter(double length) const {
    return length >= minEdgeLength_ && length <= maxEdgeLength_;
}

bool EdgeDetector::passAngleFilter(double angle) const {
    if (!angleFilterEnabled_) {
        return true;
    }
    return angle >= minEdgeAngle_ && angle <= maxEdgeAngle_;
}

// ========================================
// パラメータ設定
// ========================================

void EdgeDetector::setDetectionMode(EdgeDetectionMode mode) {
    mode_ = mode;
    LOG_DEBUG("EdgeDetector mode set to: {}", edgeModeToString(mode));
}

void EdgeDetector::setCannyParams(const CannyParams& params) {
    cannyParams_ = params;
    LOG_DEBUG("EdgeDetector Canny params updated");
}

void EdgeDetector::setSobelParams(const SobelParams& params) {
    sobelParams_ = params;
    LOG_DEBUG("EdgeDetector Sobel params updated");
}

void EdgeDetector::setLaplacianParams(const LaplacianParams& params) {
    laplacianParams_ = params;
    LOG_DEBUG("EdgeDetector Laplacian params updated");
}

void EdgeDetector::setEdgeLengthFilter(double minLength, double maxLength) {
    minEdgeLength_ = minLength;
    maxEdgeLength_ = maxLength;
    LOG_DEBUG("EdgeDetector length filter: {} - {}", minLength, maxLength);
}

void EdgeDetector::setEdgeAngleFilter(double minAngle, double maxAngle) {
    minEdgeAngle_ = minAngle;
    maxEdgeAngle_ = maxAngle;
    angleFilterEnabled_ = true;
    LOG_DEBUG("EdgeDetector angle filter: {} - {} degrees", minAngle, maxAngle);
}

void EdgeDetector::setParameters(const json& params) {
    try {
        // モード
        if (params.contains("mode")) {
            std::string modeStr = params["mode"].get<std::string>();
            setDetectionMode(stringToEdgeMode(modeStr));
        }

        // Cannyパラメータ
        if (params.contains("low_threshold")) {
            cannyParams_.lowThreshold = params["low_threshold"].get<double>();
        }
        if (params.contains("high_threshold")) {
            cannyParams_.highThreshold = params["high_threshold"].get<double>();
        }
        if (params.contains("canny_aperture_size")) {
            cannyParams_.apertureSize = params["canny_aperture_size"].get<int>();
        }
        if (params.contains("canny_l2_gradient")) {
            cannyParams_.L2gradient = params["canny_l2_gradient"].get<bool>();
        }

        // Sobelパラメータ
        if (params.contains("sobel_kernel_size")) {
            sobelParams_.kernelSize = params["sobel_kernel_size"].get<int>();
        }
        if (params.contains("sobel_threshold")) {
            sobelParams_.threshold = params["sobel_threshold"].get<double>();
        }

        // Laplacianパラメータ
        if (params.contains("laplacian_kernel_size")) {
            laplacianParams_.kernelSize = params["laplacian_kernel_size"].get<int>();
        }
        if (params.contains("laplacian_threshold")) {
            laplacianParams_.threshold = params["laplacian_threshold"].get<double>();
        }

        // フィルタ
        if (params.contains("min_edge_length")) {
            minEdgeLength_ = params["min_edge_length"].get<double>();
        }
        if (params.contains("max_edge_length")) {
            maxEdgeLength_ = params["max_edge_length"].get<double>();
        }
        if (params.contains("min_edge_angle")) {
            minEdgeAngle_ = params["min_edge_angle"].get<double>();
        }
        if (params.contains("max_edge_angle")) {
            maxEdgeAngle_ = params["max_edge_angle"].get<double>();
        }
        if (params.contains("angle_filter_enabled")) {
            angleFilterEnabled_ = params["angle_filter_enabled"].get<bool>();
        }

        // 信頼度閾値
        if (params.contains("confidence_threshold")) {
            setConfidenceThreshold(params["confidence_threshold"].get<double>());
        }

        LOG_INFO("EdgeDetector parameters updated");

    } catch (const json::exception& e) {
        LOG_ERROR("Failed to set EdgeDetector parameters: {}", e.what());
    }
}

json EdgeDetector::getParameters() const {
    json params;

    params["type"] = getType();
    params["name"] = getName();
    params["enabled"] = isEnabled();
    params["mode"] = edgeModeToString(mode_);

    // Cannyパラメータ
    params["low_threshold"] = cannyParams_.lowThreshold;
    params["high_threshold"] = cannyParams_.highThreshold;
    params["canny_aperture_size"] = cannyParams_.apertureSize;
    params["canny_l2_gradient"] = cannyParams_.L2gradient;

    // Sobelパラメータ
    params["sobel_kernel_size"] = sobelParams_.kernelSize;
    params["sobel_scale"] = sobelParams_.scale;
    params["sobel_delta"] = sobelParams_.delta;
    params["sobel_threshold"] = sobelParams_.threshold;

    // Laplacianパラメータ
    params["laplacian_kernel_size"] = laplacianParams_.kernelSize;
    params["laplacian_scale"] = laplacianParams_.scale;
    params["laplacian_delta"] = laplacianParams_.delta;
    params["laplacian_threshold"] = laplacianParams_.threshold;

    // フィルタ
    params["min_edge_length"] = minEdgeLength_;
    params["max_edge_length"] = maxEdgeLength_;
    params["min_edge_angle"] = minEdgeAngle_;
    params["max_edge_angle"] = maxEdgeAngle_;
    params["angle_filter_enabled"] = angleFilterEnabled_;

    // 信頼度
    params["confidence_threshold"] = getConfidenceThreshold();

    return params;
}

std::unique_ptr<DetectorBase> EdgeDetector::clone() const {
    auto cloned = std::make_unique<EdgeDetector>(mode_);
    cloned->setEnabled(isEnabled());
    cloned->setConfidenceThreshold(getConfidenceThreshold());
    cloned->setCannyParams(cannyParams_);
    cloned->setSobelParams(sobelParams_);
    cloned->setLaplacianParams(laplacianParams_);
    cloned->setEdgeLengthFilter(minEdgeLength_, maxEdgeLength_);
    if (angleFilterEnabled_) {
        cloned->setEdgeAngleFilter(minEdgeAngle_, maxEdgeAngle_);
    }
    return cloned;
}

} // namespace inspection
