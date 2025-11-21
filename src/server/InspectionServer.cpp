#include "server/InspectionServer.h"
#include "utils/Logger.h"
#include "io/ImageIO.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "filters/ThresholdFilter.h"
#include "detectors/TemplateMatcher.h"
#include "detectors/FeatureDetector.h"
#include "detectors/BlobDetector.h"
#include "detectors/EdgeDetector.h"
#include "pipeline/Pipeline.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace inspection {

InspectionServer::InspectionServer(const std::string& configPath)
    : configPath_(configPath),
      running_(false),
      triggerHandlerEnabled_(true),
      apiServerEnabled_(true),
      triggerPort_(9000),
      apiPort_(8080),
      csvOutputDir_("data/output/csv"),
      imageOutputDir_("data/output/images")
{
}

InspectionServer::~InspectionServer() {
    stop();
}

bool InspectionServer::start() {
    if (running_) {
        LOG_WARN("InspectionServer is already running");
        return false;
    }

    LOG_INFO("Starting InspectionServer...");

    // 設定を読み込む
    if (!loadConfig()) {
        LOG_ERROR("Failed to load configuration");
        return false;
    }

    // InspectionControllerを初期化
    if (!initializeController()) {
        LOG_ERROR("Failed to initialize InspectionController");
        return false;
    }

    // CSVWriterを初期化
    csvWriter_ = std::make_shared<CSVWriter>(csvOutputDir_);
    csvWriter_->createNewCSV(csvOutputDir_ + "/server_results.csv");
    LOG_INFO("CSVWriter initialized: {}", csvOutputDir_);

    // ImageSaverを初期化
    imageSaver_ = std::make_shared<ImageSaver>(imageOutputDir_);
    imageSaver_->setFilenamePrefix("server");
    LOG_INFO("ImageSaver initialized: {}", imageOutputDir_);

    // ExternalTriggerHandlerを初期化
    if (triggerHandlerEnabled_) {
        triggerHandler_ = std::make_shared<ExternalTriggerHandler>(triggerPort_);
        triggerHandler_->setTriggerCallback([this](const TriggerMessage& msg) {
            return handleExternalTrigger(msg);
        });

        if (!triggerHandler_->start()) {
            LOG_ERROR("Failed to start ExternalTriggerHandler");
            return false;
        }
        LOG_INFO("ExternalTriggerHandler started on port {}", triggerPort_);
    }

    // RestApiServerを初期化
    if (apiServerEnabled_) {
        apiServer_ = std::make_shared<RestApiServer>(apiPort_, controller_);
        apiServer_->setCsvWriter(csvWriter_);
        apiServer_->setImageSaver(imageSaver_);
        apiServer_->setAutoSaveEnabled(true);

        if (!apiServer_->start()) {
            LOG_ERROR("Failed to start RestApiServer");
            if (triggerHandler_) {
                triggerHandler_->stop();
            }
            return false;
        }
        LOG_INFO("RestApiServer started on port {}", apiPort_);
    }

    running_ = true;
    LOG_INFO("InspectionServer started successfully");
    LOG_INFO("  - Trigger Port: {}", triggerPort_);
    LOG_INFO("  - API Port: {}", apiPort_);
    LOG_INFO("  - CSV Output: {}", csvOutputDir_);
    LOG_INFO("  - Image Output: {}", imageOutputDir_);

    return true;
}

void InspectionServer::stop() {
    if (!running_) {
        return;
    }

    LOG_INFO("Stopping InspectionServer...");
    running_ = false;

    // 各コンポーネントを停止
    if (triggerHandler_) {
        triggerHandler_->stop();
    }

    if (apiServer_) {
        apiServer_->stop();
    }

    LOG_INFO("InspectionServer stopped");
}

bool InspectionServer::loadConfig() {
    try {
        ConfigManager& config = ConfigManager::getInstance();
        config.loadConfig(configPath_);

        // サーバー設定
        triggerHandlerEnabled_ = config.getValueOr<bool>("/server/trigger_handler/enabled", true);
        triggerPort_ = config.getValueOr<int>("/server/trigger_handler/port", 9000);
        apiServerEnabled_ = config.getValueOr<bool>("/server/rest_api/enabled", true);
        apiPort_ = config.getValueOr<int>("/server/rest_api/port", 8080);

        // データ出力設定
        csvOutputDir_ = config.getValueOr<std::string>("/data_output/csv/directory", "data/output/csv");
        imageOutputDir_ = config.getValueOr<std::string>("/data_output/images/directory", "data/output/images");

        LOG_INFO("Configuration loaded from: {}", configPath_);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load configuration: {}", e.what());
        return false;
    }
}

