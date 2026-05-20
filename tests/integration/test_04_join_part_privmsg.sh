#!/bin/bash
# Test 04 — JOIN, PART y PRIVMSG
source "$(dirname "$0")/common.sh"

echo ""
echo "=== Test 04: JOIN / PART / PRIVMSG ==="
start_server

# ── 1. JOIN sin registro → ERR_NOTREGISTERED (451) ───────────────────────────
resp=$(irc_send 2 "JOIN #test")
echo "$resp" | grep -q "451" \
    && pass "JOIN sin registro → ERR_NOTREGISTERED (451)" \
    || fail "JOIN sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 2. JOIN correcto → notificación JOIN + RPL_NAMREPLY (353) ─────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK alice" "USER alice 0 * :Alice" "JOIN #general")
echo "$resp" | grep -q "JOIN" \
    && pass "JOIN correcto → notificación JOIN recibida" \
    || fail "JOIN correcto → sin notificación JOIN (resp: $(echo "$resp" | tr '\r' ' '))"

echo "$resp" | grep -q "353" \
    && pass "JOIN correcto → RPL_NAMREPLY (353)" \
    || fail "JOIN correcto → falta RPL_NAMREPLY 353 (resp: $(echo "$resp" | tr '\r' ' '))"

echo "$resp" | grep -q "366" \
    && pass "JOIN correcto → RPL_ENDOFNAMES (366)" \
    || fail "JOIN correcto → falta RPL_ENDOFNAMES 366 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 3. El primer miembro es operador (@ en NAMES) ────────────────────────────
echo "$resp" | grep -q "@alice" \
    && pass "Primer miembro en NAMES tiene prefijo @" \
    || fail "Primer miembro no tiene prefijo @ (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 4. Canal sin topic → RPL_NOTOPIC (331) ────────────────────────────────────
echo "$resp" | grep -q "331" \
    && pass "Canal sin topic → RPL_NOTOPIC (331)" \
    || fail "Canal sin topic → falta RPL_NOTOPIC 331 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 5. JOIN canal inexistente lo crea (nombre inválido → ERR_NOSUCHCHANNEL) ───
resp=$(irc_send 2 "PASS $PASS" "NICK bob" "USER bob 0 * :Bob" "JOIN sinAlmohadilla")
echo "$resp" | grep -q "403" \
    && pass "Canal sin '#' → ERR_NOSUCHCHANNEL (403)" \
    || fail "Canal sin '#' → esperado 403 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 6. PART sin registro → ERR_NOTREGISTERED (451) ───────────────────────────
resp=$(irc_send 2 "PART #general")
echo "$resp" | grep -q "451" \
    && pass "PART sin registro → ERR_NOTREGISTERED (451)" \
    || fail "PART sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 7. PART de canal en el que no estás → ERR_NOTONCHANNEL (442) ─────────────
resp=$(irc_send 2 "PASS $PASS" "NICK carol" "USER carol 0 * :Carol" "PART #nosuchchan")
echo "$resp" | grep -q "403\|442" \
    && pass "PART de canal inexistente → ERR_NOSUCHCHANNEL/NOTONCHANNEL" \
    || fail "PART de canal inexistente → esperado 403 o 442 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 8. PART correcto → notificación PART ──────────────────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK dave" "USER dave 0 * :Dave" "JOIN #lobby" "PART #lobby :bye")
echo "$resp" | grep -q "PART" \
    && pass "PART correcto → notificación PART recibida" \
    || fail "PART correcto → sin notificación PART (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 9. PRIVMSG sin registro → ERR_NOTREGISTERED (451) ─────────────────────────
resp=$(irc_send 2 "PRIVMSG #general :hola")
echo "$resp" | grep -q "451" \
    && pass "PRIVMSG sin registro → ERR_NOTREGISTERED (451)" \
    || fail "PRIVMSG sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 10. PRIVMSG sin parámetros → ERR_NORECIPIENT (411) ────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK eve" "USER eve 0 * :Eve" "PRIVMSG")
