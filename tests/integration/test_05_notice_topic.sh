#!/bin/bash
# Test 05 — NOTICE y TOPIC
source "$(dirname "$0")/common.sh"

echo ""
echo "=== Test 05: NOTICE / TOPIC ==="
start_server

# ════════════════════════════════════════════════════════════
#  NOTICE
# ════════════════════════════════════════════════════════════

# ── 1. NOTICE sin registro → silencio (sin error) ────────────────────────────
resp=$(irc_send 2 "NOTICE alice :hola")
if echo "$resp" | grep -qE "451|ERROR|NOTICE"; then
    fail "NOTICE sin registro → debe ser silencioso (resp: $(echo "$resp" | tr '\r' ' '))"
else
    pass "NOTICE sin registro → silencioso (sin error)"
fi

# ── 2. NOTICE sin texto → silencio (sin error) ───────────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK na" "USER na 0 * :Na" "NOTICE alice")
if echo "$resp" | grep -q "412\|411\|ERROR"; then
    fail "NOTICE sin texto → debe ser silencioso (resp: $(echo "$resp" | tr '\r' ' '))"
else
    pass "NOTICE sin texto → silencioso (sin error)"
fi

# ── 3. NOTICE a nick inexistente → silencio (sin error) ──────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK nb" "USER nb 0 * :Nb" "NOTICE nobody :test")
if echo "$resp" | grep -q "401\|ERROR"; then
    fail "NOTICE a nick inexistente → debe ser silencioso (resp: $(echo "$resp" | tr '\r' ' '))"
else
    pass "NOTICE a nick inexistente → silencioso (sin error)"
fi

# ── 4. NOTICE a canal sin pertenecer → silencio (sin error) ──────────────────
(printf "PASS %s\r\nNICK ncowner\r\nUSER ncowner 0 * :O\r\nJOIN #nctestchan\r\n" "$PASS"; sleep 5) \
    | nc -w 6 "$HOST" "$PORT" &>/dev/null &
BG1=$!
sleep 0.4

resp=$(irc_send 2 "PASS $PASS" "NICK nc_out" "USER nc_out 0 * :Out" "NOTICE #nctestchan :intento")
if echo "$resp" | grep -q "404\|ERROR"; then
    fail "NOTICE a canal sin pertenecer → debe ser silencioso (resp: $(echo "$resp" | tr '\r' ' '))"
else
    pass "NOTICE a canal sin pertenecer → silencioso (sin error)"
fi
kill "$BG1" 2>/dev/null; wait "$BG1" 2>/dev/null

# ── 5. NOTICE nick-a-nick → mensaje llega al destinatario ────────────────────
(printf "PASS %s\r\nNICK nrecv\r\nUSER nrecv 0 * :NRecv\r\n" "$PASS"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" > /tmp/irc_nrecv_out 2>/dev/null &
BG2=$!
sleep 0.5

(printf "PASS %s\r\nNICK nsender\r\nUSER nsender 0 * :NSender\r\nNOTICE nrecv :hola notice\r\n" "$PASS") \
    | timeout 2 nc -q 1 -w 2 "$HOST" "$PORT" &>/dev/null
sleep 0.5

kill "$BG2" 2>/dev/null; wait "$BG2" 2>/dev/null
if grep -q "hola notice" /tmp/irc_nrecv_out 2>/dev/null; then
    pass "NOTICE nick-a-nick → mensaje llega al destinatario"
else
    fail "NOTICE nick-a-nick → mensaje no llegó (out: $(cat /tmp/irc_nrecv_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_nrecv_out

# ── 6. NOTICE en canal → mensaje llega a otro miembro ────────────────────────
(printf "PASS %s\r\nNICK nlistener\r\nUSER nlistener 0 * :NListener\r\nJOIN #nchat\r\n" "$PASS"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" > /tmp/irc_nlistener_out 2>/dev/null &
BG3=$!
sleep 0.5

(printf "PASS %s\r\nNICK ntalker\r\nUSER ntalker 0 * :NTalker\r\nJOIN #nchat\r\nNOTICE #nchat :hi notice channel\r\n" "$PASS") \
    | timeout 2 nc -q 1 -w 2 "$HOST" "$PORT" &>/dev/null
sleep 0.5

kill "$BG3" 2>/dev/null; wait "$BG3" 2>/dev/null
if grep -q "hi notice channel" /tmp/irc_nlistener_out 2>/dev/null; then
    pass "NOTICE al canal → mensaje llega a otro miembro"
else
    fail "NOTICE al canal → mensaje no llegó al otro miembro (out: $(cat /tmp/irc_nlistener_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_nlistener_out

# ── 7. Emisor NO recibe su propio NOTICE en canal ────────────────────────────
(printf "PASS %s\r\nNICK nother\r\nUSER nother 0 * :NOther\r\nJOIN #nself\r\n" "$PASS"; sleep 5) \
    | nc -w 6 "$HOST" "$PORT" &>/dev/null &
BG4=$!
sleep 0.4

resp=$(irc_send 2 \
    "PASS $PASS" "NICK nself" "USER nself 0 * :NSelf" \
    "JOIN #nself" "NOTICE #nself :solo para otros")
# El texto no debería llegar de vuelta al emisor con su propio nick como origen
if echo "$resp" | grep "NOTICE #nself" | grep -q "nself!"; then
    fail "NOTICE al canal → emisor recibió su propio mensaje"
else
    pass "NOTICE al canal → emisor NO recibe eco de su propio mensaje"
fi
kill "$BG4" 2>/dev/null; wait "$BG4" 2>/dev/null

# ════════════════════════════════════════════════════════════
#  TOPIC
# ════════════════════════════════════════════════════════════

# ── 8. TOPIC sin registro → ERR_NOTREGISTERED (451) ──────────────────────────
resp=$(irc_send 2 "TOPIC #test")
echo "$resp" | grep -q "451" \
    && pass "TOPIC sin registro → ERR_NOTREGISTERED (451)" \
    || fail "TOPIC sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 9. TOPIC sin nombre de canal → ERR_NEEDMOREPARAMS (461) ──────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK ta" "USER ta 0 * :Ta" "TOPIC")
echo "$resp" | grep -q "461" \
    && pass "TOPIC sin parámetros → ERR_NEEDMOREPARAMS (461)" \
    || fail "TOPIC sin parámetros → esperado 461 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 10. TOPIC en canal inexistente → ERR_NOSUCHCHANNEL (403) ─────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK tb" "USER tb 0 * :Tb" "TOPIC #noexiste")
