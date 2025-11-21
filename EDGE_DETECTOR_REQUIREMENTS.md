# エッジ検出器 要件定義書

**作成日**: 2025-11-21
**バージョン**: 1.0
**ステータス**: 要件定義中

---

## 1. 概要

### 1.1 目的
画像内のエッジ（輪郭、境界線）を検出し、外観検査における欠陥（傷、クラック、欠け、バリ、境界不良など）を自動的に識別する。

### 1.2 エッジ検出とは
エッジは画像内の明度が急激に変化する箇所を指し、物体の境界や表面の変化を表す。

### 1.3 用途
- **傷・クラックの検出**: 細い線状の欠陥
- **欠けの検出**: 境界線の不連続
- **バリの検出**: 余分な突起
- **境界不良**: 形状の歪み
- **溶接不良**: 溶接線の異常
- **印刷不良**: 文字・パターンのかすれ

---

## 2. 機能要件

### 2.1 必須機能

#### 2.1.1 エッジ検出アルゴリズム

**1. Cannyエッジ検出**
- 最も一般的で高精度なエッジ検出
- ヒステリシス閾値による安定した検出
- ノイズ除去機能内蔵

**2. Sobelエッジ検出**
- X方向・Y方向のエッジを個別に検出
- エッジの方向（角度）を取得可能
- 高速処理

**3. Laplacianエッジ検出**
- 2次微分による全方向エッジ検出
- 微細なエッジも検出可能
- ノイズに敏感

#### 2.1.2 検出モード

```cpp
enum class EdgeDetectionMode {
    Canny,      // Cannyエッジ検出（デフォルト）
    Sobel,      // Sobelエッジ検出
    Laplacian,  // Laplacianエッジ検出
    Combined    // 複数手法の組み合わせ
};
```

#### 2.1.3 エッジ特徴量の抽出

各検出されたエッジについて以下の情報を抽出：
- エッジの長さ
- エッジの方向（角度）
- エッジの強度（勾配の大きさ）
- エッジの連続性
- エッジの曲率
- エッジの位置

#### 2.1.4 欠陥タイプの自動分類

```cpp
DefectType classifyEdge(const EdgeFeatures& features) {
    // 長く細い直線エッジ → 傷
    if (features.length > 100 && features.straightness > 0.9)
        return DefectType::Scratch;

    // 短い途切れたエッジ → クラック
    if (features.length < 50 && features.discontinuity > 0.5)
        return DefectType::Crack;

    // 境界線の異常 → 形状不良
    if (features.isOnBoundary && features.curvature > threshold)
        return DefectType::Deformation;

    // その他
    return DefectType::Unknown;
}
```

### 2.2 オプション機能

#### 2.2.1 エッジのフィルタリング
- 最小/最大エッジ長さ
- エッジ角度範囲（水平、垂直、斜めなど）
- エッジ強度閾値

#### 2.2.2 エッジの後処理
- 細線化（Thinning）
- 断片化されたエッジの接続
- ノイズ除去

#### 2.2.3 方向別検出
- 水平エッジのみ検出
- 垂直エッジのみ検出
- 特定角度範囲のエッジ検出

---

## 3. パラメータ仕様

### 3.1 検出パラメータ

```json
{
  "edge_detector": {
    "enabled": true,
    "mode": "canny",
    "canny": {
      "low_threshold": 50,
      "high_threshold": 150,
      "aperture_size": 3,
      "l2_gradient": true
    },
    "sobel": {
      "kernel_size": 3,
      "scale": 1.0,
      "delta": 0.0,
      "threshold": 50
    },
    "laplacian": {
      "kernel_size": 3,
      "scale": 1.0,
      "delta": 0.0,
      "threshold": 30
    },
    "filters": {
      "min_edge_length": 10,
      "max_edge_length": 1000,
      "min_edge_strength": 50,
      "angle_filter_enabled": false,
      "min_angle": 0.0,
      "max_angle": 180.0
    },
    "preprocessing": {
      "gaussian_blur": true,
      "blur_kernel_size": 5,
      "blur_sigma": 1.0
    },
    "confidence_threshold": 0.5
  }
}
```

### 3.2 パラメータ詳細

#### Canny パラメータ

| パラメータ | 型 | 範囲 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| `low_threshold` | int | 0-255 | 50 | 低閾値（弱いエッジ） |
| `high_threshold` | int | 0-255 | 150 | 高閾値（強いエッジ） |
| `aperture_size` | int | 3,5,7 | 3 | Sobelオペレータのサイズ |
| `l2_gradient` | bool | - | true | L2ノルムを使用 |

#### Sobel パラメータ

| パラメータ | 型 | 範囲 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| `kernel_size` | int | 1,3,5,7 | 3 | カーネルサイズ |
| `scale` | double | >0 | 1.0 | 微分値のスケール |
| `delta` | double | - | 0.0 | オフセット値 |
| `threshold` | int | 0-255 | 50 | 2値化閾値 |

#### Laplacian パラメータ

