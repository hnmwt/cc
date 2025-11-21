#include "detectors/Defect.h"
#include <stdexcept>
#include <sstream>

namespace inspection {

/**
 * @brief DefectをJSON形式に変換
 */
json Defect::toJson() const {
    json j;

    j["type"] = getTypeString();
    j["bbox"] = {
        {"x", bbox.x},
        {"y", bbox.y},
        {"width", bbox.width},
        {"height", bbox.height}
    };
    j["confidence"] = confidence;
    j["center"] = {
        {"x", center.x},
        {"y", center.y}
    };
    j["area"] = area;
    j["circularity"] = circularity;

    // 輪郭点をJSON配列に変換
    json contourArray = json::array();
    for (const auto& point : contour) {
        contourArray.push_back({
            {"x", point.x},
            {"y", point.y}
        });
    }
    j["contour"] = contourArray;

    return j;
}

/**
 * @brief JSONからDefectを構築
 */
Defect Defect::fromJson(const json& j) {
    Defect defect;

    // 欠陥タイプ
    if (j.contains("type") && j["type"].is_string()) {
        defect.type = stringToDefectType(j["type"].get<std::string>());
    }

    // バウンディングボックス
    if (j.contains("bbox") && j["bbox"].is_object()) {
        const auto& bbox = j["bbox"];
        defect.bbox.x = bbox.value("x", 0);
        defect.bbox.y = bbox.value("y", 0);
        defect.bbox.width = bbox.value("width", 0);
        defect.bbox.height = bbox.value("height", 0);
    }

    // 信頼度
    defect.confidence = j.value("confidence", 0.0);

    // 中心座標
    if (j.contains("center") && j["center"].is_object()) {
        const auto& center = j["center"];
        defect.center.x = center.value("x", 0.0f);
        defect.center.y = center.value("y", 0.0f);
    }

    // 面積と円形度
    defect.area = j.value("area", 0.0);
    defect.circularity = j.value("circularity", 0.0);

    // 輪郭点
    if (j.contains("contour") && j["contour"].is_array()) {
        defect.contour.clear();
        for (const auto& point : j["contour"]) {
            if (point.is_object()) {
                cv::Point pt(
                    point.value("x", 0),
                    point.value("y", 0)
                );
                defect.contour.push_back(pt);
            }
        }
    }

    return defect;
}

/**
 * @brief 欠陥の種類を文字列に変換
 */
std::string Defect::getTypeString() const {
    return defectTypeToString(type);
}

/**
 * @brief 欠陥の可視化用カラーを取得
 */
cv::Scalar Defect::getColor() const {
    return getDefectColor(type);
}

/**
 * @brief 欠陥タイプを文字列に変換
 */
std::string defectTypeToString(DefectType type) {
    switch (type) {
        case DefectType::Scratch:
            return "Scratch";
        case DefectType::Stain:
            return "Stain";
        case DefectType::Discoloration:
            return "Discoloration";
        case DefectType::Deformation:
            return "Deformation";
        case DefectType::Unknown:
        default:
            return "Unknown";
    }
}

/**
 * @brief 文字列を欠陥タイプに変換
 */
DefectType stringToDefectType(const std::string& str) {
    if (str == "Scratch") {
        return DefectType::Scratch;
    } else if (str == "Stain") {
        return DefectType::Stain;
    } else if (str == "Discoloration") {
        return DefectType::Discoloration;
    } else if (str == "Deformation") {
        return DefectType::Deformation;
    } else {
        return DefectType::Unknown;
    }
}

/**
 * @brief 欠陥タイプの可視化用カラーを取得
 * @return BGR形式のカラー
 */
cv::Scalar getDefectColor(DefectType type) {
    switch (type) {
        case DefectType::Scratch:
            return cv::Scalar(0, 0, 255);      // 赤 (BGR)
        case DefectType::Stain:
            return cv::Scalar(0, 165, 255);    // オレンジ
        case DefectType::Discoloration:
            return cv::Scalar(0, 255, 255);    // 黄色
        case DefectType::Deformation:
            return cv::Scalar(255, 0, 255);    // マゼンタ
        case DefectType::Unknown:
        default:
            return cv::Scalar(128, 128, 128);  // グレー
    }
}

/**
 * @brief 複数の欠陥をJSON配列に変換
 */
json defectsToJson(const Defects& defects) {
    json j = json::array();
    for (const auto& defect : defects) {
        j.push_back(defect.toJson());
    }
    return j;
}

/**
 * @brief JSON配列から複数の欠陥を構築
 */
Defects defectsFromJson(const json& j) {
    Defects defects;

    if (!j.is_array()) {
        throw std::invalid_argument("JSON must be an array");
    }

    for (const auto& item : j) {
        defects.push_back(Defect::fromJson(item));
    }

    return defects;
}

} // namespace inspection
