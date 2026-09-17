#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/native
xcrun clang++ -std=c++20 -fobjc-arc -fobjc-exceptions -Wall -Wextra \
  -framework Foundation -framework Security -Imacos \
  macos/RNSecureStoreNamespace.mm macos/RNSecureStoreBackend.mm tests/native/macos.mm \
  -o build/native/macos-tests
build/native/macos-tests "$@"