| パラメータ | 型 | 範囲 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| `kernel_size` | int | 1,3,5,7 | 3 | カーネルサイズ |
| `scale` | double | >0 | 1.0 | 微分値のスケール |
| `delta` | double | - | 0.0 | オフセット値 |
| `threshold` | int | 0-255 | 30 | 2値化閾値 |

---

## 4. 検出アルゴリズム

### 4.1 処理フロー

```
入力画像
  ↓
前処理（グレースケール変換、ノイズ除去）
  ↓
エッジ検出
  ├─ Cannyエッジ検出
  ├─ Sobelエッジ検出
  └─ Laplacianエッジ検出
  ↓
エッジの2値画像
  ↓
輪郭抽出（findContours）
  ↓
エッジ特徴量の計算
  ├─ 長さ
  ├─ 角度
  ├─ 強度
  ├─ 連続性
  └─ 曲率
  ↓
フィルタリング
  ├─ 長さフィルタ
  ├─ 角度フィルタ
  └─ 強度フィルタ
  ↓
欠陥タイプの自動分類
  ├─ 傷（長い直線）
  ├─ クラック（短い途切れた線）
  ├─ 欠け（境界の不連続）
  └─ 形状不良（境界の歪み）
  ↓
結果出力（Defectリスト）
```

### 4.2 欠陥タイプの分類ロジック

```cpp
DefectType EdgeDetector::classifyEdge(const EdgeFeatures& features) {
    // 傷: 長く（>100px）、直線的（straightness > 0.9）
    if (features.length > 100 && features.straightness > 0.9) {
        return DefectType::Scratch;
    }

    // クラック: 短く、途切れている
    if (features.length < 50 && features.gaps > 0) {
        return DefectType::Crack;
    }

    // 欠け: 境界上にあり、急激な変化
    if (features.isOnBoundary && features.angleChange > 45.0) {
        return DefectType::Chip;
    }

    // 形状不良: 境界上にあり、曲率が異常
    if (features.isOnBoundary && features.curvature > 0.1) {
        return DefectType::Deformation;
    }

    // その他
    return DefectType::Unknown;
}
```

---

## 5. 実装仕様

### 5.1 クラス設計

#### 5.1.1 EdgeDetector クラス

```cpp
class EdgeDetector : public DetectorBase {
public:
    enum class EdgeDetectionMode {
        Canny,
        Sobel,
        Laplacian,
        Combined
    };

    struct CannyParams {
        double lowThreshold = 50.0;
        double highThreshold = 150.0;
        int apertureSize = 3;
        bool L2gradient = true;
    };

    struct SobelParams {
        int kernelSize = 3;
        double scale = 1.0;
        double delta = 0.0;
        double threshold = 50.0;
    };

    struct LaplacianParams {
        int kernelSize = 3;
        double scale = 1.0;
        double delta = 0.0;
        double threshold = 30.0;
    };

    // コンストラクタ
    EdgeDetector(EdgeDetectionMode mode = EdgeDetectionMode::Canny);

    // DetectorBaseインターフェース実装
    Defects detect(const cv::Mat& image) override;
    std::string getName() const override;
    std::string getType() const override;
    void setParameters(const json& params) override;
    json getParameters() const override;
    std::unique_ptr<DetectorBase> clone() const override;

    // EdgeDetector固有のメソッド
    void setDetectionMode(EdgeDetectionMode mode);
    void setCannyParams(const CannyParams& params);
    void setSobelParams(const SobelParams& params);
    void setLaplacianParams(const LaplacianParams& params);
    void setEdgeLengthFilter(double minLength, double maxLength);
    void setEdgeAngleFilter(double minAngle, double maxAngle);

    // エッジ画像を取得（デバッグ用）
    cv::Mat getLastEdgeImage() const;

private:
    EdgeDetectionMode mode_;
    CannyParams cannyParams_;
    SobelParams sobelParams_;
    LaplacianParams laplacianParams_;

    double minEdgeLength_ = 10.0;
    double maxEdgeLength_ = 1000.0;
    double minEdgeAngle_ = 0.0;
    double maxEdgeAngle_ = 180.0;
    bool angleFilterEnabled_ = false;

    cv::Mat lastEdgeImage_;

    // ヘルパー関数
    cv::Mat detectCannyEdges(const cv::Mat& image);
    cv::Mat detectSobelEdges(const cv::Mat& image);
    cv::Mat detectLaplacianEdges(const cv::Mat& image);

    struct EdgeFeatures {
        double length;
        double angle;
        double strength;
        double straightness;
        double curvature;
        bool isOnBoundary;
        int gaps;
        cv::Rect boundingBox;
        std::vector<cv::Point> points;
    };

    EdgeFeatures calculateEdgeFeatures(const std::vector<cv::Point>& contour);
    DefectType classifyEdge(const EdgeFeatures& features);
    double calculateConfidence(const EdgeFeatures& features);
    Defect edgeToDefect(const EdgeFeatures& features);
};
```

#### 5.1.2 ファイル構成

