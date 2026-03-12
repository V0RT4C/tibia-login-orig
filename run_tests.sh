#!/bin/sh
set -e
cd "$(dirname "$0")"
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
ctest --test-dir build -j"$(nproc)" --output-on-failure
