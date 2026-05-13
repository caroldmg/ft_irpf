#!/bin/bash
# Funciones comunes para los tests de integración IRC

HOST=127.0.0.1
PORT=16667
PASS=testpass
SERVER_BIN="$(cd "$(dirname "$0")/../.." && pwd)/ircserv"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0

pass() { echo -e "  ${GREEN}[PASS]${NC} $1"; PASS_COUNT=$((PASS_COUNT + 1)); }
fail() { echo -e "  ${RED}[FAIL]${NC} $1"; FAIL_COUNT=$((FAIL_COUNT + 1)); }
info() { echo -e "  ${YELLOW}[INFO]${NC} $1"; }

# Envía líneas IRC y recoge la respuesta del servidor.
# Uso: irc_send <timeout_seg> "COMANDO param" "COMANDO2 param2" ...
# Cada argumento se envía como una línea \r\n separada.
# NOTA: se usa subshell para que printf no pase por $() — así no se pierden \n finales.
irc_send() {
    local timeout="$1"; shift
    (
        for line in "$@"; do
            printf "%s\r\n" "$line"
        done
    ) | timeout "$timeout" nc -q 1 -w "$timeout" "$HOST" "$PORT" 2>/dev/null
}

start_server() {
    "$SERVER_BIN" "$PORT" "$PASS" &>/dev/null &
    SERVER_PID=$!
    sleep 0.4
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo -e "${RED}ERROR: No se pudo arrancar el servidor (${SERVER_BIN})${NC}"
        exit 1
    fi
}

stop_server() {
    if [[ -n "$SERVER_PID" ]]; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
        unset SERVER_PID
    fi
    sleep 0.2   # tiempo para que el SO libere el puerto
}

print_summary() {
    local total=$((PASS_COUNT + FAIL_COUNT))
    echo ""
    echo -e "  Resultado: ${GREEN}${PASS_COUNT}${NC}/${total} pass"
    [[ $FAIL_COUNT -eq 0 ]] && return 0 || return 1
}
