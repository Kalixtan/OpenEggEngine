#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
mkdir -p build/docs/api
python3 tools/generate_script_api.py docs/api/_generated
doxygen docs/api/Doxyfile
