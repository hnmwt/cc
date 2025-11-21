#include "filters/GaussianFilter.h"
#include "utils/Logger.h"

namespace inspection {

GaussianFilter::GaussianFilter(int kernelSize, double sigma)
    : kernelSize_(kernelSize), sigma_(sigma) {

    if (!isValidKernelSize(kernelSize_)) {
        LOG_WARN("Invalid kernel size {}, using default 5", kernelSize_);
        kernelSize_ = 5;
    }

    if (sigma_ < 0) {
        LOG_WARN("Invalid sigma {}, using default 1.0", sigma_);
        sigma_ = 1.0;
    }

    LOG_DEBUG("GaussianFilter created (kernelSize={}, sigma={})", kernelSize_, sigma_);
}

cv::Mat GaussianFilter::process(const cv::Mat& input) {
    if (input.empty()) {
        LOG_ERROR("GaussianFilter: Input image is empty");
        return cv::Mat();
    }

    cv::Mat output;

    try {
        cv::GaussianBlur(input, output, cv::Size(kernelSize_, kernelSize_), sigma_);

        LOG_DEBUG("GaussianFilter: Applied Gaussian blur (kernelSize={}, sigma={}) to {}x{} image",
                  kernelSize_, sigma_, input.cols, input.rows);

    } catch (const cv::Exception& e) {
        LOG_ERROR("GaussianFilter: OpenCV exception: {}", e.what());
        return cv::Mat();
    }

    return output;
}

std::string GaussianFilter::getName() const {
    return "Gaussian Blur Filter";
}

std::string GaussianFilter::getType() const {
    return "gaussian_blur";
}

void GaussianFilter::setParameters(const json& params) {
    try {
        if (params.contains("kernel_size")) {
            int size = params["kernel_size"].get<int>();
            setKernelSize(size);
        }

        if (params.contains("sigma")) {
            double s = params["sigma"].get<double>();
            setSigma(s);
        }

        LOG_DEBUG("GaussianFilter: Parameters updated (kernelSize={}, sigma={})",
                  kernelSize_, sigma_);

    } catch (const json::exception& e) {
        LOG_ERROR("GaussianFilter: Failed to parse parameters: {}", e.what());
    }
}

json GaussianFilter::getParameters() const {
    json params;
    params["kernel_size"] = kernelSize_;
    params["sigma"] = sigma_;
    return params;
}

std::unique_ptr<FilterBase> GaussianFilter::clone() const {
    return std::make_unique<GaussianFilter>(*this);
}

std::string GaussianFilter::getDescription() const {
    return "Applies Gaussian blur to reduce noise and smooth the image. "
           "Kernel size must be an odd number. Sigma controls the blur strength.";
}

void GaussianFilter::setKernelSize(int size) {
    if (isValidKernelSize(size)) {
        kernelSize_ = size;
        LOG_DEBUG("GaussianFilter: Kernel size set to {}", kernelSize_);
    } else {
        LOG_WARN("GaussianFilter: Invalid kernel size {}, keeping current value {}", size, kernelSize_);
    }
}

int GaussianFilter::getKernelSize() const {
    return kernelSize_;
}

void GaussianFilter::setSigma(double sigma) {
    if (sigma >= 0) {
        sigma_ = sigma;
        LOG_DEBUG("GaussianFilter: Sigma set to {}", sigma_);
    } else {
        LOG_WARN("GaussianFilter: Invalid sigma {}, keeping current value {}", sigma, sigma_);
    }
}

double GaussianFilter::getSigma() const {
    return sigma_;
}

bool GaussianFilter::isValidKernelSize(int size) const {
    // カーネルサイズは1以上の奇数である必要がある
    return (size > 0) && (size % 2 == 1);
}

} // namespace inspection