echo "$resp" | grep -q "411" \
    && pass "PRIVMSG sin parámetros → ERR_NORECIPIENT (411)" \
    || fail "PRIVMSG sin parámetros → esperado 411 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 11. PRIVMSG sin texto → ERR_NOTEXTTOSEND (412) ────────────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK frank" "USER frank 0 * :Frank" "PRIVMSG #general")
echo "$resp" | grep -q "412" \
    && pass "PRIVMSG sin texto → ERR_NOTEXTTOSEND (412)" \
    || fail "PRIVMSG sin texto → esperado 412 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 12. PRIVMSG a canal en que no estás → ERR_CANNOTSENDTOCHAN (404) ──────────
# Primero se crea el canal con un cliente; luego otro intenta enviar sin unirse
(printf "PASS %s\r\nNICK owner\r\nUSER owner 0 * :Owner\r\nJOIN #priv\r\n" "$PASS"; sleep 5) \
    | nc -w 6 "$HOST" "$PORT" &>/dev/null &
BG1=$!
sleep 0.4

resp=$(irc_send 2 "PASS $PASS" "NICK outsider" "USER outsider 0 * :Out" "PRIVMSG #priv :intento")
echo "$resp" | grep -q "404" \
    && pass "PRIVMSG a canal sin pertenecer → ERR_CANNOTSENDTOCHAN (404)" \
    || fail "PRIVMSG a canal sin pertenecer → esperado 404 (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BG1" 2>/dev/null; wait "$BG1" 2>/dev/null

# ── 13. PRIVMSG a nick inexistente → ERR_NOSUCHNICK (401) ─────────────────────
resp=$(irc_send 2 "PASS $PASS" "NICK grace" "USER grace 0 * :Grace" "PRIVMSG nobody :hola")
echo "$resp" | grep -q "401" \
    && pass "PRIVMSG a nick inexistente → ERR_NOSUCHNICK (401)" \
    || fail "PRIVMSG a nick inexistente → esperado 401 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 14. PRIVMSG entre clientes (mensaje llega al destino) ─────────────────────
# Receptor queda abierto con sleep; emisor envía PRIVMSG
(printf "PASS %s\r\nNICK henry\r\nUSER henry 0 * :Henry\r\n" "$PASS"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" > /tmp/irc_henry_out 2>/dev/null &
BG2=$!
sleep 0.5

(printf "PASS %s\r\nNICK sender\r\nUSER sender 0 * :Sender\r\nPRIVMSG henry :hello henry\r\n" "$PASS") \
    | timeout 2 nc -q 1 -w 2 "$HOST" "$PORT" &>/dev/null
sleep 0.5

kill "$BG2" 2>/dev/null; wait "$BG2" 2>/dev/null
if grep -q "hello henry" /tmp/irc_henry_out 2>/dev/null; then
    pass "PRIVMSG nick-a-nick → mensaje llega al destinatario"
else
    fail "PRIVMSG nick-a-nick → mensaje no llegó (out: $(cat /tmp/irc_henry_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_henry_out

# ── 15. PRIVMSG en canal (mensaje llega a otro miembro) ───────────────────────
(printf "PASS %s\r\nNICK listener\r\nUSER listener 0 * :Listener\r\nJOIN #chat\r\n" "$PASS"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" > /tmp/irc_listener_out 2>/dev/null &
BG3=$!
sleep 0.5

(printf "PASS %s\r\nNICK talker\r\nUSER talker 0 * :Talker\r\nJOIN #chat\r\nPRIVMSG #chat :hi channel\r\n" "$PASS") \
    | timeout 2 nc -q 1 -w 2 "$HOST" "$PORT" &>/dev/null
sleep 0.5

kill "$BG3" 2>/dev/null; wait "$BG3" 2>/dev/null
if grep -q "hi channel" /tmp/irc_listener_out 2>/dev/null; then
    pass "PRIVMSG al canal → mensaje llega a otro miembro"
else
    fail "PRIVMSG al canal → mensaje no llegó al otro miembro (out: $(cat /tmp/irc_listener_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_listener_out

stop_server
print_summary
