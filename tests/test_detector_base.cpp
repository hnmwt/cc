#include "detectors/DetectorBase.h"
#include "detectors/Defect.h"
#include "io/ImageIO.h"
#include <iostream>
#include <cassert>

using namespace inspection;

/**
 * @brief 可視化機能のテスト
 */
void testVisualization() {
    std::cout << "=== Test: Defect Visualization ===" << std::endl;

    // テスト用の画像を作成（640x480の白い画像）
    cv::Mat testImage(480, 640, CV_8UC3, cv::Scalar(255, 255, 255));

    // テスト用の欠陥を作成
    Defects defects;

    // 欠陥1: 傷（Scratch）
    Defect defect1(DefectType::Scratch, cv::Rect(100, 100, 80, 60), 0.95);
    defect1.contour = {
        cv::Point(100, 100),
        cv::Point(180, 100),
        cv::Point(180, 160),
        cv::Point(100, 160)
    };
    defect1.circularity = 0.65;
    defects.push_back(defect1);

    // 欠陥2: 汚れ（Stain）
    Defect defect2(DefectType::Stain, cv::Rect(300, 200, 50, 50), 0.87);
    defect2.contour = {
        cv::Point(300, 225),
        cv::Point(325, 200),
        cv::Point(350, 225),
        cv::Point(325, 250)
    };
    defect2.circularity = 0.82;
    defects.push_back(defect2);

    // 欠陥3: 変色（Discoloration）
    Defect defect3(DefectType::Discoloration, cv::Rect(450, 300, 120, 90), 0.72);
    defect3.contour = {
        cv::Point(450, 300),
        cv::Point(570, 300),
        cv::Point(570, 390),
        cv::Point(450, 390)
    };
    defect3.circularity = 0.55;
    defects.push_back(defect3);

    std::cout << "Created " << defects.size() << " test defects" << std::endl;

    // 可視化画像を生成
    cv::Mat visualized = DetectorBase::visualizeDefects(
        testImage,
        defects,
        true,   // drawContour
        true,   // drawBbox
        true    // drawLabel
    );

    assert(!visualized.empty());
    assert(visualized.size() == testImage.size());
    assert(visualized.channels() == 3);
    std::cout << "✓ Visualization image created: " << visualized.cols << "x" << visualized.rows << std::endl;

    // 結果画像を保存
    bool saved = ImageIO::saveImage(visualized, "data/output/test_visualization.jpg");
    if (saved) {
        std::cout << "✓ Visualization saved to: data/output/test_visualization.jpg" << std::endl;
    } else {
        std::cout << "⚠ Failed to save visualization" << std::endl;
    }

    // 輪郭のみの可視化
    cv::Mat visualizedContour = DetectorBase::visualizeDefects(
        testImage,
        defects,
        true,   // drawContour
        false,  // drawBbox
        false   // drawLabel
    );
    saved = ImageIO::saveImage(visualizedContour, "data/output/test_visualization_contour.jpg");
    if (saved) {
        std::cout << "✓ Contour-only visualization saved" << std::endl;
    }

    // バウンディングボックスのみの可視化
    cv::Mat visualizedBbox = DetectorBase::visualizeDefects(
        testImage,
        defects,
        false,  // drawContour
        true,   // drawBbox
        true    // drawLabel
    );
    saved = ImageIO::saveImage(visualizedBbox, "data/output/test_visualization_bbox.jpg");
    if (saved) {
        std::cout << "✓ Bbox-only visualization saved" << std::endl;
    }
}

/**
 * @brief グレースケール画像の可視化テスト
 */
void testGrayscaleVisualization() {
    std::cout << "\n=== Test: Grayscale Image Visualization ===" << std::endl;

    // グレースケール画像を作成
    cv::Mat grayImage(480, 640, CV_8UC1, cv::Scalar(200));

    // 欠陥を作成
    Defects defects;
    defects.push_back(Defect(DefectType::Deformation, cv::Rect(200, 150, 100, 80), 0.91));

    // 可視化（グレースケール→カラー変換）
    cv::Mat visualized = DetectorBase::visualizeDefects(
        grayImage,
        defects,
        true, true, true
    );

    assert(!visualized.empty());
    assert(visualized.channels() == 3);  // カラーに変換されているはず
    std::cout << "✓ Grayscale image converted to color for visualization" << std::endl;

    bool saved = ImageIO::saveImage(visualized, "data/output/test_visualization_gray.jpg");
    if (saved) {
        std::cout << "✓ Grayscale visualization saved" << std::endl;
    }
}

