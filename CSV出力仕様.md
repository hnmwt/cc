# CSV出力仕様書

## 概要

検査完了時に自動的にCSVファイルへ結果を記録し、画像を保存する機能の詳細仕様。

---

## CSV出力仕様

### ファイル形式

- **形式**: CSV (Comma-Separated Values)
- **文字エンコーディング**: UTF-8 BOM付き（Excel対応）
- **改行コード**: CRLF (`\r\n`) Windows/Excel互換
- **区切り文字**: カンマ (`,`)
- **引用符**: ダブルクォート (`"`) ※カンマや改行を含む場合

### ファイル名規則

```
inspection_{日付}.csv
```

**例:**
- `inspection_2025-11-16.csv`
- `inspection_2025-11-17.csv`

### 保存先ディレクトリ

```
data/
└── csv/
    ├── inspection_2025-11-16.csv
    ├── inspection_2025-11-17.csv
    └── ...
```

---

## CSV項目定義

### ヘッダー行

```csv
検査ID,タイムスタンプ,トリガー元,トリガータイプ,製品ID,ライン番号,判定結果,欠陥数,欠陥詳細,処理時間ms,信頼度,元画像パス,処理結果画像パス,マーキング画像パス,パイプライン設定
```

### データ行の例

```csv
insp-00001,2025-11-16 10:30:15.123,plc-line-01,external,PROD-12345,1,NG,2,"scratch(0.98,100,150,50,30);stain(0.92,300,200,40,40)",87,0.95,images/2025-11-16/insp-00001_original.jpg,images/2025-11-16/insp-00001_processed.jpg,images/2025-11-16/insp-00001_marked.jpg,"grayscale;gaussian(5,1.0);threshold(128)"
insp-00002,2025-11-16 10:30:45.678,plc-line-01,external,PROD-12346,1,OK,0,,65,0.99,images/2025-11-16/insp-00002_original.jpg,images/2025-11-16/insp-00002_processed.jpg,images/2025-11-16/insp-00002_marked.jpg,"grayscale;gaussian(5,1.0);threshold(128)"
```

---

## 項目詳細

| 列番号 | 項目名 | データ型 | 必須 | 説明 | 例 |
|-------|--------|---------|-----|------|-----|
| 1 | 検査ID | 文字列 | ✅ | 一意な検査識別子 | `insp-00001` |
| 2 | タイムスタンプ | 日時 | ✅ | 検査実行日時（ミリ秒まで） | `2025-11-16 10:30:15.123` |
| 3 | トリガー元 | 文字列 | ✅ | 撮影指示を出した機器ID | `plc-line-01` |
| 4 | トリガータイプ | 列挙 | ✅ | `external` または `manual` | `external` |
| 5 | 製品ID | 文字列 | ❌ | 外部機器から受信した製品ID | `PROD-12345` |
| 6 | ライン番号 | 整数 | ❌ | 製造ラインの番号 | `1` |
| 7 | 判定結果 | 列挙 | ✅ | `OK` または `NG` | `NG` |
| 8 | 欠陥数 | 整数 | ✅ | 検出された欠陥の数 | `2` |
| 9 | 欠陥詳細 | 文字列 | ❌ | 欠陥情報（後述の形式） | `scratch(0.98,100,150,50,30)` |
| 10 | 処理時間ms | 整数 | ✅ | 検査処理にかかった時間（ミリ秒） | `87` |
| 11 | 信頼度 | 小数 | ✅ | 判定の信頼度（0.0～1.0） | `0.95` |
| 12 | 元画像パス | 文字列 | ✅ | 元画像の相対パス | `images/2025-11-16/insp-00001_original.jpg` |
| 13 | 処理結果画像パス | 文字列 | ✅ | 処理後画像の相対パス | `images/2025-11-16/insp-00001_processed.jpg` |
| 14 | マーキング画像パス | 文字列 | ✅ | 欠陥マーキング画像の相対パス | `images/2025-11-16/insp-00001_marked.jpg` |
| 15 | パイプライン設定 | 文字列 | ✅ | 使用したフィルタの設定 | `grayscale;gaussian(5,1.0)` |

---

## 欠陥詳細の形式

複数の欠陥はセミコロン(`;`)で区切り、各欠陥は以下の形式：

