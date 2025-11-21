#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <string>
#include <memory>

#include "InspectionController.h"
#include "detectors/TemplateMatcher.h"
#include "detectors/FeatureDetector.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "filters/ThresholdFilter.h"
#include "pipeline/Pipeline.h"
#include "io/ImageIO.h"

using namespace inspection;

/**
 * @brief 簡易検査UIアプリケーション
 */
class InspectionUI {
public:
    InspectionUI() {
        // ウィンドウ作成
        cv::namedWindow("Original Image", cv::WINDOW_NORMAL);
        cv::namedWindow("Processed Image", cv::WINDOW_NORMAL);
        cv::namedWindow("Detection Result", cv::WINDOW_NORMAL);
        cv::namedWindow("Control Panel", cv::WINDOW_NORMAL);
        cv::namedWindow("Algorithm Info", cv::WINDOW_NORMAL);

        // コントロールパネルのサイズ調整
        cv::resizeWindow("Control Panel", 500, 600);
        cv::resizeWindow("Algorithm Info", 600, 400);

        // トラックバー用変数の初期化
        diffThreshold_ = 30;
        minArea_ = 100;
        maxArea_ = 10000;
        confidenceThreshold_ = 30;  // 0-100 (実際は0.0-1.0)
        blurKernelSize_ = 2;  // 実際は blurKernelSize_ * 2 + 1
        detectorType_ = 0;  // 0: Template, 1: Feature

        // トラックバー作成
        cv::createTrackbar("Algorithm\n0:Template Match 1:Feature",
                          "Control Panel", &detectorType_, 1,
                          onTrackbarChange, this);
        cv::createTrackbar("Diff Threshold", "Control Panel",
                          &diffThreshold_, 100,
                          onTrackbarChange, this);
        cv::createTrackbar("Min Area", "Control Panel",
                          &minArea_, 1000,
                          onTrackbarChange, this);
        cv::createTrackbar("Max Area / 10", "Control Panel",
                          &maxArea_, 10000,
                          onTrackbarChange, this);
        cv::createTrackbar("Confidence x100", "Control Panel",
                          &confidenceThreshold_, 100,
                          onTrackbarChange, this);
        cv::createTrackbar("Blur Kernel\n(size=val*2+1)", "Control Panel",
                          &blurKernelSize_, 10,
                          onTrackbarChange, this);

        // InspectionController初期化
        controller_ = std::make_unique<InspectionController>();
        updateDetector();
    }

    void run(const std::string& imagePath, const std::string& referencePath = "") {
        // 画像読み込み
        testImage_ = ImageIO::loadImage(imagePath);
        if (testImage_.empty()) {
            std::cerr << "Failed to load image: " << imagePath << std::endl;
            return;
        }

        // リファレンス画像読み込み（TemplateMatcherの場合）
        if (!referencePath.empty()) {
            referenceImage_ = ImageIO::loadImage(referencePath);
        } else {
            // リファレンスがない場合は、テスト画像をコピー
            referenceImage_ = testImage_.clone();
        }

        std::cout << "=== Inspection UI ===" << std::endl;
        std::cout << "Image loaded: " << testImage_.cols << "x" << testImage_.rows << std::endl;
        std::cout << "\nControls:" << std::endl;
        std::cout << "  ESC    - Exit" << std::endl;
        std::cout << "  SPACE  - Run inspection" << std::endl;
        std::cout << "  s      - Save result" << std::endl;
        std::cout << "  r      - Reset parameters" << std::endl;
        std::cout << std::endl;

        // アルゴリズム情報表示
        updateAlgorithmInfo();

        // 初回検査実行
        runInspection();

        // メインループ
        while (true) {
            int key = cv::waitKey(30);

            if (key == 27) {  // ESC
                break;
            } else if (key == ' ') {  // SPACE
                runInspection();
            } else if (key == 's' || key == 'S') {  // Save
                saveResult();
            } else if (key == 'r' || key == 'R') {  // Reset
                resetParameters();
            }
        }

        cv::destroyAllWindows();
    }

private:
    static void onTrackbarChange(int, void* userData) {
        InspectionUI* ui = static_cast<InspectionUI*>(userData);
        ui->updateDetector();
        ui->updateAlgorithmInfo();
        ui->runInspection();
    }

