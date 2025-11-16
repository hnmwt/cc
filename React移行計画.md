# React移行を考慮した設計

## 概要

現在はQt 6.xでデスクトップGUIを構築しますが、将来的にReactへの移行を考慮した設計を採用します。

---

## アーキテクチャ戦略

### 1. **レイヤー分離アーキテクチャ**

```
┌─────────────────────────────────────────────────────────┐
│              Frontend Layer                              │
│  ┌──────────────────┐  OR  ┌────────────────────────┐  │
│  │   Qt GUI         │      │   React + Electron     │  │
│  │   (Phase 1-7)    │      │   (将来の移行先)      │  │
│  └──────────────────┘      └────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                          │
                  ┌───────┴───────┐
                  │   API Layer   │  ← 重要: フロントエンド非依存
                  └───────┬───────┘
                          │
┌─────────────────────────────────────────────────────────┐
│              Backend Layer (C++)                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Business Logic (InspectionController等)         │  │
│  │  - 画像処理                                       │  │
│  │  - 欠陥検出                                       │  │
│  │  - データ管理                                     │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 2. **API層の設計（重要）**

バックエンドとフロントエンドの間にAPI層を設けることで、GUIの変更がバックエンドに影響しないようにします。

---

## 実装戦略

### Phase 1-7: Qt版の実装（フロントエンド非依存を意識）

```cpp
// バックエンド: API層（抽象インターフェース）
class IInspectionAPI {
public:
    virtual ~IInspectionAPI() = default;

    // JSON形式でデータをやり取り
    virtual std::string inspectImage(const std::string& requestJson) = 0;
    virtual std::string loadImage(const std::string& requestJson) = 0;
    virtual std::string getPipelineConfig(const std::string& requestJson) = 0;
    virtual std::string setPipelineConfig(const std::string& requestJson) = 0;
    virtual std::string getHistory(const std::string& requestJson) = 0;
    virtual std::string getStatistics(const std::string& requestJson) = 0;
};

// 実装クラス
class InspectionAPI : public IInspectionAPI {
public:
    std::string inspectImage(const std::string& requestJson) override {
        // 1. JSONをパース
        auto request = nlohmann::json::parse(requestJson);
        std::string imagePath = request["image_path"];

        // 2. ビジネスロジック実行
        cv::Mat image = ImageIO::loadImage(imagePath);
        auto result = inspectionController_->inspect(image);

        // 3. 結果をJSONで返す
        nlohmann::json response;
        response["success"] = true;
        response["result"] = {
            {"is_ok", result.isOK},
            {"defect_count", result.defects.size()},
            {"processing_time", result.processingTime}
        };

        // 欠陥情報
        response["defects"] = nlohmann::json::array();
        for (const auto& defect : result.defects) {
            response["defects"].push_back({
                {"type", static_cast<int>(defect.type)},
                {"confidence", defect.confidence},
                {"bbox", {
                    {"x", defect.boundingBox.x},
                    {"y", defect.boundingBox.y},
                    {"width", defect.boundingBox.width},
                    {"height", defect.boundingBox.height}
                }}
            });
        }

        return response.dump();
    }

    // 他のメソッドも同様にJSON入出力

private:
    std::unique_ptr<InspectionController> inspectionController_;
};
```

---

## React移行時の選択肢

### オプション1: Electron + React（推奨）

デスクトップアプリとして動作し、既存のC++バックエンドを活用できます。

**アーキテクチャ:**

```
┌─────────────────────────────────────────────────────────┐
│                    Electron App                          │
│  ┌────────────────────────────────────────────────┐    │
│  │  React Frontend                                 │    │
│  │  - UI Components                                │    │
│  │  - State Management (Redux/Zustand)            │    │
│  └────────────────────────────────────────────────┘    │
│                       │                                  │
│                       │ IPC / HTTP / WebSocket          │
│                       ↓                                  │
│  ┌────────────────────────────────────────────────┐    │
│  │  Node.js Backend (Electron Main Process)       │    │
│  │  - C++ Addon (node-addon-api)                  │    │
│  │  - または HTTP Server経由でC++と通信            │    │
│  └────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
                       │
                       │ (Native Addon / HTTP)
                       ↓
