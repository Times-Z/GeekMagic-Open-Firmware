#!/bin/bash

IMAGE_NAME="pio-$(head /dev/urandom | tr -dc 'a-z0-9' | head -c 8)"

DOCKERFILE='FROM python:3.11-slim
RUN pip install --no-cache-dir platformio
WORKDIR /workspace
ENTRYPOINT ["pio"]'

cleanup() {
  docker rmi "$IMAGE_NAME" &>/dev/null
}
trap cleanup EXIT

echo "Building Docker image..."
echo "$DOCKERFILE" | docker build -q -t "$IMAGE_NAME" -f - .

mkdir -p .pio

echo "Building firmware..."
docker run --rm \
  -v "$(pwd):/workspace" \
  -v "$(pwd)/.pio:/tmp/.platformio" \
  -e PLATFORMIO_CORE_DIR=/tmp/.platformio \
  -u $(id -u):$(id -g) \
  "$IMAGE_NAME" run

echo "Building filesystem..."
docker run --rm \
  -v "$(pwd):/workspace" \
  -v "$(pwd)/.pio:/tmp/.platformio" \
  -e PLATFORMIO_CORE_DIR=/tmp/.platformio \
  -u $(id -u):$(id -g) \
  "$IMAGE_NAME" run --target buildfs

echo "Done! Binaries in .pio/build/esp12e/"
ls -la .pio/build/esp12e/*.bin 2>/dev/null || echo "No .bin files found :("
