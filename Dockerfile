# ================================
# ビルドステージ
# ================================
FROM ubuntu:22.04 AS builder

# 非対話的インストール設定
ENV DEBIAN_FRONTEND=noninteractive

# 基本パッケージとビルドツールをインストール
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    && rm -rf /var/lib/apt/lists/*

# OpenCV 依存関係
RUN apt-get update && apt-get install -y \
    libopencv-dev \
    libopencv-core4.5d \
    libopencv-imgproc4.5d \
    libopencv-imgcodecs4.5d \
    libopencv-features2d4.5d \
    && rm -rf /var/lib/apt/lists/*

# Boost ライブラリ
RUN apt-get update && apt-get install -y \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# nlohmann-json
RUN apt-get update && apt-get install -y \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# spdlog
RUN apt-get update && apt-get install -y \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# 作業ディレクトリ設定
WORKDIR /app

# ソースコードをコピー
COPY CMakeLists.txt /app/
COPY include/ /app/include/
COPY src/ /app/src/
COPY tests/ /app/tests/
COPY config/ /app/config/

# ビルド
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc) inspection_server

# ================================
# 実行ステージ（軽量）
# ================================
FROM ubuntu:22.04

# 非対話的インストール設定
ENV DEBIAN_FRONTEND=noninteractive

# ランタイム依存関係のみインストール
RUN apt-get update && apt-get install -y \
    libopencv-core4.5d \
    libopencv-imgproc4.5d \
    libopencv-imgcodecs4.5d \
    libopencv-features2d4.5d \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libboost-filesystem1.74.0 \
    libspdlog1 \
    && rm -rf /var/lib/apt/lists/*

# 実行ユーザー作成（セキュリティのため非rootで実行）
RUN useradd -m -u 1000 -s /bin/bash inspector && \
    mkdir -p /app/data /app/logs /app/config && \
    chown -R inspector:inspector /app

# 作業ディレクトリ
WORKDIR /app

# ビルドステージから実行ファイルをコピー
COPY --from=builder /app/build/bin/inspection_server /usr/local/bin/inspection_server
COPY --from=builder /app/config /app/config

# データディレクトリ作成
RUN mkdir -p /app/data/input /app/data/output /app/logs && \
    chown -R inspector:inspector /app

# ユーザー切り替え
USER inspector

# ポート公開
EXPOSE 8080 9000

# ヘルスチェック
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/ || exit 1

# 環境変数
ENV LOG_LEVEL=info
ENV CONFIG_PATH=/app/config/default_config.json

# エントリポイント
CMD ["inspection_server", "-c", "/app/config/default_config.json"]
