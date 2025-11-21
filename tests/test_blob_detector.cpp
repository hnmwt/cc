/**
 * @file test_blob_detector.cpp
 * @brief BlobDetector のテストプログラム
 */

#include "detectors/BlobDetector.h"
#include "io/ImageIO.h"
#include "utils/Logger.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace inspection;

/**
 * @brief テスト用の画像を生成（さまざまなブロブを含む）
 */
cv::Mat createTestImage() {
    // 640x480 のグレースケール画像を作成
    cv::Mat image = cv::Mat::zeros(480, 640, CV_8UC1);
    image.setTo(cv::Scalar(200));  // 明るい背景

    // 1. 暗い円形ブロブ（汚れ/異物）
    cv::circle(image, cv::Point(100, 100), 15, cv::Scalar(50), -1);

    // 2. 暗い小円形ブロブ
    cv::circle(image, cv::Point(200, 100), 8, cv::Scalar(30), -1);

    // 3. 暗い中円形ブロブ
    cv::circle(image, cv::Point(300, 100), 20, cv::Scalar(40), -1);

    // 4. 細長いブロブ（傷）
    cv::ellipse(image, cv::Point(150, 250), cv::Size(50, 5), 0, 0, 360, cv::Scalar(20), -1);

    // 5. 細長いブロブ（傷）- 角度あり
    cv::ellipse(image, cv::Point(350, 250), cv::Size(60, 4), 45, 0, 360, cv::Scalar(10), -1);

    // 6. 大きな不定形ブロブ（形状不良）
    std::vector<cv::Point> contour = {
        cv::Point(100, 350), cv::Point(150, 330), cv::Point(200, 340),
        cv::Point(220, 380), cv::Point(180, 420), cv::Point(120, 410)
    };
    cv::fillConvexPoly(image, contour, cv::Scalar(60));

    // 7. 小さなノイズ（フィルタリングされるべき）
    cv::circle(image, cv::Point(500, 100), 2, cv::Scalar(0), -1);
    cv::circle(image, cv::Point(510, 105), 3, cv::Scalar(0), -1);

    // 8. 大きな円形ブロブ
    cv::circle(image, cv::Point(450, 350), 40, cv::Scalar(70), -1);

    return image;
}

/**
 * @brief テスト1: デフォルトパラメータでの検出
 */
