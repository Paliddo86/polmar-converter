#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p dist
c++ -std=c++17 -O2 polmar_core.cpp main_test.cpp -o dist/polmar-core-test
echo "Creato: $(pwd)/dist/polmar-core-test"
