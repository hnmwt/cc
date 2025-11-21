#include "detectors/TemplateMatcher.h"
#include "detectors/DetectorBase.h"
#include "io/ImageIO.h"
#include <iostream>
#include <cassert>

using namespace inspection;

/**
 * @brief テスト用の画像を生成
 */
cv::Mat createTestImage(int width, int height, bool addDefects = false) {
    // 均一な背景画像を作成
    cv::Mat image(height, width, CV_8UC1, cv::Scalar(200));

    if (addDefects) {
        // 欠陥1: 暗い円形領域（汚れ）
        cv::circle(image, cv::Point(150, 150), 30, cv::Scalar(100), -1);

        // 欠陥2: 明るい長方形領域（傷）
        cv::rectangle(image, cv::Point(300, 100), cv::Point(450, 120), cv::Scalar(250), -1);

        // 欠陥3: 不定形領域（変色）
        std::vector<cv::Point> pts = {
            cv::Point(400, 300),
            cv::Point(450, 320),
            cv::Point(480, 350),
            cv::Point(460, 380),
            cv::Point(420, 390),
            cv::Point(390, 360)
        };
        cv::fillConvexPoly(image, pts, cv::Scalar(150));

        // 欠陥4: 細長い領域（傷）
        cv::rectangle(image, cv::Point(100, 350), cv::Point(200, 360), cv::Scalar(250), -1);
    }

    return image;
}

/**
 * @brief 基本的な検出テスト
 */
void testBasicDetection() {
    std::cout << "=== Test 1: Basic Defect Detection ===" << std::endl;

    // リファレンス画像（良品）を作成
    cv::Mat reference = createTestImage(640, 480, false);

    // テスト画像（欠陥あり）を作成
    cv::Mat testImage = createTestImage(640, 480, true);

    // 画像を保存（確認用）
    ImageIO::saveImage(reference, "data/output/template_reference.jpg");
    ImageIO::saveImage(testImage, "data/output/template_test_input.jpg");
    std::cout << "✓ Test images created" << std::endl;

    // TemplateMatcherを作成（より感度を高く設定）
    TemplateMatcher detector(20.0, 50.0, 50000.0);
    detector.setConfidenceThreshold(0.1);  // 低い閾値で感度を上げる
    detector.setBlurKernelSize(3);  // ブラーを弱めに
    detector.setReferenceImage(reference);

    assert(detector.hasReferenceImage());
    std::cout << "✓ Reference image set" << std::endl;

    // 欠陥検出
    Defects defects = detector.detect(testImage);

    std::cout << "✓ Detection completed" << std::endl;
    std::cout << "  Detected " << defects.size() << " defects" << std::endl;

    // 少なくとも1つは検出されるはず
    assert(defects.size() > 0);

    // 各欠陥の情報を表示
    for (size_t i = 0; i < defects.size(); ++i) {
        const auto& defect = defects[i];
        std::cout << "  Defect #" << (i + 1) << ":" << std::endl;
        std::cout << "    Type: " << defect.getTypeString() << std::endl;
        std::cout << "    Confidence: " << (defect.confidence * 100.0) << "%" << std::endl;
        std::cout << "    Area: " << defect.area << " pixels" << std::endl;
        std::cout << "    Circularity: " << defect.circularity << std::endl;
        std::cout << "    BBox: (" << defect.bbox.x << ", " << defect.bbox.y
                  << ", " << defect.bbox.width << ", " << defect.bbox.height << ")" << std::endl;
    }

    // 可視化
    cv::Mat visualized = DetectorBase::visualizeDefects(testImage, defects, true, true, true);
    ImageIO::saveImage(visualized, "data/output/template_result.jpg");
    std::cout << "✓ Visualization saved to: data/output/template_result.jpg" << std::endl;

    // 差分画像を保存（デバッグ用）
    cv::Mat diffImage = detector.getDiffImage();
    if (!diffImage.empty()) {
        ImageIO::saveImage(diffImage, "data/output/template_diff.jpg");
        std::cout << "✓ Diff image saved" << std::endl;
    }

    // 二値化画像を保存（デバッグ用）
    cv::Mat thresholdImage = detector.getThresholdImage();
    if (!thresholdImage.empty()) {
        ImageIO::saveImage(thresholdImage, "data/output/template_threshold.jpg");
        std::cout << "✓ Threshold image saved" << std::endl;
    }
}

/**
 * @brief パラメータ調整テスト
 */