    void updateAlgorithmInfo() {
        // アルゴリズム情報パネルを作成
        cv::Mat infoPanel = cv::Mat::zeros(400, 600, CV_8UC3);
        infoPanel.setTo(cv::Scalar(40, 40, 40));  // 暗いグレー背景

        int y = 30;
        int lineHeight = 25;

        // タイトル
        cv::putText(infoPanel, "=== ACTIVE ALGORITHM ===",
                   cv::Point(20, y), cv::FONT_HERSHEY_DUPLEX, 0.7,
                   cv::Scalar(0, 255, 255), 2);
        y += lineHeight * 1.5;

        if (detectorType_ == 0) {
            // TemplateMatcher の説明
            cv::putText(infoPanel, "Algorithm: Template Matcher",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                       cv::Scalar(100, 255, 100), 2);
            y += lineHeight;

            cv::putText(infoPanel, "Description:",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(200, 200, 200), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  Compares test image with reference",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(180, 180, 180), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  image to detect differences.",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(180, 180, 180), 1);
            y += lineHeight * 1.5;

            cv::putText(infoPanel, "Detecting:",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(200, 200, 200), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Scratches (linear defects)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 0, 255), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Stains (circular defects)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 165, 255), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Discoloration (brightness diff)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 255, 255), 1);
            y += lineHeight * 1.5;

            cv::putText(infoPanel, "Active Parameters:",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(200, 200, 200), 1);
            y += lineHeight;

            std::string param1 = "  Diff Threshold: " + std::to_string(diffThreshold_) +
                               " (sensitivity to differences)";
            cv::putText(infoPanel, param1,
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       cv::Scalar(150, 150, 255), 1);
            y += lineHeight;

            std::string param2 = "  Min Area: " + std::to_string(minArea_) +
                               " px (ignore small noise)";
            cv::putText(infoPanel, param2,
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       cv::Scalar(150, 150, 255), 1);
            y += lineHeight;

            std::string param3 = "  Max Area: " + std::to_string(maxArea_ * 10) +
                               " px (ignore large areas)";
            cv::putText(infoPanel, param3,
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       cv::Scalar(150, 150, 255), 1);
            y += lineHeight;

            int kernelSize = blurKernelSize_ * 2 + 1;
            std::string param4 = "  Blur Kernel: " + std::to_string(kernelSize) +
                               "x" + std::to_string(kernelSize) + " (noise reduction)";
            cv::putText(infoPanel, param4,
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       cv::Scalar(150, 150, 255), 1);
            y += lineHeight;

        } else {
            // FeatureDetector の説明
            cv::putText(infoPanel, "Algorithm: Feature Detector",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                       cv::Scalar(100, 255, 100), 2);
            y += lineHeight;

            cv::putText(infoPanel, "Description:",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(200, 200, 200), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  Analyzes image features without",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(180, 180, 180), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  reference (adaptive thresholding).",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(180, 180, 180), 1);
            y += lineHeight * 1.5;

            cv::putText(infoPanel, "Detecting:",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(200, 200, 200), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Scratches (aspect ratio > 4)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 0, 255), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Stains (circularity > 0.85)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 165, 255), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Discoloration (low intensity)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 255, 255), 1);
            y += lineHeight;

            cv::putText(infoPanel, "  - Deformation (low circularity)",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(255, 0, 255), 1);
            y += lineHeight * 1.5;

            cv::putText(infoPanel, "Active Parameters:",
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(200, 200, 200), 1);
            y += lineHeight;

            std::string param1 = "  Min Area: " + std::to_string(minArea_) +
                               " px (minimum defect size)";
            cv::putText(infoPanel, param1,
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       cv::Scalar(150, 150, 255), 1);
            y += lineHeight;

            std::string param2 = "  Max Area: " + std::to_string(maxArea_ * 10) +
                               " px (maximum defect size)";
            cv::putText(infoPanel, param2,
                       cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                       cv::Scalar(150, 150, 255), 1);
            y += lineHeight;
        }

        // 共通パラメータ
        y += lineHeight * 0.5;
        std::string confidenceParam = "  Confidence Threshold: " +
                                     std::to_string(confidenceThreshold_) +
                                     "% (min to report)";
        cv::putText(infoPanel, confidenceParam,
                   cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                   cv::Scalar(150, 150, 255), 1);

        cv::imshow("Algorithm Info", infoPanel);
    }

    void updateDetector() {
        controller_->clearDetectors();

        if (detectorType_ == 0) {
            // TemplateMatcher
            auto detector = std::make_unique<TemplateMatcher>();
            detector->setDiffThreshold(static_cast<double>(diffThreshold_));
            detector->setMinDefectArea(static_cast<double>(minArea_));
            detector->setMaxDefectArea(static_cast<double>(maxArea_ * 10));
            detector->setConfidenceThreshold(confidenceThreshold_ / 100.0);
            detector->setBlurKernelSize(blurKernelSize_ * 2 + 1);

            if (!referenceImage_.empty()) {
                detector->setReferenceImage(referenceImage_);
            }

            controller_->addDetector(std::move(detector));
        } else {
            // FeatureDetector
            auto detector = std::make_unique<FeatureDetector>(
                FeatureDetector::DetectionMode::Adaptive,
                static_cast<double>(minArea_),
                static_cast<double>(maxArea_ * 10)
            );
            detector->setConfidenceThreshold(confidenceThreshold_ / 100.0);

            controller_->addDetector(std::move(detector));
        }

        // 判定基準を設定
        controller_->setJudgmentCriteria(0, confidenceThreshold_ / 100.0);
    }

    void runInspection() {
        if (testImage_.empty()) {
            return;
        }

        // 検査実行
        auto result = controller_->inspect(testImage_);

        // 元画像表示
        cv::imshow("Original Image", testImage_);

        // 処理済み画像表示
        if (!result.processedImage.empty()) {
            cv::imshow("Processed Image", result.processedImage);
        }

        // 検出結果表示
        if (!result.visualizedImage.empty()) {
            cv::Mat display = result.visualizedImage.clone();

            // 情報をオーバーレイ
            std::string info = "Defects: " + std::to_string(result.defects.size());
            std::string judgment = result.isOK ? "OK" : "NG";
            std::string timeInfo = "Time: " +
                std::to_string(static_cast<int>(result.totalTime)) + " ms";

            cv::putText(display, info, cv::Point(20, 60),
                       cv::FONT_HERSHEY_SIMPLEX, 0.7,
                       cv::Scalar(255, 255, 255), 2);

            cv::Scalar judgmentColor = result.isOK ?
                cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
            cv::putText(display, judgment, cv::Point(20, 100),
                       cv::FONT_HERSHEY_SIMPLEX, 1.0,
                       judgmentColor, 3);

            cv::putText(display, timeInfo, cv::Point(20, 140),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6,
                       cv::Scalar(255, 255, 255), 2);

            cv::imshow("Detection Result", display);
            lastResult_ = result;
        }

        // コンソールに結果表示
        std::cout << "\n--- Inspection Result ---" << std::endl;
        std::cout << "Detected " << result.defects.size() << " defects" << std::endl;
        std::cout << "Judgment: " << (result.isOK ? "OK" : "NG") << std::endl;
        std::cout << "Processing time: " << result.totalTime << " ms" << std::endl;
    }

    void saveResult() {
        if (lastResult_.visualizedImage.empty()) {
            std::cout << "No result to save" << std::endl;
            return;
        }

        std::string filename = "data/output/ui_result_" +
            std::to_string(std::time(nullptr)) + ".jpg";

        if (ImageIO::saveImage(lastResult_.visualizedImage, filename)) {
            std::cout << "Result saved to: " << filename << std::endl;
        } else {
            std::cout << "Failed to save result" << std::endl;
        }
    }

    void resetParameters() {
        diffThreshold_ = 30;
        minArea_ = 100;
        maxArea_ = 1000;
        confidenceThreshold_ = 30;
        blurKernelSize_ = 2;
        detectorType_ = 0;

        cv::setTrackbarPos("Algorithm\n0:Template Match 1:Feature",
                          "Control Panel", detectorType_);
        cv::setTrackbarPos("Diff Threshold", "Control Panel", diffThreshold_);
        cv::setTrackbarPos("Min Area", "Control Panel", minArea_);
        cv::setTrackbarPos("Max Area / 10", "Control Panel", maxArea_);
        cv::setTrackbarPos("Confidence x100", "Control Panel", confidenceThreshold_);
        cv::setTrackbarPos("Blur Kernel\n(size=val*2+1)", "Control Panel", blurKernelSize_);

        updateDetector();
        updateAlgorithmInfo();
        runInspection();

        std::cout << "Parameters reset to defaults" << std::endl;
    }

    cv::Mat testImage_;
    cv::Mat referenceImage_;
    InspectionResult lastResult_;

    std::unique_ptr<InspectionController> controller_;

    // トラックバー変数
    int diffThreshold_;
    int minArea_;
    int maxArea_;
    int confidenceThreshold_;
    int blurKernelSize_;
    int detectorType_;
};

/**
 * @brief メイン関数
 */
int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Inspection UI Application" << std::endl;
    std::cout << "========================================\n" << std::endl;

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_image> [reference_image]" << std::endl;
        std::cout << "\nExample:" << std::endl;
        std::cout << "  " << argv[0] << " data/input/test.jpg" << std::endl;
        std::cout << "  " << argv[0] << " data/input/test.jpg data/input/reference.jpg" << std::endl;
        return 1;
    }

    std::string testImagePath = argv[1];
    std::string referenceImagePath = (argc >= 3) ? argv[2] : "";

    InspectionUI ui;
    ui.run(testImagePath, referenceImagePath);

    return 0;
}