echo "$resp" | grep -q "403" \
    && pass "TOPIC canal inexistente → ERR_NOSUCHCHANNEL (403)" \
    || fail "TOPIC canal inexistente → esperado 403 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 11. TOPIC en canal que no has unido → ERR_NOTONCHANNEL (442) ─────────────
(printf "PASS %s\r\nNICK tcowner\r\nUSER tcowner 0 * :O\r\nJOIN #tocchan\r\n" "$PASS"; sleep 5) \
    | nc -w 6 "$HOST" "$PORT" &>/dev/null &
BG5=$!
sleep 0.4

resp=$(irc_send 2 "PASS $PASS" "NICK tc_out" "USER tc_out 0 * :Out" "TOPIC #tocchan")
echo "$resp" | grep -q "442" \
    && pass "TOPIC sin estar en canal → ERR_NOTONCHANNEL (442)" \
    || fail "TOPIC sin estar en canal → esperado 442 (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BG5" 2>/dev/null; wait "$BG5" 2>/dev/null

# ── 12. TOPIC consulta en canal sin topic → RPL_NOTOPIC (331) ────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK td" "USER td 0 * :Td" "JOIN #tdchan" "TOPIC #tdchan")
echo "$resp" | grep -q "331" \
    && pass "TOPIC consulta sin topic → RPL_NOTOPIC (331)" \
    || fail "TOPIC consulta sin topic → esperado 331 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 13. TOPIC establece topic → broadcast TOPIC al canal ─────────────────────
(printf "PASS %s\r\nNICK tlistener\r\nUSER tlistener 0 * :TListener\r\nJOIN #tchan2\r\n" "$PASS"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" > /tmp/irc_tlistener_out 2>/dev/null &
BG6=$!
sleep 0.5

(printf "PASS %s\r\nNICK tsetter\r\nUSER tsetter 0 * :TSetter\r\nJOIN #tchan2\r\nTOPIC #tchan2 :nuevo topic de prueba\r\n" "$PASS") \
    | timeout 2 nc -q 1 -w 2 "$HOST" "$PORT" &>/dev/null
sleep 0.5

kill "$BG6" 2>/dev/null; wait "$BG6" 2>/dev/null
if grep -q "nuevo topic de prueba" /tmp/irc_tlistener_out 2>/dev/null; then
    pass "TOPIC set → broadcast TOPIC llega a otro miembro"
else
    fail "TOPIC set → broadcast no llegó al otro miembro (out: $(cat /tmp/irc_tlistener_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_tlistener_out

# ── 14. TOPIC consulta tras haber seteado → RPL_TOPIC (332) ──────────────────
resp=$(irc_send 2 \
    "PASS $PASS" "NICK te" "USER te 0 * :Te" \
    "JOIN #techan" "TOPIC #techan :el topic" "TOPIC #techan")
echo "$resp" | grep -q "332" \
    && pass "TOPIC consulta con topic → RPL_TOPIC (332)" \
    || fail "TOPIC consulta con topic → esperado 332 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 15. TOPIC borra topic con cadena vacía → RPL_NOTOPIC en consulta ─────────
resp=$(irc_send 2 \
    "PASS $PASS" "NICK tf" "USER tf 0 * :Tf" \
    "JOIN #tfchan" "TOPIC #tfchan :primero" "TOPIC #tfchan :" "TOPIC #tfchan")
echo "$resp" | grep -q "331" \
    && pass "TOPIC vacío → topic borrado (RPL_NOTOPIC 331 en consulta posterior)" \
    || fail "TOPIC vacío → esperado 331 tras borrar topic (resp: $(echo "$resp" | tr '\r' ' '))"

stop_server
print_summary
