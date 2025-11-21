/**
 * @file create_blob_samples.cpp
 * @brief BlobDetectorテスト用のサンプル画像生成プログラム
 *
 * さまざまな欠陥パターンを含むテスト画像を生成します。
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief サンプル1: 傷（Scratch）のテスト画像
 */
cv::Mat createScratchSample() {
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);
    image.setTo(cv::Scalar(220, 220, 220));  // 明るい背景

    // 1. 水平の傷
    cv::line(image, cv::Point(100, 200), cv::Point(500, 205),
             cv::Scalar(30, 30, 30), 3);

    // 2. 斜めの傷
    cv::line(image, cv::Point(600, 150), cv::Point(900, 300),
             cv::Scalar(20, 20, 20), 2);

    // 3. 細かい傷
    cv::line(image, cv::Point(200, 400), cv::Point(400, 410),
             cv::Scalar(40, 40, 40), 1);

    // 4. 長い傷
    cv::line(image, cv::Point(50, 600), cv::Point(1100, 620),
             cv::Scalar(25, 25, 25), 4);

    // 5. Y字型の傷
    cv::line(image, cv::Point(800, 500), cv::Point(900, 550),
             cv::Scalar(30, 30, 30), 2);
    cv::line(image, cv::Point(900, 550), cv::Point(950, 500),
             cv::Scalar(30, 30, 30), 2);

    // ノイズを少し追加
    cv::Mat noise(image.size(), CV_8UC3);
    cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(5, 5, 5));
    cv::add(image, noise, image);

    return image;
}

/**
 * @brief サンプル2: 汚れ・異物（Stain）のテスト画像
 */
cv::Mat createStainSample() {
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);
    image.setTo(cv::Scalar(200, 200, 200));  // 灰色背景

    // 1. 小さな円形汚れ
    cv::circle(image, cv::Point(150, 150), 12, cv::Scalar(50, 50, 50), -1);
    cv::GaussianBlur(image(cv::Rect(138, 138, 24, 24)),
                     image(cv::Rect(138, 138, 24, 24)),
                     cv::Size(5, 5), 2);

    // 2. 中サイズの汚れ
    cv::circle(image, cv::Point(400, 200), 25, cv::Scalar(60, 60, 60), -1);
    cv::GaussianBlur(image(cv::Rect(375, 175, 50, 50)),
                     image(cv::Rect(375, 175, 50, 50)),
                     cv::Size(7, 7), 3);

    // 3. 不定形の汚れ
    std::vector<cv::Point> contour = {
        cv::Point(700, 300), cv::Point(750, 280), cv::Point(800, 320),
        cv::Point(780, 370), cv::Point(720, 360), cv::Point(680, 330)
    };
    cv::fillConvexPoly(image, contour, cv::Scalar(70, 70, 70));
    cv::GaussianBlur(image(cv::Rect(670, 270, 140, 110)),
                     image(cv::Rect(670, 270, 140, 110)),
                     cv::Size(9, 9), 4);

    // 4. 小さな点状汚れの集合
    cv::circle(image, cv::Point(300, 500), 8, cv::Scalar(40, 40, 40), -1);
    cv::circle(image, cv::Point(320, 510), 6, cv::Scalar(45, 45, 45), -1);
    cv::circle(image, cv::Point(310, 530), 7, cv::Scalar(42, 42, 42), -1);

    // 5. 大きな汚れ
    cv::circle(image, cv::Point(900, 600), 50, cv::Scalar(80, 80, 80), -1);
    cv::GaussianBlur(image(cv::Rect(850, 550, 100, 100)),
                     image(cv::Rect(850, 550, 100, 100)),
                     cv::Size(11, 11), 5);

    return image;
}

/**
 * @brief サンプル3: 形状不良（Deformation）のテスト画像
 */
cv::Mat createDeformationSample() {
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);
    image.setTo(cv::Scalar(210, 210, 210));

    // 1. 大きな凹み
    cv::ellipse(image, cv::Point(300, 250), cv::Size(120, 80),
                0, 0, 360, cv::Scalar(100, 100, 100), -1);
    cv::GaussianBlur(image(cv::Rect(180, 170, 240, 160)),
                     image(cv::Rect(180, 170, 240, 160)),
                     cv::Size(15, 15), 6);

    // 2. バリ（突起）
    std::vector<cv::Point> bari = {
        cv::Point(700, 200), cv::Point(750, 190), cv::Point(800, 195),
        cv::Point(820, 220), cv::Point(810, 250), cv::Point(760, 260),
        cv::Point(710, 240)
    };
    cv::fillConvexPoly(image, bari, cv::Scalar(60, 60, 60));

    // 3. 歪み領域
    cv::ellipse(image, cv::Point(500, 500), cv::Size(150, 100),
                30, 0, 360, cv::Scalar(90, 90, 90), -1);
    cv::GaussianBlur(image(cv::Rect(350, 400, 300, 200)),
                     image(cv::Rect(350, 400, 300, 200)),
                     cv::Size(17, 17), 7);

    // 4. 欠け
    std::vector<cv::Point> chip = {
        cv::Point(100, 600), cv::Point(200, 580), cv::Point(250, 650),
        cv::Point(200, 700), cv::Point(120, 680)
    };
    cv::fillConvexPoly(image, chip, cv::Scalar(50, 50, 50));

    return image;
}

