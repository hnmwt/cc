#include "server/ExternalTriggerHandler.h"
#include "utils/Logger.h"
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace inspection {

// ========================================
// TriggerMessage パース関数
// ========================================

TriggerMessage parseTriggerMessage(const std::string& rawMessage) {
    TriggerMessage msg;

    try {
        // JSON形式を試す
        auto j = json::parse(rawMessage);

        if (j.contains("command")) {
            msg.command = j["command"].get<std::string>();
        }
        if (j.contains("image_path")) {
            msg.imagePath = j["image_path"].get<std::string>();
        }
        if (j.contains("parameters")) {
            msg.parameters = j["parameters"].dump();
        }
    } catch (...) {
        // JSON以外の場合、スペース区切りテキストとして解析
        std::istringstream iss(rawMessage);
        iss >> msg.command;
        if (iss) {
            iss >> msg.imagePath;
        }
    }

    return msg;
}

std::string triggerMessageToString(const TriggerMessage& message) {
    json j;
    j["command"] = message.command;
    j["image_path"] = message.imagePath;
    j["client_address"] = message.clientAddress;
    j["client_port"] = message.clientPort;

    if (!message.parameters.empty()) {
        try {
            j["parameters"] = json::parse(message.parameters);
        } catch (...) {
            j["parameters"] = message.parameters;
        }
    }

    return j.dump();
}

// ========================================
// ExternalTriggerHandler::Session 実装
// ========================================

ExternalTriggerHandler::Session::Session(tcp::socket socket, ExternalTriggerHandler* handler)
    : socket_(std::move(socket)),
      handler_(handler),
      clientPort_(0)
{
    try {
        auto endpoint = socket_.remote_endpoint();
        clientAddress_ = endpoint.address().to_string();
        clientPort_ = endpoint.port();
    } catch (...) {
        clientAddress_ = "unknown";
        clientPort_ = 0;
    }
}

void ExternalTriggerHandler::Session::start() {
    LOG_INFO("New connection from {}:{}", clientAddress_, clientPort_);
    handler_->activeConnections_++;
    doRead();
}

void ExternalTriggerHandler::Session::doRead() {
    auto self(shared_from_this());

    boost::asio::async_read_until(
        socket_,
        buffer_,
        handler_->delimiter_,
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                // メッセージを取得
                std::istream is(&buffer_);
                std::string message;
                std::getline(is, message);

                // 改行文字を削除
                if (!message.empty() && message.back() == '\r') {
                    message.pop_back();
                }

                LOG_DEBUG("Received message from {}:{}: {}", clientAddress_, clientPort_, message);

                // メッセージを処理
                std::string response = processMessage(message);

                // 応答を送信
                doWrite(response);

                // 次のメッセージを待機
                doRead();
            } else {
                LOG_INFO("Connection closed from {}:{}: {}", clientAddress_, clientPort_, ec.message());
                handler_->activeConnections_--;
            }
        }
    );
}

void ExternalTriggerHandler::Session::doWrite(const std::string& message) {
    auto self(shared_from_this());
    std::string response = message + handler_->delimiter_;

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(response),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                LOG_ERROR("Write error to {}:{}: {}", clientAddress_, clientPort_, ec.message());
            }
        }
    );
}

std::string ExternalTriggerHandler::Session::processMessage(const std::string& message) {
    try {
        // トリガーメッセージをパース
        TriggerMessage triggerMsg = parseTriggerMessage(message);
        triggerMsg.clientAddress = clientAddress_;
        triggerMsg.clientPort = clientPort_;

        handler_->totalTriggers_++;

        // コールバックを呼び出す
        if (handler_->triggerCallback_) {
            std::string response = handler_->triggerCallback_(triggerMsg);
            LOG_INFO("Trigger processed: command={}, response_size={}", triggerMsg.command, response.size());
            return response;
        } else {
            LOG_WARN("No trigger callback set");
            json errorResponse;
            errorResponse["status"] = "error";
            errorResponse["message"] = "No callback configured";
            return errorResponse.dump();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing trigger message: {}", e.what());
        json errorResponse;
        errorResponse["status"] = "error";
        errorResponse["message"] = e.what();
        return errorResponse.dump();
    }
}

// ========================================
// ExternalTriggerHandler 実装
// ========================================

ExternalTriggerHandler::ExternalTriggerHandler(uint16_t port)
    : port_(port),
      running_(false),
      maxConnections_(10),
      timeoutSeconds_(30),
      delimiter_("\n"),
      totalConnections_(0),
      totalTriggers_(0),
      activeConnections_(0),
      failedConnections_(0)
{
}

ExternalTriggerHandler::~ExternalTriggerHandler() {
    stop();
}

bool ExternalTriggerHandler::start() {
    if (running_) {
        LOG_WARN("ExternalTriggerHandler is already running");
        return false;
    }

    try {
        // IOコンテキストとアクセプタを作成
        ioContext_ = std::make_unique<boost::asio::io_context>();
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

        LOG_INFO("ExternalTriggerHandler started on port {}", port_);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start ExternalTriggerHandler: {}", e.what());
        running_ = false;
        return false;
    }
}

void ExternalTriggerHandler::stop() {
    if (!running_) {
        return;
    }

    LOG_INFO("Stopping ExternalTriggerHandler...");
    running_ = false;

    // アクセプタを閉じる
    if (acceptor_ && acceptor_->is_open()) {
        boost::system::error_code ec;
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

    LOG_INFO("ExternalTriggerHandler stopped");
}

void ExternalTriggerHandler::doAccept() {
    acceptor_->async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                totalConnections_++;

                // 最大接続数チェック
                if (activeConnections_ < maxConnections_) {
                    std::make_shared<Session>(std::move(socket), this)->start();
                } else {
                    LOG_WARN("Max connections reached, rejecting new connection");
                    failedConnections_++;
                }
            } else {
                LOG_ERROR("Accept error: {}", ec.message());
                failedConnections_++;
            }

            // 次の接続を待機
            if (running_) {
                doAccept();
            }
        }
    );
}

void ExternalTriggerHandler::runIOContext() {
    LOG_DEBUG("IO context thread started");

    try {
        ioContext_->run();
    } catch (const std::exception& e) {
        LOG_ERROR("IO context error: {}", e.what());
    }

    LOG_DEBUG("IO context thread stopped");
}

ExternalTriggerHandler::Statistics ExternalTriggerHandler::getStatistics() const {
    Statistics stats;
    stats.totalConnections = totalConnections_.load();
    stats.totalTriggers = totalTriggers_.load();
    stats.activeConnections = activeConnections_.load();
    stats.failedConnections = failedConnections_.load();
    return stats;
}

void ExternalTriggerHandler::resetStatistics() {
    totalConnections_ = 0;
    totalTriggers_ = 0;
    failedConnections_ = 0;
    // activeConnections_ はリセットしない（現在の接続数）
}

} // namespace inspection