bool InspectionServer::saveConfig() {
    try {
        ConfigManager& config = ConfigManager::getInstance();

        // 現在の設定を保存
        config.setValue("/server/trigger_handler/enabled", triggerHandlerEnabled_);
        config.setValue("/server/trigger_handler/port", static_cast<int>(triggerPort_));
        config.setValue("/server/rest_api/enabled", apiServerEnabled_);
        config.setValue("/server/rest_api/port", static_cast<int>(apiPort_));
        config.setValue("/data_output/csv/directory", csvOutputDir_);
        config.setValue("/data_output/images/directory", imageOutputDir_);

        config.saveConfig(configPath_);
        LOG_INFO("Configuration saved to: {}", configPath_);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save configuration: {}", e.what());
        return false;
    }
}

bool InspectionServer::initializeController() {
    controller_ = std::make_shared<InspectionController>();

    // パイプラインを構築
    if (!buildPipeline()) {
        return false;
    }

    // 検出器を構築
    if (!buildDetectors()) {
        return false;
    }

    // 判定基準を設定
    controller_->setJudgmentCriteria(0, 0.5);  // 欠陥0個以下、信頼度0.5以上
    controller_->setVisualizationEnabled(true);

    LOG_INFO("InspectionController initialized");
    return true;
}

bool InspectionServer::buildPipeline() {
    try {
        auto pipeline = std::make_unique<Pipeline>();

        ConfigManager& config = ConfigManager::getInstance();

        // 設定からフィルタを追加
        auto filtersOpt = config.getValue<json>("/pipeline/filters");

        if (filtersOpt.has_value()) {
            auto filters = filtersOpt.value();

            for (const auto& filterConfig : filters) {
                std::string type = filterConfig.value("type", "");

                if (type == "grayscale") {
                    pipeline->addFilter(std::make_unique<GrayscaleFilter>());
                    LOG_DEBUG("Added GrayscaleFilter to pipeline");

                } else if (type == "gaussian") {
                    int kernelSize = filterConfig.value("kernel_size", 5);
                    double sigma = filterConfig.value("sigma", 1.0);
                    pipeline->addFilter(std::make_unique<GaussianFilter>(kernelSize, sigma));
                    LOG_DEBUG("Added GaussianFilter to pipeline (kernel={}, sigma={})", kernelSize, sigma);

                } else if (type == "threshold") {
                    double threshold = filterConfig.value("threshold", 127.0);
                    pipeline->addFilter(std::make_unique<ThresholdFilter>(threshold));
                    LOG_DEBUG("Added ThresholdFilter to pipeline (threshold={})", threshold);
                }
            }
        } else {
            // デフォルトのパイプライン
            pipeline->addFilter(std::make_unique<GrayscaleFilter>());
            pipeline->addFilter(std::make_unique<GaussianFilter>(5, 1.0));
            LOG_INFO("Using default pipeline");
        }

        controller_->setPipeline(std::move(pipeline));
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to build pipeline: {}", e.what());
        return false;
    }
}

bool InspectionServer::buildDetectors() {
    try {
        ConfigManager& config = ConfigManager::getInstance();

        // 設定から検出器を追加
        auto detectorsOpt = config.getValue<json>("/detection/detectors");

        if (detectorsOpt.has_value()) {
            auto detectors = detectorsOpt.value();

            for (const auto& detectorConfig : detectors) {
                std::string type = detectorConfig.value("type", "");
                bool enabled = detectorConfig.value("enabled", true);

                if (!enabled) {
                    continue;
                }

                if (type == "template") {
                    double diffThreshold = detectorConfig.value("diff_threshold", 30.0);
                    double minArea = detectorConfig.value("min_area", 100.0);
                    double maxArea = detectorConfig.value("max_area", 50000.0);

                    auto detector = std::make_unique<TemplateMatcher>(diffThreshold, minArea, maxArea);
                    controller_->addDetector(std::move(detector));
                    LOG_DEBUG("Added TemplateMatcher");

                } else if (type == "feature") {
                    double minArea = detectorConfig.value("min_area", 100.0);
                    double maxArea = detectorConfig.value("max_area", 50000.0);

                    auto detector = std::make_unique<FeatureDetector>(
                        FeatureDetector::DetectionMode::Adaptive,
                        minArea,
                        maxArea
                    );
                    controller_->addDetector(std::move(detector));
                    LOG_DEBUG("Added FeatureDetector");

                } else if (type == "blob") {
                    // BlobDetectorのパラメータをJSONから設定
                    auto detector = std::make_unique<BlobDetector>();

                    // パラメータがある場合は設定
                    detector->setParameters(detectorConfig);

                    controller_->addDetector(std::move(detector));
                    LOG_DEBUG("Added BlobDetector");

                } else if (type == "edge") {
                    // EdgeDetectorのパラメータをJSONから設定
                    auto detector = std::make_unique<EdgeDetector>();

                    // パラメータがある場合は設定
                    detector->setParameters(detectorConfig);

                    controller_->addDetector(std::move(detector));
                    LOG_DEBUG("Added EdgeDetector");
                }
            }
        } else {
            // デフォルトの検出器
            controller_->addDetector(std::make_unique<FeatureDetector>(
                FeatureDetector::DetectionMode::Adaptive,
                100.0,
                50000.0
            ));
            LOG_INFO("Using default detector (FeatureDetector)");
        }

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to build detectors: {}", e.what());
        return false;
    }
}

