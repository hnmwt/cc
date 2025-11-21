#include "InspectionController.h"
#include "detectors/DetectorBase.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace inspection {

/**
 * @brief InspectionResultのJSON変換
 */
json InspectionResult::toJson() const {
    json j;
    j["success"] = success;
    j["errorMessage"] = errorMessage;
    j["isOK"] = isOK;
    j["defectCount"] = defects.size();
    j["defects"] = defectsToJson(defects);
    j["preprocessingTime"] = preprocessingTime;
    j["detectionTime"] = detectionTime;
    j["totalTime"] = totalTime;
    j["timestamp"] = timestamp;
    return j;
}

/**
 * @brief JSONからInspectionResultを構築
 */
InspectionResult InspectionResult::fromJson(const json& j) {
    InspectionResult result;
    result.success = j.value("success", false);
    result.errorMessage = j.value("errorMessage", "");
    result.isOK = j.value("isOK", true);
    result.preprocessingTime = j.value("preprocessingTime", 0.0);
    result.detectionTime = j.value("detectionTime", 0.0);
    result.totalTime = j.value("totalTime", 0.0);
    result.timestamp = j.value("timestamp", "");

    if (j.contains("defects") && j["defects"].is_array()) {
        result.defects = defectsFromJson(j["defects"]);
    }

    return result;
}

/**
 * @brief コンストラクタ
 */
InspectionController::InspectionController()
    : maxAllowedDefects_(0),
      minDefectConfidence_(0.5),
      visualizationEnabled_(true),
      saveIntermediateImages_(false),
      totalInspections_(0),
      totalDefectsFound_(0),
      totalNGCount_(0),
      totalProcessingTime_(0.0)
{
}

/**
 * @brief パイプラインを設定
 */
void InspectionController::setPipeline(std::unique_ptr<Pipeline> pipeline) {
    pipeline_ = std::move(pipeline);
}

/**
 * @brief 検出器を追加
 */
void InspectionController::addDetector(std::unique_ptr<DetectorBase> detector) {
    detectors_.push_back(std::move(detector));
}

/**
 * @brief 全ての検出器をクリア
 */
void InspectionController::clearDetectors() {
    detectors_.clear();
}

/**
 * @brief 指定インデックスの検出器を取得
 */
DetectorBase* InspectionController::getDetector(size_t index) const {
    if (index < detectors_.size()) {
        return detectors_[index].get();
    }
    return nullptr;
}

/**
 * @brief 検査を実行
 */
InspectionResult InspectionController::inspect(const cv::Mat& image) {
    auto startTime = cv::getTickCount();
    InspectionResult result;

    // 入力チェック
    if (image.empty()) {
        result.success = false;
        result.errorMessage = "Input image is empty";
        return result;
    }

    result.originalImage = image.clone();
    result.timestamp = getCurrentTimestamp();

    try {
        // 前処理
        cv::Mat processedImage = image.clone();
        double preprocessingTime = 0.0;

        if (pipeline_ && pipeline_->getFilterCount() > 0) {
            auto preprocessStart = cv::getTickCount();
            auto pipelineResult = pipeline_->processWithIntermediates(image);

            if (!pipelineResult.success) {
                result.success = false;
                result.errorMessage = "Preprocessing failed: " + pipelineResult.errorMessage;
                return result;
            }

            processedImage = pipelineResult.finalImage;
            preprocessingTime = pipelineResult.totalTime;

            auto preprocessEnd = cv::getTickCount();
            result.preprocessingTime = (preprocessEnd - preprocessStart) * 1000.0 / cv::getTickFrequency();
        }

        result.processedImage = processedImage.clone();

        // 欠陥検出
        Defects allDefects;
        auto detectionStart = cv::getTickCount();

        for (auto& detector : detectors_) {
            if (detector && detector->isEnabled()) {
                Defects defects = detector->detect(processedImage);
                allDefects.insert(allDefects.end(), defects.begin(), defects.end());
            }
        }

        auto detectionEnd = cv::getTickCount();
        result.detectionTime = (detectionEnd - detectionStart) * 1000.0 / cv::getTickFrequency();

        // 信頼度でフィルタリング
        Defects filteredDefects;
        for (const auto& defect : allDefects) {
            if (defect.confidence >= minDefectConfidence_) {
                filteredDefects.push_back(defect);
            }
        }

        result.defects = filteredDefects;

        // OK/NG判定
        result.isOK = judgeResult(filteredDefects);

        // 可視化
        if (visualizationEnabled_ && !filteredDefects.empty()) {
            result.visualizedImage = DetectorBase::visualizeDefects(
                image,
                filteredDefects,
                true,  // drawContour
                true,  // drawBbox
                true   // drawLabel
            );
        } else if (visualizationEnabled_) {
            result.visualizedImage = image.clone();
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Exception during inspection: ") + e.what();
    }

    // 合計時間
    auto endTime = cv::getTickCount();
    result.totalTime = (endTime - startTime) * 1000.0 / cv::getTickFrequency();

    // 統計更新
    totalInspections_++;
    totalDefectsFound_ += result.defects.size();
    if (!result.isOK) {
        totalNGCount_++;
    }
    totalProcessingTime_ += result.totalTime;

    return result;
}

/**
 * @brief バッチ検査を実行
 */
std::vector<InspectionResult> InspectionController::inspectBatch(
    const std::vector<cv::Mat>& images
) {
    std::vector<InspectionResult> results;
    results.reserve(images.size());

    for (const auto& image : images) {
        results.push_back(inspect(image));
    }

    return results;
}

/**
 * @brief OK/NG判定閾値を設定
 */
void InspectionController::setJudgmentCriteria(size_t maxDefects, double minConfidence) {
    maxAllowedDefects_ = maxDefects;
    if (minConfidence >= 0.0 && minConfidence <= 1.0) {
        minDefectConfidence_ = minConfidence;
    }
}

/**
 * @brief OK/NG判定を実行
 */
bool InspectionController::judgeResult(const Defects& defects) const {
    // 欠陥数が許容範囲内ならOK
    return defects.size() <= maxAllowedDefects_;
}

/**
 * @brief タイムスタンプ文字列を取得
 */
std::string InspectionController::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

/**
 * @brief 統計情報を取得
 */
json InspectionController::getStatistics() const {
    json stats;
    stats["total_inspections"] = totalInspections_;
    stats["total_defects_found"] = totalDefectsFound_;
    stats["total_ng_count"] = totalNGCount_;
    stats["total_processing_time_ms"] = totalProcessingTime_;

    if (totalInspections_ > 0) {
        stats["average_processing_time_ms"] = totalProcessingTime_ / totalInspections_;
        stats["average_defects_per_inspection"] = static_cast<double>(totalDefectsFound_) / totalInspections_;
        stats["ng_rate"] = static_cast<double>(totalNGCount_) / totalInspections_;
    } else {
        stats["average_processing_time_ms"] = 0.0;
        stats["average_defects_per_inspection"] = 0.0;
        stats["ng_rate"] = 0.0;
    }

    stats["detector_count"] = detectors_.size();
    stats["pipeline_filter_count"] = pipeline_ ? pipeline_->getFilterCount() : 0;

    return stats;
}

/**
 * @brief 統計情報をリセット
 */
void InspectionController::resetStatistics() {
    totalInspections_ = 0;
    totalDefectsFound_ = 0;
    totalNGCount_ = 0;
    totalProcessingTime_ = 0.0;
}

} // namespace inspection
