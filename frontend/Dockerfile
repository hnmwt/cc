# フロントエンドビルド用Dockerfile
# マルチステージビルド: ビルドステージ + 成果物コピー

# ========================================
# ビルドステージ
# ========================================
FROM node:20-alpine AS builder

WORKDIR /app

# package.jsonとpackage-lock.jsonをコピー
COPY package*.json ./

# 依存パッケージインストール
RUN npm ci --only=production=false

# ソースコードをコピー
COPY . .

# ビルド実行
RUN npm run build

# ========================================
# 成果物ステージ（軽量イメージ）
# ========================================
FROM scratch AS dist

# ビルド成果物をコピー
COPY --from=builder /app/dist /dist

# このイメージは実行されず、成果物を他のコンテナにコピーするためだけに使用される
