#!/bin/bash
# Test 01 — Handshake completo y flujos de registro
source "$(dirname "$0")/common.sh"

echo ""
echo "=== Test 01: Handshake IRC ==="
start_server

# ── 1. Registro completo → RPL_WELCOME ────────────────────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK alice" "USER alice 0 * :Alice Doe")
if echo "$resp" | grep -q "001"; then
    pass "Registro completo → RPL_WELCOME (001)"
else
    fail "Registro completo → RPL_WELCOME (001) — respuesta: $(echo "$resp" | tr '\r' ' ')"
fi

# ── 2-4. Numericos de bienvenida ───────────────────────────────────────────────
echo "$resp" | grep -q "002" && pass "Bienvenida incluye RPL_YOURHOST (002)" || fail "Falta RPL_YOURHOST (002)"
echo "$resp" | grep -q "003" && pass "Bienvenida incluye RPL_CREATED (003)" || fail "Falta RPL_CREATED (003)"
echo "$resp" | grep -q "004" && pass "Bienvenida incluye RPL_MYINFO (004)"  || fail "Falta RPL_MYINFO (004)"

# ── 5. USER antes que NICK → bienvenida cuando llega NICK ─────────────────────
resp=$(irc_send 2 "PASS $PASS" "USER bob 0 * :Bob" "NICK bob")
if echo "$resp" | grep -q "001"; then
    pass "USER antes que NICK → bienvenida igualmente"
else
    fail "USER antes que NICK → sin bienvenida"
fi

# ── 6. Cambio de nick después de registrado ───────────────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK carol" "USER carol 0 * :Carol" "NICK carol2")
if echo "$resp" | grep -qE "NICK :?carol2"; then
    pass "Cambio de nick post-registro → mensaje NICK"
else
    fail "Cambio de nick post-registro → sin notificación NICK (resp: $(echo "$resp" | tr '\r' ' '))"
fi

# ── 7. QUIT limpio (el servidor no debe crashear) ─────────────────────────────
irc_send 2 "PASS $PASS" "NICK dave" "USER dave 0 * :Dave" "QUIT :adios" &>/dev/null
sleep 0.2
if kill -0 "$SERVER_PID" 2>/dev/null; then
    pass "QUIT no provoca crash del servidor"
else
    fail "QUIT crasheó el servidor"
    start_server
fi

stop_server
print_summary
