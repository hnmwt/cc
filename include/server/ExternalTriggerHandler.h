#ifndef EXTERNAL_TRIGGER_HANDLER_H
#define EXTERNAL_TRIGGER_HANDLER_H

#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <string>
#include <thread>
#include <atomic>

namespace inspection {

/**
 * @brief 外部トリガーメッセージ
 */
struct TriggerMessage {
    std::string command;           ///< コマンド（"INSPECT", "STATUS", "STOP"など）
    std::string imagePath;         ///< 画像パス（オプション）
    std::string parameters;        ///< パラメータ（JSON文字列など）
    std::string clientAddress;     ///< クライアントアドレス
    uint16_t clientPort;           ///< クライアントポート

    /**
     * @brief デフォルトコンストラクタ
     */
    TriggerMessage()
        : command(""),
          imagePath(""),
          parameters(""),
          clientAddress(""),
          clientPort(0) {}
};

/**
 * @brief 外部トリガーハンドラー
 *
 * TCP/IPソケットで外部機器（PLC、センサーなど）からのトリガー信号を受信し、
 * 検査処理を開始します。
 */
class ExternalTriggerHandler {
public:
    /**
     * @brief トリガーコールバック関数型
     *
     * トリガー受信時に呼び出される関数
     * @param message トリガーメッセージ
     * @return 応答メッセージ
     */
    using TriggerCallback = std::function<std::string(const TriggerMessage&)>;

    /**
     * @brief コンストラクタ
     * @param port リスニングポート
     */
    explicit ExternalTriggerHandler(uint16_t port = 9000);

    /**
     * @brief デストラクタ
     */
    ~ExternalTriggerHandler();

    /**
     * @brief サーバーを起動
     * @return 成功した場合true
     */
    bool start();

    /**
     * @brief サーバーを停止
     */
    void stop();

    /**
     * @brief サーバーが実行中か
     * @return 実行中の場合true
     */
    bool isRunning() const {
        return running_;
    }

    /**
     * @brief リスニングポートを設定
     * @param port ポート番号
     */
    void setPort(uint16_t port) {
        port_ = port;
    }

    /**
     * @brief リスニングポートを取得
     * @return ポート番号
     */
    uint16_t getPort() const {
        return port_;
    }

    /**
     * @brief トリガーコールバックを設定
     * @param callback コールバック関数
     */
    void setTriggerCallback(TriggerCallback callback) {
        triggerCallback_ = callback;
    }

    /**
     * @brief 最大同時接続数を設定
     * @param maxConnections 最大接続数
     */
    void setMaxConnections(size_t maxConnections) {
        maxConnections_ = maxConnections;
    }

    /**
     * @brief 最大同時接続数を取得
     * @return 最大接続数
     */
    size_t getMaxConnections() const {
        return maxConnections_;
    }

    /**
     * @brief タイムアウト時間を設定（秒）
     * @param seconds タイムアウト秒数
     */
    void setTimeout(int seconds) {
        timeoutSeconds_ = seconds;
    }

    /**
     * @brief タイムアウト時間を取得
     * @return タイムアウト秒数
     */
    int getTimeout() const {
        return timeoutSeconds_;
    }

    /**
     * @brief メッセージデリミタを設定
     * @param delimiter デリミタ文字列（デフォルト: "\n"）
     */
    void setDelimiter(const std::string& delimiter) {
        delimiter_ = delimiter;
    }

    /**
     * @brief メッセージデリミタを取得
     * @return デリミタ文字列
     */
    std::string getDelimiter() const {
        return delimiter_;
    }

    /**
     * @brief 統計情報を取得
     * @return 統計情報（総接続数、総トリガー数など）
     */
    struct Statistics {
        size_t totalConnections = 0;
        size_t totalTriggers = 0;
        size_t activeConnections = 0;
        size_t failedConnections = 0;
    };

    Statistics getStatistics() const;

    /**
     * @brief 統計情報をリセット
     */
    void resetStatistics();

private:
    /**
     * @brief TCPセッションクラス（内部使用）
     */
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(boost::asio::ip::tcp::socket socket, ExternalTriggerHandler* handler);
        void start();

    private:
        void doRead();
        void doWrite(const std::string& message);
        std::string processMessage(const std::string& message);

        boost::asio::ip::tcp::socket socket_;
        ExternalTriggerHandler* handler_;
        boost::asio::streambuf buffer_;
        std::string clientAddress_;
        uint16_t clientPort_;
    };

    /**
     * @brief 新しい接続を待機
     */
    void doAccept();

    /**
     * @brief IOコンテキストを実行（別スレッド）
     */
    void runIOContext();

    uint16_t port_;                           ///< リスニングポート
    std::atomic<bool> running_;               ///< 実行中フラグ
    size_t maxConnections_;                   ///< 最大同時接続数
    int timeoutSeconds_;                      ///< タイムアウト秒数
    std::string delimiter_;                   ///< メッセージデリミタ

    TriggerCallback triggerCallback_;         ///< トリガーコールバック

    // 統計情報
    std::atomic<size_t> totalConnections_;
    std::atomic<size_t> totalTriggers_;
    std::atomic<size_t> activeConnections_;
    std::atomic<size_t> failedConnections_;

    // Boost.Asio
    std::unique_ptr<boost::asio::io_context> ioContext_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<std::thread> ioThread_;
};

/**
 * @brief トリガーメッセージをパース
 * @param rawMessage 生メッセージ
 * @return パースされたトリガーメッセージ
 */
TriggerMessage parseTriggerMessage(const std::string& rawMessage);

/**
 * @brief トリガーメッセージを文字列に変換
 * @param message トリガーメッセージ
 * @return 文字列表現
 */
std::string triggerMessageToString(const TriggerMessage& message);

} // namespace inspection

#endif // EXTERNAL_TRIGGER_HANDLER_H
