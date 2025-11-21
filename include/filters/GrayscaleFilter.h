#ifndef GRAYSCALEFILTER_H
#define GRAYSCALEFILTER_H

#include "filters/FilterBase.h"

namespace inspection {

/**
 * @brief グレースケール変換フィルタ
 *
 * カラー画像をグレースケール画像に変換します。
 */
class GrayscaleFilter : public FilterBase {
public:
    GrayscaleFilter();
    ~GrayscaleFilter() override = default;

    cv::Mat process(const cv::Mat& input) override;
    std::string getName() const override;
    std::string getType() const override;
    void setParameters(const json& params) override;
    json getParameters() const override;
    std::unique_ptr<FilterBase> clone() const override;
    std::string getDescription() const override;

private:
    // グレースケールフィルタは特別なパラメータを持たない
};

} // namespace inspection

#endif // GRAYSCALEFILTER_H
