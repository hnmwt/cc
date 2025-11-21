# ブロブ検出器 要件定義書

**作成日**: 2025-11-21
**バージョン**: 1.0
**ステータス**: 要件定義中

---

## 1. 概要

### 1.1 目的
画像内の塊（blob）状の領域を検出し、外観検査における欠陥（傷、汚れ、異物、形状不良など）を自動的に識別する。

### 1.2 ブロブとは
ブロブ（blob）は、周囲と異なる特性（明るさ、色、テクスチャなど）を持つ連結領域のこと。

### 1.3 用途
- **傷の検出**: 表面の引っかき傷、打痕
- **汚れの検出**: 異物付着、シミ、変色
- **異物の検出**: 埃、ゴミ、気泡
- **形状不良**: 凹凸、バリ、欠け
- **粒子検出**: 粉体、粒状物の計数

---

## 2. 機能要件

### 2.1 必須機能

#### 2.1.1 ブロブ検出
- OpenCVの`SimpleBlobDetector`をベースとする
- カスタムパラメータでの検出設定が可能
- グレースケール画像およびカラー画像に対応

#### 2.1.2 検出基準
以下の基準で ブロブをフィルタリング可能：

1. **色（明度）**
   - 明るいブロブ（白っぽい異物）
   - 暗いブロブ（黒っぽい傷）

2. **面積**
   - 最小面積（ノイズ除去）
   - 最大面積（過大検出除外）

3. **円形度**
   - 真円に近いブロブのみ検出
   - 範囲: 0.0（線状）〜 1.0（完全な円）

4. **凸性（Convexity）**
   - ブロブの凸度合い
   - 範囲: 0.0（凹凸が多い）〜 1.0（凸）

5. **慣性比（Inertia Ratio）**
   - ブロブの伸び具合
   - 範囲: 0.0（細長い）〜 1.0（正方形/円形）

#### 2.1.3 出力形式
各検出されたブロブについて以下の情報を出力：
- 中心座標 (x, y)
- サイズ（直径、面積）
- バウンディングボックス
- 信頼度スコア
- 欠陥タイプ（自動分類）

### 2.2 オプション機能

#### 2.2.1 適応的閾値処理
- 照明条件が不均一な場合に対応
- ブロック単位での閾値計算

#### 2.2.2 マルチスケール検出
- 異なるサイズのブロブを効率的に検出
- スケールピラミッドの使用

#### 2.2.3 ブロブの統合
- 近接するブロブを統合
- 重複検出の除去

---

## 3. パラメータ仕様

### 3.1 検出パラメータ

```json
{
  "blob_detector": {
    "enabled": true,
    "threshold": {
      "min_threshold": 10,
      "max_threshold": 220,
      "threshold_step": 10
    },
    "color": {
      "filter_by_color": true,
      "blob_color": 0
    },
    "area": {
      "filter_by_area": true,
      "min_area": 50.0,
      "max_area": 50000.0
    },
    "circularity": {
      "filter_by_circularity": true,
      "min_circularity": 0.1,
      "max_circularity": 1.0
    },
    "convexity": {
      "filter_by_convexity": true,
      "min_convexity": 0.5,
      "max_convexity": 1.0
    },
    "inertia": {
      "filter_by_inertia": true,
      "min_inertia_ratio": 0.1,
      "max_inertia_ratio": 1.0
    },
    "distance": {
      "min_distance_between_blobs": 10.0
    },
    "repeatability": {
      "min_repeatability": 2
    }
  }
}
```

### 3.2 パラメータ詳細

| パラメータ | 型 | 範囲 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| `min_threshold` | int | 0-255 | 10 | 二値化の最小閾値 |
| `max_threshold` | int | 0-255 | 220 | 二値化の最大閾値 |
| `threshold_step` | int | 1-100 | 10 | 閾値のステップ幅 |
| `blob_color` | int | 0 or 255 | 0 | 0=暗いブロブ, 255=明るいブロブ |
| `min_area` | float | >0 | 50.0 | 最小面積（ピクセル²） |
| `max_area` | float | >0 | 50000.0 | 最大面積（ピクセル²） |
| `min_circularity` | float | 0.0-1.0 | 0.1 | 最小円形度 |
| `max_circularity` | float | 0.0-1.0 | 1.0 | 最大円形度 |
| `min_convexity` | float | 0.0-1.0 | 0.5 | 最小凸性 |
| `max_convexity` | float | 0.0-1.0 | 1.0 | 最大凸性 |
| `min_inertia_ratio` | float | 0.0-1.0 | 0.1 | 最小慣性比 |
| `max_inertia_ratio` | float | 0.0-1.0 | 1.0 | 最大慣性比 |
| `min_distance_between_blobs` | float | >=0 | 10.0 | ブロブ間の最小距離 |
| `min_repeatability` | int | >=1 | 2 | 繰り返し検出の最小回数 |

