/**
 * @file test_rest_api.cpp
 * @brief RestApiServerのテストプログラム
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include "server/RestApiServer.h"
#include "InspectionController.h"
#include "io/CSVWriter.h"
#include "io/ImageSaver.h"
#include "detectors/FeatureDetector.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "pipeline/Pipeline.h"
#include "utils/Logger.h"
#include <nlohmann/json.hpp>

using namespace inspection;
using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/**
 * @brief HTTPリクエストを送信
 */
std::string sendHttpRequest(
    const std::string& host,
    const std::string& port,
    http::verb method,
    const std::string& target,
    const std::string& body = ""
) {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // サーバーに接続
        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        // リクエストを作成
        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "InspectionTest/1.0");
        req.set(http::field::content_type, "application/json");

        if (!body.empty()) {
            req.body() = body;
            req.prepare_payload();
        }

        // リクエストを送信
        http::write(stream, req);

        // レスポンスを受信
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        // 接続を閉じる
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.body();

    } catch (const std::exception& e) {
        std::cerr << "HTTP request error: " << e.what() << std::endl;
        return "";
    }
}

int main() {
    // ロガー初期化
    Logger::init(Logger::Level::Info, true, "logs/test_rest_api.log");

    std::cout << "========================================\n";
    std::cout << "REST API Server Test\n";
    std::cout << "========================================\n\n";

    // 検査コントローラーをセットアップ
    auto controller = std::make_shared<InspectionController>();

    auto pipeline = std::make_unique<Pipeline>();
    pipeline->addFilter(std::make_unique<GrayscaleFilter>());
    pipeline->addFilter(std::make_unique<GaussianFilter>(5, 1.0));
    controller->setPipeline(std::move(pipeline));

    controller->addDetector(std::make_unique<FeatureDetector>(
        FeatureDetector::DetectionMode::Adaptive, 100.0, 50000.0
    ));
    controller->setVisualizationEnabled(true);

    // CSV Writer と Image Saver をセットアップ
    auto csvWriter = std::make_shared<CSVWriter>("data/output/csv");
    csvWriter->createNewCSV("data/output/csv/api_results.csv");

    auto imageSaver = std::make_shared<ImageSaver>("data/output/images");
    imageSaver->setFilenamePrefix("api");

    std::cout << "Inspection system initialized\n\n";

    // REST APIサーバーを作成
    uint16_t port = 8080;
    RestApiServer apiServer(port, controller);
    apiServer.setCsvWriter(csvWriter);
    apiServer.setImageSaver(imageSaver);
    apiServer.setAutoSaveEnabled(true);

    // サーバーを起動
    std::cout << "Starting REST API server on port " << port << "...\n";
    if (!apiServer.start()) {
        std::cerr << "Failed to start REST API server\n";
        return 1;
    }

    std::cout << "REST API server started successfully!\n";
    std::cout << "API Endpoints:\n";
    std::cout << "  GET  http://localhost:" << port << "/\n";
    std::cout << "  POST http://localhost:" << port << "/api/v1/inspect\n";
    std::cout << "  GET  http://localhost:" << port << "/api/v1/status\n";
    std::cout << "  GET  http://localhost:" << port << "/api/v1/statistics\n";
    std::cout << "  GET  http://localhost:" << port << "/api/v1/detectors\n";
    std::cout << "  POST http://localhost:" << port << "/api/v1/config\n\n";

    // 少し待機
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string host = "127.0.0.1";
    std::string portStr = std::to_string(port);

    // ===== テスト開始 =====

    // Test 1: ルートパス
    std::cout << "Test 1: GET /\n";
    {
        std::string response = sendHttpRequest(host, portStr, http::verb::get, "/");
        std::cout << "Response: " << response << "\n\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Test 2: ステータス確認
    std::cout << "Test 2: GET /api/v1/status\n";
    {
        std::string response = sendHttpRequest(host, portStr, http::verb::get, "/api/v1/status");
        std::cout << "Response: " << response << "\n\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Test 3: 検出器一覧
    std::cout << "Test 3: GET /api/v1/detectors\n";
    {
        std::string response = sendHttpRequest(host, portStr, http::verb::get, "/api/v1/detectors");
        auto j = json::parse(response);
        std::cout << "Detectors: " << j.dump(2) << "\n\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Test 4: 検査実行
    std::cout << "Test 4: POST /api/v1/inspect\n";
    {
        json requestBody;
        requestBody["image_path"] = "data/input/1346653592-potato-N92z-1920x1200-MM-100.jpg";

        std::string response = sendHttpRequest(
            host, portStr, http::verb::post, "/api/v1/inspect", requestBody.dump()
        );

        auto j = json::parse(response);
        std::cout << "Inspection Result:\n";
        std::cout << "  Success: " << j["success"] << "\n";
        std::cout << "  Judgment: " << (j["isOK"].get<bool>() ? "OK" : "NG") << "\n";
        std::cout << "  Defects: " << j["defectCount"] << "\n";
        std::cout << "  Processing Time: " << j["totalTime"] << " ms\n\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Test 5: 複数回の検査
    std::cout << "Test 5: Multiple inspections\n";
    for (int i = 0; i < 3; ++i) {
        json requestBody;
        requestBody["image_path"] = "data/input/1346653592-potato-N92z-1920x1200-MM-100.jpg";

        std::string response = sendHttpRequest(
            host, portStr, http::verb::post, "/api/v1/inspect", requestBody.dump()
        );

        auto j = json::parse(response);
        std::cout << "  [" << (i + 1) << "] Judgment: " << (j["isOK"].get<bool>() ? "OK" : "NG")
                  << ", Defects: " << j["defectCount"]
                  << ", Time: " << j["totalTime"] << " ms\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Test 6: 設定変更
    std::cout << "Test 6: POST /api/v1/config\n";
    {
        json requestBody;
        requestBody["visualization_enabled"] = false;
        requestBody["auto_save"] = false;

        std::string response = sendHttpRequest(
            host, portStr, http::verb::post, "/api/v1/config", requestBody.dump()
        );
        std::cout << "Response: " << response << "\n\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Test 7: 統計情報
    std::cout << "Test 7: GET /api/v1/statistics\n";
    {
        std::string response = sendHttpRequest(host, portStr, http::verb::get, "/api/v1/statistics");
        auto j = json::parse(response);
        std::cout << "Statistics:\n";
        std::cout << j.dump(2) << "\n\n";
    }

    // 少し待機
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 統計情報を表示
    auto stats = apiServer.getStatistics();
    std::cout << "========================================\n";
    std::cout << "Server Statistics\n";
    std::cout << "========================================\n";
    std::cout << "Total Requests: " << stats.totalRequests << "\n";
    std::cout << "Total Inspections: " << stats.totalInspections << "\n";
    std::cout << "Successful Requests: " << stats.successfulRequests << "\n";
    std::cout << "Failed Requests: " << stats.failedRequests << "\n\n";

    // サーバーを停止
    std::cout << "Stopping REST API server...\n";
    apiServer.stop();

    std::cout << "\nTest completed successfully!\n";
    std::cout << "Check output files:\n";
    std::cout << "  - CSV: data/output/csv/api_results.csv\n";
    std::cout << "  - Images: data/output/images/\n";

    return 0;
}
