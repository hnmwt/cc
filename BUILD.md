# ビルドガイド

## 必要な依存ライブラリ

このプロジェクトをビルドするには、以下のライブラリが必要です：

### 必須ライブラリ

1. **CMake** (3.15以上)
   - ビルドシステム

2. **OpenCV** (4.x)
   - 画像処理ライブラリ

3. **Boost** (1.70以上)
   - Boost.Asio (TCP/UDP通信用)
   - Boost.System

4. **nlohmann/json** (3.9以上)
   - JSON解析ライブラリ

5. **spdlog**
   - ロギングライブラリ

6. **Crow** (オプション: CMakeが自動ダウンロード)
   - HTTP/WebSocketサーバーライブラリ

## macOSでのインストール

### Homebrewを使用

```bash
# Homebrewをインストール（未インストールの場合）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# CMakeをインストール
brew install cmake

# OpenCV 4をインストール
brew install opencv

# Boostをインストール
brew install boost

# nlohmann-jsonをインストール
brew install nlohmann-json

# spdlogをインストール
brew install spdlog
```

### インストール確認

```bash
# CMakeのバージョン確認
cmake --version

# OpenCVの確認
pkg-config --modversion opencv4

# Boostの確認
brew list boost

# nlohmann-jsonの確認
brew list nlohmann-json

# spdlogの確認
brew list spdlog
```

## Linuxでのインストール (Ubuntu/Debian)

```bash
# CMakeをインストール
sudo apt update
sudo apt install cmake

# OpenCVをインストール
sudo apt install libopencv-dev

# Boostをインストール
sudo apt install libboost-all-dev

# nlohmann-jsonをインストール
sudo apt install nlohmann-json3-dev

# spdlogをインストール
sudo apt install libspdlog-dev
```

## ビルド手順

### 1. ビルドディレクトリを作成

```bash
mkdir build
cd build
```

### 2. CMakeで設定

```bash
cmake ..
```

#### ビルドタイプの指定

**Debugビルド（開発用）:**
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

**Releaseビルド（本番用）:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### 3. ビルド実行

```bash
cmake --build .
```

または

```bash
make -j4  # 4並列でビルド
```

### 4. 実行

```bash
./bin/inspection_app
```

画像処理のテスト:
```bash
./bin/inspection_app ../data/input/sample.jpg
```

## テストのビルド（オプション）

```bash
cmake -DBUILD_TESTS=ON ..
cmake --build .
ctest
```

## トラブルシューティング

### OpenCVが見つからない

**macOS:**
```bash
# OpenCVのパスを確認
brew info opencv

# CMakeに明示的にパスを指定
cmake -DOpenCV_DIR=/opt/homebrew/opt/opencv/lib/cmake/opencv4 ..
```

**Linux:**
```bash
# pkg-configのパスを確認
pkg-config --cflags opencv4
pkg-config --libs opencv4
```

### Boostが見つからない

```bash
# Boostのパスを確認
brew info boost  # macOS
dpkg -L libboost-dev  # Linux

# CMakeに明示的にパスを指定
cmake -DBOOST_ROOT=/opt/homebrew/opt/boost ..
```

### nlohmann-jsonが見つからない

```bash
# macOS
cmake -Dnlohmann_json_DIR=/opt/homebrew/opt/nlohmann-json/lib/cmake/nlohmann_json ..
```

### spdlogが見つからない

```bash
# macOS
cmake -Dspdlog_DIR=/opt/homebrew/opt/spdlog/lib/cmake/spdlog ..
```

## ビルド成果物

ビルドが成功すると、以下のファイルが生成されます：

```
build/
├── bin/
│   └── inspection_app        # 実行ファイル
├── lib/                       # ライブラリ（あれば）
└── ...
```

## 実行前の準備

### 1. ディレクトリ構造の確認

```bash
data/
├── input/       # 入力画像を配置
├── output/      # 出力画像が保存される
└── reference/   # 良品画像を配置

config/
└── default_config.json  # 設定ファイル

logs/            # ログファイルが保存される
```

### 2. テスト画像の準備

```bash
# サンプル画像を配置
cp /path/to/your/image.jpg data/input/sample.jpg
```

### 3. 実行

```bash
cd build
./bin/inspection_app ../data/input/sample.jpg
```

## 開発時のヒント

### デバッグビルド

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# デバッガで実行
lldb ./bin/inspection_app  # macOS
gdb ./bin/inspection_app   # Linux
```

### クリーンビルド

```bash
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

### ログレベルの変更

設定ファイル `config/default_config.json` で変更:

```json
{
  "application": {
    "log_level": "debug"  // trace, debug, info, warn, error, critical
  }
}
```

または、コード内で:

```cpp
Logger::init(Logger::Level::Debug);
```
