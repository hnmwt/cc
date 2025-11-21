#include "utils/Logger.h"
#include "utils/ConfigManager.h"
#include "io/ImageIO.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "pipeline/Pipeline.h"
#include <iostream>

using namespace inspection;

int main(int argc, char** argv) {
    // ロガーを初期化
    Logger::init(Logger::Level::Info, true, "logs/inspection.log");

    LOG_INFO("===========================================");
    LOG_INFO("Inspection Application Starting...");
    LOG_INFO("===========================================");

    // 設定ファイルを読み込み
    auto& config = ConfigManager::getInstance();
    if (!config.loadConfig("config/default_config.json")) {
        LOG_WARN("Failed to load config, using defaults");
    }

    // アプリケーション名とバージョンを表示
    auto appName = config.getValueOr<std::string>("/application/name", "InspectionApp");
    auto appVersion = config.getValueOr<std::string>("/application/version", "1.0.0");

    LOG_INFO("Application: {} v{}", appName, appVersion);

    // パイプラインのテスト
    LOG_INFO("Creating image processing pipeline...");
    Pipeline pipeline;

    // フィルタを追加
    pipeline.addFilter(std::make_unique<GrayscaleFilter>());
    pipeline.addFilter(std::make_unique<GaussianFilter>(5, 1.5));

    LOG_INFO("Pipeline created with {} filters", pipeline.getFilterCount());

    // フィルタ名を表示
    auto filterNames = pipeline.getFilterNames();
    for (size_t i = 0; i < filterNames.size(); ++i) {
        LOG_INFO("  Filter {}: {}", i + 1, filterNames[i]);
    }

    LOG_INFO("===========================================");
    LOG_INFO("Initialization complete");
    LOG_INFO("===========================================");

    // 画像処理のテスト (画像ファイルがある場合)
    if (argc > 1) {
        std::string imagePath = argv[1];
        LOG_INFO("Loading test image: {}", imagePath);

        cv::Mat image = ImageIO::loadImage(imagePath);
        if (!image.empty()) {
            LOG_INFO("Image loaded: {}x{}, {} channels",
                     image.cols, image.rows, image.channels());

            // パイプライン処理
            LOG_INFO("Processing image through pipeline...");
            auto result = pipeline.processWithIntermediates(image);

            if (result.success) {
                LOG_INFO("Processing successful!");
                LOG_INFO("Total processing time: {:.2f} ms", result.totalTime);

                for (size_t i = 0; i < result.filterNames.size(); ++i) {
                    LOG_INFO("  {}: {:.2f} ms",
                             result.filterNames[i],
                             result.processingTimes[i]);
                }

                // 結果を保存
                std::string outputPath = "data/output/processed_result.jpg";
                if (ImageIO::saveImage(result.finalImage, outputPath)) {
                    LOG_INFO("Result saved to: {}", outputPath);
                }
            } else {
                LOG_ERROR("Processing failed: {}", result.errorMessage);
            }
        } else {
            LOG_ERROR("Failed to load image");
        }
    } else {
        LOG_INFO("No input image specified. Usage: {} <image_path>", argv[0]);
        LOG_INFO("Example: {} data/input/sample.jpg", argv[0]);
    }

    LOG_INFO("Application shutting down...");
    Logger::shutdown();

    return 0;
}
