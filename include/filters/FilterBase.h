#ifndef FILTERBASE_H
#define FILTERBASE_H

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace inspection {

using json = nlohmann::json;

/**
 * @brief 画像フィルタの基底抽象クラス
 *
 * すべての画像フィルタはこのクラスを継承します。
 * Strategy パターンを使用して、フィルタの切り替えを可能にします。
 */
class FilterBase {
public:
    virtual ~FilterBase() = default;

    /**
     * @brief 画像にフィルタを適用
     * @param input 入力画像
     * @return 処理後の画像
     */
    virtual cv::Mat process(const cv::Mat& input) = 0;

    /**
     * @brief フィルタ名を取得
     * @return フィルタ名
     */
    virtual std::string getName() const = 0;

    /**
     * @brief フィルタタイプを取得 (例: "grayscale", "gaussian_blur")
     * @return フィルタタイプ
     */
    virtual std::string getType() const = 0;

    /**
     * @brief パラメータを設定 (JSON形式)
     * @param params パラメータJSON
     */
    virtual void setParameters(const json& params) = 0;

    /**
     * @brief 現在のパラメータを取得
     * @return パラメータJSON
     */
    virtual json getParameters() const = 0;

    /**
     * @brief フィルタを複製 (Prototypeパターン)
     *
     * スレッドセーフな並列処理のために使用します。
     *
     * @return 複製されたフィルタのユニークポインタ
     */
    virtual std::unique_ptr<FilterBase> clone() const = 0;

    /**
     * @brief フィルタが有効かどうか
     * @return 有効な場合true
     */
    virtual bool isEnabled() const {
        return enabled_;
    }

    /**
     * @brief フィルタの有効/無効を設定
     * @param enabled 有効にする場合true
     */
    virtual void setEnabled(bool enabled) {
        enabled_ = enabled;
    }

    /**
     * @brief フィルタの説明を取得
     * @return 説明文
     */
    virtual std::string getDescription() const {
        return "No description available";
    }

protected:
    bool enabled_ = true;  ///< フィルタが有効かどうか
};

/**
 * @brief フィルタのファクトリ関数の型定義
 */
using FilterFactory = std::function<std::unique_ptr<FilterBase>()>;

} // namespace inspection

#endif // FILTERBASE_H
