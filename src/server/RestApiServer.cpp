#include "server/RestApiServer.h"
#include "utils/Logger.h"
#include "io/ImageIO.h"
#include <sstream>

namespace inspection {

// ========================================
// RestApiServer::Session 実装
// ========================================

RestApiServer::Session::Session(tcp::socket socket, RestApiServer* server)
    : socket_(std::move(socket)),
      server_(server)
{
}

void RestApiServer::Session::start() {
    doRead();
}

void RestApiServer::Session::doRead() {
    auto self(shared_from_this());

    // HTTPリクエストを読み込む
    http::async_read(
        socket_,
        buffer_,
        request_,
        [this, self](beast::error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            if (!ec) {
                server_->totalRequests_++;
                processRequest();
            } else if (ec != http::error::end_of_stream) {
                LOG_ERROR("HTTP read error: {}", ec.message());
                server_->failedRequests_++;
            }
        }
    );
}

void RestApiServer::Session::processRequest() {
    auto self(shared_from_this());

    LOG_INFO("HTTP {} {}", request_.method_string(), request_.target());

    try {
        std::string target = std::string(request_.target());

        // ルーティング
        if (request_.method() == http::verb::post && target == "/api/v1/inspect") {
            handleInspectRequest();
        } else if (request_.method() == http::verb::get && target == "/api/v1/status") {
            handleStatusRequest();
        } else if (request_.method() == http::verb::get && target == "/api/v1/statistics") {
            handleStatisticsRequest();
        } else if (request_.method() == http::verb::post && target == "/api/v1/config") {
            handleConfigRequest();
        } else if (request_.method() == http::verb::get && target == "/api/v1/detectors") {
            handleDetectorsRequest();
        } else if (request_.method() == http::verb::get && target == "/") {
            // ルートパス - サーバー情報
            json response;
            response["name"] = "Inspection API Server";
            response["version"] = "1.0.0";
            response["status"] = "running";
            sendResponse(http::status::ok, response.dump());
        } else {
            // 404 Not Found
            json error;
            error["error"] = "Not Found";
            error["path"] = target;
            sendResponse(http::status::not_found, error.dump());
        }

        server_->successfulRequests_++;

    } catch (const std::exception& e) {
        LOG_ERROR("Request processing error: {}", e.what());
        json error;
        error["error"] = "Internal Server Error";
        error["message"] = e.what();
        sendResponse(http::status::internal_server_error, error.dump());
        server_->failedRequests_++;
    }
}

void RestApiServer::Session::handleInspectRequest() {
    try {
        // リクエストボディをパース
        auto requestBody = json::parse(request_.body());

        std::string imagePath;
        if (requestBody.contains("image_path")) {
            imagePath = requestBody["image_path"].get<std::string>();
        } else {
            json error;
            error["error"] = "Bad Request";
            error["message"] = "image_path is required";
            sendResponse(http::status::bad_request, error.dump());
            return;
        }

        // 画像を読み込む
        ImageIO imageIO;
        cv::Mat image = imageIO.loadImage(imagePath);

        if (image.empty()) {
            json error;
            error["error"] = "Bad Request";
            error["message"] = "Failed to load image: " + imagePath;
            sendResponse(http::status::bad_request, error.dump());
            return;
        }

        // 検査を実行
        auto result = server_->controller_->inspect(image);

        if (!result.success) {
            json error;
            error["error"] = "Inspection Failed";
            error["message"] = result.errorMessage;
            sendResponse(http::status::internal_server_error, error.dump());
            return;
        }

        server_->totalInspections_++;

        // 自動保存
        if (server_->autoSave_) {
            if (server_->csvWriter_) {
                server_->csvWriter_->appendResult(result, imagePath, "data/output/csv/api_results.csv");
            }
            if (server_->imageSaver_) {
                server_->imageSaver_->saveImages(result, ImageSaver::ImageType::All);
            }
        }

        // レスポンスを作成
        json response = result.toJson();
        sendResponse(http::status::ok, response.dump());

        LOG_INFO("Inspection completed: judgment={}, defects={}, time={}ms",
                 result.isOK ? "OK" : "NG", result.defects.size(), result.totalTime);

    } catch (const json::exception& e) {
        LOG_ERROR("JSON parse error: {}", e.what());
        json error;
        error["error"] = "Bad Request";
        error["message"] = "Invalid JSON";
        sendResponse(http::status::bad_request, error.dump());
    }
}

void RestApiServer::Session::handleStatusRequest() {
    json response;
    response["status"] = "running";
    response["port"] = server_->port_;
    response["auto_save"] = server_->autoSave_;

    if (server_->controller_) {
        response["controller"] = {
            {"detector_count", server_->controller_->getDetectorCount()},
            {"visualization_enabled", server_->controller_->isVisualizationEnabled()}
        };
    }

    sendResponse(http::status::ok, response.dump());
}

void RestApiServer::Session::handleStatisticsRequest() {
    json response;

    // サーバー統計
    response["server"] = {
        {"total_requests", server_->totalRequests_.load()},
        {"total_inspections", server_->totalInspections_.load()},
        {"successful_requests", server_->successfulRequests_.load()},
        {"failed_requests", server_->failedRequests_.load()}
    };

    // コントローラー統計
    if (server_->controller_) {
        response["controller"] = server_->controller_->getStatistics();
    }

    sendResponse(http::status::ok, response.dump());
}

void RestApiServer::Session::handleConfigRequest() {
    try {
        auto requestBody = json::parse(request_.body());

        // 設定の更新（例: 可視化の有効/無効）
        if (requestBody.contains("visualization_enabled")) {
            bool enabled = requestBody["visualization_enabled"].get<bool>();
            server_->controller_->setVisualizationEnabled(enabled);
        }

        if (requestBody.contains("auto_save")) {
            bool enabled = requestBody["auto_save"].get<bool>();
            server_->autoSave_ = enabled;
        }

        json response;
        response["status"] = "ok";
        response["message"] = "Configuration updated";
        sendResponse(http::status::ok, response.dump());

    } catch (const json::exception& e) {
        json error;
        error["error"] = "Bad Request";
        error["message"] = "Invalid JSON";
        sendResponse(http::status::bad_request, error.dump());
    }
}

void RestApiServer::Session::handleDetectorsRequest() {
    json response = json::array();

    if (server_->controller_) {
        for (size_t i = 0; i < server_->controller_->getDetectorCount(); ++i) {
            auto detector = server_->controller_->getDetector(i);
            if (detector) {
                json detectorInfo;
                detectorInfo["index"] = i;
                detectorInfo["name"] = detector->getName();
                detectorInfo["type"] = detector->getType();
                detectorInfo["enabled"] = detector->isEnabled();
                detectorInfo["confidence_threshold"] = detector->getConfidenceThreshold();
                response.push_back(detectorInfo);
            }
        }
    }

    sendResponse(http::status::ok, response.dump());
}

void RestApiServer::Session::sendResponse(
    http::status status,
    const std::string& body,
    const std::string& contentType
) {
    auto self(shared_from_this());

    response_.version(request_.version());
    response_.result(status);
    response_.set(http::field::content_type, contentType);
    response_.body() = body;
    response_.prepare_payload();

    // CORSヘッダーを追加
    server_->addCorsHeaders(response_);

    // レスポンスを送信
    http::async_write(
        socket_,
        response_,
        [this, self](beast::error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);
            socket_.shutdown(tcp::socket::shutdown_send, ec);
        }
    );
}

