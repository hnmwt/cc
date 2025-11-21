#include "detectors/DetectorBase.h"
#include <sstream>
#include <iomanip>

namespace inspection {

/**
 * @brief 検出結果を可視化した画像を生成
 */
cv::Mat DetectorBase::visualizeDefects(
    const cv::Mat& image,
    const Defects& defects,
    bool drawContour,
    bool drawBbox,
    bool drawLabel
) {
    // 入力画像が空の場合はそのまま返す
    if (image.empty()) {
        return image;
    }

    // 結果画像の作成（カラー画像として扱う）
    cv::Mat result;
    if (image.channels() == 1) {
        cv::cvtColor(image, result, cv::COLOR_GRAY2BGR);
    } else {
        result = image.clone();
    }

    // 各欠陥を描画
    for (size_t i = 0; i < defects.size(); ++i) {
        const auto& defect = defects[i];

        // 欠陥タイプに応じた色を取得
        cv::Scalar color = defect.getColor();

        // 輪郭を描画
        if (drawContour && !defect.contour.empty()) {
            std::vector<std::vector<cv::Point>> contours = { defect.contour };
            cv::drawContours(result, contours, 0, color, 2);
        }

        // バウンディングボックスを描画
        if (drawBbox && defect.bbox.area() > 0) {
            cv::rectangle(result, defect.bbox, color, 2);

            // 中心点を描画
            cv::circle(result, defect.center, 3, color, -1);
        }

        // ラベルを描画
        if (drawLabel) {
            // ラベルテキストの作成
            std::ostringstream labelStream;
            labelStream << defect.getTypeString() << " "
                       << std::fixed << std::setprecision(2)
                       << (defect.confidence * 100.0) << "%";
            std::string label = labelStream.str();

            // テキストサイズを取得
            int fontFace = cv::FONT_HERSHEY_SIMPLEX;
            double fontScale = 0.5;
            int thickness = 1;
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);

            // ラベル背景の位置計算
            cv::Point labelPos(defect.bbox.x, defect.bbox.y - 5);
            if (labelPos.y < textSize.height + 5) {
                labelPos.y = defect.bbox.y + defect.bbox.height + textSize.height + 5;
            }

            // ラベル背景を描画
            cv::Rect labelRect(
                labelPos.x,
                labelPos.y - textSize.height - 3,
                textSize.width + 6,
                textSize.height + 6
            );

            // 画像の範囲内に収める
            labelRect.x = std::max(0, labelRect.x);
            labelRect.y = std::max(0, labelRect.y);
            labelRect.width = std::min(result.cols - labelRect.x, labelRect.width);
            labelRect.height = std::min(result.rows - labelRect.y, labelRect.height);

            // 背景を塗りつぶし
            cv::rectangle(result, labelRect, color, -1);

            // テキストを描画（白色）
            cv::Point textPos(labelPos.x + 3, labelPos.y - 3);
            cv::putText(result, label, textPos, fontFace, fontScale,
                       cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);

            // 欠陥番号を中心に描画
            std::ostringstream numStream;
            numStream << "#" << (i + 1);
            std::string num = numStream.str();
            cv::Size numSize = cv::getTextSize(num, fontFace, fontScale, thickness, &baseline);
            cv::Point numPos(
                static_cast<int>(defect.center.x - numSize.width / 2),
                static_cast<int>(defect.center.y + numSize.height / 2)
            );
            cv::putText(result, num, numPos, fontFace, fontScale,
                       cv::Scalar(255, 255, 255), thickness + 1, cv::LINE_AA);
        }
    }

    // 検出数のサマリーを左上に表示
    if (!defects.empty()) {
        std::ostringstream summaryStream;
        summaryStream << "Defects: " << defects.size();
        std::string summary = summaryStream.str();

        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 0.7;
        int thickness = 2;
        cv::Size textSize = cv::getTextSize(summary, fontFace, fontScale, thickness, nullptr);

        cv::Rect summaryRect(10, 10, textSize.width + 20, textSize.height + 20);
        cv::rectangle(result, summaryRect, cv::Scalar(0, 0, 0), -1);
        cv::rectangle(result, summaryRect, cv::Scalar(255, 255, 255), 2);

        cv::Point textPos(20, 20 + textSize.height);
        cv::putText(result, summary, textPos, fontFace, fontScale,
                   cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
    }

    return result;
}

} // namespace inspection