```
欠陥タイプ(信頼度,x,y,width,height)
```

**例:**
```
scratch(0.98,100,150,50,30);stain(0.92,300,200,40,40)
```

**パース例:**
- 欠陥1: タイプ=scratch, 信頼度=0.98, x=100, y=150, width=50, height=30
- 欠陥2: タイプ=stain, 信頼度=0.92, x=300, y=200, width=40, height=40

**欠陥タイプ一覧:**
- `scratch`: 傷
- `stain`: 汚れ
- `discoloration`: 変色
- `deformation`: 形状不良
- `unknown`: 不明

---

## パイプライン設定の形式

フィルタをセミコロン(`;`)で区切り、各フィルタはパラメータをカッコ内に記載：

```
フィルタ名(パラメータ1,パラメータ2,...);フィルタ名(パラメータ);...
```

**例:**
```
grayscale;gaussian(5,1.0);threshold(128);edge_canny(50,150)
```

---

## 画像保存仕様

### ディレクトリ構造

```
data/
└── images/
    ├── 2025-11-16/
    │   ├── insp-00001_original.jpg      # 元画像
    │   ├── insp-00001_processed.jpg     # 処理後画像
    │   ├── insp-00001_marked.jpg        # マーキング画像
    │   ├── insp-00002_original.jpg
    │   ├── insp-00002_processed.jpg
    │   ├── insp-00002_marked.jpg
    │   └── ...
    ├── 2025-11-17/
    │   └── ...
    └── ...
```

### 画像ファイル名規則

```
{検査ID}_{種類}.jpg
```

**種類:**
- `original`: 元画像
- `processed`: 処理後画像（グレースケール、フィルタ適用後等）
- `marked`: 欠陥マーキング画像（矩形・円で欠陥箇所をマーク）

### 画像形式

- **フォーマット**: JPEG
- **品質**: 90%（デフォルト、設定可能）
- **解像度**: 元画像と同じ

### マーキング画像の仕様

- 元画像（カラー）に欠陥箇所を描画
- 欠陥の種類ごとに色分け:
  - 傷 (scratch): 赤色 `RGB(255, 0, 0)`
  - 汚れ (stain): 黄色 `RGB(255, 255, 0)`
  - 変色 (discoloration): 緑色 `RGB(0, 255, 0)`
  - 形状不良 (deformation): 青色 `RGB(0, 0, 255)`
  - 不明 (unknown): 白色 `RGB(255, 255, 255)`
- 矩形の線幅: 3px
- 信頼度をテキストで表示（矩形の上部）

---

## C++実装例

### CSV Writer クラス

