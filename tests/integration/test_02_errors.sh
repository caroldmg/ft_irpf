#!/bin/bash
# Test 02 — Códigos de error del handshake
source "$(dirname "$0")/common.sh"

echo ""
echo "=== Test 02: Errores de registro ==="
start_server

# ── 1. Password incorrecto → ERR_PASSWDMISMATCH (464) ─────────────────────────
resp=$(irc_send 2 "PASS wrongpass" "NICK alice" "USER alice 0 * :Alice")
echo "$resp" | grep -q "464" \
    && pass "Password incorrecto → ERR_PASSWDMISMATCH (464)" \
    || fail "Password incorrecto → esperado 464 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 2. NICK sin PASS previo → ERR_PASSWDMISMATCH (464) ────────────────────────
resp=$(irc_send 2 "NICK alice" "USER alice 0 * :Alice")
echo "$resp" | grep -q "464" \
    && pass "NICK sin PASS → ERR_PASSWDMISMATCH (464)" \
    || fail "NICK sin PASS → esperado 464 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 3. NICK en uso → ERR_NICKNAMEINUSE (433) ──────────────────────────────────
# Primer cliente se registra; usamos sleep para mantener stdin abierto (nc no cierra)
(printf "PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n" "$PASS"; sleep 5) \
    | nc -w 6 "$HOST" "$PORT" &>/dev/null &
BG_PID=$!
sleep 0.5

resp=$(irc_send 2 "PASS $PASS" "NICK alice" "USER alice2 0 * :Alice2")
echo "$resp" | grep -q "433" \
    && pass "NICK duplicado → ERR_NICKNAMEINUSE (433)" \
    || fail "NICK duplicado → esperado 433 (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BG_PID" 2>/dev/null; wait "$BG_PID" 2>/dev/null

# ── 4. Nick inválido (empieza por dígito) → ERR_ERRONEUSNICKNAME (432) ────────
resp=$(irc_send 2 "PASS $PASS" "NICK 1badnick")
echo "$resp" | grep -q "432" \
    && pass "Nick empieza por dígito → ERR_ERRONEUSNICKNAME (432)" \
    || fail "Nick inválido → esperado 432 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 5. Nick con carácter prohibido (#) → ERR_ERRONEUSNICKNAME (432) ──────────
resp=$(irc_send 2 "PASS $PASS" "NICK #chan")
echo "$resp" | grep -q "432" \
    && pass "Nick con '#' → ERR_ERRONEUSNICKNAME (432)" \
    || fail "Nick con '#' → esperado 432 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 6. NICK sin parámetro → ERR_NONICKNAMEGIVEN (431) ────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK")
echo "$resp" | grep -q "431" \
    && pass "NICK sin parámetro → ERR_NONICKNAMEGIVEN (431)" \
    || fail "NICK vacío → esperado 431 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 7. USER con menos de 4 parámetros → ERR_NEEDMOREPARAMS (461) ──────────────
resp=$(irc_send 2 "PASS $PASS" "NICK eve" "USER eve")
echo "$resp" | grep -q "461" \
    && pass "USER con params insuficientes → ERR_NEEDMOREPARAMS (461)" \
    || fail "USER con params insuficientes → esperado 461 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 8. USER repetido tras registro → ERR_ALREADYREGISTRED (462) ──────────────
resp=$(irc_send 2 "PASS $PASS" "NICK frank" "USER frank 0 * :Frank" "USER frank 0 * :Frank2")
echo "$resp" | grep -q "462" \
    && pass "USER repetido → ERR_ALREADYREGISTRED (462)" \
    || fail "USER repetido → esperado 462 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 9. Comando desconocido (ya registrado) → ERR_UNKNOWNCOMMAND (421) ─────────
resp=$(irc_send 2 "PASS $PASS" "NICK gary" "USER gary 0 * :Gary" "FOOBAR arg")
echo "$resp" | grep -q "421" \
    && pass "Comando desconocido → ERR_UNKNOWNCOMMAND (421)" \
    || fail "Comando desconocido → esperado 421 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 10. Comando de canal sin registro previo → ERR_NOTREGISTERED (451) ────────
resp=$(irc_send 2 "JOIN #canal")
echo "$resp" | grep -q "451" \
    && pass "JOIN sin registro → ERR_NOTREGISTERED (451)" \
    || fail "JOIN sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

stop_server
print_summary
