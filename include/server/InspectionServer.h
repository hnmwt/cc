#ifndef INSPECTION_SERVER_H
#define INSPECTION_SERVER_H

#include <memory>
#include <string>
#include <atomic>
#include "InspectionController.h"
#include "server/ExternalTriggerHandler.h"
#include "server/RestApiServer.h"
#include "io/CSVWriter.h"
#include "io/ImageSaver.h"
#include "utils/ConfigManager.h"

namespace inspection {

/**
 * @brief 統合検査サーバー
 *
 * すべての機能を統合したメインサーバー：
 * - 外部トリガー受信（ExternalTriggerHandler）
 * - REST API（RestApiServer）
 * - 検査処理（InspectionController）
 * - データ保存（CSVWriter, ImageSaver）
 */
class InspectionServer {
public:
    /**
     * @brief コンストラクタ
     * @param configPath 設定ファイルのパス
     */
    explicit InspectionServer(const std::string& configPath = "config/default_config.json");

    /**
     * @brief デストラクタ
     */
    ~InspectionServer();

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
     * @brief 設定を読み込む
     * @return 成功した場合true
     */
    bool loadConfig();

    /**
     * @brief 設定を保存
     * @return 成功した場合true
     */
    bool saveConfig();

    /**
     * @brief 設定ファイルのパスを設定
     * @param configPath 設定ファイルのパス
     */
    void setConfigPath(const std::string& configPath) {
        configPath_ = configPath;
    }

    /**
     * @brief 設定ファイルのパスを取得
     * @return 設定ファイルのパス
     */
    std::string getConfigPath() const {
        return configPath_;
    }

    /**
     * @brief InspectionControllerを取得
     * @return InspectionController
     */
    std::shared_ptr<InspectionController> getController() const {
        return controller_;
    }

    /**
     * @brief ExternalTriggerHandlerを取得
     * @return ExternalTriggerHandler
     */
    std::shared_ptr<ExternalTriggerHandler> getTriggerHandler() const {
        return triggerHandler_;
    }

    /**
     * @brief RestApiServerを取得
     * @return RestApiServer
     */
    std::shared_ptr<RestApiServer> getApiServer() const {
        return apiServer_;
    }

    /**
     * @brief CSVWriterを取得
     * @return CSVWriter
     */
    std::shared_ptr<CSVWriter> getCsvWriter() const {
        return csvWriter_;
    }

    /**
     * @brief ImageSaverを取得
     * @return ImageSaver
     */
    std::shared_ptr<ImageSaver> getImageSaver() const {
        return imageSaver_;
    }

    /**
     * @brief 外部トリガーハンドラーを有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setTriggerHandlerEnabled(bool enabled) {
        triggerHandlerEnabled_ = enabled;
    }

    /**
     * @brief REST APIサーバーを有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setApiServerEnabled(bool enabled) {
        apiServerEnabled_ = enabled;
    }

    /**
     * @brief 統計情報を取得
     */
    struct Statistics {
        // コントローラー統計
        size_t totalInspections = 0;
        size_t totalDefects = 0;
        size_t totalNgCount = 0;
        double averageProcessingTime = 0.0;

        // トリガーハンドラー統計
        size_t triggerTotalConnections = 0;
        size_t triggerTotalTriggers = 0;

        // APIサーバー統計
        size_t apiTotalRequests = 0;
        size_t apiTotalInspections = 0;
        size_t apiSuccessfulRequests = 0;
        size_t apiFailedRequests = 0;
    };

    Statistics getStatistics() const;

    /**
     * @brief 統計情報をリセット
     */
    void resetStatistics();

    /**
     * @brief サーバー情報を取得
     */
    struct ServerInfo {
        std::string version;
        bool running;
        bool triggerHandlerRunning;
        bool apiServerRunning;
        uint16_t triggerPort;
        uint16_t apiPort;
        std::string configPath;
    };

    ServerInfo getServerInfo() const;

private:
    /**
     * @brief InspectionControllerを初期化
     * @return 成功した場合true
     */
    bool initializeController();

    /**
     * @brief パイプラインを設定から構築
     * @return 成功した場合true
     */
    bool buildPipeline();

    /**
     * @brief 検出器を設定から構築
     * @return 成功した場合true
     */
    bool buildDetectors();

    /**
     * @brief 外部トリガーのコールバック
     */
    std::string handleExternalTrigger(const TriggerMessage& message);

    std::string configPath_;                                   ///< 設定ファイルのパス
    std::atomic<bool> running_;                                ///< 実行中フラグ

    // コンポーネント
    std::shared_ptr<InspectionController> controller_;         ///< 検査コントローラー
    std::shared_ptr<ExternalTriggerHandler> triggerHandler_;   ///< 外部トリガーハンドラー
    std::shared_ptr<RestApiServer> apiServer_;                 ///< REST APIサーバー
    std::shared_ptr<CSVWriter> csvWriter_;                     ///< CSVWriter
    std::shared_ptr<ImageSaver> imageSaver_;                   ///< ImageSaver

    // 設定
    bool triggerHandlerEnabled_;                               ///< 外部トリガー有効フラグ
    bool apiServerEnabled_;                                    ///< REST API有効フラグ
    uint16_t triggerPort_;                                     ///< 外部トリガーポート
    uint16_t apiPort_;                                         ///< REST APIポート
    std::string csvOutputDir_;                                 ///< CSV出力ディレクトリ
    std::string imageOutputDir_;                               ///< 画像出力ディレクトリ
};

} // namespace inspection

#endif // INSPECTION_SERVER_H