```cpp
// csv_writer.hpp
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class CSVWriter {
public:
    CSVWriter(const std::string& outputDir) : outputDir_(outputDir) {}

    void writeInspectionResult(const InspectionResult& result,
                               const std::string& triggerSource,
                               const std::string& productId = "") {
        // ファイル名を日付ごとに生成
        std::string filename = getFilenameForToday();
        std::string filepath = outputDir_ + "/" + filename;

        // ファイルが存在しない場合はヘッダーを書き込む
        bool fileExists = std::filesystem::exists(filepath);

        std::ofstream file(filepath, std::ios::app | std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open CSV file: {}", filepath);
            return;
        }

        // UTF-8 BOM
        if (!fileExists) {
            file << "\xEF\xBB\xBF"; // BOM
            writeHeader(file);
        }

        // データ行を書き込み
        writeDataRow(file, result, triggerSource, productId);

        file.close();
        LOG_INFO("CSV written: {}", filepath);
    }

private:
    std::string outputDir_;

    std::string getFilenameForToday() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "inspection_" << std::put_time(std::localtime(&time), "%Y-%m-%d") << ".csv";
        return ss.str();
    }

    void writeHeader(std::ofstream& file) {
        file << "検査ID,タイムスタンプ,トリガー元,トリガータイプ,製品ID,ライン番号,"
             << "判定結果,欠陥数,欠陥詳細,処理時間ms,信頼度,"
             << "元画像パス,処理結果画像パス,マーキング画像パス,パイプライン設定\r\n";
    }

    void writeDataRow(std::ofstream& file,
                      const InspectionResult& result,
                      const std::string& triggerSource,
                      const std::string& productId) {
        // 検査ID
        file << result.id << ",";

        // タイムスタンプ
        file << formatTimestamp(result.timestamp) << ",";

        // トリガー元
        file << escapeCsv(triggerSource) << ",";

        // トリガータイプ
        file << escapeCsv(result.triggerType) << ",";

        // 製品ID
        file << escapeCsv(productId) << ",";

        // ライン番号（仮に0）
        file << "0,";

        // 判定結果
        file << (result.isOK ? "OK" : "NG") << ",";

        // 欠陥数
        file << result.defects.size() << ",";

        // 欠陥詳細
        file << "\"" << formatDefects(result.defects) << "\",";

        // 処理時間ms
        file << result.processingTime << ",";

        // 信頼度
        file << result.confidence << ",";

        // 画像パス
        file << escapeCsv(result.originalImagePath) << ",";
        file << escapeCsv(result.processedImagePath) << ",";
        file << escapeCsv(result.markedImagePath) << ",";

        // パイプライン設定
        file << "\"" << formatPipeline(result.pipelineConfig) << "\"";

        file << "\r\n";
    }

    std::string escapeCsv(const std::string& str) {
        if (str.find(',') != std::string::npos ||
            str.find('"') != std::string::npos ||
            str.find('\n') != std::string::npos) {
            std::string escaped = str;
            // ダブルクォートをエスケープ
            size_t pos = 0;
            while ((pos = escaped.find('"', pos)) != std::string::npos) {
                escaped.replace(pos, 1, "\"\"");
                pos += 2;
            }
            return "\"" + escaped + "\"";
        }
        return str;
    }

    std::string formatDefects(const std::vector<Defect>& defects) {
        if (defects.empty()) return "";

        std::stringstream ss;
        for (size_t i = 0; i < defects.size(); ++i) {
            const auto& d = defects[i];
            ss << defectTypeToString(d.type) << "("
               << d.confidence << ","
               << d.boundingBox.x << ","
               << d.boundingBox.y << ","
               << d.boundingBox.width << ","
               << d.boundingBox.height << ")";

            if (i < defects.size() - 1) {
                ss << ";";
            }
        }
        return ss.str();
    }

    std::string formatPipeline(const std::vector<FilterConfig>& pipeline) {
        std::stringstream ss;
        for (size_t i = 0; i < pipeline.size(); ++i) {
            const auto& filter = pipeline[i];
            ss << filter.name;

            if (!filter.params.empty()) {
                ss << "(";
                // パラメータを出力
                bool first = true;
                for (const auto& [key, value] : filter.params) {
                    if (!first) ss << ",";
                    ss << value;
                    first = false;
                }
                ss << ")";
            }

            if (i < pipeline.size() - 1) {
                ss << ";";
            }
        }
        return ss.str();
    }

    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
        auto time = std::chrono::system_clock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string defectTypeToString(DefectType type) {
        switch (type) {
            case DefectType::SCRATCH: return "scratch";
            case DefectType::STAIN: return "stain";
            case DefectType::DISCOLORATION: return "discoloration";
            case DefectType::DEFORMATION: return "deformation";
            default: return "unknown";
        }
    }
};
```

### 画像保存クラス

