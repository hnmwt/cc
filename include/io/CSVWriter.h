#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include "InspectionController.h"

namespace inspection {

/**
 * @brief CSV出力クラス
 *
 * 検査結果をCSV形式で出力します。
 * UTF-8 BOM付きで出力し、Excel互換性を確保します。
 */
class CSVWriter {
public:
    /**
     * @brief コンストラクタ
     * @param outputDir 出力ディレクトリ
     * @param filenamePrefix ファイル名プレフィックス（デフォルト: "inspection_result"）
     */
    explicit CSVWriter(
        const std::string& outputDir,
        const std::string& filenamePrefix = "inspection_result"
    );

    /**
     * @brief デストラクタ
     */
    ~CSVWriter() = default;

    /**
     * @brief 検査結果をCSVファイルに書き込む
     * @param result 検査結果
     * @param imagePath 元画像のパス
     * @return 成功した場合true
     */
    bool writeResult(const InspectionResult& result, const std::string& imagePath = "");

    /**
     * @brief 複数の検査結果をCSVファイルに書き込む
     * @param results 検査結果のリスト
     * @param imagePaths 元画像のパスのリスト
     * @return 成功した場合true
     */
    bool writeResults(
        const std::vector<InspectionResult>& results,
        const std::vector<std::string>& imagePaths = {}
    );

    /**
     * @brief 検査結果を既存のCSVファイルに追記
     * @param result 検査結果
     * @param imagePath 元画像のパス
     * @param csvPath CSVファイルのパス
     * @return 成功した場合true
     */
    bool appendResult(
        const InspectionResult& result,
        const std::string& imagePath,
        const std::string& csvPath
    );

    /**
     * @brief 新しいCSVファイルを作成（ヘッダー行のみ）
     * @param csvPath CSVファイルのパス
     * @return 成功した場合true
     */
    bool createNewCSV(const std::string& csvPath);

    /**
     * @brief 出力ディレクトリを設定
     * @param outputDir 出力ディレクトリ
     */
    void setOutputDirectory(const std::string& outputDir);

    /**
     * @brief 出力ディレクトリを取得
     * @return 出力ディレクトリ
     */
    std::string getOutputDirectory() const {
        return outputDir_;
    }

    /**
     * @brief ファイル名プレフィックスを設定
     * @param prefix ファイル名プレフィックス
     */
    void setFilenamePrefix(const std::string& prefix) {
        filenamePrefix_ = prefix;
    }

    /**
     * @brief ファイル名プレフィックスを取得
     * @return ファイル名プレフィックス
     */
    std::string getFilenamePrefix() const {
        return filenamePrefix_;
    }

    /**
     * @brief 自動ファイル名生成を有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setAutoFilenameEnabled(bool enabled) {
        autoFilename_ = enabled;
    }

    /**
     * @brief 自動ファイル名生成が有効か
     * @return 有効な場合true
     */
    bool isAutoFilenameEnabled() const {
        return autoFilename_;
    }

    /**
     * @brief 欠陥詳細の出力を有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setDefectDetailsEnabled(bool enabled) {
        includeDefectDetails_ = enabled;
    }

    /**
     * @brief 欠陥詳細の出力が有効か
     * @return 有効な場合true
     */
    bool isDefectDetailsEnabled() const {
        return includeDefectDetails_;
    }

    /**
     * @brief タイムスタンプ付きファイル名を生成
     * @return 生成されたファイル名
     */
    std::string generateFilename() const;

    /**
     * @brief 最後に書き込んだCSVファイルのパスを取得
     * @return CSVファイルのパス
     */
    std::string getLastWrittenFile() const {
        return lastWrittenFile_;
    }

private:
    /**
     * @brief UTF-8 BOMを書き込む
     * @param ofs 出力ストリーム
     */
    void writeBOM(std::ofstream& ofs);

    /**
     * @brief CSVヘッダー行を書き込む
     * @param ofs 出力ストリーム
     * @param includeDefects 欠陥詳細を含めるか
     */
    void writeHeader(std::ofstream& ofs, bool includeDefects);

    /**
     * @brief 検査結果のサマリー行を書き込む
     * @param ofs 出力ストリーム
     * @param result 検査結果
     * @param imagePath 元画像のパス
     */
    void writeSummaryRow(
        std::ofstream& ofs,
        const InspectionResult& result,
        const std::string& imagePath
    );

    /**
     * @brief 欠陥詳細行を書き込む
     * @param ofs 出力ストリーム
     * @param result 検査結果
     * @param imagePath 元画像のパス
     */
    void writeDefectRows(
        std::ofstream& ofs,
        const InspectionResult& result,
        const std::string& imagePath
    );

    /**
     * @brief 文字列をCSVエスケープ
     * @param str エスケープする文字列
     * @return エスケープされた文字列
     */
    std::string escapeCSV(const std::string& str) const;

    /**
     * @brief 出力ディレクトリが存在するか確認し、なければ作成
     * @return 成功した場合true
     */
    bool ensureOutputDirectory();

    std::string outputDir_;              ///< 出力ディレクトリ
    std::string filenamePrefix_;         ///< ファイル名プレフィックス
    bool autoFilename_;                  ///< 自動ファイル名生成を使うか
    bool includeDefectDetails_;          ///< 欠陥詳細を含めるか
    std::string lastWrittenFile_;        ///< 最後に書き込んだファイルのパス
};

} // namespace inspection

#endif // CSV_WRITER_H
