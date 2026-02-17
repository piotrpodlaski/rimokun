#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <coverage.info> <path:min_pct> [<path:min_pct> ...]" >&2
  exit 2
fi

coverage_info="$1"
shift

if [[ ! -f "$coverage_info" ]]; then
  echo "Coverage info file not found: $coverage_info" >&2
  exit 2
fi

fail=0

for spec in "$@"; do
  path="${spec%%:*}"
  min_pct="${spec##*:}"
  if [[ -z "$path" || -z "$min_pct" || "$path" == "$min_pct" ]]; then
    echo "Invalid threshold spec: $spec (expected path:min_pct)" >&2
    fail=1
    continue
  fi

  summary="$(lcov --summary "$coverage_info" --include "*/$path" 2>/dev/null || true)"
  line_row="$(printf '%s\n' "$summary" | awk '/lines\.*:/{print; exit}')"
  pct="$(printf '%s\n' "$line_row" | sed -E 's/.*: *([0-9]+(\.[0-9]+)?)%.*/\1/')"

  if [[ -z "$pct" || "$pct" == "$line_row" ]]; then
    echo "Coverage gate could not read line coverage for $path" >&2
    fail=1
    continue
  fi

  if awk "BEGIN { exit !($pct + 0 >= $min_pct + 0) }"; then
    echo "[coverage gate] PASS $path: ${pct}% >= ${min_pct}%"
  else
    echo "[coverage gate] FAIL $path: ${pct}% < ${min_pct}%"
    fail=1
  fi
done

if [[ $fail -ne 0 ]]; then
  exit 1
fi

