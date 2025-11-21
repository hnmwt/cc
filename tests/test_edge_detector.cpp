/**
 * @file test_edge_detector.cpp
 * @brief EdgeDetector のテストプログラム
 */

#include "detectors/EdgeDetector.h"
#include "io/ImageIO.h"
#include "utils/Logger.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace inspection;

/**
 * @brief テスト用の画像を生成（傷とクラックを含む）
 */
cv::Mat createTestImage() {
    // 800x1200 のカラー画像を作成
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);
    image.setTo(cv::Scalar(210, 210, 210));  // 明るい灰色背景

    // 1. 水平の傷（長い直線）
    cv::line(image, cv::Point(100, 200), cv::Point(600, 205),
             cv::Scalar(30, 30, 30), 3);

    // 2. 垂直の傷
    cv::line(image, cv::Point(800, 100), cv::Point(805, 500),
             cv::Scalar(25, 25, 25), 2);

    // 3. 斜めの傷
    cv::line(image, cv::Point(200, 400), cv::Point(500, 600),
             cv::Scalar(20, 20, 20), 3);

    // 4. クラック（途切れた線）
    cv::line(image, cv::Point(900, 300), cv::Point(950, 320),
             cv::Scalar(35, 35, 35), 2);
    cv::line(image, cv::Point(960, 325), cv::Point(1000, 340),
             cv::Scalar(35, 35, 35), 2);

    // 5. 短い傷
    cv::line(image, cv::Point(300, 700), cv::Point(380, 710),
             cv::Scalar(40, 40, 40), 2);

    // 6. 曲線状の欠陥
    cv::ellipse(image, cv::Point(700, 600), cv::Size(80, 50),
                30, 0, 180, cv::Scalar(28, 28, 28), 2);

    // 軽いノイズを追加
    cv::Mat noise(image.size(), CV_8UC3);
    cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(3, 3, 3));
    cv::add(image, noise, image);

    return image;
}

/**
 * @brief テスト1: Cannyエッジ検出
 */
bool testCannyDetection() {
    std::cout << "\n=== Test 1: Canny Edge Detection ===" << std::endl;

    EdgeDetector detector(EdgeDetector::EdgeDetectionMode::Canny);
    detector.setConfidenceThreshold(0.3);
    detector.setEdgeLengthFilter(20.0, 1000.0);

    // Cannyパラメータを調整
    EdgeDetector::CannyParams cannyParams;
    cannyParams.lowThreshold = 50.0;
    cannyParams.highThreshold = 150.0;
    detector.setCannyParams(cannyParams);

    cv::Mat testImage = createTestImage();

    // 検出実行
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " edge defects" << std::endl;

    // 検出結果を表示
    for (size_t i = 0; i < defects.size(); ++i) {
        const auto& defect = defects[i];
        std::cout << "  Edge " << i + 1 << ": "
                  << "Type=" << static_cast<int>(defect.type)
                  << ", Center=(" << defect.center.x << "," << defect.center.y << ")"
                  << ", Length=" << defect.area
                  << ", Confidence=" << defect.confidence
                  << std::endl;
    }

    // 可視化
    cv::Mat visualized = testImage.clone();
    for (const auto& defect : defects) {
        cv::rectangle(visualized, defect.bbox, cv::Scalar(0, 255, 0), 2);
        cv::circle(visualized, defect.center, 3, cv::Scalar(0, 0, 255), -1);
    }

    // エッジ画像も保存
    cv::Mat edgeImage = detector.getLastEdgeImage();
    cv::Mat edgeVis;
    cv::cvtColor(edgeImage, edgeVis, cv::COLOR_GRAY2BGR);

    ImageIO imageIO;
    imageIO.saveImage(testImage, "data/output/test_edge_input.jpg");
    imageIO.saveImage(edgeVis, "data/output/test_edge_canny.jpg");
    imageIO.saveImage(visualized, "data/output/test_edge_result_canny.jpg");

    std::cout << "Results saved to: data/output/" << std::endl;

    return !defects.empty();
}

/**
 * @brief テスト2: Sobelエッジ検出
 */
bool testSobelDetection() {
    std::cout << "\n=== Test 2: Sobel Edge Detection ===" << std::endl;

    EdgeDetector detector(EdgeDetector::EdgeDetectionMode::Sobel);
    detector.setConfidenceThreshold(0.3);
    detector.setEdgeLengthFilter(20.0, 1000.0);

    // Sobelパラメータを調整
    EdgeDetector::SobelParams sobelParams;
    sobelParams.kernelSize = 3;
    sobelParams.threshold = 50.0;
    detector.setSobelParams(sobelParams);

    cv::Mat testImage = createTestImage();
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " edge defects (Sobel)" << std::endl;

    // エッジ画像を保存
    cv::Mat edgeImage = detector.getLastEdgeImage();
    cv::Mat edgeVis;
    cv::cvtColor(edgeImage, edgeVis, cv::COLOR_GRAY2BGR);

    ImageIO imageIO;
    imageIO.saveImage(edgeVis, "data/output/test_edge_sobel.jpg");

    return !defects.empty();
}

/**
 * @brief テスト3: Laplacianエッジ検出
 */
bool testLaplacianDetection() {
    std::cout << "\n=== Test 3: Laplacian Edge Detection ===" << std::endl;

    EdgeDetector detector(EdgeDetector::EdgeDetectionMode::Laplacian);
    detector.setConfidenceThreshold(0.3);
    detector.setEdgeLengthFilter(20.0, 1000.0);

    // Laplacianパラメータを調整
    EdgeDetector::LaplacianParams laplacianParams;
    laplacianParams.kernelSize = 3;
    laplacianParams.threshold = 30.0;
    detector.setLaplacianParams(laplacianParams);

    cv::Mat testImage = createTestImage();
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " edge defects (Laplacian)" << std::endl;

    // エッジ画像を保存
    cv::Mat edgeImage = detector.getLastEdgeImage();
    cv::Mat edgeVis;
    cv::cvtColor(edgeImage, edgeVis, cv::COLOR_GRAY2BGR);

    ImageIO imageIO;
    imageIO.saveImage(edgeVis, "data/output/test_edge_laplacian.jpg");

    return !defects.empty();
}

