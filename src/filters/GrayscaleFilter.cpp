#include "filters/GrayscaleFilter.h"
#include "utils/Logger.h"

namespace inspection {

GrayscaleFilter::GrayscaleFilter() {
    LOG_DEBUG("GrayscaleFilter created");
}

cv::Mat GrayscaleFilter::process(const cv::Mat& input) {
    if (input.empty()) {
        LOG_ERROR("GrayscaleFilter: Input image is empty");
        return cv::Mat();
    }

    // すでにグレースケールの場合はそのまま返す
    if (input.channels() == 1) {
        LOG_DEBUG("GrayscaleFilter: Input is already grayscale");
        return input.clone();
    }

    cv::Mat output;

    try {
        cv::cvtColor(input, output, cv::COLOR_BGR2GRAY);
        LOG_DEBUG("GrayscaleFilter: Converted {} channels to 1 channel ({}x{})",
                  input.channels(), output.cols, output.rows);
    } catch (const cv::Exception& e) {
        LOG_ERROR("GrayscaleFilter: OpenCV exception: {}", e.what());
        return cv::Mat();
    }

    return output;
}

std::string GrayscaleFilter::getName() const {
    return "Grayscale Filter";
}

std::string GrayscaleFilter::getType() const {
    return "grayscale";
}

void GrayscaleFilter::setParameters(const json& params) {
    // グレースケールフィルタはパラメータを持たない
    (void)params;  // 未使用パラメータ警告を抑制
    LOG_DEBUG("GrayscaleFilter: No parameters to set");
}

json GrayscaleFilter::getParameters() const {
    // パラメータなし
    return json::object();
}

std::unique_ptr<FilterBase> GrayscaleFilter::clone() const {
    return std::make_unique<GrayscaleFilter>(*this);
}

std::string GrayscaleFilter::getDescription() const {
    return "Converts a color image to grayscale using OpenCV's cvtColor function";
}

} // namespace inspection
