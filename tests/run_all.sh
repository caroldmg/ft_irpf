#!/bin/bash
# Runner principal — ejecuta unit tests y todos los integration tests

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TESTS_DIR="$ROOT/tests"

RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'
NC='\033[0m'

TOTAL_PASS=0
TOTAL_FAIL=0

run_suite() {
    local label="$1"
    local cmd="$2"

    output=$(eval "$cmd" 2>&1)
    local exit_code=$?

    echo "$output"

    local p f
    p=$(echo "$output" | grep -c '\[PASS\]' || true)
    f=$(echo "$output" | grep -c '\[FAIL\]' || true)
    TOTAL_PASS=$((TOTAL_PASS + p))
    TOTAL_FAIL=$((TOTAL_FAIL + f))

    return $exit_code
}

# ── Compilar unit tests ───────────────────────────────────────────────────────
echo -e "\n${BOLD}Compilando unit tests...${NC}"
make -C "$ROOT" test_build 2>&1
if [[ $? -ne 0 ]]; then
    echo -e "${RED}Error compilando unit tests${NC}"
    exit 1
fi

# ── Unit tests ────────────────────────────────────────────────────────────────
run_suite "parser" "$TESTS_DIR/unit/test_parser"

# ── Integration tests ─────────────────────────────────────────────────────────
for script in "$TESTS_DIR"/integration/test_*.sh; do
    chmod +x "$script"
    run_suite "$(basename "$script")" "bash $script"
done

# ── Resumen global ────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}══════════════════════════════════════${NC}"
total=$((TOTAL_PASS + TOTAL_FAIL))
if [[ $TOTAL_FAIL -eq 0 ]]; then
    echo -e "${BOLD}${GREEN}  TODOS LOS TESTS APROBADOS: ${TOTAL_PASS}/${total}${NC}"
else
    echo -e "${BOLD}${RED}  FALLOS: ${TOTAL_FAIL}/${total}${NC}"
fi
echo -e "${BOLD}══════════════════════════════════════${NC}"
echo ""

[[ $TOTAL_FAIL -eq 0 ]]
