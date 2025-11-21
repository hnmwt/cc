/**
 * @file test_external_trigger.cpp
 * @brief ExternalTriggerHandlerのテストプログラム
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include "server/ExternalTriggerHandler.h"
#include "InspectionController.h"
#include "io/CSVWriter.h"
#include "io/ImageSaver.h"
#include "io/ImageIO.h"
#include "detectors/FeatureDetector.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "pipeline/Pipeline.h"
#include "utils/Logger.h"
#include <nlohmann/json.hpp>

using namespace inspection;
using json = nlohmann::json;

// グローバルな検査コントローラー
std::unique_ptr<InspectionController> g_controller;
std::unique_ptr<CSVWriter> g_csvWriter;
std::unique_ptr<ImageSaver> g_imageSaver;

/**
 * @brief トリガーコールバック関数
 */
std::string handleTrigger(const TriggerMessage& message) {
    LOG_INFO("=== Trigger Received ===");
    LOG_INFO("Command: {}", message.command);
    LOG_INFO("Image Path: {}", message.imagePath);
    LOG_INFO("Client: {}:{}", message.clientAddress, message.clientPort);

    json response;
    response["status"] = "ok";
    response["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

    try {
        if (message.command == "INSPECT") {
            // 画像パスが指定されている場合
            std::string imagePath = message.imagePath;
            if (imagePath.empty()) {
                imagePath = "data/input/1346653592-potato-N92z-1920x1200-MM-100.jpg";
            }

            // 画像を読み込む
            ImageIO imageIO;
            cv::Mat image = imageIO.loadImage(imagePath);

            if (image.empty()) {
                response["status"] = "error";
                response["message"] = "Failed to load image: " + imagePath;
                LOG_ERROR("Failed to load image: {}", imagePath);
                return response.dump();
            }

            // 検査を実行
            auto result = g_controller->inspect(image);

            if (!result.success) {
                response["status"] = "error";
                response["message"] = result.errorMessage;
                LOG_ERROR("Inspection failed: {}", result.errorMessage);
                return response.dump();
            }

            // 結果を保存
            g_csvWriter->appendResult(result, imagePath, "data/output/csv/trigger_results.csv");
            g_imageSaver->saveImages(result, ImageSaver::ImageType::All);

            // レスポンスを作成
            response["result"] = {
                {"judgment", result.isOK ? "OK" : "NG"},
                {"defect_count", result.defects.size()},
                {"processing_time_ms", result.totalTime},
                {"timestamp", result.timestamp}
            };

            LOG_INFO("Inspection completed: judgment={}, defects={}, time={}ms",
                     result.isOK ? "OK" : "NG", result.defects.size(), result.totalTime);

        } else if (message.command == "STATUS") {
            // ステータス確認
            auto stats = g_controller->getStatistics();
            response["controller_stats"] = stats;
            LOG_INFO("Status request processed");

        } else if (message.command == "STOP") {
            // 停止コマンド
            response["message"] = "Stop command received";
            LOG_INFO("Stop command received");

        } else {
            response["status"] = "error";
            response["message"] = "Unknown command: " + message.command;
            LOG_WARN("Unknown command: {}", message.command);
        }

    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = std::string("Exception: ") + e.what();
        LOG_ERROR("Exception in trigger handler: {}", e.what());
    }

    return response.dump();
}

/**
 * @brief テストクライアント（トリガー送信）
 */
void sendTestTriggers(uint16_t port) {
    using boost::asio::ip::tcp;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    try {
        boost::asio::io_context ioContext;
        tcp::socket socket(ioContext);

        // サーバーに接続
        tcp::resolver resolver(ioContext);
        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
        boost::asio::connect(socket, endpoints);

        std::cout << "\n=== Test Client Connected ===\n";

        // テスト1: INSPECT コマンド
        std::cout << "\nTest 1: Sending INSPECT command...\n";
        {
            json cmd;
            cmd["command"] = "INSPECT";
            cmd["image_path"] = "data/input/1346653592-potato-N92z-1920x1200-MM-100.jpg";
            std::string message = cmd.dump() + "\n";

            boost::asio::write(socket, boost::asio::buffer(message));

            // 応答を受信
            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, "\n");
            std::istream is(&buffer);
            std::string response;
            std::getline(is, response);

            std::cout << "Response: " << response << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // テスト2: STATUS コマンド
        std::cout << "\nTest 2: Sending STATUS command...\n";
        {
            json cmd;
            cmd["command"] = "STATUS";
            std::string message = cmd.dump() + "\n";

            boost::asio::write(socket, boost::asio::buffer(message));

            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, "\n");
            std::istream is(&buffer);
            std::string response;
            std::getline(is, response);

            std::cout << "Response: " << response << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // テスト3: 複数回のINSPECT
        std::cout << "\nTest 3: Sending multiple INSPECT commands...\n";
        for (int i = 0; i < 3; ++i) {
            json cmd;
            cmd["command"] = "INSPECT";
            std::string message = cmd.dump() + "\n";

            boost::asio::write(socket, boost::asio::buffer(message));

            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, "\n");
            std::istream is(&buffer);
            std::string response;
            std::getline(is, response);

            auto resp = json::parse(response);
            std::cout << "  [" << (i + 1) << "] Status: " << resp["status"]
                      << ", Judgment: " << resp["result"]["judgment"] << "\n";

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        socket.close();
        std::cout << "\n=== Test Client Finished ===\n";

    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
    }
}

int main() {
    // ロガー初期化
    Logger::init(Logger::Level::Info, true, "logs/test_external_trigger.log");

    std::cout << "========================================\n";
    std::cout << "ExternalTriggerHandler Test\n";
    std::cout << "========================================\n\n";

    // 検査コントローラーをセットアップ
    g_controller = std::make_unique<InspectionController>();

    auto pipeline = std::make_unique<Pipeline>();
    pipeline->addFilter(std::make_unique<GrayscaleFilter>());
    pipeline->addFilter(std::make_unique<GaussianFilter>(5, 1.0));
    g_controller->setPipeline(std::move(pipeline));

    g_controller->addDetector(std::make_unique<FeatureDetector>(
        FeatureDetector::DetectionMode::Adaptive, 100.0, 50000.0
    ));
    g_controller->setVisualizationEnabled(true);

    // CSV Writer と Image Saver をセットアップ
    g_csvWriter = std::make_unique<CSVWriter>("data/output/csv");
    g_csvWriter->createNewCSV("data/output/csv/trigger_results.csv");

    g_imageSaver = std::make_unique<ImageSaver>("data/output/images");
    g_imageSaver->setFilenamePrefix("trigger");

    std::cout << "Inspection system initialized\n\n";

    // ExternalTriggerHandler を作成
    uint16_t port = 9000;
    ExternalTriggerHandler triggerHandler(port);
    triggerHandler.setTriggerCallback(handleTrigger);

    // サーバーを起動
    std::cout << "Starting trigger handler on port " << port << "...\n";
    if (!triggerHandler.start()) {
        std::cerr << "Failed to start trigger handler\n";
        return 1;
    }

    std::cout << "Trigger handler started successfully!\n";
    std::cout << "Waiting for trigger messages...\n\n";

    // テストクライアントを別スレッドで起動
    std::thread clientThread([port]() {
        sendTestTriggers(port);
    });

    // クライアントスレッドの終了を待つ
    clientThread.join();

    // 少し待機
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 統計情報を表示
    auto stats = triggerHandler.getStatistics();
    std::cout << "\n========================================\n";
    std::cout << "Statistics\n";
    std::cout << "========================================\n";
    std::cout << "Total Connections: " << stats.totalConnections << "\n";
    std::cout << "Total Triggers: " << stats.totalTriggers << "\n";
    std::cout << "Active Connections: " << stats.activeConnections << "\n";
    std::cout << "Failed Connections: " << stats.failedConnections << "\n\n";

    // サーバーを停止
    std::cout << "Stopping trigger handler...\n";
    triggerHandler.stop();

    std::cout << "\nTest completed successfully!\n";
    std::cout << "Check output files:\n";
    std::cout << "  - CSV: data/output/csv/trigger_results.csv\n";
    std::cout << "  - Images: data/output/images/\n";

    return 0;
}
