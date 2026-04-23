#!/usr/bin/env bash
set -euo pipefail

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-6380}"
REQUESTS="${REQUESTS:-100000}"
CLIENTS="${CLIENTS:-50}"
TESTS="${TESTS:-set,get,incr}"

if ! command -v redis-benchmark >/dev/null 2>&1; then
    echo "redis-benchmark not found. Please install redis-tools first." >&2
    exit 1
fi

echo "TinyRedis benchmark"
echo "host=${HOST} port=${PORT} requests=${REQUESTS} clients=${CLIENTS} tests=${TESTS}"
echo

redis-benchmark -h "${HOST}" -p "${PORT}" -n "${REQUESTS}" -c "${CLIENTS}" -q -t "${TESTS}"