┌─────────────────────────────────────────────────────────┐
│              C++ Backend (既存コード)                    │
│  - InspectionAPI                                        │
│  - InspectionController                                 │
│  - Pipeline, Detectors                                  │
└─────────────────────────────────────────────────────────┘
```

**実装方法:**

#### 1. C++をNode.js Addonとして公開

```cpp
// binding.cpp (Node.js Native Addon)
#include <napi.h>
#include "inspection_api.hpp"

Napi::String InspectImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // JavaScript から JSON文字列を受け取る
    std::string requestJson = info[0].As<Napi::String>().Utf8Value();

    // C++ API 呼び出し
    InspectionAPI api;
    std::string responseJson = api.inspectImage(requestJson);

    // JavaScriptに返す
    return Napi::String::New(env, responseJson);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("inspectImage", Napi::Function::New(env, InspectImage));
    // 他のAPI関数も登録
    return exports;
}

NODE_API_MODULE(inspection_native, Init)
```

#### 2. React側から呼び出し

```typescript
// React + TypeScript
import { inspectImage } from './native/inspection';

interface InspectionRequest {
  image_path: string;
}

interface InspectionResponse {
  success: boolean;
  result: {
    is_ok: boolean;
    defect_count: number;
    processing_time: number;
  };
  defects: Array<{
    type: number;
    confidence: number;
    bbox: { x: number; y: number; width: number; height: number };
  }>;
}

async function runInspection(imagePath: string) {
  const request: InspectionRequest = { image_path: imagePath };
  const responseJson = inspectImage(JSON.stringify(request));
  const response: InspectionResponse = JSON.parse(responseJson);

  return response;
}

// Reactコンポーネント
function InspectionPanel() {
  const [result, setResult] = useState<InspectionResponse | null>(null);

  const handleInspect = async () => {
    const response = await runInspection('/path/to/image.jpg');
    setResult(response);
  };

  return (
    <div>
      <button onClick={handleInspect}>検査実行</button>
      {result && (
        <div>
          <h3>結果: {result.result.is_ok ? 'OK' : 'NG'}</h3>
          <p>欠陥数: {result.result.defect_count}</p>
          <p>処理時間: {result.result.processing_time}ms</p>
        </div>
      )}
    </div>
  );
}
```

---

### オプション2: C++ HTTP Server + React SPA

C++でHTTPサーバーを立て、ReactアプリからRESTful APIで通信します。

**アーキテクチャ:**

```
┌─────────────────────────────────────────────────────────┐
│  React Frontend (SPA)                                    │
│  - ブラウザまたは Electron で実行                        │
└─────────────────────────────────────────────────────────┘
                       │
                       │ HTTP (REST API)
                       ↓
┌─────────────────────────────────────────────────────────┐
│  C++ HTTP Server (crow, Pistache, cpp-httplib等)        │
│  ┌────────────────────────────────────────────────┐    │
│  │  REST API Endpoints                             │    │
│  │  - POST /api/inspect                            │    │
│  │  - GET  /api/history                            │    │
│  │  - POST /api/pipeline/config                    │    │
│  └────────────────────────────────────────────────┘    │
│                       │                                  │
│                       ↓                                  │
│  ┌────────────────────────────────────────────────┐    │
│  │  InspectionAPI                                  │    │
│  │  - inspectImage()                               │    │
│  │  - getPipelineConfig()                          │    │
│  └────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

**実装例（crow使用）:**

```cpp
// http_server.cpp
#include "crow.h"
#include "inspection_api.hpp"

int main() {
    crow::SimpleApp app;
    InspectionAPI inspectionAPI;

    // POST /api/inspect
    CROW_ROUTE(app, "/api/inspect")
        .methods("POST"_method)
    ([&inspectionAPI](const crow::request& req) {
        std::string requestJson = req.body;
        std::string responseJson = inspectionAPI.inspectImage(requestJson);

        crow::response res(responseJson);
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // GET /api/history
    CROW_ROUTE(app, "/api/history")
    ([&inspectionAPI](const crow::request& req) {
        std::string requestJson = "{}";  // クエリパラメータから構築
        std::string responseJson = inspectionAPI.getHistory(requestJson);

        crow::response res(responseJson);
        res.add_header("Content-Type", "application/json");
        return res;
    });

    // サーバー起動
    app.port(8080).multithreaded().run();
}
```

