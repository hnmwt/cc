# タスク管理

**プロジェクト**: 外観検査アプリケーション
**最終更新**: 2025-11-16
**進捗率**: 40% (10/25タスク完了)

---

## ✅ 完了済み (10タスク)

### Phase 1: 基本機能

- [x] **1. CMakeLists.txt作成** (OpenCV、Boost.Asio、crow、nlohmann/json、spdlog)
- [x] **2. ディレクトリ構造構築** (include/src/tests/config/data分離)
- [x] **3. ImageIOクラス実装** (loadImage/saveImage/loadBatch)
- [x] **4. ConfigManagerクラス実装** (JSON設定読み書き)
- [x] **5. Loggerユーティリティ実装** (spdlog統合)
- [x] **6. FilterBase抽象クラス定義**
- [x] **7. Pipelineクラス実装** (addFilter/process/processWithIntermediates)
- [x] **8. GrayscaleFilter実装**
- [x] **9. GaussianFilter実装** (kernelSize, sigma パラメータ)
- [x] **10. Phase 1動作確認** (画像読み込み→フィルタ適用→保存) ✨

---

## 📋 残タスク (15タスク)

### Phase 1: 基本機能 (残り2タスク)

- [ ] **11. ThresholdFilter実装** (threshold, method パラメータ)
  - 優先度: 中
  - 推定: 1時間
  - 説明: 二値化処理フィルタの実装

- [ ] **12. 簡易UI実装** (OpenCV HighGUI、画像表示・トラックバー)
  - 優先度: 低
  - 推定: 2時間
  - 説明: パラメータ調整用の簡易GUI

### その他

- [ ] **13. InspectionAPIクラス実装** (JSON入出力、React移行対応)
  - 優先度: 中
  - 推定: 2時間
  - 説明: フロントエンド連携用API層

---

### Phase 2: 欠陥検出機能 (5タスク) 🎯

#### 優先度: 最高 ⭐

- [ ] **14. Defectデータ構造定義** (type, bbox, confidence)
  - 優先度: 高
  - 推定: 0.5時間
  - 説明: 欠陥情報を格納する構造体定義
  - ファイル: `include/detectors/Defect.h`

- [ ] **15. DetectorBase抽象クラス定義**
  - 優先度: 高
  - 推定: 1時間
  - 説明: 欠陥検出器の基底クラス
  - ファイル: `include/detectors/DetectorBase.h`

- [ ] **16. TemplateMatcher実装** (良品画像との差分検出)
  - 優先度: 高
  - 推定: 2時間
  - 説明: テンプレートマッチングによる欠陥検出
  - ファイル: `include/detectors/TemplateMatcher.h`, `src/detectors/TemplateMatcher.cpp`

- [ ] **17. FeatureDetector実装** (輪郭・面積・円形度)
  - 優先度: 高
  - 推定: 2時間
  - 説明: 特徴量ベースの欠陥検出
  - ファイル: `include/detectors/FeatureDetector.h`, `src/detectors/FeatureDetector.cpp`

- [ ] **18. InspectionController実装** (inspect/setPipeline/addDetector)
  - 優先度: 高
  - 推定: 2時間
  - 説明: 検査処理全体を制御するコントローラー
  - ファイル: `include/InspectionController.h`, `src/InspectionController.cpp`

**Phase 2 合計**: 7.5時間

---

### Phase 4.5: 外部機器連携・データ出力 (7タスク) 🚀

#### 優先度: 高

- [ ] **19. ExternalTriggerHandler実装** (TCP Socket、外部機器トリガー受信)
  - 優先度: 高
  - 推定: 3時間
  - 説明: PLCなどからのトリガー受信処理
  - ファイル: `include/server/ExternalTriggerHandler.h`, `src/server/ExternalTriggerHandler.cpp`