---

## 4. 検出アルゴリズム

### 4.1 処理フロー

```
入力画像
  ↓
前処理（グレースケール変換、ノイズ除去）
  ↓
マルチスケール二値化
  ├─ 閾値1で二値化
  ├─ 閾値2で二値化
  ├─ 閾値3で二値化
  └─ ...
  ↓
各二値画像で輪郭検出
  ↓
フィルタリング
  ├─ 面積フィルタ
  ├─ 円形度フィルタ
  ├─ 凸性フィルタ
  └─ 慣性比フィルタ
  ↓
ブロブの統合・重複除去
  ↓
欠陥タイプの自動分類
  ├─ 傷（細長い、低円形度）
  ├─ 汚れ（不定形、中程度の面積）
  ├─ 異物（円形、小面積）
  └─ 形状不良（大面積、低凸性）
  ↓
結果出力（Defectリスト）
```

### 4.2 欠陥タイプの自動分類ロジック

```cpp
DefectType classifyBlob(const KeyPoint& blob, const BlobFeatures& features) {
    // 傷: 細長い、低円形度
    if (features.inertia_ratio < 0.3 && features.circularity < 0.5) {
        return DefectType::Scratch;
    }

    // 異物: 円形、小〜中面積
    if (features.circularity > 0.7 && features.area < 1000) {
        return DefectType::Stain;  // または新しいタイプ: ForeignMatter
    }

    // 形状不良: 大面積、低凸性
    if (features.area > 5000 && features.convexity < 0.7) {
        return DefectType::Deformation;
    }

    // 汚れ: その他
    return DefectType::Stain;
}
```

---

## 5. 実装仕様

### 5.1 クラス設計

#### 5.1.1 BlobDetector クラス

```cpp
class BlobDetector : public DetectorBase {
public:
    // コンストラクタ
    BlobDetector();
    explicit BlobDetector(const cv::SimpleBlobDetector::Params& params);

    // DetectorBaseインターフェース実装
    Defects detect(const cv::Mat& image) override;
    std::string getName() const override;
    std::string getType() const override;
    void setParameters(const json& params) override;
    json getParameters() const override;
    std::unique_ptr<DetectorBase> clone() const override;

    // BlobDetector固有のメソッド
    void setBlobParams(const cv::SimpleBlobDetector::Params& params);
    cv::SimpleBlobDetector::Params getBlobParams() const;

    void setColorThreshold(int blobColor);  // 0 or 255
    void setAreaThreshold(double minArea, double maxArea);
    void setCircularityThreshold(double minCirc, double maxCirc);
    void setConvexityThreshold(double minConv, double maxConv);
    void setInertiaThreshold(double minInertia, double maxInertia);

    // 検出されたキーポイントを取得（デバッグ用）
    std::vector<cv::KeyPoint> getLastKeyPoints() const;

private:
    cv::Ptr<cv::SimpleBlobDetector> detector_;
    cv::SimpleBlobDetector::Params params_;
    std::vector<cv::KeyPoint> lastKeyPoints_;

    // ヘルパー関数
    Defect keyPointToDefect(const cv::KeyPoint& kp, const cv::Mat& image);
    DefectType classifyBlob(const cv::KeyPoint& kp, const cv::Mat& image);
    double calculateConfidence(const cv::KeyPoint& kp, const cv::Mat& image);
};
```

#### 5.1.2 ファイル構成

```
include/detectors/BlobDetector.h
src/detectors/BlobDetector.cpp
tests/test_blob_detector.cpp
```

### 5.2 依存関係

- OpenCV 4.x (SimpleBlobDetector)
- DetectorBase (既存)
- Defect (既存)

---

## 6. 使用例

### 6.1 基本的な使用法

```cpp
// BlobDetectorを作成
auto blobDetector = std::make_unique<BlobDetector>();

// パラメータを設定
blobDetector->setAreaThreshold(50.0, 10000.0);
blobDetector->setCircularityThreshold(0.3, 1.0);
blobDetector->setColorThreshold(0);  // 暗いブロブを検出

// InspectionControllerに追加
controller->addDetector(std::move(blobDetector));

// 検査実行
auto result = controller->inspect(image);
```