**React側:**

```typescript
// api.ts
const API_BASE_URL = 'http://localhost:8080/api';

export async function inspectImage(imagePath: string) {
  const response = await fetch(`${API_BASE_URL}/inspect`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ image_path: imagePath })
  });

  return await response.json();
}

export async function getHistory() {
  const response = await fetch(`${API_BASE_URL}/history`);
  return await response.json();
}

// React Component
function InspectionPanel() {
  const [result, setResult] = useState(null);

  const handleInspect = async () => {
    const data = await inspectImage('/path/to/image.jpg');
    setResult(data);
  };

  // ...
}
```

---

### オプション3: WebAssembly（最先端）

C++コードをWebAssemblyにコンパイルし、ブラウザ上で直接実行します。

**メリット:**
- ネイティブ並みのパフォーマンス
- サーバー不要（完全なクライアントサイド）

**デメリット:**
- OpenCVのWasm版が必要
- ファイルサイズが大きい
- カメラアクセスが制限される

**実装（Emscripten使用）:**

```cpp
// inspection_wasm.cpp
#include <emscripten/bind.h>
#include "inspection_api.hpp"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(inspection_module) {
    class_<InspectionAPI>("InspectionAPI")
        .constructor<>()
        .function("inspectImage", &InspectionAPI::inspectImage)
        .function("loadImage", &InspectionAPI::loadImage);
}
```

```typescript
// React側
import createModule from './inspection.js';

const InspectionModule = await createModule();
const api = new InspectionModule.InspectionAPI();

const result = api.inspectImage(JSON.stringify({ image_path: '...' }));
```

---

## 推奨実装ロードマップ

### Phase 1-7: Qt版で開発（現在の計画通り）

```cpp
// バックエンドとフロントエンドを分離した設計
class InspectionController {
    // ビジネスロジックのみ
    // UI依存コードは一切含まない
};

// Qtフロントエンド
class MainWindow : public QMainWindow {
    // UIロジックのみ
    // InspectionController を使用
};
```

### Phase 8: API層の導入

```cpp
// API層を追加（Qt版でも使用可能）
class InspectionAPI {
    std::string inspectImage(const std::string& requestJson);
    std::string getPipelineConfig(const std::string& requestJson);
    // ...
};

// Qt版から使用
MainWindow::onInspectClicked() {
    nlohmann::json request = {{"image_path", imagePath}};
    std::string response = api_->inspectImage(request.dump());
    auto result = nlohmann::json::parse(response);
    // UIを更新
}
```

### Phase 9: React移行の準備

1. **HTTP Serverの追加**
   - crow または cpp-httplib を統合
   - RESTful API エンドポイント実装

2. **React プロトタイプ作成**
   - Electron + React環境構築
   - 基本的なUI実装
   - C++ APIとの通信確認

### Phase 10: React版の本格実装

- Qtから段階的に移行
- デュアル対応（Qt版とReact版の両方をサポート）

---

## 設計原則（React移行を考慮）

### 1. **Separation of Concerns（関心の分離）**

```
Backend (C++)               Frontend (Qt/React)
├── Business Logic         ├── UI Components
├── Data Management        ├── State Management
├── Image Processing       ├── Event Handling
└── API Layer              └── Rendering
```

### 2. **JSON-First Communication**

すべてのデータ交換をJSONで行う：

```cpp
// ❌ 悪い例: Qt依存のデータ構造
QVariant inspectImage(const QImage& image);

// ✅ 良い例: JSON文字列でやり取り
std::string inspectImage(const std::string& requestJson);
```

### 3. **Backend First Design**

フロントエンドに依存しないバックエンド設計：

