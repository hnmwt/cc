#include "detectors/TemplateMatcher.h"
#include "detectors/DetectorBase.h"
#include "io/ImageIO.h"
#include "utils/Logger.h"
#include <iostream>
#include <iomanip>

using namespace inspection;

/**
 * @brief TemplateMatcher デモプログラム
 *
 * 使い方:
 *   ./demo_template_matcher <reference_image> <test_image>
 *
 * 例:
 *   ./demo_template_matcher data/input/good.jpg data/input/test.jpg
 */
int main(int argc, char** argv) {
    // ロガー初期化
    Logger::init(Logger::Level::Info, true, "logs/inspection.log");

    std::cout << "========================================" << std::endl;
    std::cout << "TemplateMatcher Demo" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // デモモードチェック
    if (argc >= 2 && std::string(argv[1]) == "demo") {
        std::cout << "Running in DEMO mode..." << std::endl;
        std::cout << "Creating artificial reference and test images\n" << std::endl;

        // リファレンス画像（良品）を作成
        cv::Mat reference(600, 800, CV_8UC1, cv::Scalar(200));
        cv::rectangle(reference, cv::Point(100, 100), cv::Point(700, 500), cv::Scalar(180), -1);
        cv::circle(reference, cv::Point(400, 300), 80, cv::Scalar(190), -1);

        // テスト画像（欠陥あり）を作成
        cv::Mat testImage = reference.clone();

        // 欠陥1: 大きな暗い円（汚れ）
        cv::circle(testImage, cv::Point(250, 200), 40, cv::Scalar(100), -1);
        LOG_INFO("Added defect 1: Dark circle (Stain)");

        // 欠陥2: 細長い明るい線（傷）
        cv::rectangle(testImage, cv::Point(500, 150), cv::Point(650, 160), cv::Scalar(250), -1);
        LOG_INFO("Added defect 2: Bright line (Scratch)");

        // 欠陥3: 不定形の領域（変色）
        std::vector<cv::Point> pts = {
            cv::Point(300, 400),
            cv::Point(350, 420),
            cv::Point(380, 450),
            cv::Point(350, 480),
            cv::Point(310, 470)
        };
        cv::fillConvexPoly(testImage, pts, cv::Scalar(150));
        LOG_INFO("Added defect 3: Irregular shape (Discoloration)");

        // 欠陥4: 小さな点（汚れ）
        cv::circle(testImage, cv::Point(600, 350), 25, cv::Scalar(120), -1);
        LOG_INFO("Added defect 4: Small spot (Stain)");

        // 画像を保存
        ImageIO::saveImage(reference, "data/output/demo_reference.jpg");
        ImageIO::saveImage(testImage, "data/output/demo_test.jpg");
        std::cout << "\n✓ Images saved:" << std::endl;
        std::cout << "  Reference: data/output/demo_reference.jpg" << std::endl;
        std::cout << "  Test:      data/output/demo_test.jpg\n" << std::endl;

        // TemplateMatcherを作成
        std::cout << "Creating TemplateMatcher..." << std::endl;
        TemplateMatcher detector(25.0, 100.0, 100000.0);
        detector.setConfidenceThreshold(0.1);
        detector.setBlurKernelSize(3);
        detector.setMorphologyKernelSize(3);
        detector.setReferenceImage(reference);

        // パラメータ表示
        json params = detector.getParameters();
        std::cout << "\nDetector Parameters:" << std::endl;
        std::cout << std::setw(30) << std::left << "  Diff Threshold:" << params["diff_threshold"] << std::endl;
        std::cout << std::setw(30) << std::left << "  Min Area:" << params["min_area"] << " pixels" << std::endl;
        std::cout << std::setw(30) << std::left << "  Max Area:" << params["max_area"] << " pixels" << std::endl;
        std::cout << std::setw(30) << std::left << "  Confidence Threshold:" << params["confidence_threshold"] << std::endl;
        std::cout << std::setw(30) << std::left << "  Blur Kernel Size:" << params["blur_kernel_size"] << std::endl;
        std::cout << std::setw(30) << std::left << "  Morphology Kernel Size:" << params["morphology_kernel_size"] << std::endl;

        // 欠陥検出
        std::cout << "\n" << std::string(40, '=') << std::endl;
        std::cout << "Running defect detection..." << std::endl;
        std::cout << std::string(40, '=') << "\n" << std::endl;

        Defects defects = detector.detect(testImage);

        // 結果表示
        std::cout << "Detection Results:" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        std::cout << "Total Defects Found: " << defects.size() << "\n" << std::endl;

        if (defects.empty()) {
            std::cout << "No defects detected." << std::endl;
        } else {
            for (size_t i = 0; i < defects.size(); ++i) {
                const auto& defect = defects[i];
                std::cout << "Defect #" << (i + 1) << ":" << std::endl;
                std::cout << "  Type:        " << defect.getTypeString() << std::endl;
                std::cout << "  Confidence:  " << std::fixed << std::setprecision(2)
                          << (defect.confidence * 100.0) << "%" << std::endl;
                std::cout << "  Position:    (" << defect.center.x << ", " << defect.center.y << ")" << std::endl;
                std::cout << "  Area:        " << static_cast<int>(defect.area) << " pixels" << std::endl;
                std::cout << "  Circularity: " << std::setprecision(3) << defect.circularity << std::endl;
                std::cout << "  BBox:        [" << defect.bbox.x << ", " << defect.bbox.y << ", "
                          << defect.bbox.width << ", " << defect.bbox.height << "]" << std::endl;
                std::cout << std::endl;
            }
        }

        // 可視化
        cv::Mat visualized = DetectorBase::visualizeDefects(testImage, defects, true, true, true);
        ImageIO::saveImage(visualized, "data/output/demo_result.jpg");

        // 差分画像と二値化画像を保存
        cv::Mat diffImage = detector.getDiffImage();
        cv::Mat thresholdImage = detector.getThresholdImage();
        if (!diffImage.empty()) {
            ImageIO::saveImage(diffImage, "data/output/demo_diff.jpg");
        }
        if (!thresholdImage.empty()) {
            ImageIO::saveImage(thresholdImage, "data/output/demo_threshold.jpg");
        }

        // 統計情報
        json stats = detector.getStatistics();
        std::cout << std::string(40, '=') << std::endl;
        std::cout << "Statistics:" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        std::cout << "  Detector:         " << stats["name"] << std::endl;
        std::cout << "  Total Detections: " << stats["total_detections"] << std::endl;
        std::cout << "  Processing Time:  " << std::fixed << std::setprecision(3)
                  << stats["total_processing_time_ms"].get<double>() << " ms" << std::endl;

        std::cout << "\n" << std::string(40, '=') << std::endl;
        std::cout << "Output files saved:" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        std::cout << "  ✓ data/output/demo_reference.jpg   (Reference image)" << std::endl;
        std::cout << "  ✓ data/output/demo_test.jpg        (Test image)" << std::endl;
        std::cout << "  ✓ data/output/demo_result.jpg      (Visualization)" << std::endl;
        std::cout << "  ✓ data/output/demo_diff.jpg        (Difference image)" << std::endl;
        std::cout << "  ✓ data/output/demo_threshold.jpg   (Binary image)" << std::endl;
        std::cout << std::string(40, '=') << std::endl;

        return 0;
    }

    // 引数チェック
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <reference_image> <test_image>" << std::endl;
        std::cout << "\nExample:" << std::endl;
        std::cout << "  " << argv[0] << " data/input/reference.jpg data/input/test.jpg" << std::endl;
        std::cout << "\nDemo mode (automatic test):" << std::endl;
        std::cout << "  " << argv[0] << " demo" << std::endl;
        return 1;
    }

    // 通常モード: ユーザー指定の画像を使用
    std::string referencePath = argv[1];
    std::string testPath = argv[2];

    LOG_INFO("Loading images...");
    LOG_INFO("  Reference: {}", referencePath);
    LOG_INFO("  Test:      {}", testPath);

    // 画像読み込み
    cv::Mat reference = ImageIO::loadImage(referencePath, cv::IMREAD_GRAYSCALE);
    cv::Mat testImage = ImageIO::loadImage(testPath, cv::IMREAD_GRAYSCALE);

    if (reference.empty()) {
        LOG_ERROR("Failed to load reference image: {}", referencePath);
        return 1;
    }

    if (testImage.empty()) {
        LOG_ERROR("Failed to load test image: {}", testPath);
        return 1;
    }

    std::cout << "✓ Images loaded successfully" << std::endl;
    std::cout << "  Reference: " << reference.cols << "x" << reference.rows << std::endl;
    std::cout << "  Test:      " << testImage.cols << "x" << testImage.rows << "\n" << std::endl;

    // TemplateMatcherを作成
    TemplateMatcher detector(30.0, 100.0, 50000.0);
    detector.setConfidenceThreshold(0.3);
    detector.setReferenceImage(reference);

    std::cout << "Running defect detection...\n" << std::endl;

    // 欠陥検出
    Defects defects = detector.detect(testImage);

    // 結果表示
    std::cout << "Detection Results:" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    std::cout << "Total Defects Found: " << defects.size() << "\n" << std::endl;

    for (size_t i = 0; i < defects.size(); ++i) {
        const auto& defect = defects[i];
        std::cout << "Defect #" << (i + 1) << ": "
                  << defect.getTypeString() << " ("
                  << std::fixed << std::setprecision(1) << (defect.confidence * 100.0) << "%)"
                  << " at (" << static_cast<int>(defect.center.x) << ", "
                  << static_cast<int>(defect.center.y) << ")" << std::endl;
    }

    // 可視化
    cv::Mat visualized = DetectorBase::visualizeDefects(testImage, defects, true, true, true);
    ImageIO::saveImage(visualized, "data/output/user_result.jpg");

    std::cout << "\n✓ Result saved to: data/output/user_result.jpg" << std::endl;

    return 0;
}
