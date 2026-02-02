#!/usr/bin/env bash
set -euo pipefail
PY=$(command -v python3 || command -v python)
"${PY}" "$(dirname "$0")/generate_openapi.py" --json-out "$(pwd)/openapi.json" --yaml-out "$(pwd)/swagger.yml"
echo "Wrote openapi.json and swagger.yml"