void testParameterAdjustment() {
    std::cout << "\n=== Test 2: Parameter Adjustment ===" << std::endl;

    TemplateMatcher detector;

    // パラメータ設定
    detector.setDiffThreshold(50.0);
    detector.setMinDefectArea(200.0);
    detector.setMaxDefectArea(5000.0);
    detector.setBlurKernelSize(7);
    detector.setMorphologyKernelSize(5);
    detector.setConfidenceThreshold(0.6);

    // パラメータ確認
    assert(detector.getDiffThreshold() == 50.0);
    assert(detector.getMinDefectArea() == 200.0);
    assert(detector.getMaxDefectArea() == 5000.0);
    assert(detector.getBlurKernelSize() == 7);
    assert(detector.getMorphologyKernelSize() == 5);
    assert(detector.getConfidenceThreshold() == 0.6);

    std::cout << "✓ Parameter setting works" << std::endl;

    // JSONパラメータ
    json params;
    params["diff_threshold"] = 40.0;
    params["min_area"] = 150.0;
    params["max_area"] = 8000.0;
    params["blur_kernel_size"] = 5;
    params["morphology_kernel_size"] = 3;
    params["confidence_threshold"] = 0.5;

    detector.setParameters(params);

    json retrievedParams = detector.getParameters();
    std::cout << "Parameters:\n" << retrievedParams.dump(2) << std::endl;

    assert(retrievedParams["diff_threshold"] == 40.0);
    assert(retrievedParams["min_area"] == 150.0);
    std::cout << "✓ JSON parameter setting works" << std::endl;
}

/**
 * @brief 実画像でのテスト
 */
void testWithRealImage() {
    std::cout << "\n=== Test 3: Real Image Detection ===" << std::endl;

    // ポテト画像を使用
    std::string imagePath = "data/input/1346653592-potato-N92z-1920x1200-MM-100.jpg";
    cv::Mat realImage = ImageIO::loadImage(imagePath, cv::IMREAD_GRAYSCALE);

    if (realImage.empty()) {
        std::cout << "⚠ Real image not found, skipping this test" << std::endl;
        return;
    }

    std::cout << "✓ Real image loaded: " << realImage.cols << "x" << realImage.rows << std::endl;

    // リファレンスとして同じ画像を使用（差分なし）
    TemplateMatcher detector(20.0, 50.0, 5000.0);
    detector.setReferenceImage(realImage);

    Defects defects = detector.detect(realImage);
    std::cout << "  Detected " << defects.size() << " defects (should be 0 or very few)" << std::endl;

    // 同じ画像なので欠陥はほとんど検出されないはず
    assert(defects.size() < 10);

    // 少しノイズを加えた画像でテスト
    cv::Mat noisyImage = realImage.clone();
    cv::Mat noise(noisyImage.size(), noisyImage.type());
    cv::randn(noise, 0, 10);  // 平均0、標準偏差10のガウスノイズ
    noisyImage += noise;

    // 人工的な欠陥を追加
    cv::rectangle(noisyImage, cv::Point(500, 300), cv::Point(600, 400), cv::Scalar(0), -1);
    cv::circle(noisyImage, cv::Point(1000, 600), 50, cv::Scalar(255), -1);

    ImageIO::saveImage(noisyImage, "data/output/template_real_defective.jpg");

    defects = detector.detect(noisyImage);
    std::cout << "  Detected " << defects.size() << " defects in noisy image" << std::endl;

    if (defects.size() > 0) {
        cv::Mat visualized = DetectorBase::visualizeDefects(noisyImage, defects, true, true, true);
        ImageIO::saveImage(visualized, "data/output/template_real_result.jpg");
        std::cout << "✓ Real image result saved" << std::endl;
    }
}

/**
 * @brief 統計情報のテスト
 */
void testStatistics() {
    std::cout << "\n=== Test 4: Statistics ===" << std::endl;

    TemplateMatcher detector(20.0, 50.0, 50000.0);
    detector.setConfidenceThreshold(0.1);  // 低い閾値で感度を上げる
    cv::Mat reference = createTestImage(640, 480, false);
    cv::Mat testImage = createTestImage(640, 480, true);

    detector.setReferenceImage(reference);
    detector.detect(testImage);
    detector.detect(testImage);  // 2回実行

    json stats = detector.getStatistics();
    std::cout << "Statistics:\n" << stats.dump(2) << std::endl;

    assert(stats["name"] == "TemplateMatcher");
    assert(stats["type"] == "template");
    assert(stats["total_detections"].get<size_t>() > 0);
    std::cout << "✓ Statistics collection works" << std::endl;

    detector.resetStatistics();
    stats = detector.getStatistics();
    assert(stats["total_detections"] == 0);
    std::cout << "✓ Statistics reset works" << std::endl;
}

/**
 * @brief クローンテスト
 */
void testClone() {
    std::cout << "\n=== Test 5: Clone ===" << std::endl;

    TemplateMatcher original;
    original.setDiffThreshold(35.0);
    original.setMinDefectArea(150.0);
    original.setConfidenceThreshold(0.7);

    cv::Mat reference = createTestImage(640, 480, false);
    original.setReferenceImage(reference);

    auto cloned = original.clone();
    auto* clonedMatcher = dynamic_cast<TemplateMatcher*>(cloned.get());

    assert(clonedMatcher != nullptr);
    assert(clonedMatcher->getDiffThreshold() == 35.0);
    assert(clonedMatcher->getMinDefectArea() == 150.0);
    assert(clonedMatcher->getConfidenceThreshold() == 0.7);
    assert(clonedMatcher->hasReferenceImage());

    std::cout << "✓ Clone works correctly" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TemplateMatcher Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    try {
        testBasicDetection();
        testParameterAdjustment();
        testWithRealImage();
        testStatistics();
        testClone();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ All TemplateMatcher tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