```cpp
// バックエンド: フロントエンド非依存
class InspectionController {
    InspectionResult inspect(const cv::Mat& image);
    // Qtの型は使わない
};

// フロントエンド（Qt）: バックエンドを使用
class QtInspectionController {
    InspectionResult inspectQImage(const QImage& image) {
        cv::Mat mat = qImageToMat(image);  // 変換
        return backend_.inspect(mat);
    }
private:
    InspectionController backend_;
};
```

---

## ディレクトリ構造（React対応版）

```
inspection_app/
├── backend/                    # C++ バックエンド
│   ├── include/
│   ├── src/
│   ├── api/                   # API層（重要）
│   │   ├── inspection_api.hpp
│   │   ├── inspection_api.cpp
│   │   └── http_server.cpp    # HTTPサーバー（オプション）
│   └── CMakeLists.txt
│
├── frontend-qt/               # Qt版フロントエンド
│   ├── src/
│   │   ├── main_window.cpp
│   │   └── qt_inspection_controller.cpp
│   └── CMakeLists.txt
│
├── frontend-react/            # React版フロントエンド（将来）
│   ├── src/
│   │   ├── components/
│   │   ├── hooks/
│   │   ├── api/              # バックエンドAPI呼び出し
│   │   └── App.tsx
│   ├── package.json
│   └── electron.js           # Electron使用の場合
│
└── shared/                    # 共有データ型定義
    └── api_schema.json       # APIスキーマ定義
```

---

## CMakeLists.txt（フレキシブル構成）

```cmake
cmake_minimum_required(VERSION 3.15)
project(InspectionApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# オプション
option(BUILD_QT_FRONTEND "Build Qt frontend" ON)
option(BUILD_HTTP_SERVER "Build HTTP server for React" OFF)

# バックエンド（必須）
add_subdirectory(backend)

# Qtフロントエンド（オプション）
if(BUILD_QT_FRONTEND)
    add_subdirectory(frontend-qt)
endif()

# HTTPサーバー（オプション）
if(BUILD_HTTP_SERVER)
    find_package(crow REQUIRED)
    add_executable(inspection_server
        backend/api/http_server.cpp
    )
    target_link_libraries(inspection_server
        PRIVATE
        inspection_backend
        crow::crow
    )
endif()
```

---

## API設計例（OpenAPI/Swagger）

```yaml
# api_schema.yaml
openapi: 3.0.0
info:
  title: Inspection API
  version: 1.0.0

paths:
  /api/inspect:
    post:
      summary: 画像検査を実行
      requestBody:
        content:
          application/json:
            schema:
              type: object
              properties:
                image_path:
                  type: string
                pipeline_preset:
                  type: string
      responses:
        '200':
          description: 検査結果
          content:
            application/json:
              schema:
                type: object
                properties:
                  success:
                    type: boolean
                  result:
                    type: object
                    properties:
                      is_ok:
                        type: boolean
                      defect_count:
                        type: integer
                      processing_time:
                        type: number
                  defects:
                    type: array
                    items:
                      type: object

  /api/pipeline/config:
    get:
      summary: パイプライン設定を取得
    post:
      summary: パイプライン設定を更新

  /api/history:
    get:
      summary: 検査履歴を取得
```

---

## まとめ

### React移行を可能にする設計のポイント

1. ✅ **API層の導入**: JSON入出力のAPI層を設ける
2. ✅ **Backend/Frontend分離**: ビジネスロジックをUI非依存にする
3. ✅ **HTTP Server対応**: 将来的にRESTful APIを追加可能
4. ✅ **データ交換の標準化**: JSONを使用
5. ✅ **モジュラー設計**: フロントエンドを交換可能にする

### 現時点での対応

Phase 1-7ではQt版を実装しますが、以下を意識：
- InspectionController等はQt非依存
- API層（JSON入出力）を早期に導入
- ビジネスロジックとUIロジックの明確な分離

### 移行時のメリット

- バックエンド（C++）はそのまま使用可能
- 段階的な移行が可能（Qt版とReact版を並行運用）
- HTTPサーバーを追加するだけでWeb版も可能

---

**最終更新**: 2025-11-16
