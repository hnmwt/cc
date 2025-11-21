#ifndef GAUSSIANFILTER_H
#define GAUSSIANFILTER_H

#include "filters/FilterBase.h"

namespace inspection {

/**
 * @brief ガウシアンブラーフィルタ
 *
 * ガウシアンカーネルを使用して画像をぼかし、ノイズを除去します。
 */
class GaussianFilter : public FilterBase {
public:
    /**
     * @brief コンストラクタ
     * @param kernelSize カーネルサイズ (奇数のみ、デフォルト5)
     * @param sigma 標準偏差 (デフォルト1.0、0の場合は自動計算)
     */
    GaussianFilter(int kernelSize = 5, double sigma = 1.0);
    ~GaussianFilter() override = default;

    cv::Mat process(const cv::Mat& input) override;
    std::string getName() const override;
    std::string getType() const override;
    void setParameters(const json& params) override;
    json getParameters() const override;
    std::unique_ptr<FilterBase> clone() const override;
    std::string getDescription() const override;

    /**
     * @brief カーネルサイズを設定
     * @param size カーネルサイズ (1以上の奇数)
     */
    void setKernelSize(int size);

    /**
     * @brief カーネルサイズを取得
     * @return カーネルサイズ
     */
    int getKernelSize() const;

    /**
     * @brief 標準偏差を設定
     * @param sigma 標準偏差 (0の場合は自動計算)
     */
    void setSigma(double sigma);

    /**
     * @brief 標準偏差を取得
     * @return 標準偏差
     */
    double getSigma() const;

private:
    int kernelSize_;    ///< カーネルサイズ
    double sigma_;      ///< 標準偏差

    /**
     * @brief カーネルサイズを検証
     * @param size カーネルサイズ
     * @return 有効な場合true
     */
    bool isValidKernelSize(int size) const;
};

} // namespace inspection

#endif // GAUSSIANFILTER_H
