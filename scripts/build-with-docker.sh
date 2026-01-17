#!/bin/bash

readonly IMAGE_NAME="ghcr.io/times-z/devcontainer:latest"
readonly BUILD_DIR=".pio/build/esp12e/"
readonly PIO_BIN="/home/debian/.platformio/penv/bin/pio"

mkdir -p .pio

docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  -v "$(pwd)/.pio:/tmp/.platformio" \
  -e PLATFORMIO_CORE_DIR=/tmp/.platformio \
  -u $(id -u):$(id -g) \
  "$IMAGE_NAME" ${PIO_BIN} run

docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  -v "$(pwd)/.pio:/tmp/.platformio" \
  -e PLATFORMIO_CORE_DIR=/tmp/.platformio \
  -u $(id -u):$(id -g) \
  "$IMAGE_NAME" ${PIO_BIN} run --target buildfs

echo "Done! Binaries in ${BUILD_DIR}"

ls -la ${BUILD_DIR}*.bin 2>/dev/null || echo "No .bin files found :("