/**
 * @brief 空の欠陥リストのテスト
 */
void testEmptyDefects() {
    std::cout << "\n=== Test: Empty Defects List ===" << std::endl;

    cv::Mat testImage(480, 640, CV_8UC3, cv::Scalar(255, 255, 255));
    Defects emptyDefects;

    cv::Mat visualized = DetectorBase::visualizeDefects(
        testImage,
        emptyDefects,
        true, true, true
    );

    assert(!visualized.empty());
    assert(visualized.size() == testImage.size());
    std::cout << "✓ Empty defects list handled correctly" << std::endl;
}

/**
 * @brief JSON統計情報のテスト（モックDetector使用）
 */
class MockDetector : public DetectorBase {
public:
    MockDetector() {
        confidenceThreshold_ = 0.75;
    }

    Defects detect(const cv::Mat& image) override {
        auto start = cv::getTickCount();

        // ダミーの検出結果
        Defects defects;
        defects.push_back(Defect(DefectType::Scratch, cv::Rect(10, 10, 20, 20), 0.9));

        auto end = cv::getTickCount();
        double time = (end - start) * 1000.0 / cv::getTickFrequency();
        recordStatistics(defects.size(), time);

        return defects;
    }

    std::string getName() const override { return "MockDetector"; }
    std::string getType() const override { return "mock"; }

    void setParameters(const json& params) override {
        if (params.contains("threshold")) {
            confidenceThreshold_ = params["threshold"].get<double>();
        }
    }

    json getParameters() const override {
        json params;
        params["threshold"] = confidenceThreshold_;
        return params;
    }

    std::unique_ptr<DetectorBase> clone() const override {
        return std::make_unique<MockDetector>(*this);
    }
};

void testDetectorStatistics() {
    std::cout << "\n=== Test: Detector Statistics ===" << std::endl;

    MockDetector detector;

    // パラメータ設定
    detector.setConfidenceThreshold(0.8);
    assert(detector.getConfidenceThreshold() == 0.8);
    std::cout << "✓ Confidence threshold setting works" << std::endl;

    // リファレンス画像設定
    cv::Mat refImage(100, 100, CV_8UC1, cv::Scalar(128));
    detector.setReferenceImage(refImage);
    assert(detector.hasReferenceImage());
    assert(detector.getReferenceImage().size() == refImage.size());
    std::cout << "✓ Reference image setting works" << std::endl;

    // 検出実行
    cv::Mat testImage(100, 100, CV_8UC1, cv::Scalar(100));
    Defects result = detector.detect(testImage);
    assert(result.size() == 1);
    std::cout << "✓ Detection works" << std::endl;

    // 統計情報確認
    json stats = detector.getStatistics();
    std::cout << "Statistics:\n" << stats.dump(2) << std::endl;

    assert(stats["name"] == "MockDetector");
    assert(stats["type"] == "mock");
    assert(stats["confidence_threshold"] == 0.8);
    assert(stats["has_reference"] == true);
    assert(stats["total_detections"] == 1);
    std::cout << "✓ Statistics collection works" << std::endl;

    // クローン作成
    auto cloned = detector.clone();
    assert(cloned->getName() == detector.getName());
    assert(cloned->getConfidenceThreshold() == detector.getConfidenceThreshold());
    std::cout << "✓ Clone works" << std::endl;

    // 統計リセット
    detector.resetStatistics();
    stats = detector.getStatistics();
    assert(stats["total_detections"] == 0);
    std::cout << "✓ Statistics reset works" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "DetectorBase Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    try {
        testVisualization();
        testGrayscaleVisualization();
        testEmptyDefects();
        testDetectorStatistics();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ All DetectorBase tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
