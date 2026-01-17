#!/bin/bash
IMAGE_NAME="pio:$(head /dev/urandom | tr -dc 'a-z0-9' | head -c 8)"
PIO_BIN="/home/debian/.platformio/penv/bin/pio"

cleanup() {
  docker rmi "$IMAGE_NAME" &>/dev/null
}
trap cleanup EXIT

echo "Building Docker image..."
docker build -q -f .devcontainer/Dockerfile -t "$IMAGE_NAME" .devcontainer

mkdir -p .pio

echo "Building firmware..."
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  -v "$(pwd)/.pio:/tmp/.platformio" \
  -e PLATFORMIO_CORE_DIR=/tmp/.platformio \
  -u $(id -u):$(id -g) \
  "$IMAGE_NAME" "$PIO_BIN" run

echo "Building filesystem..."
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  -v "$(pwd)/.pio:/tmp/.platformio" \
  -e PLATFORMIO_CORE_DIR=/tmp/.platformio \
  -u $(id -u):$(id -g) \
  "$IMAGE_NAME" "$PIO_BIN" run --target buildfs

echo "Done! Binaries in .pio/build/esp12e/"
ls -la .pio/build/esp12e/*.bin 2>/dev/null || echo "No .bin files found :("
