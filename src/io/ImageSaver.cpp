#include "io/ImageSaver.h"
#include "utils/Logger.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace inspection {

ImageSaver::ImageSaver(const std::string& outputDir, const std::string& filenamePrefix)
    : outputDir_(outputDir),
      filenamePrefix_(filenamePrefix),
      imageFormat_("jpg"),
      createSubdirectories_(true),
      useTimestamp_(true),
      jpegQuality_(95),
      pngCompression_(3)
{
}

bool ImageSaver::saveImages(const InspectionResult& result, ImageType imageTypes) {
    lastSavedFiles_.clear();

    if (imageTypes == ImageType::All || imageTypes == ImageType::Original) {
        if (!result.originalImage.empty()) {
            std::string path = saveOriginal(result.originalImage);
            if (!path.empty()) {
                lastSavedFiles_.push_back(path);
            }
        }
    }

    if (imageTypes == ImageType::All || imageTypes == ImageType::Processed) {
        if (!result.processedImage.empty()) {
            std::string path = saveProcessed(result.processedImage);
            if (!path.empty()) {
                lastSavedFiles_.push_back(path);
            }
        }
    }

    if (imageTypes == ImageType::All || imageTypes == ImageType::Visualized) {
        if (!result.visualizedImage.empty()) {
            std::string path = saveVisualized(result.visualizedImage);
            if (!path.empty()) {
                lastSavedFiles_.push_back(path);
            }
        }
    }

    return !lastSavedFiles_.empty();
}

std::string ImageSaver::saveOriginal(const cv::Mat& image, const std::string& filename) {
    return saveImage(image, "original", "original", filename);
}

std::string ImageSaver::saveProcessed(const cv::Mat& image, const std::string& filename) {
    return saveImage(image, "processed", "processed", filename);
}

std::string ImageSaver::saveVisualized(const cv::Mat& image, const std::string& filename) {
    return saveImage(image, "visualized", "visualized", filename);
}

void ImageSaver::setOutputDirectory(const std::string& outputDir) {
    outputDir_ = outputDir;
}

std::string ImageSaver::generateFilename(const std::string& type) const {
    std::stringstream ss;
    ss << filenamePrefix_ << "_" << type;

    if (useTimestamp_) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        ss << "_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    }

    ss << "." << imageFormat_;
    return ss.str();
}

std::string ImageSaver::saveImage(
    const cv::Mat& image,
    const std::string& subdir,
    const std::string& type,
    const std::string& filename
) {
    if (image.empty()) {
        LOG_WARN("Cannot save empty image");
        return "";
    }

    // 保存先ディレクトリを決定
    std::string targetDir = outputDir_;
    if (createSubdirectories_) {
        targetDir += "/" + subdir;
    }

    // ディレクトリを作成
    if (!ensureDirectory(targetDir)) {
        LOG_ERROR("Failed to create directory: {}", targetDir);
        return "";
    }

    // ファイル名を決定
    std::string fname = filename.empty() ? generateFilename(type) : filename;
    std::string filepath = targetDir + "/" + fname;

    // 画像を保存
    try {
        std::vector<int> params = getSaveParams();
        bool success = cv::imwrite(filepath, image, params);

        if (success) {
            LOG_INFO("Image saved: {} ({}x{})", filepath, image.cols, image.rows);
            return filepath;
        } else {
            LOG_ERROR("Failed to save image: {}", filepath);
            return "";
        }
    } catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV error while saving image: {}", e.what());
        return "";
    } catch (const std::exception& e) {
        LOG_ERROR("Error while saving image: {}", e.what());
        return "";
    }
}

bool ImageSaver::ensureDirectory(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
            LOG_INFO("Created directory: {}", path);
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create directory {}: {}", path, e.what());
        return false;
    }
}

std::vector<int> ImageSaver::getSaveParams() const {
    std::vector<int> params;

    if (imageFormat_ == "jpg" || imageFormat_ == "jpeg") {
        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(jpegQuality_);
    } else if (imageFormat_ == "png") {
        params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        params.push_back(pngCompression_);
    }

    return params;
}

} // namespace inspection
