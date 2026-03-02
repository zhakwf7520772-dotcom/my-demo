#!/usr/bin/env bash
# Exit on error, unset variable usage, and pipeline failures to make smoke checks deterministic.
set -euo pipefail

# Resolve project root from script path so commands work no matter current working directory.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# Define build output directory used by cmake.
BUILD_DIR="${ROOT_DIR}/build"
# Define user_service binary path.
SERVICE_BIN="${BUILD_DIR}/user_service"
# Define user_client binary path used to call RPC methods.
CLIENT_BIN="${BUILD_DIR}/user_client"
# Define compose file directory.
COMPOSE_DIR="${ROOT_DIR}"

# Define target gRPC endpoint for client requests.
TARGET="127.0.0.1:50051"
# Define Redis host for cache assertions.
REDIS_HOST="127.0.0.1"
# Define Redis port for cache assertions.
REDIS_PORT="16379"
# Define log file path for cache hit assertions.
LOG_FILE="/tmp/user_service_smoke.log"

# Keep service pid for cleanup.
SERVICE_PID=""

# Cleanup function ensures background process and dependency containers are restored.
cleanup() {
  # Stop user_service background process if started.
  if [[ -n "${SERVICE_PID}" ]] && kill -0 "${SERVICE_PID}" 2>/dev/null; then
    kill "${SERVICE_PID}" >/dev/null 2>&1 || true
    wait "${SERVICE_PID}" >/dev/null 2>&1 || true
  fi
  # Try to restart mysql and redis so environment is left usable after failure tests.
  docker start my-demo-mysql >/dev/null 2>&1 || true
  docker start my-demo-redis >/dev/null 2>&1 || true
}

# Register cleanup to run on script exit.
trap cleanup EXIT

# Helper to run user_client with target endpoint.
run_client() {
  "${CLIENT_BIN}" --target "${TARGET}" "$@"
}

# Helper to extract numeric code from one-line client output.
extract_code() {
  # Use sed to parse "code=<number>" token.
  sed -n 's/.*code=\([0-9]\+\).*/\1/p' <<<"$1"
}

# Helper to extract user_id from create output.
extract_user_id() {
  # Use sed to parse "user_id=<number>" token.
  sed -n 's/.*user_id=\([0-9]\+\).*/\1/p' <<<"$1"
}

# Helper to assert response code equals expected value.
assert_code() {
  local output="$1"
  local expected="$2"
  local label="$3"
  local actual
  actual="$(extract_code "${output}")"
  if [[ "${actual}" != "${expected}" ]]; then
    echo "[FAIL] ${label}: expected code=${expected}, actual output=${output}"
    exit 1
  fi
  echo "[PASS] ${label}: code=${actual}"
}

# Move to project root so relative paths are stable.
cd "${ROOT_DIR}"

# Ensure mysql/redis dependencies are running before service startup.
docker compose -f "${COMPOSE_DIR}/docker-compose.yml" up -d mysql redis >/dev/null

# Configure project build directory.
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" >/dev/null
# Build user_service and user_client binaries.
cmake --build "${BUILD_DIR}" -j >/dev/null

# Start user_service in background and write logs to file for cache-hit checks.
USER_SVC__LOG__LOG_PATH="${LOG_FILE}" "${SERVICE_BIN}" --config "${ROOT_DIR}/config/user_service.yaml" >/tmp/user_service_stdout.log 2>&1 &
# Save process id for cleanup.
SERVICE_PID="$!"

# Wait briefly for server startup.
sleep 2

# Build unique email to avoid collisions across repeated smoke runs.
EMAIL="smoke_$(date +%s)@example.com"

# Case 1: create user should succeed.
out_create_ok="$(run_client create --name smoke_user --email "${EMAIL}")"
assert_code "${out_create_ok}" "0" "CreateUser success"

# Parse created user id for subsequent get tests.
USER_ID="$(extract_user_id "${out_create_ok}")"
if [[ -z "${USER_ID}" || "${USER_ID}" == "0" ]]; then
  echo "[FAIL] unable to parse user_id from output: ${out_create_ok}"
  exit 1
fi

# Case 2: duplicate email should return conflict.
out_create_dup="$(run_client create --name smoke_user2 --email "${EMAIL}")"
assert_code "${out_create_dup}" "40901" "CreateUser duplicate email"

# Case 3: invalid argument should return 40001.
out_create_invalid="$(run_client create --name "" --email bad_email || true)"
assert_code "${out_create_invalid}" "40001" "CreateUser invalid argument"

# Case 4: missing user should return 40401.
out_get_missing="$(run_client get --user_id 999999999)"
assert_code "${out_get_missing}" "40401" "GetUser missing user"

# Case 5: first get should succeed and populate redis cache.
out_get_first="$(run_client get --user_id "${USER_ID}")"
assert_code "${out_get_first}" "0" "GetUser first read"

# Assert redis key exists after first get.
redis_exists="$(redis-cli -h "${REDIS_HOST}" -p "${REDIS_PORT}" EXISTS "user:${USER_ID}" | tr -d '\r')"
if [[ "${redis_exists}" != "1" ]]; then
  echo "[FAIL] redis key user:${USER_ID} not found after first get"
  exit 1
fi
echo "[PASS] Redis cache key exists: user:${USER_ID}"

# Case 6: second get should still succeed; service log should include cache_hit=true.
out_get_second="$(run_client get --user_id "${USER_ID}")"
assert_code "${out_get_second}" "0" "GetUser second read"

# Give logger a moment to flush and then verify cache-hit marker.
sleep 1
if ! grep -q "cache_hit=true" "${LOG_FILE}"; then
  echo "[FAIL] cache_hit=true not found in log file ${LOG_FILE}"
  exit 1
fi
echo "[PASS] cache_hit=true found in log"

# Case 7: Redis unavailable should degrade to DB and still succeed.
docker stop my-demo-redis >/dev/null
sleep 1
out_get_redis_down="$(run_client get --user_id "${USER_ID}")"
assert_code "${out_get_redis_down}" "0" "GetUser with redis down"
docker start my-demo-redis >/dev/null
sleep 1

# Ensure cache miss before mysql-down check so request must hit DB.
redis-cli -h "${REDIS_HOST}" -p "${REDIS_PORT}" DEL "user:${USER_ID}" >/dev/null || true

# Case 8: MySQL unavailable should return database error code.
docker stop my-demo-mysql >/dev/null
sleep 1
out_get_mysql_down="$(run_client get --user_id "${USER_ID}")"
assert_code "${out_get_mysql_down}" "50001" "GetUser with mysql down"
docker start my-demo-mysql >/dev/null

echo "[PASS] smoke_user_service completed"
