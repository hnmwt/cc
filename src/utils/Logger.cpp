#include "utils/Logger.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace inspection {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;

void Logger::init(Level logLevel,
                 bool logToFile,
                 const std::string& logFilePath,
                 size_t maxFileSize,
                 size_t maxFiles) {
    try {
        std::vector<spdlog::sink_ptr> sinks;

        // コンソール出力 (色付き)
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
        sinks.push_back(consoleSink);

        // ファイル出力 (ローテーション付き)
        if (logToFile) {
            // ログディレクトリが存在しない場合は作成
            fs::path logPath(logFilePath);
            if (logPath.has_parent_path()) {
                fs::path logDir = logPath.parent_path();
                if (!fs::exists(logDir)) {
                    fs::create_directories(logDir);
                    std::cout << "[Logger] Created log directory: " << logDir << std::endl;
                }
            }

            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logFilePath, maxFileSize, maxFiles);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
            sinks.push_back(fileSink);
        }

        // マルチシンクロガーを作成
        logger_ = std::make_shared<spdlog::logger>("InspectionApp", sinks.begin(), sinks.end());
        logger_->set_level(toSpdlogLevel(logLevel));

        // グローバルロガーとして登録
        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);

        // 即座にフラッシュ（エラー時）
        logger_->flush_on(spdlog::level::err);

        LOG_INFO("Logger initialized successfully");
        LOG_INFO("Log level: {}", spdlog::level::to_string_view(toSpdlogLevel(logLevel)));
        if (logToFile) {
            LOG_INFO("Logging to file: {}", logFilePath);
        }

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "[Logger] Error: Failed to initialize logger: " << ex.what() << std::endl;

        // フォールバック: 最小限のコンソールロガー
        logger_ = spdlog::stdout_color_mt("InspectionApp");
        logger_->set_level(toSpdlogLevel(logLevel));
    }
}

void Logger::setLevel(Level level) {
    if (logger_) {
        logger_->set_level(toSpdlogLevel(level));
        LOG_INFO("Log level changed to: {}", spdlog::level::to_string_view(toSpdlogLevel(level)));
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        // ロガーが初期化されていない場合は自動初期化
        init();
    }
    return logger_;
}

void Logger::shutdown() {
    if (logger_) {
        LOG_INFO("Logger shutting down...");
        logger_->flush();
        spdlog::shutdown();
        logger_ = nullptr;
    }
}

spdlog::level::level_enum Logger::toSpdlogLevel(Level level) {
    switch (level) {
        case Level::Trace:    return spdlog::level::trace;
        case Level::Debug:    return spdlog::level::debug;
        case Level::Info:     return spdlog::level::info;
        case Level::Warn:     return spdlog::level::warn;
        case Level::Error:    return spdlog::level::err;
        case Level::Critical: return spdlog::level::critical;
        case Level::Off:      return spdlog::level::off;
        default:              return spdlog::level::info;
    }
}

} // namespace inspection
