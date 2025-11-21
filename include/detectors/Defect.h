#ifndef DEFECT_H
#define DEFECT_H

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace inspection {

using json = nlohmann::json;

/**
 * @brief 欠陥の種類
 */
enum class DefectType {
    Scratch,        ///< 傷
    Stain,          ///< 汚れ
    Discoloration,  ///< 変色
    Deformation,    ///< 形状不良
    Unknown         ///< 不明
};

/**
 * @brief 欠陥情報を格納する構造体
 */
struct Defect {
    DefectType type;            ///< 欠陥の種類
    cv::Rect bbox;              ///< バウンディングボックス (x, y, width, height)
    double confidence;          ///< 信頼度 (0.0 - 1.0)
    cv::Point2f center;         ///< 欠陥の中心座標
    double area;                ///< 欠陥の面積 (ピクセル数)
    double circularity;         ///< 円形度 (0.0 - 1.0, 1.0が完全な円)
    std::vector<cv::Point> contour;  ///< 欠陥の輪郭点

    /**
     * @brief デフォルトコンストラクタ
     */
    Defect()
        : type(DefectType::Unknown),
          bbox(0, 0, 0, 0),
          confidence(0.0),
          center(0.0f, 0.0f),
          area(0.0),
          circularity(0.0) {}

    /**
     * @brief パラメータ付きコンストラクタ
     */
    Defect(DefectType t, const cv::Rect& bb, double conf)
        : type(t),
          bbox(bb),
          confidence(conf),
          center(bb.x + bb.width / 2.0f, bb.y + bb.height / 2.0f),
          area(bb.width * bb.height),
          circularity(0.0) {}

    /**
     * @brief JSON形式に変換
     * @return JSON表現
     */
    json toJson() const;

    /**
     * @brief JSONから構築
     * @param j JSON表現
     * @return Defectオブジェクト
     */
    static Defect fromJson(const json& j);

    /**
     * @brief 欠陥の種類を文字列に変換
     * @return 欠陥タイプの文字列
     */
    std::string getTypeString() const;

    /**
     * @brief 欠陥の可視化用カラーを取得
     * @return BGR形式のカラー
     */
    cv::Scalar getColor() const;

    /**
     * @brief 欠陥が有効かどうか
     * @return 有効な場合true
     */
    bool isValid() const {
        return confidence > 0.0 && bbox.area() > 0;
    }
};

/**
 * @brief 複数の欠陥情報
 */
using Defects = std::vector<Defect>;

/**
 * @brief 欠陥タイプを文字列に変換
 * @param type 欠陥タイプ
 * @return 文字列表現
 */
std::string defectTypeToString(DefectType type);

/**
 * @brief 文字列を欠陥タイプに変換
 * @param str 文字列
 * @return 欠陥タイプ
 */
DefectType stringToDefectType(const std::string& str);

/**
 * @brief 欠陥タイプの可視化用カラーを取得
 * @param type 欠陥タイプ
 * @return BGR形式のカラー
 */
cv::Scalar getDefectColor(DefectType type);

/**
 * @brief 複数の欠陥をJSON配列に変換
 * @param defects 欠陥リスト
 * @return JSON配列
 */
json defectsToJson(const Defects& defects);

/**
 * @brief JSON配列から複数の欠陥を構築
 * @param j JSON配列
 * @return 欠陥リスト
 */
Defects defectsFromJson(const json& j);

} // namespace inspection

#endif // DEFECT_H
