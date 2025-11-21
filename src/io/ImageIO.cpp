#include "io/ImageIO.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace inspection {

cv::Mat ImageIO::loadImage(const std::string& path, int flags) {
    if (path.empty()) {
        std::cerr << "[ImageIO] Error: Empty path provided" << std::endl;
        return cv::Mat();
    }

    if (!fs::exists(path)) {
        std::cerr << "[ImageIO] Error: File does not exist: " << path << std::endl;
        return cv::Mat();
    }

    cv::Mat image = cv::imread(path, flags);

    if (image.empty()) {
        std::cerr << "[ImageIO] Error: Failed to load image: " << path << std::endl;
        return cv::Mat();
    }

    std::cout << "[ImageIO] Successfully loaded image: " << path
              << " (size: " << image.cols << "x" << image.rows << ")" << std::endl;

    return image;
}

bool ImageIO::saveImage(const cv::Mat& image,
                       const std::string& path,
                       const std::vector<int>& params) {
    if (!isValid(image)) {
        std::cerr << "[ImageIO] Error: Invalid image provided for saving" << std::endl;
        return false;
    }

    if (path.empty()) {
        std::cerr << "[ImageIO] Error: Empty path provided" << std::endl;
        return false;
    }

    // ディレクトリが存在しない場合は作成
    fs::path filePath(path);
    if (filePath.has_parent_path()) {
        fs::path parentDir = filePath.parent_path();
        if (!fs::exists(parentDir)) {
            try {
                fs::create_directories(parentDir);
                std::cout << "[ImageIO] Created directory: " << parentDir << std::endl;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "[ImageIO] Error: Failed to create directory: " << e.what() << std::endl;
                return false;
            }
        }
    }

    bool success = cv::imwrite(path, image, params);

    if (success) {
        std::cout << "[ImageIO] Successfully saved image: " << path << std::endl;
    } else {
        std::cerr << "[ImageIO] Error: Failed to save image: " << path << std::endl;
    }

    return success;
}

std::vector<cv::Mat> ImageIO::loadBatch(const std::vector<std::string>& paths, int flags) {
    std::vector<cv::Mat> images;
    images.reserve(paths.size());

    std::cout << "[ImageIO] Loading batch of " << paths.size() << " images..." << std::endl;

    int successCount = 0;
    for (const auto& path : paths) {
        cv::Mat image = loadImage(path, flags);
        images.push_back(image);
        if (!image.empty()) {
            successCount++;
        }
    }

    std::cout << "[ImageIO] Batch loading complete: " << successCount << "/" << paths.size() << " successful" << std::endl;

    return images;
}

std::vector<cv::Mat> ImageIO::loadDirectory(const std::string& directory,
                                           const std::vector<std::string>& extensions,
                                           int flags) {
    std::vector<std::string> imagePaths = getImagePaths(directory, extensions);
    return loadBatch(imagePaths, flags);
}

int ImageIO::saveBatch(const std::vector<cv::Mat>& images,
                      const std::string& outputDir,
                      const std::string& prefix,
                      const std::string& extension) {
    // 出力ディレクトリが存在しない場合は作成
    if (!fs::exists(outputDir)) {
        try {
            fs::create_directories(outputDir);
            std::cout << "[ImageIO] Created output directory: " << outputDir << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[ImageIO] Error: Failed to create output directory: " << e.what() << std::endl;
            return 0;
        }
    }

    int successCount = 0;
    for (size_t i = 0; i < images.size(); ++i) {
        if (!isValid(images[i])) {
            std::cerr << "[ImageIO] Warning: Skipping invalid image at index " << i << std::endl;
            continue;
        }

        // ファイル名を生成 (例: image_0001.jpg)
        std::ostringstream filename;
        filename << prefix << "_" << std::setfill('0') << std::setw(4) << i << extension;

        fs::path outputPath = fs::path(outputDir) / filename.str();

        if (saveImage(images[i], outputPath.string())) {
            successCount++;
        }
    }

    std::cout << "[ImageIO] Batch save complete: " << successCount << "/" << images.size() << " successful" << std::endl;

    return successCount;
}

bool ImageIO::isValid(const cv::Mat& image) {
    return !image.empty() && image.data != nullptr;
}

std::vector<std::string> ImageIO::getImagePaths(const std::string& directory,
                                                const std::vector<std::string>& extensions) {
    std::vector<std::string> imagePaths;

    if (!fs::exists(directory)) {
        std::cerr << "[ImageIO] Error: Directory does not exist: " << directory << std::endl;
        return imagePaths;
    }

    if (!fs::is_directory(directory)) {
        std::cerr << "[ImageIO] Error: Path is not a directory: " << directory << std::endl;
        return imagePaths;
    }

    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (hasValidExtension(filename, extensions)) {
                    imagePaths.push_back(entry.path().string());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ImageIO] Error: Failed to iterate directory: " << e.what() << std::endl;
        return imagePaths;
    }

    // ファイル名でソート
    std::sort(imagePaths.begin(), imagePaths.end());

    std::cout << "[ImageIO] Found " << imagePaths.size() << " image files in " << directory << std::endl;

    return imagePaths;
}

bool ImageIO::hasValidExtension(const std::string& filename,
                                const std::vector<std::string>& extensions) {
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    for (const auto& ext : extensions) {
        std::string lowerExt = ext;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (lowerFilename.length() >= lowerExt.length()) {
            if (lowerFilename.compare(lowerFilename.length() - lowerExt.length(),
                                     lowerExt.length(), lowerExt) == 0) {
                return true;
            }
        }
    }

    return false;
}

} // namespace inspection