std::string InspectionServer::handleExternalTrigger(const TriggerMessage& message) {
    LOG_INFO("External trigger received: command={}", message.command);

    json response;
    response["status"] = "ok";

    try {
        if (message.command == "INSPECT") {
            // 画像パスを取得
            std::string imagePath = message.imagePath;
            if (imagePath.empty()) {
                response["status"] = "error";
                response["message"] = "image_path is required";
                return response.dump();
            }

            // 画像を読み込む
            ImageIO imageIO;
            cv::Mat image = imageIO.loadImage(imagePath);

            if (image.empty()) {
                response["status"] = "error";
                response["message"] = "Failed to load image: " + imagePath;
                return response.dump();
            }

            // 検査を実行
            auto result = controller_->inspect(image);

            if (!result.success) {
                response["status"] = "error";
                response["message"] = result.errorMessage;
                return response.dump();
            }

            // 結果を保存
            csvWriter_->appendResult(result, imagePath, csvOutputDir_ + "/server_results.csv");
            imageSaver_->saveImages(result, ImageSaver::ImageType::All);

            // レスポンスを作成
            response["result"] = result.toJson();
            LOG_INFO("Inspection completed via trigger: judgment={}, defects={}",
                     result.isOK ? "OK" : "NG", result.defects.size());

        } else if (message.command == "STATUS") {
            auto info = getServerInfo();
            response["server_info"] = {
                {"version", info.version},
                {"running", info.running},
                {"trigger_handler_running", info.triggerHandlerRunning},
                {"api_server_running", info.apiServerRunning},
                {"trigger_port", info.triggerPort},
                {"api_port", info.apiPort}
            };

        } else if (message.command == "STATISTICS") {
            auto stats = getStatistics();
            response["statistics"] = {
                {"total_inspections", stats.totalInspections},
                {"total_defects", stats.totalDefects},
                {"total_ng_count", stats.totalNgCount},
                {"average_processing_time", stats.averageProcessingTime}
            };

        } else {
            response["status"] = "error";
            response["message"] = "Unknown command: " + message.command;
        }

    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = std::string("Exception: ") + e.what();
        LOG_ERROR("Exception in trigger handler: {}", e.what());
    }

    return response.dump();
}

InspectionServer::Statistics InspectionServer::getStatistics() const {
    Statistics stats;

    // コントローラー統計
    if (controller_) {
        auto controllerStats = controller_->getStatistics();
        stats.totalInspections = controllerStats.value("total_inspections", 0);
        stats.totalDefects = controllerStats.value("total_defects_found", 0);
        stats.totalNgCount = controllerStats.value("total_ng_count", 0);
        stats.averageProcessingTime = controllerStats.value("average_processing_time_ms", 0.0);
    }

    // トリガーハンドラー統計
    if (triggerHandler_) {
        auto triggerStats = triggerHandler_->getStatistics();
        stats.triggerTotalConnections = triggerStats.totalConnections;
        stats.triggerTotalTriggers = triggerStats.totalTriggers;
    }

    // APIサーバー統計
    if (apiServer_) {
        auto apiStats = apiServer_->getStatistics();
        stats.apiTotalRequests = apiStats.totalRequests;
        stats.apiTotalInspections = apiStats.totalInspections;
        stats.apiSuccessfulRequests = apiStats.successfulRequests;
        stats.apiFailedRequests = apiStats.failedRequests;
    }

    return stats;
}

void InspectionServer::resetStatistics() {
    if (controller_) {
        controller_->resetStatistics();
    }
    if (triggerHandler_) {
        triggerHandler_->resetStatistics();
    }
    if (apiServer_) {
        apiServer_->resetStatistics();
    }
    LOG_INFO("Statistics reset");
}

InspectionServer::ServerInfo InspectionServer::getServerInfo() const {
    ServerInfo info;
    info.version = "1.0.0";
    info.running = running_;
    info.triggerHandlerRunning = triggerHandler_ && triggerHandler_->isRunning();
    info.apiServerRunning = apiServer_ && apiServer_->isRunning();
    info.triggerPort = triggerPort_;
    info.apiPort = apiPort_;
    info.configPath = configPath_;
    return info;
}

} // namespace inspection
