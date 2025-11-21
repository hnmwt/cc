#ifndef PIPELINE_H
#define PIPELINE_H

#include "filters/FilterBase.h"
#include <vector>
#include <memory>
#include <string>
#include <chrono>

namespace inspection {

/**
 * @brief 複数のフィルタを連鎖させる画像処理パイプライン
 *
 * Chain of Responsibility パターンを使用して、複数のフィルタを順次適用します。
 */
class Pipeline {
public:
    /**
     * @brief 処理結果の詳細情報
     */
    struct ProcessingResult {
        cv::Mat finalImage;                           ///< 最終処理画像
        std::vector<cv::Mat> intermediateImages;      ///< 各フィルタ適用後の中間画像
        std::vector<std::string> filterNames;         ///< 適用されたフィルタ名
        std::vector<double> processingTimes;          ///< 各フィルタの処理時間 (ms)
        double totalTime = 0.0;                       ///< 合計処理時間 (ms)
        bool success = true;                          ///< 処理が成功したか
        std::string errorMessage;                     ///< エラーメッセージ
    };

    Pipeline() = default;
    ~Pipeline() = default;

    /**
     * @brief パイプラインにフィルタを追加
     * @param filter 追加するフィルタ
     */
    void addFilter(std::unique_ptr<FilterBase> filter);

    /**
     * @brief パイプラインからフィルタを削除
     * @param index 削除するフィルタのインデックス
     * @return 削除に成功した場合true
     */
    bool removeFilter(size_t index);

    /**
     * @brief パイプライン内の全フィルタをクリア
     */
    void clear();

    /**
     * @brief 画像にパイプラインを適用
     * @param input 入力画像
     * @return 処理後の画像
     */
    cv::Mat process(const cv::Mat& input);

    /**
     * @brief 画像にパイプラインを適用し、中間結果も返す
     * @param input 入力画像
     * @return 処理結果の詳細情報
     */
    ProcessingResult processWithIntermediates(const cv::Mat& input);

    /**
     * @brief パイプライン内のフィルタ数を取得
     * @return フィルタ数
     */
    size_t getFilterCount() const;

    /**
     * @brief 指定インデックスのフィルタを取得
     * @param index フィルタのインデックス
     * @return フィルタへのポインタ (範囲外の場合nullptr)
     */
    FilterBase* getFilter(size_t index) const;

    /**
     * @brief パイプライン内の全フィルタ名を取得
     * @return フィルタ名のリスト
     */
    std::vector<std::string> getFilterNames() const;

    /**
     * @brief パイプライン設定をJSONで取得
     * @return パイプライン設定のJSON
     */
    json toJson() const;

    /**
     * @brief JSONからパイプライン設定を読み込み
     *
     * 注意: フィルタのファクトリが必要です
     *
     * @param config パイプライン設定のJSON
     */
    void fromJson(const json& config);

    /**
     * @brief パイプラインが空かどうか
     * @return 空の場合true
     */
    bool isEmpty() const;

private:
    std::vector<std::unique_ptr<FilterBase>> filters_;  ///< フィルタのリスト

    /**
     * @brief 処理時間を計測してフィルタを適用
     * @param filter 適用するフィルタ
     * @param input 入力画像
     * @param output 出力画像
     * @return 処理時間 (ms)
     */
    double applyFilterTimed(FilterBase* filter, const cv::Mat& input, cv::Mat& output);
};

} // namespace inspection

#endif // PIPELINE_H