/**
 * @brief サンプル4: 混合欠陥のテスト画像
 */
cv::Mat createMixedDefectSample() {
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);
    image.setTo(cv::Scalar(215, 215, 215));

    // 傷
    cv::line(image, cv::Point(100, 100), cv::Point(400, 110),
             cv::Scalar(30, 30, 30), 2);

    // 汚れ
    cv::circle(image, cv::Point(600, 150), 20, cv::Scalar(60, 60, 60), -1);
    cv::GaussianBlur(image(cv::Rect(580, 130, 40, 40)),
                     image(cv::Rect(580, 130, 40, 40)),
                     cv::Size(7, 7), 3);

    // 形状不良
    cv::ellipse(image, cv::Point(300, 400), cv::Size(100, 70),
                0, 0, 360, cv::Scalar(90, 90, 90), -1);
    cv::GaussianBlur(image(cv::Rect(200, 330, 200, 140)),
                     image(cv::Rect(200, 330, 200, 140)),
                     cv::Size(13, 13), 5);

    // 小さな傷
    cv::line(image, cv::Point(800, 200), cv::Point(950, 210),
             cv::Scalar(25, 25, 25), 1);

    // 点状汚れ
    cv::circle(image, cv::Point(500, 600), 10, cv::Scalar(50, 50, 50), -1);
    cv::circle(image, cv::Point(700, 650), 15, cv::Scalar(55, 55, 55), -1);

    // ノイズ
    cv::Mat noise(image.size(), CV_8UC3);
    cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(3, 3, 3));
    cv::add(image, noise, image);

    return image;
}

/**
 * @brief サンプル5: 正常品のテスト画像
 */
cv::Mat createNormalSample() {
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);
    image.setTo(cv::Scalar(210, 210, 210));

    // 軽いノイズのみ（正常範囲）
    cv::Mat noise(image.size(), CV_8UC3);
    cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(2, 2, 2));
    cv::add(image, noise, image);

    // 軽い明度ムラ（正常範囲）
    cv::circle(image, cv::Point(400, 400), 300, cv::Scalar(5, 5, 5), -1);
    cv::GaussianBlur(image, image, cv::Size(51, 51), 20);

    return image;
}

/**
 * @brief サンプル6: 実際の製造現場を想定した画像
 */
cv::Mat createRealisticSample() {
    cv::Mat image = cv::Mat::zeros(800, 1200, CV_8UC3);

    // グラデーション背景（照明ムラを想定）
    for (int y = 0; y < image.rows; ++y) {
        for (int x = 0; x < image.cols; ++x) {
            double dist = std::sqrt(std::pow(x - 600, 2) + std::pow(y - 400, 2));
            int brightness = static_cast<int>(210 - dist / 10);
            brightness = std::max(180, std::min(230, brightness));
            image.at<cv::Vec3b>(y, x) = cv::Vec3b(brightness, brightness, brightness);
        }
    }

    // 微細な傷
    cv::line(image, cv::Point(200, 300), cv::Point(500, 310),
             cv::Scalar(40, 40, 40), 1);

    // 埃
    cv::circle(image, cv::Point(700, 200), 5, cv::Scalar(60, 60, 60), -1);
    cv::circle(image, cv::Point(850, 350), 4, cv::Scalar(55, 55, 55), -1);

    // 指紋跡（薄い汚れ）
    cv::ellipse(image, cv::Point(400, 500), cv::Size(40, 60),
                20, 0, 360, cv::Scalar(190, 190, 190), -1);
    cv::GaussianBlur(image(cv::Rect(360, 440, 80, 120)),
                     image(cv::Rect(360, 440, 80, 120)),
                     cv::Size(9, 9), 4);

    // ノイズ
    cv::Mat noise(image.size(), CV_8UC3);
    cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(4, 4, 4));
    cv::add(image, noise, image);

    return image;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  BlobDetector Sample Image Generator" << std::endl;
    std::cout << "========================================" << std::endl;

    // 出力ディレクトリを作成
    std::string outputDir = "data/input/blob_samples";
    fs::create_directories(outputDir);

    std::cout << "\nGenerating sample images..." << std::endl;

    // サンプル画像を生成
    struct Sample {
        std::string filename;
        cv::Mat image;
        std::string description;
    };

    std::vector<Sample> samples = {
        {"scratch_sample.jpg", createScratchSample(), "Scratch defects (scratches, lines)"},
        {"stain_sample.jpg", createStainSample(), "Stain defects (dirt, foreign matter)"},
        {"deformation_sample.jpg", createDeformationSample(), "Deformation defects (dents, burrs)"},
        {"mixed_defect_sample.jpg", createMixedDefectSample(), "Mixed defects"},
        {"normal_sample.jpg", createNormalSample(), "Normal product (no defects)"},
        {"realistic_sample.jpg", createRealisticSample(), "Realistic manufacturing sample"}
    };

    for (const auto& sample : samples) {
        std::string path = outputDir + "/" + sample.filename;
        cv::imwrite(path, sample.image);
        std::cout << "  ✓ " << sample.filename << " - " << sample.description << std::endl;
    }

    // data/input/sample.jpg も作成（テストプログラム用）
    cv::imwrite("data/input/sample.jpg", createMixedDefectSample());
    std::cout << "  ✓ sample.jpg - Created for test programs" << std::endl;

    std::cout << "\nAll sample images generated successfully!" << std::endl;
    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
