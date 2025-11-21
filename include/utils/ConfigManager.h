#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <nlohmann/json.hpp>
#include <string>
#include <mutex>
#include <optional>

namespace inspection {

using json = nlohmann::json;

/**
 * @brief アプリケーション設定を管理するシングルトンクラス
 *
 * JSON形式の設定ファイルを読み込み、アプリケーション全体で設定にアクセスできるようにします。
 * スレッドセーフな実装になっています。
 */
class ConfigManager {
public:
    /**
     * @brief シングルトンインスタンスを取得
     * @return ConfigManagerインスタンスへの参照
     */
    static ConfigManager& getInstance();

    // コピー・ムーブを禁止
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    /**
     * @brief 設定ファイルを読み込む
     * @param configPath 設定ファイルのパス
     * @return 成功時true、失敗時false
     */
    bool loadConfig(const std::string& configPath);

    /**
     * @brief 現在の設定をファイルに保存
     * @param configPath 保存先パス
     * @return 成功時true、失敗時false
     */
    bool saveConfig(const std::string& configPath) const;

    /**
     * @brief 設定値を取得 (JSON Pointer形式)
     *
     * 使用例:
     *   auto port = config.getValue<int>("/server/http/port");
     *   auto host = config.getValue<std::string>("/server/http/host");
     *
     * @tparam T 取得する値の型
     * @param jsonPointer JSON Pointer (例: "/server/http/port")
     * @return 値が存在する場合はoptionalに格納、存在しない場合はnullopt
     */
    template<typename T>
    std::optional<T> getValue(const std::string& jsonPointer) const {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            auto value = config_.at(json::json_pointer(jsonPointer));
            return value.get<T>();
        } catch (const json::exception& e) {
            return std::nullopt;
        }
    }

    /**
     * @brief 設定値を取得 (デフォルト値付き)
     *
     * @tparam T 取得する値の型
     * @param jsonPointer JSON Pointer
     * @param defaultValue 値が存在しない場合のデフォルト値
     * @return 設定値、存在しない場合はdefaultValue
     */
    template<typename T>
    T getValueOr(const std::string& jsonPointer, const T& defaultValue) const {
        auto value = getValue<T>(jsonPointer);
        return value.value_or(defaultValue);
    }

    /**
     * @brief 設定値を設定 (JSON Pointer形式)
     *
     * @tparam T 設定する値の型
     * @param jsonPointer JSON Pointer
     * @param value 設定する値
     */
    template<typename T>
    void setValue(const std::string& jsonPointer, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[json::json_pointer(jsonPointer)] = value;
    }

    /**
     * @brief 設定全体を取得
     * @return JSON設定オブジェクト
     */
    json getConfig() const;

    /**
     * @brief 設定全体を設定
     * @param config 新しい設定
     */
    void setConfig(const json& config);

    /**
     * @brief 設定がロードされているかを確認
     * @return ロード済みの場合true
     */
    bool isLoaded() const;

    /**
     * @brief 設定をクリア
     */
    void clear();

    /**
     * @brief デフォルト設定で初期化
     */
    void loadDefaultConfig();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    json config_;                       ///< 設定データ
    mutable std::mutex mutex_;          ///< スレッドセーフのためのミューテックス
    bool loaded_ = false;               ///< 設定がロードされているか

    /**
     * @brief デフォルト設定を生成
     * @return デフォルト設定のJSON
     */
    static json createDefaultConfig();
};

} // namespace inspection

#endif // CONFIGMANAGER_H
