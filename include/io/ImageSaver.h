#ifndef IMAGE_SAVER_H
#define IMAGE_SAVER_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "InspectionController.h"

namespace inspection {

/**
 * @brief 画像保存クラス
 *
 * 検査結果に関連する画像（元画像、処理済み画像、可視化画像）を
 * 自動的に保存します。
 */
class ImageSaver {
public:
    /**
     * @brief 保存する画像の種類
     */
    enum class ImageType {
        Original,       ///< 元画像
        Processed,      ///< 前処理済み画像
        Visualized,     ///< 欠陥マーキング済み画像
        All            ///< すべて
    };

    /**
     * @brief コンストラクタ
     * @param outputDir 出力ディレクトリ
     * @param filenamePrefix ファイル名プレフィックス（デフォルト: "inspection"）
     */
    explicit ImageSaver(
        const std::string& outputDir,
        const std::string& filenamePrefix = "inspection"
    );

    /**
     * @brief デストラクタ
     */
    ~ImageSaver() = default;

    /**
     * @brief 検査結果の画像を保存
     * @param result 検査結果
     * @param imageTypes 保存する画像の種類
     * @return 成功した場合true
     */
    bool saveImages(
        const InspectionResult& result,
        ImageType imageTypes = ImageType::All
    );

    /**
     * @brief 元画像を保存
     * @param image 元画像
     * @param filename ファイル名（空の場合は自動生成）
     * @return 保存したファイルのパス（失敗時は空文字列）
     */
    std::string saveOriginal(const cv::Mat& image, const std::string& filename = "");

    /**
     * @brief 処理済み画像を保存
     * @param image 処理済み画像
     * @param filename ファイル名（空の場合は自動生成）
     * @return 保存したファイルのパス（失敗時は空文字列）
     */
    std::string saveProcessed(const cv::Mat& image, const std::string& filename = "");

    /**
     * @brief 可視化画像を保存
     * @param image 可視化画像
     * @param filename ファイル名（空の場合は自動生成）
     * @return 保存したファイルのパス（失敗時は空文字列）
     */
    std::string saveVisualized(const cv::Mat& image, const std::string& filename = "");

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
     * @brief サブディレクトリ作成を有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setCreateSubdirectories(bool enabled) {
        createSubdirectories_ = enabled;
    }

    /**
     * @brief サブディレクトリ作成が有効か
     * @return 有効な場合true
     */
    bool isCreateSubdirectoriesEnabled() const {
        return createSubdirectories_;
    }

    /**
     * @brief タイムスタンプ付きファイル名を有効/無効にする
     * @param enabled 有効にする場合true
     */
    void setTimestampEnabled(bool enabled) {
        useTimestamp_ = enabled;
    }

    /**
     * @brief タイムスタンプ付きファイル名が有効か
     * @return 有効な場合true
     */
    bool isTimestampEnabled() const {
        return useTimestamp_;
    }

    /**
     * @brief JPEG品質を設定
     * @param quality 品質（0-100）
     */
    void setJpegQuality(int quality) {
        if (quality >= 0 && quality <= 100) {
            jpegQuality_ = quality;
        }
    }

    /**
     * @brief JPEG品質を取得
     * @return 品質（0-100）
     */
    int getJpegQuality() const {
        return jpegQuality_;
    }

    /**
     * @brief PNG圧縮レベルを設定
     * @param level 圧縮レベル（0-9）
     */
    void setPngCompression(int level) {
        if (level >= 0 && level <= 9) {
            pngCompression_ = level;
        }
    }

    /**
     * @brief PNG圧縮レベルを取得
     * @return 圧縮レベル（0-9）
     */
    int getPngCompression() const {
        return pngCompression_;
    }

    /**
     * @brief 画像フォーマットを設定
     * @param format ファイル拡張子（例: "jpg", "png"）
     */
    void setImageFormat(const std::string& format) {
        imageFormat_ = format;
    }

    /**
     * @brief 画像フォーマットを取得
     * @return ファイル拡張子
     */
    std::string getImageFormat() const {
        return imageFormat_;
    }

    /**
     * @brief 最後に保存した画像のパスを取得
     * @return 画像パスのリスト
     */
    std::vector<std::string> getLastSavedFiles() const {
        return lastSavedFiles_;
    }

    /**
     * @brief タイムスタンプ付きファイル名を生成
     * @param type 画像タイプの文字列
     * @return 生成されたファイル名
     */
    std::string generateFilename(const std::string& type) const;

private:
    /**
     * @brief 画像を保存
     * @param image 保存する画像
     * @param subdir サブディレクトリ名
     * @param type 画像タイプの文字列
     * @param filename ファイル名（空の場合は自動生成）
     * @return 保存したファイルのパス（失敗時は空文字列）
     */
    std::string saveImage(
        const cv::Mat& image,
        const std::string& subdir,
        const std::string& type,
        const std::string& filename = ""
    );

    /**
     * @brief 出力ディレクトリが存在するか確認し、なければ作成
     * @param path ディレクトリパス
     * @return 成功した場合true
     */
    bool ensureDirectory(const std::string& path);

    /**
     * @brief 保存パラメータを取得
     * @return OpenCV用の保存パラメータ
     */
    std::vector<int> getSaveParams() const;

    std::string outputDir_;              ///< 出力ディレクトリ
    std::string filenamePrefix_;         ///< ファイル名プレフィックス
    std::string imageFormat_;            ///< 画像フォーマット（拡張子）
    bool createSubdirectories_;          ///< サブディレクトリを作成するか
    bool useTimestamp_;                  ///< タイムスタンプを使うか
    int jpegQuality_;                    ///< JPEG品質（0-100）
    int pngCompression_;                 ///< PNG圧縮レベル（0-9）
    std::vector<std::string> lastSavedFiles_;  ///< 最後に保存したファイルのリスト
};

} // namespace inspection

#endif // IMAGE_SAVER_H
