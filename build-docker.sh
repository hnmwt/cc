#!/bin/bash

# Docker イメージビルドスクリプト
# Usage: ./build-docker.sh [OPTIONS]

set -e

# カラー出力
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# デフォルト値
IMAGE_NAME="inspection-server"
IMAGE_TAG="latest"
BUILD_TYPE="release"
PLATFORM=""

# ヘルプ表示
show_help() {
    cat << EOF
Usage: ./build-docker.sh [OPTIONS]

Docker イメージをビルドします。

OPTIONS:
    -h, --help              このヘルプを表示
    -t, --tag TAG           イメージタグ (default: latest)
    -d, --debug             デバッグビルド
    -n, --no-cache          キャッシュを使用しない
    -p, --platform PLATFORM プラットフォーム指定 (例: linux/amd64)
    --push                  ビルド後にレジストリにプッシュ

EXAMPLES:
    ./build-docker.sh
    ./build-docker.sh -t v1.0.0
    ./build-docker.sh -d -n
    ./build-docker.sh --platform linux/amd64 --push

EOF
}

# 引数解析
NO_CACHE=""
PUSH=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -t|--tag)
            IMAGE_TAG="$2"
            shift 2
            ;;
        -d|--debug)
            BUILD_TYPE="debug"
            shift
            ;;
        -n|--no-cache)
            NO_CACHE="--no-cache"
            shift
            ;;
        -p|--platform)
            PLATFORM="--platform $2"
            shift 2
            ;;
        --push)
            PUSH=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# ビルド情報表示
echo -e "${GREEN}=== Docker Image Build ===${NC}"
echo "Image Name: ${IMAGE_NAME}"
echo "Image Tag: ${IMAGE_TAG}"
echo "Build Type: ${BUILD_TYPE}"
echo ""

# Dockerがインストールされているか確認
if ! command -v docker &> /dev/null; then
    echo -e "${RED}Error: Docker is not installed${NC}"
    exit 1
fi

# Dockerが起動しているか確認
if ! docker info &> /dev/null; then
    echo -e "${RED}Error: Docker is not running${NC}"
    exit 1
fi

# ビルド開始
echo -e "${YELLOW}Building Docker image...${NC}"
START_TIME=$(date +%s)

docker build \
    $NO_CACHE \
    $PLATFORM \
    -t ${IMAGE_NAME}:${IMAGE_TAG} \
    -t ${IMAGE_NAME}:latest \
    -f Dockerfile \
    .

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo -e "${GREEN}✓ Build completed in ${DURATION} seconds${NC}"

# イメージ情報表示
echo ""
echo -e "${GREEN}=== Image Info ===${NC}"
docker images ${IMAGE_NAME}:${IMAGE_TAG}

# イメージサイズ
IMAGE_SIZE=$(docker images ${IMAGE_NAME}:${IMAGE_TAG} --format "{{.Size}}")
echo "Image Size: ${IMAGE_SIZE}"

# プッシュ
if [ "$PUSH" = true ]; then
    echo ""
    echo -e "${YELLOW}Pushing to registry...${NC}"

    # レジストリURLを環境変数から取得
    if [ -z "$DOCKER_REGISTRY" ]; then
        echo -e "${RED}Error: DOCKER_REGISTRY environment variable is not set${NC}"
        exit 1
    fi

    docker tag ${IMAGE_NAME}:${IMAGE_TAG} ${DOCKER_REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}
    docker push ${DOCKER_REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}

    echo -e "${GREEN}✓ Pushed to ${DOCKER_REGISTRY}${NC}"
fi

echo ""
echo -e "${GREEN}=== Build Summary ===${NC}"
echo "✓ Docker image built successfully"
echo "  Image: ${IMAGE_NAME}:${IMAGE_TAG}"
echo "  Size: ${IMAGE_SIZE}"
echo "  Duration: ${DURATION}s"
echo ""
echo "Next steps:"
echo "  1. Test locally:  docker run -p 8080:8080 ${IMAGE_NAME}:${IMAGE_TAG}"
echo "  2. Use compose:   docker-compose up"
echo "  3. Push to registry: ./build-docker.sh --push"
