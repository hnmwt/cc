#ifndef REST_API_SERVER_H
#define REST_API_SERVER_H

#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include "InspectionController.h"
#include "io/CSVWriter.h"
#include "io/ImageSaver.h"

// Note: Crow headers would be included here in actual implementation
// For now, we'll use a lightweight approach with Boost.Beast

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

namespace inspection {

/**
 * @brief HTTP REST APIサーバー
 *
 * RESTful APIを提供し、外部システム（Reactフロントエンドなど）から
 * 検査システムを制御できます。
 */
class RestApiServer {
public:
    /**
     * @brief コンストラクタ
     * @param port リスニングポート
     * @param controller 検査コントローラー
     */
    RestApiServer(
        uint16_t port,
        std::shared_ptr<InspectionController> controller
    );

    /**
     * @brief デストラクタ
     */
    ~RestApiServer();

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
     * @brief ポート番号を設定
     * @param port ポート番号
     */
    void setPort(uint16_t port) {
        port_ = port;
    }

    /**
     * @brief ポート番号を取得
     * @return ポート番号
     */
    uint16_t getPort() const {
        return port_;
    }

    /**
     * @brief CSVWriterを設定
     * @param csvWriter CSVWriter
     */
    void setCsvWriter(std::shared_ptr<CSVWriter> csvWriter) {
        csvWriter_ = csvWriter;
    }

    /**
     * @brief ImageSaverを設定
     * @param imageSaver ImageSaver
     */
    void setImageSaver(std::shared_ptr<ImageSaver> imageSaver) {
        imageSaver_ = imageSaver;
    }

    /**
     * @brief 自動保存を有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setAutoSaveEnabled(bool enabled) {
        autoSave_ = enabled;
    }

    /**
     * @brief 統計情報を取得
     */
    struct Statistics {
        size_t totalRequests = 0;
        size_t totalInspections = 0;
        size_t successfulRequests = 0;
        size_t failedRequests = 0;
    };

    Statistics getStatistics() const;

    /**
     * @brief 統計情報をリセット
     */
    void resetStatistics();

private:
    /**
     * @brief HTTPセッション
     */
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, RestApiServer* server);
        void start();

    private:
        void doRead();
        void processRequest();
        void sendResponse(http::status status, const std::string& body, const std::string& contentType = "application/json");
        void handleInspectRequest();
        void handleStatusRequest();
        void handleStatisticsRequest();
        void handleConfigRequest();
        void handleDetectorsRequest();

        tcp::socket socket_;
        RestApiServer* server_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        http::response<http::string_body> response_;
    };

    /**
     * @brief 新しい接続を受け付ける
     */
    void doAccept();

    /**
     * @brief IOコンテキストを実行
     */
    void runIOContext();

    /**
     * @brief CORSヘッダーを追加
     */
    void addCorsHeaders(http::response<http::string_body>& response);

    uint16_t port_;                                        ///< リスニングポート
    std::atomic<bool> running_;                            ///< 実行中フラグ

    std::shared_ptr<InspectionController> controller_;     ///< 検査コントローラー
    std::shared_ptr<CSVWriter> csvWriter_;                 ///< CSVWriter
    std::shared_ptr<ImageSaver> imageSaver_;               ///< ImageSaver

    bool autoSave_;                                        ///< 自動保存フラグ

    // 統計情報
    std::atomic<size_t> totalRequests_;
    std::atomic<size_t> totalInspections_;
    std::atomic<size_t> successfulRequests_;
    std::atomic<size_t> failedRequests_;

    // Boost.Asio
    std::unique_ptr<net::io_context> ioContext_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::unique_ptr<std::thread> ioThread_;
};

} // namespace inspection

#endif // REST_API_SERVER_H
