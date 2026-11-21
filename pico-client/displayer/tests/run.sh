#!/bin/sh
set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_dir=${TMPDIR:-/tmp}/watering-text-renderer-test

mkdir -p "$build_dir"
${CC:-cc} \
  -std=c11 -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$test_dir/.." \
  "$test_dir/../text_renderer.c" \
  "$test_dir/text_renderer_test.c" \
  -o "$build_dir/text_renderer_test"
"$build_dir/text_renderer_test"