### 6.2 JSON設定での使用

```json
{
  "detection": {
    "detectors": [
      {
        "type": "blob",
        "enabled": true,
        "blob_color": 0,
        "min_area": 50.0,
        "max_area": 10000.0,
        "min_circularity": 0.3,
        "max_circularity": 1.0,
        "min_convexity": 0.5,
        "max_convexity": 1.0,
        "min_inertia_ratio": 0.1,
        "max_inertia_ratio": 1.0,
        "min_threshold": 10,
        "max_threshold": 220
      }
    ]
  }
}
```

---

## 7. 性能要件

### 7.1 処理速度
- 1920x1200画像: 50ms以内（目標: 30ms）
- 640x480画像: 20ms以内（目標: 10ms）

### 7.2 検出精度
- 感度（Sensitivity）: 90%以上
  - 実際の欠陥のうち検出できる割合
- 特異度（Specificity）: 95%以上
  - 正常部分を欠陥と誤検出しない割合

### 7.3 メモリ使用量
- 追加メモリ使用量: 100MB以内

---

## 8. テスト計画

### 8.1 ユニットテスト

```cpp
// test_blob_detector.cpp

TEST(BlobDetectorTest, BasicDetection) {
    // 基本的なブロブ検出
}

TEST(BlobDetectorTest, ParameterSetting) {
    // パラメータ設定のテスト
}

TEST(BlobDetectorTest, ColorFiltering) {
    // 明るい/暗いブロブのフィルタリング
}

TEST(BlobDetectorTest, AreaFiltering) {
    // 面積フィルタリング
}

TEST(BlobDetectorTest, CircularityFiltering) {
    // 円形度フィルタリング
}

TEST(BlobDetectorTest, DefectClassification) {
    // 欠陥タイプの自動分類
}

TEST(BlobDetectorTest, JSONSerialization) {
    // JSON設定の読み書き
}
```

### 8.2 統合テスト

1. **InspectionControllerとの統合**
   - 他の検出器との組み合わせテスト
   - パイプライン処理との統合

2. **実画像テスト**
   - 傷画像での検出テスト
   - 汚れ画像での検出テスト
   - 異物画像での検出テスト

3. **パフォーマンステスト**
   - 各種画像サイズでの処理時間計測
   - メモリ使用量の測定

---

## 9. 実装スケジュール

### Phase 1: 基本実装 (推定: 2時間)
- [ ] BlobDetector クラスの基本構造
- [ ] SimpleBlobDetector のラッピング
- [ ] パラメータ設定機能

### Phase 2: 機能拡張 (推定: 2時間)
- [ ] 欠陥タイプの自動分類
- [ ] 信頼度スコアの計算
- [ ] JSON設定対応

### Phase 3: テスト・調整 (推定: 2時間)
- [ ] ユニットテストの作成
- [ ] 統合テスト
- [ ] パラメータチューニング

**合計推定時間: 6時間**

---

## 10. 将来の拡張案

### 10.1 高度な検出機能
- [ ] カスケードブロブ検出（粗→密）
- [ ] マルチチャンネルブロブ検出（RGB各チャンネル）
- [ ] テクスチャベースのブロブ検出

### 10.2 機械学習との統合
- [ ] 学習済みモデルによるブロブ分類
- [ ] ディープラーニングベースのブロブ検出

### 10.3 最適化
- [ ] GPU アクセラレーション
- [ ] マルチスレッド処理
- [ ] SIMD最適化

---

## 11. 参考資料

### 11.1 OpenCV ドキュメント
- SimpleBlobDetector: https://docs.opencv.org/4.x/d0/d7a/classcv_1_1SimpleBlobDetector.html
- Feature Detection: https://docs.opencv.org/4.x/dd/d1a/group__imgproc__feature.html

### 11.2 アルゴリズム解説
- Blob Detection論文: "A Combined Corner and Edge Detector" (1988)
- 画像処理のためのブロブ解析手法

### 11.3 実装例
- OpenCV サンプルコード
- 既存のDetectorBase実装（TemplateMatcher, FeatureDetector）

---

## 12. 承認

### 12.1 レビュアー
- [ ] 技術責任者
- [ ] 品質保証担当
- [ ] プロジェクトマネージャー

### 12.2 承認日
- 要件定義承認: 未定
- 実装開始承認: 未定

---

**文書バージョン履歴:**
- v1.0 (2025-11-21): 初版作成
