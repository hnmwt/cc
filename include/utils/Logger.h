#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

namespace inspection {

/**
 * @brief ロギングユーティリティクラス (spdlog統合)
 *
 * アプリケーション全体で統一されたログ出力を提供します。
 * コンソールとファイルの両方に出力可能で、スレッドセーフです。
 */
class Logger {
public:
    /**
     * @brief ログレベル
     */
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    /**
     * @brief ロガーを初期化
     * @param logLevel ログレベル
     * @param logToFile ファイルにも出力するか
     * @param logFilePath ログファイルのパス
     * @param maxFileSize 最大ファイルサイズ (バイト)
     * @param maxFiles ローテーションするファイル数
     */
    static void init(Level logLevel = Level::Info,
                    bool logToFile = false,
                    const std::string& logFilePath = "logs/inspection.log",
                    size_t maxFileSize = 1024 * 1024 * 5,  // 5MB
                    size_t maxFiles = 3);

    /**
     * @brief ログレベルを設定
     * @param level ログレベル
     */
    static void setLevel(Level level);

    /**
     * @brief ロガーインスタンスを取得
     * @return spdlogロガーへの共有ポインタ
     */
    static std::shared_ptr<spdlog::logger> get();

    /**
     * @brief ロガーをシャットダウン
     */
    static void shutdown();

private:
    static std::shared_ptr<spdlog::logger> logger_;

    /**
     * @brief Levelをspdlogのログレベルに変換
     * @param level ログレベル
     * @return spdlog::level::level_enum
     */
    static spdlog::level::level_enum toSpdlogLevel(Level level);
};

} // namespace inspection

// ログ出力用マクロ (便利のため)
#define LOG_TRACE(...)    ::inspection::Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::inspection::Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::inspection::Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::inspection::Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::inspection::Logger::get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::inspection::Logger::get()->critical(__VA_ARGS__)

#endif // LOGGER_H
