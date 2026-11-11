#!/usr/bin/env bash
# Basic functional smoke tests. Run from src/: make test
set -euo pipefail
cd "$(dirname "$0")/../src"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
printf 'echo HELLO\nexit\n' | ./mysh > "$tmp/out"
grep -qx HELLO "$tmp/out"
printf 'echo one > %s/a\necho two >> %s/a\ncat < %s/a\nexit\n' "$tmp" "$tmp" "$tmp" | ./mysh > "$tmp/out"
grep -qx one "$tmp/out"
grep -qx two "$tmp/out"
[ "$(cat "$tmp/a")" = $'one\ntwo' ]
printf 'printf a\\nb\\n | wc -l\nexit\n' | ./mysh > "$tmp/out" || true
printf 'seq 20|wc -l > %s/count\nexit\n' "$tmp" | ./mysh >/dev/null
[ "$(tr -d '[:space:]' < "$tmp/count")" = 20 ]
printf 'cd /tmp\npwd\nexit\n' | ./mysh > "$tmp/out"
grep -qx /tmp "$tmp/out"
printf 'unknown_command_xyz\nexit\n' | ./mysh > "$tmp/out" 2> "$tmp/errors"
grep -q unknown_command_xyz "$tmp/errors"
printf 'sleep 1 & ps\nexit\n' | ./mysh > "$tmp/out"
grep -q sleep "$tmp/out"
printf 'echo hello >\necho ok\nexit\n' | ./mysh > "$tmp/out" 2> "$tmp/errors"
grep -q 'Missing filename' "$tmp/errors"
grep -qx ok "$tmp/out"
echo 'All smoke tests passed.'