- [ ] **20. WebSocketサーバー実装** (crow、プッシュ通知機能)
  - 優先度: 高
  - 推定: 3時間
  - 説明: Reactフロントエンドへのリアルタイム通知
  - ファイル: `include/server/WebSocketServer.h`, `src/server/WebSocketServer.cpp`

- [ ] **21. HTTP REST APIサーバー実装** (crow、/api/v1/inspect等)
  - 優先度: 高
  - 推定: 3時間
  - 説明: RESTful API実装
  - ファイル: `include/server/RestApiServer.h`, `src/server/RestApiServer.cpp`

- [ ] **22. CSVWriter実装** (検査結果の自動CSV出力、UTF-8 BOM付き)
  - 優先度: 高
  - 推定: 2時間
  - 説明: 検査結果のCSV出力（Excel対応）
  - ファイル: `include/io/CSVWriter.h`, `src/io/CSVWriter.cpp`

- [ ] **23. ImageSaver実装** (元画像・処理後・マーキング画像の自動保存)
  - 優先度: 高
  - 推定: 2時間
  - 説明: 3種類の画像を自動保存
  - ファイル: `include/io/ImageSaver.h`, `src/io/ImageSaver.cpp`

- [ ] **24. InspectionServer実装** (統合サーバー、全機能統合)
  - 優先度: 高
  - 推定: 3時間
  - 説明: 全機能を統合したメインサーバー
  - ファイル: `include/server/InspectionServer.h`, `src/server/InspectionServer.cpp`

- [ ] **25. 外部機器連携テスト** (TCP経由でトリガー送信→検査→CSV出力)
  - 優先度: 高
  - 推定: 2時間
  - 説明: エンドツーエンドのシステムテスト

**Phase 4.5 合計**: 18時間

---

## 🎯 推奨実装順序

### ステップ1: Phase 2完成 (7.5時間)
欠陥検出機能を実装して、実際の検査デモができるようにする

```
14 → 15 → 16 → 17 → 18
```

### ステップ2: データ出力機能 (4時間)
検査結果の記録機能を追加

```
22 → 23
```

### ステップ3: 外部機器連携 (14時間)
本番運用に向けたサーバー機能を実装

```
19 → 20 → 21 → 24 → 25
```

### ステップ4: オプション機能 (5時間)
必要に応じて追加機能を実装

```
11 → 13 → 12
```

---

## 📊 進捗トラッキング

| フェーズ | 完了 | 残り | 進捗率 |
|---------|------|------|--------|
| Phase 1 基本機能 | 10 | 2 | 83% |
| Phase 2 欠陥検出 | 0 | 5 | 0% |
| Phase 4.5 外部連携 | 0 | 7 | 0% |
| その他 | 0 | 1 | 0% |
| **合計** | **10** | **15** | **40%** |

---

## ⏱️ 全体スケジュール

| 項目 | 推定時間 |
|------|----------|
| 完了済み | - |
| Phase 2 | 7.5時間 |
| データ出力 | 4時間 |
| 外部連携 | 14時間 |
| オプション | 5時間 |
| **残り合計** | **30.5時間** |

---

## 🚀 次のアクション

### 最優先タスク: Defectデータ構造定義

**推定時間**: 30分
**ファイル**: `include/detectors/Defect.h`

このタスクを完了すると、Phase 2の基盤が整います。

---

## 📝 メモ

### 完了したマイルストーン
- ✅ 2025-11-16: Phase 1完了 - 画像処理パイプライン動作確認成功
  - 1920x1200画像を6.92msで処理
  - GrayscaleFilter + GaussianFilter動作確認

### 技術的決定事項
- C++20使用
- Boost 1.89.0 (ヘッダーオンリー使用)
- Crow WebSocketサーバー (現在は無効化、後で有効化)
- REST + WebSocket ハイブリッドアーキテクチャ
- UTF-8 BOM付きCSV出力 (Excel対応)

---

**更新履歴**:
- 2025-11-16: Phase 1完了、タスクリスト作成