```cpp
// image_saver.hpp
class ImageSaver {
public:
    ImageSaver(const std::string& outputDir, int jpegQuality = 90)
        : outputDir_(outputDir), jpegQuality_(jpegQuality) {}

    struct SavedImages {
        std::string originalPath;
        std::string processedPath;
        std::string markedPath;
    };

    SavedImages saveInspectionImages(const std::string& inspectionId,
                                     const cv::Mat& original,
                                     const cv::Mat& processed,
                                     const std::vector<Defect>& defects) {
        // 日付ごとのディレクトリ作成
        std::string dateDir = getDateDirectory();
        std::filesystem::create_directories(dateDir);

        SavedImages paths;

        // 元画像を保存
        paths.originalPath = saveImage(original, dateDir, inspectionId, "original");

        // 処理後画像を保存
        paths.processedPath = saveImage(processed, dateDir, inspectionId, "processed");

        // マーキング画像を生成・保存
        cv::Mat marked = createMarkedImage(original, defects);
        paths.markedPath = saveImage(marked, dateDir, inspectionId, "marked");

        return paths;
    }

private:
    std::string outputDir_;
    int jpegQuality_;

    std::string getDateDirectory() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << outputDir_ << "/" << std::put_time(std::localtime(&time), "%Y-%m-%d");
        return ss.str();
    }

    std::string saveImage(const cv::Mat& image,
                         const std::string& dir,
                         const std::string& inspectionId,
                         const std::string& suffix) {
        std::string filename = inspectionId + "_" + suffix + ".jpg";
        std::string filepath = dir + "/" + filename;

        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, jpegQuality_};
        cv::imwrite(filepath, image, params);

        // 相対パスを返す（CSVに記録用）
        return "images/" + std::filesystem::path(dir).filename().string() + "/" + filename;
    }

    cv::Mat createMarkedImage(const cv::Mat& original,
                             const std::vector<Defect>& defects) {
        cv::Mat marked = original.clone();

        // カラー画像に変換（グレースケールの場合）
        if (marked.channels() == 1) {
            cv::cvtColor(marked, marked, cv::COLOR_GRAY2BGR);
        }

        for (const auto& defect : defects) {
            // 欠陥タイプに応じた色
            cv::Scalar color = getDefectColor(defect.type);

            // 矩形を描画
            cv::rectangle(marked, defect.boundingBox, color, 3);

            // 信頼度をテキストで表示
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << (defect.confidence * 100) << "%";
            cv::putText(marked, ss.str(),
                       cv::Point(defect.boundingBox.x, defect.boundingBox.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }

        return marked;
    }

    cv::Scalar getDefectColor(DefectType type) {
        switch (type) {
            case DefectType::SCRATCH:       return cv::Scalar(0, 0, 255);     // 赤
            case DefectType::STAIN:         return cv::Scalar(0, 255, 255);   // 黄
            case DefectType::DISCOLORATION: return cv::Scalar(0, 255, 0);     // 緑
            case DefectType::DEFORMATION:   return cv::Scalar(255, 0, 0);     // 青
            default:                        return cv::Scalar(255, 255, 255); // 白
        }
    }
};
```

### 統合例（InspectionServerに組み込み）

```cpp
void InspectionServer::handleExternalTrigger(const TriggerEvent& event) {
    // 1. カメラから画像取得
    cv::Mat image = cameraController_.captureFrame();

    // 2. 検査実行
    auto result = inspectionController_.inspect(image);

    // 3. 画像保存
    ImageSaver imageSaver("data/images");
    auto savedImages = imageSaver.saveInspectionImages(
        result.id, image, result.processedImage, result.defects);

    result.originalImagePath = savedImages.originalPath;
    result.processedImagePath = savedImages.processedPath;
    result.markedImagePath = savedImages.markedPath;
    result.triggerType = "external";

    // 4. CSV出力
    CSVWriter csvWriter("data/csv");
    csvWriter.writeInspectionResult(result, event.deviceId, event.productId);

    // 5. データベース保存
    dataManager_.saveInspectionResult(result);

    // 6. WebSocket通知
    broadcastWebSocket(createNotification(result));
}
```

---

## 設定例（config.json）

```json
{
  "csv_output": {
    "enabled": true,
    "output_dir": "data/csv",
    "encoding": "utf-8-bom"
  },
  "image_output": {
    "enabled": true,
    "output_dir": "data/images",
    "jpeg_quality": 90,
    "save_ng_only": false,
    "save_original": true,
    "save_processed": true,
    "save_marked": true
  }
}
```

---

## Excel での読み込み例

1. Excelを起動
2. 「データ」→「テキストファイルから」を選択
3. CSVファイルを選択
4. エンコーディング: UTF-8（BOM付きなので自動認識）
5. 区切り文字: カンマ
6. インポート

---

## まとめ

### CSV出力の特徴

- ✅ 検査完了時に自動出力
- ✅ 日付ごとにファイル分割
- ✅ UTF-8 BOM付き（Excel対応）
- ✅ 詳細な欠陥情報を記録
- ✅ 画像パスも記録

### 画像保存の特徴

- ✅ 元画像・処理後画像・マーキング画像の3種類
- ✅ 日付ごとにディレクトリ分割
- ✅ 欠陥を色分けして可視化
- ✅ 信頼度をテキスト表示

---

**最終更新**: 2025-11-16
