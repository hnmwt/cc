# クラウドデプロイ計画書

**作成日**: 2025-11-21
**対象**: 外観検査アプリケーション Webバックエンドサーバー
**ステータス**: 計画中

---

## 📋 目次

1. [現状分析](#現状分析)
2. [デプロイ戦略](#デプロイ戦略)
3. [必須タスク一覧](#必須タスク一覧)
4. [推奨タスク一覧](#推奨タスク一覧)
5. [クラウドプロバイダー比較](#クラウドプロバイダー比較)
6. [アーキテクチャ設計](#アーキテクチャ設計)
7. [スケジュール・工数見積](#スケジュール工数見積)
8. [コスト見積](#コスト見積)

---

## 現状分析

### ✅ 実装済み機能

| カテゴリ | 機能 | 状態 |
|---------|------|------|
| **API** | REST API (HTTP:8080) | ✅ 実装済み |
| **検出器** | 4種類の検出器 | ✅ 実装済み |
| **データ出力** | CSV + 画像保存 | ✅ 実装済み |
| **外部連携** | TCP トリガー (9000) | ✅ 実装済み |
| **設定管理** | JSON設定ファイル | ✅ 実装済み |
| **ロギング** | spdlog | ✅ 実装済み |

### ⚠️ クラウド対応で必要な機能

| カテゴリ | 機能 | 状態 |
|---------|------|------|
| **コンテナ化** | Docker | ❌ 未実装 |
| **認証** | API認証・認可 | ❌ 未実装 |
| **ストレージ** | クラウドストレージ連携 | ❌ 未実装 |
| **スケーリング** | 水平スケーリング対応 | ⚠️ 要検討 |
| **モニタリング** | メトリクス・ヘルスチェック | ⚠️ 要拡張 |
| **セキュリティ** | HTTPS, CORS, レート制限 | ⚠️ 要拡張 |

---

## デプロイ戦略

### アプローチ1: コンテナベース（推奨）

```
Docker化 → クラウドへデプロイ
```

**メリット:**
- ✅ 環境の一貫性（開発・本番同一）
- ✅ スケーリングが容易
- ✅ 依存関係の問題が少ない
- ✅ CI/CDとの統合が容易

**デメリット:**
- ⚠️ 初期学習コスト
- ⚠️ イメージサイズが大きい可能性

### アプローチ2: サーバーレス

**メリット:**
- ✅ 運用コスト低
- ✅ 自動スケーリング

**デメリット:**
- ❌ C++のサーバーレス対応が困難
- ❌ コールドスタート問題
- ❌ 画像処理には不向き

**→ コンテナベースを推奨**

---

## 必須タスク一覧

### Phase 1: コンテナ化 (優先度: 最高)

#### 1.1 Dockerfile作成
**推定時間**: 2時間

```dockerfile
# マルチステージビルド
FROM ubuntu:22.04 AS builder
# 依存関係インストール
RUN apt-get update && apt-get install -y \
    build-essential cmake \
    libopencv-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    libspdlog-dev

# ソースコードコピー
COPY . /app
WORKDIR /app

# ビルド
RUN mkdir build && cd build && \
    cmake .. && make -j4

# 実行用軽量イメージ
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libopencv-core4.5d libopencv-imgproc4.5d \
    libboost-system1.74.0 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/bin/inspection_server /usr/local/bin/
COPY --from=builder /app/config /etc/inspection_app/

EXPOSE 8080 9000
CMD ["inspection_server", "-c", "/etc/inspection_app/default_config.json"]
```

**成果物:**
- [ ] `Dockerfile`
- [ ] `.dockerignore`
- [ ] ビルドスクリプト `build-docker.sh`

---

#### 1.2 Docker Compose設定
**推定時間**: 1時間

```yaml
version: '3.8'

services:
  inspection_server:
    build: .
    ports:
      - "8080:8080"
      - "9000:9000"
    volumes:
      - ./config:/etc/inspection_app
      - ./data:/app/data
      - ./logs:/app/logs
    environment:
      - LOG_LEVEL=info
    restart: unless-stopped

  # オプション: データベース
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: inspection_db
      POSTGRES_USER: inspector
      POSTGRES_PASSWORD: ${DB_PASSWORD}
    volumes:
      - pgdata:/var/lib/postgresql/data

volumes:
  pgdata:
```

**成果物:**
- [ ] `docker-compose.yml`
- [ ] `docker-compose.prod.yml`

---

### Phase 2: セキュリティ強化 (優先度: 高)

#### 2.1 API認証実装
**推定時間**: 4時間

**実装内容:**
- APIキー認証
- JWT トークン認証
- レート制限

**ファイル:**
```cpp
// include/server/AuthMiddleware.h
class AuthMiddleware {
public:
    bool validateApiKey(const std::string& apiKey);
    bool validateJWT(const std::string& token);
    bool checkRateLimit(const std::string& clientId);
};
```

**成果物:**
- [ ] `include/server/AuthMiddleware.h`
- [ ] `src/server/AuthMiddleware.cpp`
- [ ] RestApiServerへの統合

---

#### 2.2 HTTPS対応
**推定時間**: 2時間

**実装方法:**
1. Nginx リバースプロキシ
2. Let's Encrypt証明書
3. SSL終端

**Docker Compose追加:**
```yaml
  nginx:
    image: nginx:alpine
    ports:
      - "443:443"
      - "80:80"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf
      - ./ssl:/etc/nginx/ssl
    depends_on:
      - inspection_server
```

**成果物:**
- [ ] `nginx.conf`
- [ ] SSL証明書設定
- [ ] HTTP→HTTPS リダイレクト

---

#### 2.3 CORS設定強化
**推定時間**: 1時間

**現状**: CORS有効化済み
**追加対応:**
- オリジン制限
- 許可メソッド制限
- 認証情報の扱い

**成果物:**
- [ ] RestApiServer CORS設定更新

---

### Phase 3: クラウドストレージ連携 (優先度: 高)

#### 3.1 S3互換ストレージ対応
**推定時間**: 6時間

**対応ストレージ:**
- AWS S3
- Google Cloud Storage
- MinIO (セルフホスト)

**実装:**
```cpp
// include/io/CloudStorage.h
class CloudStorage {
public:
    bool uploadImage(const std::string& localPath, const std::string& key);
    bool downloadImage(const std::string& key, const std::string& localPath);
    std::string getSignedUrl(const std::string& key, int expirySeconds);
};
```

**依存関係:**
- AWS SDK for C++
- または libcurl + REST API直接実装

**成果物:**
- [ ] `include/io/CloudStorage.h`
- [ ] `src/io/CloudStorage.cpp`
- [ ] ImageIOへの統合
- [ ] テストプログラム

---

#### 3.2 画像アップロードAPI
**推定時間**: 3時間

**新規エンドポイント:**
```
POST /api/v1/upload
- multipart/form-data
- 画像アップロード
- S3へ保存
- 検査キューに追加

GET /api/v1/images/{image_id}
- 署名付きURL返却
```

**成果物:**
- [ ] アップロードエンドポイント実装
- [ ] 画像取得エンドポイント実装

---

### Phase 4: CI/CD パイプライン (優先度: 中)

#### 4.1 GitHub Actions設定
**推定時間**: 3時間

```yaml
# .github/workflows/build-and-deploy.yml
name: Build and Deploy

on:
  push:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build Docker image
        run: docker build -t inspection-server .
      - name: Run tests
        run: docker run inspection-server make test
      - name: Push to registry
        run: |
          docker tag inspection-server ${{ secrets.REGISTRY }}/inspection-server:latest
          docker push ${{ secrets.REGISTRY }}/inspection-server:latest

  deploy:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - name: Deploy to Cloud
        run: |
          # デプロイスクリプト実行
```

**成果物:**
- [ ] `.github/workflows/build-and-deploy.yml`
- [ ] `.github/workflows/test.yml`

---

#### 4.2 自動テスト拡張
**推定時間**: 2時間

**追加テスト:**
- API統合テスト
- 負荷テスト
- セキュリティテスト

**成果物:**
- [ ] `tests/test_api_integration.cpp`
- [ ] 負荷テストスクリプト

---

### Phase 5: モニタリング・ロギング (優先度: 中)

#### 5.1 ヘルスチェックエンドポイント
**推定時間**: 1時間

**追加エンドポイント:**
```
GET /health
- サーバー稼働状態
- 依存サービス状態
- メモリ使用量

GET /metrics
- Prometheus形式
- リクエスト数
- レスポンスタイム
- エラー率
```

**成果物:**
- [ ] ヘルスチェック実装
- [ ] メトリクスエンドポイント実装

---

#### 5.2 構造化ログ出力
**推定時間**: 2時間

**現状**: spdlogでファイル出力
**改善:**
- JSON形式ログ
- ログレベル制御
- ログローテーション

**成果物:**
- [ ] JSON形式ログ実装
- [ ] ログ設定ファイル

---

#### 5.3 クラウドロギング連携
**推定時間**: 2時間

**対応サービス:**
- CloudWatch Logs (AWS)
- Cloud Logging (GCP)
- Elasticsearch + Kibana

**成果物:**
- [ ] ログ転送設定
- [ ] ダッシュボード設定

---

### Phase 6: データベース統合 (優先度: 中)

#### 6.1 PostgreSQL統合
**推定時間**: 8時間

**実装内容:**
- 検査履歴の保存
- 統計情報の集計
- 検索API

**スキーマ:**
```sql
CREATE TABLE inspections (
    id SERIAL PRIMARY KEY,
    image_path VARCHAR(500),
    result VARCHAR(10),
    defect_count INT,
    processing_time_ms FLOAT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE defects (
    id SERIAL PRIMARY KEY,
    inspection_id INT REFERENCES inspections(id),
    type VARCHAR(50),
    confidence FLOAT,
    bbox_x INT,
    bbox_y INT,
    bbox_width INT,
    bbox_height INT
);
```

**成果物:**
- [ ] `include/db/DatabaseManager.h`
- [ ] `src/db/DatabaseManager.cpp`
- [ ] マイグレーションスクリプト
- [ ] データベースAPI実装

---

### Phase 7: スケーリング対応 (優先度: 低)

#### 7.1 ステートレス化
**推定時間**: 4時間

**対応内容:**
- セッション管理の外部化
- ファイルストレージの外部化
- 設定の環境変数化

**成果物:**
- [ ] Redis統合（セッション管理）
- [ ] 環境変数対応

---

#### 7.2 ロードバランサー設定
**推定時間**: 2時間

**実装:**
- 複数インスタンス起動
- ロードバランサー設定
- ヘルスチェック統合

**成果物:**
- [ ] ロードバランサー設定ファイル

---

## 推奨タスク一覧

### オプション1: API ドキュメント自動生成
**推定時間**: 3時間

**ツール:**
- OpenAPI (Swagger) 仕様書作成
- Swagger UI組み込み

**成果物:**
- [ ] `openapi.yaml`
- [ ] Swagger UI統合

---

### オプション2: フロントエンドダッシュボード
**推定時間**: 20時間

**技術スタック:**
- React / Vue.js
- Chart.js (統計表示)
- 画像アップロード・表示

**成果物:**
- [ ] Webダッシュボード

---

### オプション3: WebSocket 通知
**推定時間**: 4時間

**用途:**
- リアルタイム検査結果通知
- 進捗状況の通知

**成果物:**
- [ ] WebSocketサーバー実装

---

## クラウドプロバイダー比較

### AWS (Amazon Web Services)

| サービス | 用途 | 月額概算 |
|---------|------|---------|
| **ECS Fargate** | コンテナ実行 | $30-50 |
| **S3** | 画像ストレージ | $5-20 |
| **RDS PostgreSQL** | データベース | $50-100 |
| **CloudWatch** | ロギング・監視 | $10-30 |
| **ALB** | ロードバランサー | $20-30 |
| **合計** | - | **$115-230/月** |

**メリット:**
- ✅ 豊富なサービス
- ✅ 高い信頼性
- ✅ 詳細なドキュメント

**デメリット:**
- ⚠️ コスト高め
- ⚠️ 学習曲線

---

### GCP (Google Cloud Platform)

| サービス | 用途 | 月額概算 |
|---------|------|---------|
| **Cloud Run** | コンテナ実行 | $25-40 |
| **Cloud Storage** | 画像ストレージ | $5-15 |
| **Cloud SQL** | データベース | $40-80 |
| **Cloud Logging** | ロギング | $10-20 |
| **合計** | - | **$80-155/月** |

**メリット:**
- ✅ Cloud Runが使いやすい
- ✅ コスパ良好
- ✅ AIサービスとの統合

**デメリット:**
- ⚠️ AWSより知名度低い

---

### Azure

| サービス | 用途 | 月額概算 |
|---------|------|---------|
| **Container Instances** | コンテナ実行 | $30-50 |
| **Blob Storage** | 画像ストレージ | $5-20 |
| **Azure Database** | データベース | $45-90 |
| **合計** | - | **$80-160/月** |

**メリット:**
- ✅ Microsoftエコシステム
- ✅ エンタープライズ向け

---

### DigitalOcean / Linode (低コスト選択肢)

| サービス | 用途 | 月額概算 |
|---------|------|---------|
| **Droplet** | VM (4GB) | $24 |
| **Spaces** | オブジェクトストレージ | $5 |
| **Managed Database** | PostgreSQL | $15-30 |
| **合計** | - | **$44-59/月** |

**メリット:**
- ✅ 低コスト
- ✅ シンプル
- ✅ 学習しやすい

**デメリット:**
- ⚠️ スケーラビリティに限界

---

### 推奨: **GCP Cloud Run** または **AWS ECS Fargate**

**理由:**
1. コンテナベースで移植性高い
2. 自動スケーリング
3. マネージドサービスで運用負荷低
4. コスト効率良好

---

## アーキテクチャ設計

### クラウドアーキテクチャ図

```
Internet
    ↓
[Cloud Load Balancer]
    ↓
[Nginx (HTTPS終端)]
    ↓
[Inspection Server] ×3インスタンス
    ↓         ↓
[S3/Cloud Storage]  [PostgreSQL RDS]
    (画像保存)        (検査履歴)
    ↓
[CloudWatch/Cloud Logging]
    (ロギング・監視)
```

### コンポーネント構成

```
┌─────────────────────────────────────┐
│  Frontend (オプション)                │
│  - React Dashboard                  │
│  - 画像アップロード                   │
│  - 結果表示                          │
└─────────────────────────────────────┘
            ↓ HTTPS
┌─────────────────────────────────────┐
│  API Gateway / Load Balancer        │
│  - HTTPS終端                         │
│  - レート制限                        │
│  - 認証                             │
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│  Inspection Server Container        │
│  - REST API (8080)                  │
│  - 4検出器                           │
│  - 認証ミドルウェア                   │
└─────────────────────────────────────┘
       ↓              ↓
┌──────────────┐  ┌──────────────┐
│ Cloud Storage│  │ PostgreSQL   │
│ (画像)        │  │ (履歴・統計)  │
└──────────────┘  └──────────────┘
```

---

## スケジュール・工数見積

### フェーズ別工数

| Phase | タスク | 工数 | 優先度 |
|-------|--------|------|--------|
| **Phase 1** | コンテナ化 | 3時間 | 最高 |
| **Phase 2** | セキュリティ | 7時間 | 高 |
| **Phase 3** | ストレージ連携 | 9時間 | 高 |
| **Phase 4** | CI/CD | 5時間 | 中 |
| **Phase 5** | モニタリング | 5時間 | 中 |
| **Phase 6** | データベース | 8時間 | 中 |
| **Phase 7** | スケーリング | 6時間 | 低 |
| **合計** | - | **43時間** | - |

### スケジュール（1週間集中作業の場合）

```
Day 1-2: Phase 1 (コンテナ化) + Phase 2 (セキュリティ)
Day 3-4: Phase 3 (ストレージ連携) + Phase 5 (モニタリング)
Day 5:   Phase 4 (CI/CD)
Day 6:   Phase 6 (データベース) ※オプション
Day 7:   テスト・デバッグ・ドキュメント
```

---

## コスト見積

### 初期費用

| 項目 | 費用 |
|-----|------|
| 開発工数 (43時間) | 開発者時給 × 43h |
| ドメイン取得 | $10-15/年 |
| SSL証明書 | $0 (Let's Encrypt) |
| **合計** | **開発費 + $10-15** |

### 月額ランニングコスト

#### 小規模構成 (月間1000検査)

| 項目 | GCP | AWS | DigitalOcean |
|-----|-----|-----|--------------|
| コンピューティング | $25 | $30 | $24 |
| ストレージ (10GB) | $0.5 | $1 | $5 |
| データベース | $15 | $30 | $15 |
| ネットワーク | $5 | $10 | 無料 |
| **合計** | **$45.5** | **$71** | **$44** |

#### 中規模構成 (月間10,000検査)

| 項目 | GCP | AWS |
|-----|-----|-----|
| コンピューティング | $80 | $100 |
| ストレージ (100GB) | $5 | $10 |
| データベース | $50 | $80 |
| ネットワーク | $20 | $40 |
| **合計** | **$155** | **$230** |

---

## 実装ロードマップ

### MVP (Minimum Viable Product) - 1週間

**必須機能のみ:**
- ✅ Phase 1: コンテナ化
- ✅ Phase 2: 基本的なセキュリティ
- ✅ Phase 5: 基本的なモニタリング

**デプロイ先:** DigitalOcean または GCP Cloud Run

**コスト:** $44-50/月

---

### v1.0 (本番運用) - 2週間

**すべての推奨機能:**
- ✅ Phase 1-5 すべて
- ✅ Phase 6: データベース統合

**デプロイ先:** GCP または AWS

**コスト:** $80-155/月

---

### v2.0 (エンタープライズ) - 1ヶ月

**スケーリング・高度な機能:**
- ✅ Phase 7: 完全なスケーリング対応
- ✅ WebSocket通知
- ✅ フロントエンドダッシュボード

**コスト:** $155-230/月

---

## 次のアクション

### 即座に実行可能

1. **Dockerfile作成** (2時間)
   ```bash
   touch Dockerfile .dockerignore
   ```

2. **Docker Compose設定** (1時間)
   ```bash
   touch docker-compose.yml
   ```

3. **ローカルでのDockerテスト** (30分)
   ```bash
   docker build -t inspection-server .
   docker run -p 8080:8080 inspection-server
   ```

### 推奨実装順序

```
Step 1: コンテナ化 (Phase 1)
   ↓
Step 2: ローカルDocker動作確認
   ↓
Step 3: クラウドプロバイダー選定
   ↓
Step 4: セキュリティ実装 (Phase 2)
   ↓
Step 5: クラウドデプロイ (MVP)
   ↓
Step 6: ストレージ連携 (Phase 3)
   ↓
Step 7: モニタリング (Phase 5)
   ↓
Step 8: CI/CD (Phase 4)
   ↓
Step 9: データベース (Phase 6)
   ↓
Step 10: 本番運用開始
```

---

## 結論

### 推奨プラン: **GCP Cloud Run + Cloud Storage + Cloud SQL**

**理由:**
1. ✅ コスパ最良 ($80-155/月)
2. ✅ 自動スケーリング
3. ✅ マネージドサービスで運用容易
4. ✅ C++コンテナに最適

### 最小構成で開始する場合

**Phase 1のみ実装** → DigitalOcean Droplet
**月額 $24** で開始可能

---

**作成者**: Claude Code
**最終更新**: 2025-11-21