bool testDefaultDetection() {
    std::cout << "\n=== Test 1: Default Parameter Detection ===" << std::endl;

    BlobDetector detector;
    detector.setConfidenceThreshold(0.0);  // Accept all detected blobs
    cv::Mat testImage = createTestImage();

    // 画像を保存（デバッグ用）
    ImageIO imageIO;
    imageIO.saveImage(testImage, "data/output/test_blob_input.jpg");

    // 検出実行
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " defects" << std::endl;
    std::cout << "KeyPoints: " << detector.getLastKeyPoints().size() << std::endl;

    // 検出結果を表示
    for (size_t i = 0; i < defects.size(); ++i) {
        const auto& defect = defects[i];
        std::cout << "  Defect " << i + 1 << ": "
                  << "Type=" << static_cast<int>(defect.type)
                  << ", Center=(" << defect.center.x << "," << defect.center.y << ")"
                  << ", Area=" << defect.area
                  << ", Confidence=" << defect.confidence
                  << ", Circularity=" << defect.circularity
                  << std::endl;
    }

    // 可視化
    cv::Mat visualized = testImage.clone();
    cv::cvtColor(visualized, visualized, cv::COLOR_GRAY2BGR);

    for (const auto& defect : defects) {
        // バウンディングボックス
        cv::rectangle(visualized, defect.bbox, cv::Scalar(0, 255, 0), 2);
        // 中心点
        cv::circle(visualized, defect.center, 3, cv::Scalar(0, 0, 255), -1);
        // タイプを表示
        cv::putText(visualized, std::to_string(static_cast<int>(defect.type)),
                    cv::Point(defect.bbox.x, defect.bbox.y - 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
    }

    imageIO.saveImage(visualized, "data/output/test_blob_result_default.jpg");
    std::cout << "Result saved to: data/output/test_blob_result_default.jpg" << std::endl;

    return !defects.empty();
}

/**
 * @brief テスト2: カスタムパラメータでの検出（暗いブロブのみ）
 */
bool testDarkBlobDetection() {
    std::cout << "\n=== Test 2: Dark Blob Detection ===" << std::endl;

    BlobDetector detector;
    detector.setConfidenceThreshold(0.0);  // Accept all detected blobs

    // 暗いブロブのみ検出
    detector.setColorThreshold(0);  // 0 = 暗いブロブ
    detector.setAreaThreshold(30.0, 5000.0);  // 小〜中サイズ
    detector.setCircularityThreshold(0.5, 1.0);  // 円形に近いもの

    cv::Mat testImage = createTestImage();
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " dark circular blobs" << std::endl;

    for (size_t i = 0; i < defects.size(); ++i) {
        const auto& defect = defects[i];
        std::cout << "  Blob " << i + 1 << ": "
                  << "Area=" << defect.area
                  << ", Circularity=" << defect.circularity
                  << std::endl;
    }

    return !defects.empty();
}

/**
 * @brief テスト3: 傷の検出（細長いブロブ）
 */
bool testScratchDetection() {
    std::cout << "\n=== Test 3: Scratch Detection ===" << std::endl;

    BlobDetector detector;
    detector.setConfidenceThreshold(0.0);  // Accept all detected blobs

    // 細長いブロブを検出
    detector.setColorThreshold(0);  // 暗いブロブ
    detector.setAreaThreshold(20.0, 5000.0);
    detector.setCircularityThreshold(0.01, 0.6);  // 低円形度（0.0は不可）
    detector.setInertiaThreshold(0.01, 0.4);  // 細長い（0.0は不可）

    cv::Mat testImage = createTestImage();
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " scratches" << std::endl;

    // 傷（Scratch）タイプをカウント
    int scratchCount = 0;
    for (const auto& defect : defects) {
        if (defect.type == DefectType::Scratch) {
            scratchCount++;
            std::cout << "  Scratch found at (" << defect.center.x << "," << defect.center.y << ")" << std::endl;
        }
    }

    std::cout << "Total scratches classified: " << scratchCount << std::endl;

    return scratchCount > 0;
}

/**
 * @brief テスト4: JSON設定でのパラメータ変更
 */
bool testJsonParameters() {
    std::cout << "\n=== Test 4: JSON Parameter Configuration ===" << std::endl;

    BlobDetector detector;

    // JSON形式でパラメータを設定
    nlohmann::json params;
    params["blob_color"] = 0;
    params["min_area"] = 50.0;
    params["max_area"] = 1000.0;
    params["min_circularity"] = 0.7;
    params["max_circularity"] = 1.0;
    params["confidence_threshold"] = 0.3;

    detector.setParameters(params);

    // パラメータを取得して確認
    auto currentParams = detector.getParameters();
    std::cout << "Current parameters:" << std::endl;
    std::cout << currentParams.dump(2) << std::endl;

    cv::Mat testImage = createTestImage();
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " defects with JSON config" << std::endl;

    return true;
}

/**
 * @brief テスト5: 実画像でのテスト
 */
bool testRealImage() {
    std::cout << "\n=== Test 5: Real Image Detection ===" << std::endl;

    // 実画像のパスを確認
    std::string imagePath = "data/input/sample.jpg";

    ImageIO imageIO;
    cv::Mat image = imageIO.loadImage(imagePath);

    if (image.empty()) {
        std::cout << "Warning: Sample image not found at " << imagePath << std::endl;
        std::cout << "Skipping real image test." << std::endl;
        return true;  // スキップ
    }

    BlobDetector detector;
    detector.setAreaThreshold(50.0, 10000.0);
    detector.setCircularityThreshold(0.3, 1.0);
    detector.setConfidenceThreshold(0.4);

    Defects defects = detector.detect(image);

    std::cout << "Detected " << defects.size() << " defects in real image" << std::endl;

    // 結果を可視化
    cv::Mat visualized = image.clone();
    if (visualized.channels() == 1) {
        cv::cvtColor(visualized, visualized, cv::COLOR_GRAY2BGR);
    }

    for (const auto& defect : defects) {
        cv::rectangle(visualized, defect.bbox, cv::Scalar(0, 255, 0), 2);
        cv::circle(visualized, defect.center, 3, cv::Scalar(0, 0, 255), -1);
    }

    imageIO.saveImage(visualized, "data/output/test_blob_real_result.jpg");
    std::cout << "Result saved to: data/output/test_blob_real_result.jpg" << std::endl;

    return true;
}

/**
 * @brief テスト6: クローン機能のテスト
 */
bool testCloneFunction() {
    std::cout << "\n=== Test 6: Clone Function ===" << std::endl;

    BlobDetector detector1;
    detector1.setAreaThreshold(100.0, 5000.0);
    detector1.setConfidenceThreshold(0.6);
    detector1.setEnabled(true);

    // クローン作成
    auto cloned = detector1.clone();
    BlobDetector* detector2 = dynamic_cast<BlobDetector*>(cloned.get());

    if (!detector2) {
        std::cout << "Failed to clone BlobDetector" << std::endl;
        return false;
    }

    // パラメータが同じか確認
    auto params1 = detector1.getParameters();
    auto params2 = detector2->getParameters();

    std::cout << "Original min_area: " << params1["min_area"] << std::endl;
    std::cout << "Cloned min_area: " << params2["min_area"] << std::endl;

    bool parametersMatch = (params1["min_area"] == params2["min_area"]) &&
                          (params1["max_area"] == params2["max_area"]) &&
                          (params1["confidence_threshold"] == params2["confidence_threshold"]);

    std::cout << "Clone parameters match: " << (parametersMatch ? "YES" : "NO") << std::endl;

    return parametersMatch;
}

int main() {
    // ロガー初期化
    Logger::init(Logger::Level::Debug, true, "logs/test_blob_detector.log");

    std::cout << "========================================" << std::endl;
    std::cout << "  BlobDetector Test Program" << std::endl;
    std::cout << "========================================" << std::endl;

    bool allPassed = true;

    // テスト実行
    allPassed &= testDefaultDetection();
    allPassed &= testDarkBlobDetection();
    allPassed &= testScratchDetection();
    allPassed &= testJsonParameters();
    allPassed &= testRealImage();
    allPassed &= testCloneFunction();

    // 結果サマリー
    std::cout << "\n========================================" << std::endl;
    if (allPassed) {
        std::cout << "  All tests PASSED ✓" << std::endl;
    } else {
        std::cout << "  Some tests FAILED ✗" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return allPassed ? 0 : 1;
}
