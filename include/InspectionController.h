#ifndef INSPECTION_CONTROLLER_H
#define INSPECTION_CONTROLLER_H

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include <string>
#include "pipeline/Pipeline.h"
#include "detectors/DetectorBase.h"
#include "detectors/Defect.h"

namespace inspection {

using json = nlohmann::json;

/**
 * @brief 検査結果
 */
struct InspectionResult {
    bool success = false;                    ///< 検査が成功したか
    std::string errorMessage;                ///< エラーメッセージ

    cv::Mat originalImage;                   ///< 元画像
    cv::Mat processedImage;                  ///< 前処理後の画像
    cv::Mat visualizedImage;                 ///< 可視化画像

    Defects defects;                         ///< 検出された欠陥リスト
    bool isOK = true;                        ///< OK/NG判定

    double preprocessingTime = 0.0;          ///< 前処理時間（ミリ秒）
    double detectionTime = 0.0;              ///< 検出時間（ミリ秒）
    double totalTime = 0.0;                  ///< 合計処理時間（ミリ秒）

    std::string timestamp;                   ///< 検査時刻

    /**
     * @brief JSON形式に変換
     */
    json toJson() const;

    /**
     * @brief JSONから構築
     */
    static InspectionResult fromJson(const json& j);
};

/**
 * @brief 検査制御クラス
 *
 * 画像処理パイプライン（前処理）と欠陥検出器を組み合わせて
 * 完全な検査ワークフローを提供します。
 */
class InspectionController {
public:
    /**
     * @brief コンストラクタ
     */
    InspectionController();

    /**
     * @brief デストラクタ
     */
    ~InspectionController() = default;

    /**
     * @brief パイプラインを設定
     * @param pipeline パイプライン（前処理用）
     */
    void setPipeline(std::unique_ptr<Pipeline> pipeline);

    /**
     * @brief パイプラインを取得
     */
    Pipeline* getPipeline() const {
        return pipeline_.get();
    }

    /**
     * @brief 検出器を追加
     * @param detector 検出器
     */
    void addDetector(std::unique_ptr<DetectorBase> detector);

    /**
     * @brief 全ての検出器をクリア
     */
    void clearDetectors();

    /**
     * @brief 検出器の数を取得
     */
    size_t getDetectorCount() const {
        return detectors_.size();
    }

    /**
     * @brief 指定インデックスの検出器を取得
     */
    DetectorBase* getDetector(size_t index) const;

    /**
     * @brief 検査を実行
     * @param image 入力画像
     * @return 検査結果
     */
    InspectionResult inspect(const cv::Mat& image);

    /**
     * @brief バッチ検査を実行
     * @param images 入力画像リスト
     * @return 検査結果リスト
     */
    std::vector<InspectionResult> inspectBatch(const std::vector<cv::Mat>& images);

    /**
     * @brief OK/NG判定閾値を設定
     * @param maxDefects 許容する最大欠陥数
     * @param minConfidence 欠陥と判定する最小信頼度
     */
    void setJudgmentCriteria(size_t maxDefects, double minConfidence = 0.0);

    /**
     * @brief OK/NG判定閾値を取得
     */
    void getJudgmentCriteria(size_t& maxDefects, double& minConfidence) const {
        maxDefects = maxAllowedDefects_;
        minConfidence = minDefectConfidence_;
    }

    /**
     * @brief 可視化を有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setVisualizationEnabled(bool enabled) {
        visualizationEnabled_ = enabled;
    }

    /**
     * @brief 可視化が有効か
     */
    bool isVisualizationEnabled() const {
        return visualizationEnabled_;
    }

    /**
     * @brief 中間画像の保存を有効/無効にする
     */
    void setIntermediateImagesEnabled(bool enabled) {
        saveIntermediateImages_ = enabled;
    }

    /**
     * @brief 中間画像の保存が有効か
     */
    bool isIntermediateImagesEnabled() const {
        return saveIntermediateImages_;
    }

    /**
     * @brief 統計情報を取得
     */
    json getStatistics() const;

    /**
     * @brief 統計情報をリセット
     */
    void resetStatistics();

private:
    /**
     * @brief OK/NG判定を実行
     */
    bool judgeResult(const Defects& defects) const;

    /**
     * @brief タイムスタンプ文字列を取得
     */
    std::string getCurrentTimestamp() const;

    std::unique_ptr<Pipeline> pipeline_;               ///< 前処理パイプライン
    std::vector<std::unique_ptr<DetectorBase>> detectors_;  ///< 検出器リスト

    // 判定基準
    size_t maxAllowedDefects_;                         ///< 許容する最大欠陥数
    double minDefectConfidence_;                       ///< 欠陥と判定する最小信頼度

    // 設定
    bool visualizationEnabled_;                        ///< 可視化を行うか
    bool saveIntermediateImages_;                      ///< 中間画像を保存するか

    // 統計情報
    size_t totalInspections_;                          ///< 合計検査回数
    size_t totalDefectsFound_;                         ///< 合計検出欠陥数
    size_t totalNGCount_;                              ///< NG判定回数
    double totalProcessingTime_;                       ///< 合計処理時間（ミリ秒）
};

} // namespace inspection

#endif // INSPECTION_CONTROLLER_H
