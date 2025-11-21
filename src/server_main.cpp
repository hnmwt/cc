/**
 * @file server_main.cpp
 * @brief InspectionServer メインプログラム
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include "server/InspectionServer.h"
#include "utils/Logger.h"

using namespace inspection;

// グローバルなサーバーインスタンス
std::unique_ptr<InspectionServer> g_server;
std::atomic<bool> g_shutdown(false);

/**
 * @brief シグナルハンドラー（Ctrl+Cなど）
 */
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received..." << std::endl;
        g_shutdown = true;

        if (g_server) {
            g_server->stop();
        }
    }
}

/**
 * @brief ヘルプメッセージを表示
 */
void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config <path>    Configuration file path (default: config/default_config.json)\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "  -v, --version          Show version information\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "  " << programName << " -c config/production.json\n";
    std::cout << "\n";
}

/**
 * @brief バージョン情報を表示
 */
void printVersion() {
    std::cout << "Inspection Server v1.0.0\n";
    std::cout << "Copyright (c) 2025\n";
}

/**
 * @brief サーバー情報を表示
 */
void printServerInfo(const InspectionServer& server) {
    auto info = server.getServerInfo();

    std::cout << "\n========================================\n";
    std::cout << "Inspection Server " << info.version << "\n";
    std::cout << "========================================\n";
    std::cout << "Status: " << (info.running ? "Running" : "Stopped") << "\n";
    std::cout << "Configuration: " << info.configPath << "\n";
    std::cout << "\n";
    std::cout << "Services:\n";
    std::cout << "  External Trigger: " << (info.triggerHandlerRunning ? "Running" : "Stopped")
              << " (Port: " << info.triggerPort << ")\n";
    std::cout << "  REST API: " << (info.apiServerRunning ? "Running" : "Stopped")
              << " (Port: " << info.apiPort << ")\n";
    std::cout << "\n";
    std::cout << "Endpoints:\n";
    std::cout << "  Trigger:  tcp://localhost:" << info.triggerPort << "\n";
    std::cout << "  REST API: http://localhost:" << info.apiPort << "\n";
    std::cout << "========================================\n";
}

/**
 * @brief 統計情報を定期的に表示
 */
void printStatistics(const InspectionServer& server) {
    auto stats = server.getStatistics();

    std::cout << "\n--- Statistics ---\n";
    std::cout << "Inspections: " << stats.totalInspections
              << " (NG: " << stats.totalNgCount << ")\n";
    std::cout << "Total Defects: " << stats.totalDefects << "\n";
    std::cout << "Avg Processing Time: " << stats.averageProcessingTime << " ms\n";
    std::cout << "Trigger Requests: " << stats.triggerTotalTriggers << "\n";
    std::cout << "API Requests: " << stats.apiTotalRequests
              << " (Success: " << stats.apiSuccessfulRequests
              << ", Failed: " << stats.apiFailedRequests << ")\n";
}

int main(int argc, char* argv[]) {
    // コマンドライン引数の解析
    std::string configPath = "config/default_config.json";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configPath = argv[++i];
            } else {
                std::cerr << "Error: --config requires an argument\n";
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printHelp(argv[0]);
            return 1;
        }
    }

    // ロガーを初期化
    Logger::init(Logger::Level::Info, true, "logs/inspection_server.log");

    // シグナルハンドラーを設定
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "========================================\n";
    std::cout << "Inspection Server\n";
    std::cout << "========================================\n\n";

    // サーバーを作成
    g_server = std::make_unique<InspectionServer>(configPath);

    // サーバーを起動
    std::cout << "Starting server...\n";
    if (!g_server->start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }

    // サーバー情報を表示
    printServerInfo(*g_server);

    std::cout << "\nServer is running. Press Ctrl+C to stop.\n";

    // 統計情報表示タイマー
    auto lastStatsTime = std::chrono::steady_clock::now();
    const auto statsInterval = std::chrono::seconds(30);

    // メインループ
    while (!g_shutdown) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 定期的に統計情報を表示
        auto now = std::chrono::steady_clock::now();
        if (now - lastStatsTime >= statsInterval) {
            printStatistics(*g_server);
            lastStatsTime = now;
        }
    }

    // 最終統計情報を表示
    std::cout << "\n========================================\n";
    std::cout << "Final Statistics\n";
    std::cout << "========================================\n";
    printStatistics(*g_server);

    std::cout << "\nServer stopped.\n";

    return 0;
}