/**
 * @brief テスト4: 複合検出
 */
bool testCombinedDetection() {
    std::cout << "\n=== Test 4: Combined Edge Detection ===" << std::endl;

    EdgeDetector detector(EdgeDetector::EdgeDetectionMode::Combined);
    detector.setConfidenceThreshold(0.3);
    detector.setEdgeLengthFilter(20.0, 1000.0);

    cv::Mat testImage = createTestImage();
    Defects defects = detector.detect(testImage);

    std::cout << "Detected " << defects.size() << " edge defects (Combined)" << std::endl;

    // エッジ画像を保存
    cv::Mat edgeImage = detector.getLastEdgeImage();
    cv::Mat edgeVis;
    cv::cvtColor(edgeImage, edgeVis, cv::COLOR_GRAY2BGR);

    ImageIO imageIO;
    imageIO.saveImage(edgeVis, "data/output/test_edge_combined.jpg");

    return !defects.empty();
}

/**
 * @brief テスト5: エッジ長さフィルタ
 */
bool testEdgeLengthFilter() {
    std::cout << "\n=== Test 5: Edge Length Filter ===" << std::endl;

    EdgeDetector detector(EdgeDetector::EdgeDetectionMode::Canny);
    detector.setConfidenceThreshold(0.0);  // すべて受け入れ

    cv::Mat testImage = createTestImage();

    // 長いエッジのみ検出
    detector.setEdgeLengthFilter(100.0, 1000.0);
    Defects longEdges = detector.detect(testImage);
    std::cout << "Long edges (>100px): " << longEdges.size() << std::endl;

    // 短いエッジのみ検出
    detector.setEdgeLengthFilter(10.0, 50.0);
    Defects shortEdges = detector.detect(testImage);
    std::cout << "Short edges (<50px): " << shortEdges.size() << std::endl;

    return true;
}

/**
 * @brief テスト6: エッジ角度フィルタ
 */
bool testEdgeAngleFilter() {
    std::cout << "\n=== Test 6: Edge Angle Filter ===" << std::endl;

    EdgeDetector detector(EdgeDetector::EdgeDetectionMode::Canny);
    detector.setConfidenceThreshold(0.0);
    detector.setEdgeLengthFilter(20.0, 1000.0);

    cv::Mat testImage = createTestImage();

    // 水平エッジのみ（±10度）
    detector.setEdgeAngleFilter(0.0, 10.0);
    Defects horizontalEdges = detector.detect(testImage);
    std::cout << "Horizontal edges (0-10°): " << horizontalEdges.size() << std::endl;

    // 垂直エッジのみ（80-100度）
    detector.setEdgeAngleFilter(80.0, 100.0);
    Defects verticalEdges = detector.detect(testImage);
    std::cout << "Vertical edges (80-100°): " << verticalEdges.size() << std::endl;

    return true;
}

/**
 * @brief テスト7: JSON設定
 */
bool testJsonConfiguration() {
    std::cout << "\n=== Test 7: JSON Configuration ===" << std::endl;

    EdgeDetector detector;

    // JSON形式でパラメータを設定
    nlohmann::json params;
    params["mode"] = "canny";
    params["low_threshold"] = 40.0;
    params["high_threshold"] = 120.0;
    params["min_edge_length"] = 30.0;
    params["max_edge_length"] = 800.0;
    params["confidence_threshold"] = 0.4;

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
 * @brief テスト8: Clone機能
 */
bool testCloneFunction() {
    std::cout << "\n=== Test 8: Clone Function ===" << std::endl;

    EdgeDetector detector1(EdgeDetector::EdgeDetectionMode::Sobel);
    detector1.setEdgeLengthFilter(50.0, 500.0);
    detector1.setConfidenceThreshold(0.5);

    // クローン作成
    auto cloned = detector1.clone();
    EdgeDetector* detector2 = dynamic_cast<EdgeDetector*>(cloned.get());

    if (!detector2) {
        std::cout << "Failed to clone EdgeDetector" << std::endl;
        return false;
    }

    // パラメータが同じか確認
    auto params1 = detector1.getParameters();
    auto params2 = detector2->getParameters();

    std::cout << "Original mode: " << params1["mode"] << std::endl;
    std::cout << "Cloned mode: " << params2["mode"] << std::endl;

    bool parametersMatch = (params1["mode"] == params2["mode"]) &&
                          (params1["min_edge_length"] == params2["min_edge_length"]) &&
                          (params1["confidence_threshold"] == params2["confidence_threshold"]);

    std::cout << "Clone parameters match: " << (parametersMatch ? "YES" : "NO") << std::endl;

    return parametersMatch;
}

int main() {
    // ロガー初期化
    Logger::init(Logger::Level::Debug, true, "logs/test_edge_detector.log");

    std::cout << "========================================" << std::endl;
    std::cout << "  EdgeDetector Test Program" << std::endl;
    std::cout << "========================================" << std::endl;

    bool allPassed = true;

    // テスト実行
    allPassed &= testCannyDetection();
    allPassed &= testSobelDetection();
    allPassed &= testLaplacianDetection();
    allPassed &= testCombinedDetection();
    allPassed &= testEdgeLengthFilter();
    allPassed &= testEdgeAngleFilter();
    allPassed &= testJsonConfiguration();
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
