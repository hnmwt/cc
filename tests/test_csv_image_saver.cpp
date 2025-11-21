/**
 * @file test_csv_image_saver.cpp
 * @brief CSVWriterとImageSaverのテストプログラム
 */

#include <iostream>
#include <memory>
#include "io/CSVWriter.h"
#include "io/ImageSaver.h"
#include "io/ImageIO.h"
#include "InspectionController.h"
#include "detectors/TemplateMatcher.h"
#include "detectors/FeatureDetector.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "pipeline/Pipeline.h"
#include "utils/Logger.h"

using namespace inspection;

int main(int argc, char* argv[]) {
    // ロガー初期化
    Logger::init(Logger::Level::Info, true, "logs/test_csv_image_saver.log");

    std::cout << "========================================\n";
    std::cout << "CSVWriter & ImageSaver Test\n";
    std::cout << "========================================\n\n";

    // テスト画像のパスを取得
    std::string testImagePath = "data/input/1346653592-potato-N92z-1920x1200-MM-100.jpg";
    if (argc > 1) {
        testImagePath = argv[1];
    }

    // 画像を読み込む
    ImageIO imageIO;
    cv::Mat testImage = imageIO.loadImage(testImagePath);
    if (testImage.empty()) {
        std::cerr << "Failed to load test image: " << testImagePath << std::endl;
        return 1;
    }

    std::cout << "Test image loaded: " << testImagePath << "\n";
    std::cout << "Image size: " << testImage.cols << "x" << testImage.rows << "\n\n";

    // ===== InspectionControllerのセットアップ =====

    InspectionController controller;

    // パイプライン作成
    auto pipeline = std::make_unique<Pipeline>();
    pipeline->addFilter(std::make_unique<GrayscaleFilter>());
    pipeline->addFilter(std::make_unique<GaussianFilter>(5, 1.0));
    controller.setPipeline(std::move(pipeline));

    // 検出器追加
    controller.addDetector(std::make_unique<FeatureDetector>(
        FeatureDetector::DetectionMode::Adaptive,
        100.0,  // minArea
        50000.0 // maxArea
    ));

    controller.setVisualizationEnabled(true);

    // ===== 検査実行 =====

    std::cout << "Running inspection...\n";
    auto result = controller.inspect(testImage);

    if (!result.success) {
        std::cerr << "Inspection failed: " << result.errorMessage << std::endl;
        return 1;
    }

    std::cout << "Inspection completed!\n";
    std::cout << "  - Defects found: " << result.defects.size() << "\n";
    std::cout << "  - Judgment: " << (result.isOK ? "OK" : "NG") << "\n";
    std::cout << "  - Processing time: " << result.totalTime << " ms\n\n";

    // ===== CSVWriter テスト =====

    std::cout << "========================================\n";
    std::cout << "Testing CSVWriter\n";
    std::cout << "========================================\n\n";

    CSVWriter csvWriter("data/output/csv");
    csvWriter.setFilenamePrefix("test_inspection");
    csvWriter.setDefectDetailsEnabled(true);

    // 1. 単一結果の書き込み
    std::cout << "Test 1: Writing single result...\n";
    bool csvSuccess = csvWriter.writeResult(result, testImagePath);
    if (csvSuccess) {
        std::cout << "✓ CSV file created: " << csvWriter.getLastWrittenFile() << "\n\n";
    } else {
        std::cerr << "✗ Failed to write CSV\n\n";
    }

    // 2. 複数結果の書き込み
    std::cout << "Test 2: Writing multiple results...\n";
    std::vector<InspectionResult> results;
    std::vector<std::string> imagePaths;

    for (int i = 0; i < 3; ++i) {
        auto r = controller.inspect(testImage);
        results.push_back(r);
        imagePaths.push_back(testImagePath);
    }

    csvWriter.setFilenamePrefix("test_batch");
    csvSuccess = csvWriter.writeResults(results, imagePaths);
    if (csvSuccess) {
        std::cout << "✓ Batch CSV file created: " << csvWriter.getLastWrittenFile() << "\n\n";
    } else {
        std::cerr << "✗ Failed to write batch CSV\n\n";
    }

    // 3. 追記テスト
    std::cout << "Test 3: Appending result to existing CSV...\n";
    std::string appendCsvPath = "data/output/csv/test_append.csv";
    csvWriter.createNewCSV(appendCsvPath);

    for (int i = 0; i < 3; ++i) {
        auto r = controller.inspect(testImage);
        csvWriter.appendResult(r, testImagePath, appendCsvPath);
    }
    std::cout << "✓ Results appended to: " << appendCsvPath << "\n\n";

    // ===== ImageSaver テスト =====

    std::cout << "========================================\n";
    std::cout << "Testing ImageSaver\n";
    std::cout << "========================================\n\n";

    ImageSaver imageSaver("data/output/images");
    imageSaver.setFilenamePrefix("test");
    imageSaver.setJpegQuality(95);
    imageSaver.setImageFormat("jpg");
    imageSaver.setCreateSubdirectories(true);

    // 1. すべての画像を保存
    std::cout << "Test 1: Saving all images (original, processed, visualized)...\n";
    bool imageSuccess = imageSaver.saveImages(result, ImageSaver::ImageType::All);
    if (imageSuccess) {
        auto savedFiles = imageSaver.getLastSavedFiles();
        std::cout << "✓ Saved " << savedFiles.size() << " images:\n";
        for (const auto& file : savedFiles) {
            std::cout << "  - " << file << "\n";
        }
        std::cout << "\n";
    } else {
        std::cerr << "✗ Failed to save images\n\n";
    }

    // 2. 元画像のみ保存
    std::cout << "Test 2: Saving original image only...\n";
    imageSaver.setCreateSubdirectories(false);
    std::string originalPath = imageSaver.saveOriginal(result.originalImage);
    if (!originalPath.empty()) {
        std::cout << "✓ Original image saved: " << originalPath << "\n\n";
    } else {
        std::cerr << "✗ Failed to save original image\n\n";
    }

    // 3. 可視化画像のみ保存（PNG形式）
    std::cout << "Test 3: Saving visualized image as PNG...\n";
    imageSaver.setImageFormat("png");
    imageSaver.setPngCompression(5);
    std::string visualizedPath = imageSaver.saveVisualized(result.visualizedImage);
    if (!visualizedPath.empty()) {
        std::cout << "✓ Visualized image saved: " << visualizedPath << "\n\n";
    } else {
        std::cerr << "✗ Failed to save visualized image\n\n";
    }

    // ===== まとめ =====

    std::cout << "========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n\n";
    std::cout << "CSVWriter: " << (csvSuccess ? "✓ PASSED" : "✗ FAILED") << "\n";
    std::cout << "ImageSaver: " << (imageSuccess ? "✓ PASSED" : "✗ FAILED") << "\n\n";

    std::cout << "Output files created in:\n";
    std::cout << "  - CSV: data/output/csv/\n";
    std::cout << "  - Images: data/output/images/\n\n";

    std::cout << "Test completed successfully!\n";

    return 0;
}