```
include/detectors/EdgeDetector.h
src/detectors/EdgeDetector.cpp
tests/test_edge_detector.cpp
```

### 5.2 依存関係

- OpenCV 4.x (Canny, Sobel, Laplacian, findContours)
- DetectorBase (既存)
- Defect (既存)

---

## 6. 使用例

### 6.1 基本的な使用法

```cpp
// EdgeDetectorを作成（Cannyモード）
auto edgeDetector = std::make_unique<EdgeDetector>(
    EdgeDetector::EdgeDetectionMode::Canny
);

// Cannyパラメータを設定
EdgeDetector::CannyParams cannyParams;
cannyParams.lowThreshold = 50.0;
cannyParams.highThreshold = 150.0;
edgeDetector->setCannyParams(cannyParams);

// エッジ長さフィルタを設定
edgeDetector->setEdgeLengthFilter(20.0, 500.0);

// InspectionControllerに追加
controller->addDetector(std::move(edgeDetector));

// 検査実行
auto result = controller->inspect(image);
```

### 6.2 JSON設定での使用

```json
{
  "detection": {
    "detectors": [
      {
        "type": "edge",
        "enabled": true,
        "mode": "canny",
        "low_threshold": 50,
        "high_threshold": 150,
        "min_edge_length": 20.0,
        "max_edge_length": 500.0,
        "confidence_threshold": 0.5
      }
    ]
  }
}
```

---

## 7. 性能要件

### 7.1 処理速度
- 1920x1200画像: 30ms以内（目標: 20ms）
- 640x480画像: 10ms以内（目標: 5ms）

### 7.2 検出精度
- 感度（Sensitivity）: 85%以上
  - 実際の線状欠陥のうち検出できる割合
- 特異度（Specificity）: 90%以上
  - 正常部分を欠陥と誤検出しない割合

### 7.3 メモリ使用量
- 追加メモリ使用量: 50MB以内

---

## 8. テスト計画

### 8.1 ユニットテスト

```cpp
TEST(EdgeDetectorTest, CannyDetection) {
    // Cannyエッジ検出のテスト
}

TEST(EdgeDetectorTest, SobelDetection) {
    // Sobelエッジ検出のテスト
}

TEST(EdgeDetectorTest, LaplacianDetection) {
    // Laplacianエッジ検出のテスト
}

TEST(EdgeDetectorTest, EdgeLengthFilter) {
    // エッジ長さフィルタのテスト
}

TEST(EdgeDetectorTest, EdgeAngleFilter) {
    // エッジ角度フィルタのテスト
}

TEST(EdgeDetectorTest, DefectClassification) {
    // 欠陥タイプ分類のテスト
}

TEST(EdgeDetectorTest, JSONConfiguration) {
    // JSON設定のテスト
}
```

### 8.2 統合テスト

1. **InspectionControllerとの統合**
   - 他の検出器との組み合わせテスト
   - パイプライン処理との統合

2. **実画像テスト**
   - 傷画像での検出テスト
   - クラック画像での検出テスト
   - 欠け画像での検出テスト

3. **パフォーマンステスト**
   - 各種画像サイズでの処理時間計測
   - メモリ使用量の測定

---

## 9. 実装スケジュール

### Phase 1: 基本実装 (推定: 2時間)
- [ ] EdgeDetector クラスの基本構造
- [ ] Cannyエッジ検出実装
- [ ] パラメータ設定機能

### Phase 2: 機能拡張 (推定: 2時間)
- [ ] Sobel/Laplacianエッジ検出実装
- [ ] エッジ特徴量計算
- [ ] 欠陥タイプの自動分類
- [ ] JSON設定対応

### Phase 3: テスト・調整 (推定: 1.5時間)
- [ ] ユニットテストの作成
- [ ] 統合テスト
- [ ] パラメータチューニング

**合計推定時間: 5.5時間**

---

## 10. 将来の拡張案

### 10.1 高度な検出機能
- [ ] Hough変換による直線・円検出
- [ ] エッジの階層構造分析
- [ ] マルチスケールエッジ検出

### 10.2 機械学習との統合
- [ ] 学習済みモデルによるエッジ分類
- [ ] ディープラーニングベースのエッジ検出

### 10.3 最適化
- [ ] GPU アクセラレーション
- [ ] SIMD最適化

---

## 11. 参考資料

### 11.1 OpenCV ドキュメント
- Canny: https://docs.opencv.org/4.x/dd/d1a/group__imgproc__feature.html#ga04723e007ed888ddf11d9ba04e2232de
- Sobel: https://docs.opencv.org/4.x/d4/d86/group__imgproc__filter.html#gacea54f142e81b6758cb6f375ce782c8d
- Laplacian: https://docs.opencv.org/4.x/d4/d86/group__imgproc__filter.html#gad78703e4c8fe703d479c1860d76429e6

### 11.2 アルゴリズム解説
- Canny Edge Detection (1986)
- Sobel Operator
- Laplacian of Gaussian (LoG)

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
