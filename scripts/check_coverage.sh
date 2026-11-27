#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-cov}"
MIN_COVERAGE="${2:-95.0}"

echo "=== Measuring line coverage on src/core/ in ${BUILD_DIR} (threshold >= ${MIN_COVERAGE}%) ==="
SUMMARY=$(gcovr -r . "${BUILD_DIR}" --filter "src/core/" --txt-metric line)
echo "${SUMMARY}"

PCT=$(echo "${SUMMARY}" | awk '/^TOTAL/ { gsub(/%/, "", $4); print $4 }')
if [ -z "${PCT}" ]; then
    echo "ERROR: Failed to parse coverage percentage from gcovr output."
    exit 1
fi

echo "Measured src/core/ line coverage: ${PCT}% (required: >= ${MIN_COVERAGE}%)"
awk -v pct="${PCT}" -v min="${MIN_COVERAGE}" 'BEGIN {
    if (pct + 0.0 < min + 0.0) {
        printf "FAIL: Line coverage %.2f%% is below %.2f%%!\n", pct, min;
        exit 1;
    }
    printf "PASS: Line coverage %.2f%% meets >= %.2f%% requirement.\n", pct, min;
}'
