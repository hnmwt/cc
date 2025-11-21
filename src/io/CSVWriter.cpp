#include "io/CSVWriter.h"
#include "utils/Logger.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace inspection {

CSVWriter::CSVWriter(const std::string& outputDir, const std::string& filenamePrefix)
    : outputDir_(outputDir),
      filenamePrefix_(filenamePrefix),
      autoFilename_(true),
      includeDefectDetails_(true),
      lastWrittenFile_("")
{
}

bool CSVWriter::writeResult(const InspectionResult& result, const std::string& imagePath) {
    if (!ensureOutputDirectory()) {
        LOG_ERROR("Failed to create output directory: {}", outputDir_);
        return false;
    }

    std::string csvPath;
    if (autoFilename_) {
        csvPath = outputDir_ + "/" + generateFilename();
    } else {
        csvPath = outputDir_ + "/" + filenamePrefix_ + ".csv";
    }

    std::ofstream ofs(csvPath, std::ios::binary);
    if (!ofs) {
        LOG_ERROR("Failed to open CSV file: {}", csvPath);
        return false;
    }

    // UTF-8 BOMを書き込む
    writeBOM(ofs);

    // ヘッダーを書き込む
    writeHeader(ofs, includeDefectDetails_);

    // データを書き込む
    if (includeDefectDetails_) {
        writeDefectRows(ofs, result, imagePath);
    } else {
        writeSummaryRow(ofs, result, imagePath);
    }

    ofs.close();
    lastWrittenFile_ = csvPath;
    LOG_INFO("CSV file written: {}", csvPath);
    return true;
}

bool CSVWriter::writeResults(
    const std::vector<InspectionResult>& results,
    const std::vector<std::string>& imagePaths
) {
    if (!ensureOutputDirectory()) {
        LOG_ERROR("Failed to create output directory: {}", outputDir_);
        return false;
    }

    std::string csvPath;
    if (autoFilename_) {
        csvPath = outputDir_ + "/" + generateFilename();
    } else {
        csvPath = outputDir_ + "/" + filenamePrefix_ + ".csv";
    }

    std::ofstream ofs(csvPath, std::ios::binary);
    if (!ofs) {
        LOG_ERROR("Failed to open CSV file: {}", csvPath);
        return false;
    }

    // UTF-8 BOMを書き込む
    writeBOM(ofs);

    // ヘッダーを書き込む
    writeHeader(ofs, includeDefectDetails_);

    // 各結果を書き込む
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        std::string imagePath = (i < imagePaths.size()) ? imagePaths[i] : "";

        if (includeDefectDetails_) {
            writeDefectRows(ofs, result, imagePath);
        } else {
            writeSummaryRow(ofs, result, imagePath);
        }
    }

    ofs.close();
    lastWrittenFile_ = csvPath;
    LOG_INFO("CSV file written with {} results: {}", results.size(), csvPath);
    return true;
}

bool CSVWriter::appendResult(
    const InspectionResult& result,
    const std::string& imagePath,
    const std::string& csvPath
) {
    // ファイルが存在しない場合は新規作成
    if (!fs::exists(csvPath)) {
        if (!createNewCSV(csvPath)) {
            return false;
        }
    }

    std::ofstream ofs(csvPath, std::ios::binary | std::ios::app);
    if (!ofs) {
        LOG_ERROR("Failed to open CSV file for append: {}", csvPath);
        return false;
    }

    // データを追記
    if (includeDefectDetails_) {
        writeDefectRows(ofs, result, imagePath);
    } else {
        writeSummaryRow(ofs, result, imagePath);
    }

    ofs.close();
    lastWrittenFile_ = csvPath;
    return true;
}

bool CSVWriter::createNewCSV(const std::string& csvPath) {
    std::ofstream ofs(csvPath, std::ios::binary);
    if (!ofs) {
        LOG_ERROR("Failed to create CSV file: {}", csvPath);
        return false;
    }

    // UTF-8 BOMを書き込む
    writeBOM(ofs);

    // ヘッダーを書き込む
    writeHeader(ofs, includeDefectDetails_);

    ofs.close();
    LOG_INFO("New CSV file created: {}", csvPath);
    return true;
}

void CSVWriter::setOutputDirectory(const std::string& outputDir) {
    outputDir_ = outputDir;
}

std::string CSVWriter::generateFilename() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << filenamePrefix_ << "_"
       << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S")
       << ".csv";

    return ss.str();
}

void CSVWriter::writeBOM(std::ofstream& ofs) {
    // UTF-8 BOM: EF BB BF
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    ofs.write(reinterpret_cast<const char*>(bom), sizeof(bom));
}

void CSVWriter::writeHeader(std::ofstream& ofs, bool includeDefects) {
    if (includeDefects) {
        // 欠陥詳細を含むヘッダー
        ofs << "Timestamp,Image Path,Judgment,Total Defects,Processing Time (ms),"
            << "Defect Index,Defect Type,Confidence,X,Y,Width,Height,Area,Circularity\n";
    } else {
        // サマリーのみのヘッダー
        ofs << "Timestamp,Image Path,Judgment,Total Defects,Processing Time (ms)\n";
    }
}

void CSVWriter::writeSummaryRow(
    std::ofstream& ofs,
    const InspectionResult& result,
    const std::string& imagePath
) {
    ofs << escapeCSV(result.timestamp) << ","
        << escapeCSV(imagePath) << ","
        << (result.isOK ? "OK" : "NG") << ","
        << result.defects.size() << ","
        << result.totalTime << "\n";
}

void CSVWriter::writeDefectRows(
    std::ofstream& ofs,
    const InspectionResult& result,
    const std::string& imagePath
) {
    if (result.defects.empty()) {
        // 欠陥がない場合も1行書き込む
        ofs << escapeCSV(result.timestamp) << ","
            << escapeCSV(imagePath) << ","
            << (result.isOK ? "OK" : "NG") << ","
            << "0,"
            << result.totalTime << ","
            << ",,,,,,,,\n";  // 欠陥情報は空
    } else {
        // 各欠陥について行を書き込む
        for (size_t i = 0; i < result.defects.size(); ++i) {
            const auto& defect = result.defects[i];

            ofs << escapeCSV(result.timestamp) << ","
                << escapeCSV(imagePath) << ","
                << (result.isOK ? "OK" : "NG") << ","
                << result.defects.size() << ","
                << result.totalTime << ","
                << i << ","
                << escapeCSV(defect.getTypeString()) << ","
                << defect.confidence << ","
                << defect.bbox.x << ","
                << defect.bbox.y << ","
                << defect.bbox.width << ","
                << defect.bbox.height << ","
                << defect.area << ","
                << defect.circularity << "\n";
        }
    }
}

std::string CSVWriter::escapeCSV(const std::string& str) const {
    if (str.find(',') != std::string::npos ||
        str.find('"') != std::string::npos ||
        str.find('\n') != std::string::npos) {

        std::string escaped = "\"";
        for (char c : str) {
            if (c == '"') {
                escaped += "\"\"";  // ダブルクォートをエスケープ
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }
    return str;
}

bool CSVWriter::ensureOutputDirectory() {
    try {
        if (!fs::exists(outputDir_)) {
            fs::create_directories(outputDir_);
            LOG_INFO("Created output directory: {}", outputDir_);
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create output directory: {}", e.what());
        return false;
    }
}

} // namespace inspection