// ========================================
// RestApiServer 実装
// ========================================

RestApiServer::RestApiServer(
    uint16_t port,
    std::shared_ptr<InspectionController> controller
)
    : port_(port),
      running_(false),
      controller_(controller),
      autoSave_(true),
      totalRequests_(0),
      totalInspections_(0),
      successfulRequests_(0),
      failedRequests_(0)
{
}

RestApiServer::~RestApiServer() {
    stop();
}

bool RestApiServer::start() {
    if (running_) {
        LOG_WARN("RestApiServer is already running");
        return false;
    }

    try {
        // IOコンテキストとアクセプタを作成
        ioContext_ = std::make_unique<net::io_context>();
        acceptor_ = std::make_unique<tcp::acceptor>(
            *ioContext_,
            tcp::endpoint(tcp::v4(), port_)
        );

        running_ = true;

        // 接続の受付を開始
        doAccept();

        // IOコンテキストを別スレッドで実行
        ioThread_ = std::make_unique<std::thread>([this]() {
            runIOContext();
        });

        LOG_INFO("RestApiServer started on port {}", port_);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start RestApiServer: {}", e.what());
        running_ = false;
        return false;
    }
}

void RestApiServer::stop() {
    if (!running_) {
        return;
    }

    LOG_INFO("Stopping RestApiServer...");
    running_ = false;

    // アクセプタを閉じる
    if (acceptor_ && acceptor_->is_open()) {
        beast::error_code ec;
        acceptor_->close(ec);
    }

    // IOコンテキストを停止
    if (ioContext_) {
        ioContext_->stop();
    }

    // スレッドの終了を待つ
    if (ioThread_ && ioThread_->joinable()) {
        ioThread_->join();
    }

    LOG_INFO("RestApiServer stopped");
}

void RestApiServer::doAccept() {
    acceptor_->async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), this)->start();
            } else {
                LOG_ERROR("Accept error: {}", ec.message());
                failedRequests_++;
            }

            // 次の接続を待機
            if (running_) {
                doAccept();
            }
        }
    );
}

void RestApiServer::runIOContext() {
    LOG_DEBUG("IO context thread started");

    try {
        ioContext_->run();
    } catch (const std::exception& e) {
        LOG_ERROR("IO context error: {}", e.what());
    }

    LOG_DEBUG("IO context thread stopped");
}

void RestApiServer::addCorsHeaders(http::response<http::string_body>& response) {
    response.set(http::field::access_control_allow_origin, "*");
    response.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    response.set(http::field::access_control_allow_headers, "Content-Type");
}

RestApiServer::Statistics RestApiServer::getStatistics() const {
    Statistics stats;
    stats.totalRequests = totalRequests_.load();
    stats.totalInspections = totalInspections_.load();
    stats.successfulRequests = successfulRequests_.load();
    stats.failedRequests = failedRequests_.load();
    return stats;
}

void RestApiServer::resetStatistics() {
    totalRequests_ = 0;
    totalInspections_ = 0;
    successfulRequests_ = 0;
    failedRequests_ = 0;
}

} // namespace inspection
