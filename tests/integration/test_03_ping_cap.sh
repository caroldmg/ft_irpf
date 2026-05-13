#!/bin/bash
# Test 03 — PING/PONG y negociación CAP
source "$(dirname "$0")/common.sh"

echo ""
echo "=== Test 03: PING y CAP ==="
start_server

# ── 1-2. PING → PONG con token ────────────────────────────────────────────────
resp=$(irc_send 3 "PASS $PASS" "NICK hank" "USER hank 0 * :Hank" "PING :token123")
echo "$resp" | grep -q "PONG" \
    && pass "PING → PONG recibido" \
    || fail "PING → sin PONG (resp: $(echo "$resp" | tr '\r' ' '))"

echo "$resp" | grep -q "token123" \
    && pass "PONG incluye el token del PING" \
    || fail "PONG no incluye el token"

# ── 3. CAP LS → respuesta CAP * LS ────────────────────────────────────────────
resp=$(irc_send 2 "CAP LS" "PASS $PASS" "NICK ivan" "USER ivan 0 * :Ivan")
echo "$resp" | grep -q "CAP \* LS" \
    && pass "CAP LS → servidor responde CAP * LS" \
    || fail "CAP LS → sin respuesta CAP * LS (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 4. Registro completo tras CAP LS + CAP END ────────────────────────────────
resp=$(irc_send 2 "CAP LS" "CAP END" "PASS $PASS" "NICK joan" "USER joan 0 * :Joan")
echo "$resp" | grep -q "001" \
    && pass "CAP LS + CAP END + registro → RPL_WELCOME (001)" \
    || fail "CAP LS + CAP END → sin bienvenida"

# ── 5. CAP REQ → NAK (no soportamos extensiones) ─────────────────────────────
resp=$(irc_send 2 "CAP REQ :multi-prefix" "PASS $PASS" "NICK karl" "USER karl 0 * :Karl")
echo "$resp" | grep -q "NAK" \
    && pass "CAP REQ no soportada → CAP * NAK" \
    || fail "CAP REQ → se esperaba NAK (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 6. Flujo HexChat (CAP LS 302, PASS, NICK, USER, CAP END) ─────────────────
resp=$(irc_send 2 "CAP LS 302" "PASS $PASS" "NICK leo" "USER leo 0 * :Leo" "CAP END")
echo "$resp" | grep -q "001" \
    && pass "Flujo HexChat (CAP LS 302 + PASS + NICK + USER + CAP END) → bienvenida" \
    || fail "Flujo HexChat → sin bienvenida"

stop_server
print_summary
