#!/bin/bash
set -e
cd "$(dirname "$0")/.."
cmake -B _build -DCMAKE_BUILD_TYPE=Debug
cmake --build _build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
