#ifndef IMAGEIO_H
#define IMAGEIO_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <optional>

namespace inspection {

/**
 * @brief 画像の読み込み・保存を担当するクラス
 *
 * OpenCVを使用して画像ファイルの入出力を行います。
 * 対応形式: JPEG, PNG, BMP, TIFF
 */
class ImageIO {
public:
    /**
     * @brief 画像ファイルを読み込む
     * @param path 画像ファイルのパス
     * @param flags 読み込みフラグ (cv::IMREAD_COLOR, cv::IMREAD_GRAYSCALE など)
     * @return 読み込んだ画像。失敗時は空のMat
     */
    static cv::Mat loadImage(const std::string& path,
                             int flags = cv::IMREAD_COLOR);

    /**
     * @brief 画像ファイルを保存する
     * @param image 保存する画像
     * @param path 保存先パス
     * @param params 保存パラメータ (JPEG品質など)
     * @return 成功時true、失敗時false
     */
    static bool saveImage(const cv::Mat& image,
                         const std::string& path,
                         const std::vector<int>& params = std::vector<int>());

    /**
     * @brief 複数の画像ファイルを一括読み込み (バッチ処理)
     * @param paths 画像ファイルパスのリスト
     * @param flags 読み込みフラグ
     * @return 読み込んだ画像のリスト (失敗したものは空のMat)
     */
    static std::vector<cv::Mat> loadBatch(const std::vector<std::string>& paths,
                                          int flags = cv::IMREAD_COLOR);

    /**
     * @brief ディレクトリ内の画像ファイルを全て読み込む
     * @param directory ディレクトリパス
     * @param extensions 対象とする拡張子のリスト (例: {".jpg", ".png"})
     * @param flags 読み込みフラグ
     * @return 読み込んだ画像のリスト
     */
    static std::vector<cv::Mat> loadDirectory(const std::string& directory,
                                              const std::vector<std::string>& extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff"},
                                              int flags = cv::IMREAD_COLOR);

    /**
     * @brief 複数の画像を連番で保存
     * @param images 保存する画像のリスト
     * @param outputDir 出力ディレクトリ
     * @param prefix ファイル名のプレフィックス
     * @param extension 拡張子 (例: ".jpg")
     * @return 保存に成功したファイル数
     */
    static int saveBatch(const std::vector<cv::Mat>& images,
                        const std::string& outputDir,
                        const std::string& prefix = "image",
                        const std::string& extension = ".jpg");

    /**
     * @brief 画像が有効かどうかを確認
     * @param image 確認する画像
     * @return 有効な場合true
     */
    static bool isValid(const cv::Mat& image);

    /**
     * @brief ディレクトリ内の画像ファイルパスを取得
     * @param directory ディレクトリパス
     * @param extensions 対象とする拡張子のリスト
     * @return 画像ファイルパスのリスト
     */
    static std::vector<std::string> getImagePaths(const std::string& directory,
                                                   const std::vector<std::string>& extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff"});

private:
    /**
     * @brief ファイルの拡張子が指定されたリストに含まれるか確認
     * @param filename ファイル名
     * @param extensions 拡張子のリスト
     * @return 含まれる場合true
     */
    static bool hasValidExtension(const std::string& filename,
                                   const std::vector<std::string>& extensions);
};

} // namespace inspection

#endif // IMAGEIO_H
