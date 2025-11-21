# Docker デプロイガイド

**作成日**: 2025-11-21
**対象**: 外観検査アプリケーション

---

## 📋 目次

1. [クイックスタート](#クイックスタート)
2. [ビルド方法](#ビルド方法)
3. [実行方法](#実行方法)
4. [環境変数](#環境変数)
5. [ボリューム設定](#ボリューム設定)
6. [トラブルシューティング](#トラブルシューティング)

---

## クイックスタート

### 前提条件

- Docker 20.10以上
- Docker Compose 2.0以上
- 4GB以上の空きメモリ

### 最速起動（5分）

```bash
# 1. リポジトリクローン（既に完了している場合はスキップ）
cd /Users/wataruhomma/my_app/c

# 2. Dockerイメージビルド
./build-docker.sh

# 3. コンテナ起動
docker-compose up -d

# 4. 動作確認
curl http://localhost:8080/
```

**これで外観検査サーバーが起動します！**

---

## ビルド方法

### 方法1: ビルドスクリプト使用（推奨）

```bash
# 基本ビルド
./build-docker.sh

# バージョンタグ付きビルド
./build-docker.sh -t v1.0.0

# キャッシュなしビルド（クリーンビルド）
./build-docker.sh --no-cache

# 特定プラットフォーム向けビルド
./build-docker.sh --platform linux/amd64

# ヘルプ表示
./build-docker.sh --help
```

### 方法2: 直接Dockerコマンド

```bash
# 基本ビルド
docker build -t inspection-server:latest .

# タグ指定
docker build -t inspection-server:v1.0.0 .

# マルチステージビルドの確認
docker build --target builder -t inspection-server:builder .
```

### ビルド時間

| ビルドタイプ | 初回 | 2回目以降（キャッシュあり） |
|------------|------|--------------------------|
| **通常ビルド** | 8-12分 | 1-3分 |
| **キャッシュなし** | 8-12分 | 8-12分 |

---

## 実行方法

### Docker Compose使用（推奨）

#### 開発環境

```bash
# 起動
docker-compose up -d

# ログ確認
docker-compose logs -f inspection_server

# 停止
docker-compose down

# 停止 + ボリューム削除
docker-compose down -v
```

#### 本番環境

```bash
# 環境変数設定
export DB_PASSWORD="secure_password"
export AWS_REGION="us-east-1"
export S3_BUCKET="my-inspection-bucket"

# 起動
docker-compose -f docker-compose.prod.yml up -d

# ログ確認
docker-compose -f docker-compose.prod.yml logs -f

# 停止
docker-compose -f docker-compose.prod.yml down
```

### Docker直接実行

```bash
# 基本起動
docker run -d \
  --name inspection_server \
  -p 8080:8080 \
  -p 9000:9000 \
  -v $(pwd)/config:/app/config:ro \
  -v $(pwd)/data:/app/data \
  -v $(pwd)/logs:/app/logs \
  inspection-server:latest

# 環境変数指定
docker run -d \
  --name inspection_server \
  -p 8080:8080 \
  -p 9000:9000 \
  -e LOG_LEVEL=debug \
  -e CONFIG_PATH=/app/config/custom_config.json \
  -v $(pwd)/config:/app/config:ro \
  inspection-server:latest

# インタラクティブモード（デバッグ用）
docker run -it --rm \
  -p 8080:8080 \
  -p 9000:9000 \
  -v $(pwd)/config:/app/config:ro \
  inspection-server:latest

# ワンショット実行（終了後削除）
docker run --rm \
  -v $(pwd)/data/input:/app/data/input \
  -v $(pwd)/data/output:/app/data/output \
  inspection-server:latest \
  inspection_server -c /app/config/default_config.json
```

---

## 環境変数

### 基本設定

| 変数名 | 説明 | デフォルト値 | 必須 |
|--------|------|------------|------|
| `LOG_LEVEL` | ログレベル (debug/info/warn/error) | `info` | No |
| `CONFIG_PATH` | 設定ファイルパス | `/app/config/default_config.json` | No |

### クラウド連携（将来実装予定）

| 変数名 | 説明 | デフォルト値 | 必須 |
|--------|------|------------|------|
| `CLOUD_STORAGE_ENABLED` | クラウドストレージ有効化 | `false` | No |
| `AWS_REGION` | AWSリージョン | `us-east-1` | Yes (AWS使用時) |
| `AWS_ACCESS_KEY_ID` | AWS アクセスキー | - | Yes (AWS使用時) |
| `AWS_SECRET_ACCESS_KEY` | AWS シークレットキー | - | Yes (AWS使用時) |
| `S3_BUCKET` | S3バケット名 | - | Yes (S3使用時) |

### データベース（オプション）

| 変数名 | 説明 | デフォルト値 | 必須 |
|--------|------|------------|------|
| `DB_HOST` | データベースホスト | `postgres` | No |
| `DB_PORT` | データベースポート | `5432` | No |
| `DB_NAME` | データベース名 | `inspection_db` | No |
| `DB_USER` | データベースユーザー | `inspector` | No |
| `DB_PASSWORD` | データベースパスワード | - | Yes (DB使用時) |

### 使用例

```bash
# 環境変数ファイル作成
cat > .env << EOF
LOG_LEVEL=debug
CONFIG_PATH=/app/config/production_config.json
DB_PASSWORD=my_secure_password
S3_BUCKET=my-inspection-images
AWS_REGION=ap-northeast-1
EOF

# Docker Composeで使用
docker-compose --env-file .env up -d
```

---

## ボリューム設定

### 推奨マウント

```yaml
volumes:
  # 設定ファイル（読み取り専用）
  - ./config:/app/config:ro

  # 入力画像ディレクトリ
  - ./data/input:/app/data/input

  # 出力ディレクトリ（検査結果、画像）
  - ./data/output:/app/data/output

  # ログディレクトリ
  - ./logs:/app/logs
```

### ディレクトリ構成

```
/app
├── config/              # 設定ファイル
│   └── default_config.json
├── data/
│   ├── input/          # 入力画像
│   └── output/         # 出力データ
│       ├── images/     # 処理済み画像
│       └── results/    # CSV等の結果
└── logs/               # ログファイル
    └── inspection.log
```

### 権限設定

コンテナ内では `inspector` ユーザー（UID: 1000）で実行されます。

```bash
# ホスト側のディレクトリ権限設定
sudo chown -R 1000:1000 data/ logs/
sudo chmod -R 755 data/ logs/
```

---

## API エンドポイント

### ヘルスチェック

```bash
# サーバー稼働確認
curl http://localhost:8080/

# レスポンス例
{
  "status": "ok",
  "version": "1.0.0",
  "uptime": 3600
}
```

### 検査実行

```bash
# 画像検査
curl -X POST http://localhost:8080/api/v1/inspect \
  -H "Content-Type: application/json" \
  -d '{
    "image_path": "/app/data/input/sample.jpg"
  }'

# レスポンス例
{
  "result": "NG",
  "defect_count": 2,
  "defects": [...]
}
```

### 統計情報

```bash
# 統計取得
curl http://localhost:8080/api/v1/statistics

# レスポンス例
{
  "total_inspections": 1234,
  "ng_count": 56,
  "ok_count": 1178,
  "average_processing_time_ms": 45.2
}
```

---

## トラブルシューティング

### コンテナが起動しない

```bash
# ログ確認
docker-compose logs inspection_server

# コンテナ状態確認
docker ps -a

# コンテナ再起動
docker-compose restart inspection_server
```

### ポートが使用中

```bash
# ポート8080を使用しているプロセス確認
lsof -i :8080

# 別のポートで起動
docker run -p 9080:8080 inspection-server:latest
```

### ビルドエラー

```bash
# キャッシュクリア
docker builder prune -a

# 完全再ビルド
./build-docker.sh --no-cache

# ビルドログ詳細表示
docker build --progress=plain -t inspection-server:latest .
```

### メモリ不足

```bash
# Dockerメモリ制限確認
docker info | grep Memory

# メモリ制限付きで起動
docker run -m 4g -p 8080:8080 inspection-server:latest
```

### 設定ファイルが読み込まれない

```bash
# コンテナ内の設定確認
docker exec inspection_server ls -la /app/config/

# 設定ファイル内容確認
docker exec inspection_server cat /app/config/default_config.json

# ボリュームマウント確認
docker inspect inspection_server | grep -A 10 Mounts
```

### OpenCV関連エラー

```bash
# 依存ライブラリ確認
docker exec inspection_server ldd /usr/local/bin/inspection_server

# OpenCVバージョン確認
docker exec inspection_server pkg-config --modversion opencv4
```

---

## パフォーマンス最適化

### イメージサイズ削減

現在のイメージサイズ: 約800MB-1.2GB

**最適化のポイント:**
- ✅ マルチステージビルド使用済み
- ✅ 不要なファイル除外（.dockerignore）
- ⚠️ さらなる削減: Alpine Linuxベース化（但し互換性要確認）

### リソース制限

```yaml
# docker-compose.yml
services:
  inspection_server:
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 4G
        reservations:
          cpus: '1.0'
          memory: 2G
```

### ビルドキャッシュ活用

```bash
# BuildKitを使用（高速化）
export DOCKER_BUILDKIT=1
docker build -t inspection-server:latest .
```

---

## セキュリティ

### 非rootユーザーで実行

Dockerfileで`inspector`ユーザー（UID: 1000）を作成し、非rootで実行。

```dockerfile
USER inspector
```

### 読み取り専用マウント

```yaml
volumes:
  - ./config:/app/config:ro  # 読み取り専用
```

### シークレット管理

```bash
# Docker secretsを使用（Swarm mode）
echo "my_secret_password" | docker secret create db_password -

# または環境変数ファイル（.envを.gitignoreに追加）
echo ".env" >> .gitignore
```

---

## 次のステップ

1. **ローカルテスト完了後**
   - クラウドプロバイダー選定
   - コンテナレジストリ設定

2. **本番デプロイ準備**
   - SSL証明書取得
   - ドメイン設定
   - CI/CD パイプライン構築

3. **モニタリング追加**
   - Prometheus + Grafana
   - CloudWatch / Cloud Logging

詳細は `CLOUD_DEPLOYMENT_PLAN.md` を参照。

---

**作成者**: Claude Code
**最終更新**: 2025-11-21
