# 外観検査アプリケーション

製造現場における製品の外観検査を自動化・効率化するためのC++アプリケーションです。

## 🎯 プロジェクト概要

このアプリケーションは画像処理技術を用いて、製品の傷、汚れ、変色などの欠陥を自動検出します。

### 主要機能

- 📸 **画像入力**: カメラ/ファイルからの画像取得
- 🔍 **画像処理**: 複数フィルタのパイプライン処理
- 🤖 **欠陥検出**: テンプレートマッチング、特徴量ベース検出
- 🌐 **外部機器連携**: PLC等からのトリガー受信
- 📊 **データ出力**: CSV自動出力、画像保存
- 🔄 **React対応**: WebSocket + REST APIによるフロントエンド連携

## 📂 プロジェクト構成

```
inspection_app/
├── include/           # ヘッダーファイル
├── src/              # ソースファイル
├── config/           # 設定ファイル
├── data/             # データディレクトリ
├── tests/            # テストコード
├── BUILD.md          # ビルド手順
├── IMPLEMENTATION_STATUS.md  # 実装状況
└── CMakeLists.txt    # CMake設定
```

## 🚀 クイックスタート

### 1. 依存ライブラリのインストール

**macOS (Homebrew):**
```bash
brew install cmake opencv boost nlohmann-json spdlog
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install cmake libopencv-dev libboost-all-dev nlohmann-json3-dev libspdlog-dev
```

### 2. ビルド

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 3. 実行

```bash
# 基本実行
./bin/inspection_app

# 画像処理のテスト
./bin/inspection_app ../data/input/sample.jpg
```

詳細は [BUILD.md](BUILD.md) を参照してください。

## 📖 ドキュメント

| ドキュメント | 説明 |
|-------------|------|
| [BUILD.md](BUILD.md) | ビルド手順とトラブルシューティング |
| [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) | 実装状況と進捗 |
| [要件.md](要件.md) | 機能要件・非機能要件 |
| [実装計画.md](実装計画.md) | 10フェーズの実装計画 |
| [通信フォーマット設計.md](通信フォーマット設計.md) | API・WebSocket仕様 |
| [CSV出力仕様.md](CSV出力仕様.md) | CSV/画像出力仕様 |
| [React移行計画.md](React移行計画.md) | フロントエンド移行計画 |

## 🏗️ アーキテクチャ

### 技術スタック

- **言語**: C++20
- **画像処理**: OpenCV 4.x
- **通信**: Boost.Asio, Crow (HTTP/WebSocket)
- **データ形式**: nlohmann/json
- **ログ**: spdlog

### デザインパターン

- **Singleton**: ConfigManager
- **Strategy**: FilterBase
- **Chain of Responsibility**: Pipeline
- **Prototype**: FilterBase::clone()
- **Factory**: 動的フィルタ生成 (実装予定)

### システムフロー

```
外部機器 (PLC) ──[TCP]──> Backend
                            ├─> カメラ撮影
                            ├─> 画像処理パイプライン
                            ├─> 欠陥検出
                            ├─> CSV自動出力
                            └─> [WebSocket] ──> React Frontend
```

## 📊 実装状況

**進捗率: 36% (9/25タスク完了)**

### ✅ 完了
- CMakeLists.txt
- ディレクトリ構造
- ImageIO (画像I/O)
- ConfigManager (設定管理)
- Logger (ログ)
- FilterBase (フィルタ基底クラス)
- Pipeline (パイプライン)
- GrayscaleFilter
- GaussianFilter

### 🔄 進行中
- Phase 1: 基本機能 (75%完了)

### 📋 予定
- Phase 2: 欠陥検出機能
- Phase 4.5: 外部機器連携・CSV出力

詳細は [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) を参照してください。

## 🔧 開発

### ディレクトリ構造

```
include/
├── filters/          # 画像フィルタ
│   ├── FilterBase.h
│   ├── GrayscaleFilter.h
│   └── GaussianFilter.h
├── io/               # 入出力
│   └── ImageIO.h
├── pipeline/         # パイプライン
│   └── Pipeline.h
└── utils/            # ユーティリティ
    ├── ConfigManager.h
    └── Logger.h
```

### ビルドコマンド

```bash
# Debugビルド
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Releaseビルド
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# テストビルド
cmake -DBUILD_TESTS=ON ..
cmake --build .
ctest
```

### ログレベル設定

`config/default_config.json`:
```json
{
  "application": {
    "log_level": "debug"  // trace, debug, info, warn, error, critical
  }
}
```

## 📝 使用例

### 基本的な画像処理

```cpp
#include "io/ImageIO.h"
#include "filters/GrayscaleFilter.h"
#include "filters/GaussianFilter.h"
#include "pipeline/Pipeline.h"

using namespace inspection;

int main() {
    // 画像を読み込み
    cv::Mat image = ImageIO::loadImage("input.jpg");

    // パイプライン作成
    Pipeline pipeline;
    pipeline.addFilter(std::make_unique<GrayscaleFilter>());
    pipeline.addFilter(std::make_unique<GaussianFilter>(5, 1.5));

    // 処理実行
    cv::Mat result = pipeline.process(image);

    // 保存
    ImageIO::saveImage(result, "output.jpg");

    return 0;
}
```

### 設定管理

```cpp
#include "utils/ConfigManager.h"

auto& config = ConfigManager::getInstance();
config.loadConfig("config/default_config.json");

// 値を取得
auto port = config.getValue<int>("/server/http/port");
auto host = config.getValueOr<std::string>("/server/http/host", "0.0.0.0");

// 値を設定
config.setValue("/server/http/port", 8080);
config.saveConfig("config/custom_config.json");
```

### ロギング

```cpp
#include "utils/Logger.h"

Logger::init(Logger::Level::Info, true, "logs/app.log");

LOG_INFO("Application started");
LOG_DEBUG("Debug information: {}", value);
LOG_WARN("Warning: {}", message);
LOG_ERROR("Error occurred: {}", error);

Logger::shutdown();
```

## 🧪 テスト

```bash
# テストビルド
cd build
cmake -DBUILD_TESTS=ON ..
cmake --build .

# テスト実行
ctest

# または
./inspection_tests
```

## 📄 ライセンス

TBD

## 👥 コントリビューション

TBD

## 📞 サポート

問題が発生した場合:
1. [BUILD.md](BUILD.md) のトラブルシューティングを確認
2. [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) で実装状況を確認
3. ログファイル (`logs/inspection.log`) を確認

---

**バージョン:** 1.0.0
**最終更新:** 2025-11-16
