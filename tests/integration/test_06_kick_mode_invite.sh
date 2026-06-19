#!/bin/bash
# Test 06 — KICK, MODE e INVITE (comandos de operador de canal)
source "$(dirname "$0")/common.sh"

echo ""
echo "=== Test 06: KICK / MODE / INVITE ==="
start_server

R="PASS $PASS"   # prefijo de registro abreviado

# ═══════════════════════════════════════ KICK ═══════════════════════════════
# ── 1. KICK sin registro → ERR_NOTREGISTERED (451) ───────────────────────────
resp=$(irc_send 2 "KICK #x bob")
echo "$resp" | grep -q "451" \
    && pass "KICK sin registro → ERR_NOTREGISTERED (451)" \
    || fail "KICK sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 2. KICK sin parámetros suficientes → ERR_NEEDMOREPARAMS (461) ────────────
resp=$(irc_send 2 "$R" "NICK k1" "USER k1 0 * :K1" "KICK #x")
echo "$resp" | grep -q "461" \
    && pass "KICK con un solo parámetro → ERR_NEEDMOREPARAMS (461)" \
    || fail "KICK con un solo parámetro → esperado 461 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 3. KICK en canal inexistente → ERR_NOSUCHCHANNEL (403) ───────────────────
resp=$(irc_send 2 "$R" "NICK k2" "USER k2 0 * :K2" "KICK #nope bob")
echo "$resp" | grep -q "403" \
    && pass "KICK en canal inexistente → ERR_NOSUCHCHANNEL (403)" \
    || fail "KICK en canal inexistente → esperado 403 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 4. KICK de un usuario que no está en el canal → ERR_NOTONCHANNEL (442) ────
resp=$(irc_send 2 "$R" "NICK k3" "USER k3 0 * :K3" "JOIN #kc4" "KICK #kc4 fantasma")
echo "$resp" | grep -q "442" \
    && pass "KICK de usuario ausente → ERR_NOTONCHANNEL (442)" \
    || fail "KICK de usuario ausente → esperado 442 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 5. KICK efectivo → la víctima recibe el mensaje KICK ─────────────────────
# 'boss' crea el canal primero (es operador) y luego expulsa a 'victim', que captura su salida.
(printf "%s\r\nNICK boss\r\nUSER boss 0 * :B\r\nJOIN #kroom\r\n" "$R"; sleep 1.6; \
 printf "KICK #kroom victim :adios\r\n"; sleep 3) \
    | timeout 6 nc -w 6 "$HOST" "$PORT" &>/dev/null &
BGB=$!
sleep 0.6
(printf "%s\r\nNICK victim\r\nUSER victim 0 * :V\r\nJOIN #kroom\r\n" "$R"; sleep 4) \
    | timeout 6 nc -w 6 "$HOST" "$PORT" > /tmp/irc_victim_out 2>/dev/null &
BGV=$!
sleep 5
kill "$BGB" "$BGV" 2>/dev/null; wait "$BGB" "$BGV" 2>/dev/null
if grep -q "KICK #kroom victim" /tmp/irc_victim_out 2>/dev/null; then
    pass "KICK efectivo → la víctima recibe ':boss... KICK #kroom victim :adios'"
else
    fail "KICK efectivo → la víctima no recibió el KICK (out: $(cat /tmp/irc_victim_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_victim_out

# ── 6. KICK sin ser operador → ERR_CHANOPRIVSNEEDED (482) ────────────────────
# 'creator' crea el canal (es op) y se queda; 'plebe' entra (no es op) e intenta KICK.
(printf "%s\r\nNICK creator\r\nUSER creator 0 * :C\r\nJOIN #kops\r\n" "$R"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" &>/dev/null &
BGC=$!
sleep 0.6
resp=$(irc_send 2 "$R" "NICK plebe" "USER plebe 0 * :P" "JOIN #kops" "KICK #kops creator")
echo "$resp" | grep -q "482" \
    && pass "KICK sin ser operador → ERR_CHANOPRIVSNEEDED (482)" \
    || fail "KICK sin ser operador → esperado 482 (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BGC" 2>/dev/null; wait "$BGC" 2>/dev/null

# ═══════════════════════════════════════ MODE ═══════════════════════════════
# ── 7. MODE sin registro → ERR_NOTREGISTERED (451) ───────────────────────────
resp=$(irc_send 2 "MODE #x +i")
echo "$resp" | grep -q "451" \
    && pass "MODE sin registro → ERR_NOTREGISTERED (451)" \
    || fail "MODE sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 8. MODE en canal inexistente → ERR_NOSUCHCHANNEL (403) ───────────────────
resp=$(irc_send 2 "$R" "NICK m1" "USER m1 0 * :M1" "MODE #nope +i")
echo "$resp" | grep -q "403" \
    && pass "MODE en canal inexistente → ERR_NOSUCHCHANNEL (403)" \
    || fail "MODE en canal inexistente → esperado 403 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 9. MODE +i → el operador recibe el eco '+i' ──────────────────────────────
resp=$(irc_send 2 "$R" "NICK m2" "USER m2 0 * :M2" "JOIN #mi" "MODE #mi +i")
echo "$resp" | grep -q "MODE #mi +i" \
    && pass "MODE +i → broadcast 'MODE #mi +i' recibido" \
    || fail "MODE +i → falta broadcast +i (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 10. MODE +i bloquea el JOIN de no invitados → ERR_INVITEONLYCHAN (473) ────
# 'guard' mantiene #lock abierto en +i; un externo intenta entrar.
(printf "%s\r\nNICK guard\r\nUSER guard 0 * :G\r\nJOIN #lock\r\nMODE #lock +i\r\n" "$R"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" &>/dev/null &
BGG=$!
sleep 0.6
resp=$(irc_send 2 "$R" "NICK extern" "USER extern 0 * :E" "JOIN #lock")
echo "$resp" | grep -q "473" \
    && pass "MODE +i bloquea JOIN externo → ERR_INVITEONLYCHAN (473)" \
    || fail "MODE +i bloquea JOIN externo → esperado 473 (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BGG" 2>/dev/null; wait "$BGG" 2>/dev/null

# ── 11. MODE +k key → JOIN con clave incorrecta → ERR_BADCHANNELKEY (475) ─────
(printf "%s\r\nNICK keeper\r\nUSER keeper 0 * :K\r\nJOIN #keyed\r\nMODE #keyed +k secret\r\n" "$R"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" &>/dev/null &
BGK=$!
sleep 0.6
resp=$(irc_send 2 "$R" "NICK noway" "USER noway 0 * :N" "JOIN #keyed wrongkey")
echo "$resp" | grep -q "475" \
    && pass "MODE +k → JOIN con clave incorrecta → ERR_BADCHANNELKEY (475)" \
    || fail "MODE +k → esperado 475 (resp: $(echo "$resp" | tr '\r' ' '))"
# JOIN con la clave correcta debe entrar (recibe JOIN, no 475)
resp=$(irc_send 2 "$R" "NICK okkey" "USER okkey 0 * :O" "JOIN #keyed secret")
if echo "$resp" | grep -q "JOIN" && ! echo "$resp" | grep -q "475"; then
    pass "MODE +k → JOIN con clave correcta → entra al canal"
else
    fail "MODE +k → con clave correcta debería entrar (resp: $(echo "$resp" | tr '\r' ' '))"
fi
kill "$BGK" 2>/dev/null; wait "$BGK" 2>/dev/null

# ── 12. MODE +l 1 → canal lleno → ERR_CHANNELISFULL (471) ────────────────────
(printf "%s\r\nNICK solo\r\nUSER solo 0 * :S\r\nJOIN #full\r\nMODE #full +l 1\r\n" "$R"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" &>/dev/null &
BGL=$!
sleep 0.6
resp=$(irc_send 2 "$R" "NICK late" "USER late 0 * :L" "JOIN #full")
echo "$resp" | grep -q "471" \
    && pass "MODE +l 1 → canal lleno → ERR_CHANNELISFULL (471)" \
    || fail "MODE +l 1 → esperado 471 (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BGL" 2>/dev/null; wait "$BGL" 2>/dev/null

# ── 13. MODE +o → otorga operador a otro miembro (eco '+o') ──────────────────
# 'granter' crea el canal (es op) y luego da +o a 'opee'; capturamos la salida de granter.
(printf "%s\r\nNICK granter\r\nUSER granter 0 * :G\r\nJOIN #opch\r\n" "$R"; sleep 1.6; \
 printf "MODE #opch +o opee\r\n"; sleep 3) \
    | timeout 6 nc -w 6 "$HOST" "$PORT" > /tmp/irc_granter_out 2>/dev/null &
BGgr=$!
sleep 0.6
(printf "%s\r\nNICK opee\r\nUSER opee 0 * :O\r\nJOIN #opch\r\n" "$R"; sleep 4) \
    | timeout 6 nc -w 6 "$HOST" "$PORT" &>/dev/null &
BGop=$!
sleep 5
kill "$BGgr" "$BGop" 2>/dev/null; wait "$BGgr" "$BGop" 2>/dev/null
if grep -q "MODE #opch +o opee" /tmp/irc_granter_out 2>/dev/null; then
    pass "MODE +o opee → broadcast '+o opee' recibido"
else
    fail "MODE +o opee → falta broadcast +o (out: $(cat /tmp/irc_granter_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_granter_out

# ── 14. MODE +t y luego TOPIC de no-operador → ERR_CHANOPRIVSNEEDED (482) ─────
(printf "%s\r\nNICK tboss\r\nUSER tboss 0 * :T\r\nJOIN #topo\r\nMODE #topo +t\r\n" "$R"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" &>/dev/null &
BGT=$!
sleep 0.6
resp=$(irc_send 2 "$R" "NICK tplebe" "USER tplebe 0 * :T" "JOIN #topo" "TOPIC #topo :nuevo")
echo "$resp" | grep -q "482" \
    && pass "MODE +t → TOPIC de no-operador → ERR_CHANOPRIVSNEEDED (482)" \
    || fail "MODE +t → esperado 482 al cambiar topic sin ser op (resp: $(echo "$resp" | tr '\r' ' '))"
kill "$BGT" 2>/dev/null; wait "$BGT" 2>/dev/null

# ── 15. MODE flag desconocido → ERR_UNKNOWNMODE (472) ────────────────────────
resp=$(irc_send 2 "$R" "NICK m9" "USER m9 0 * :M9" "JOIN #unk" "MODE #unk +Z")
echo "$resp" | grep -q "472" \
    && pass "MODE flag desconocido (+Z) → ERR_UNKNOWNMODE (472)" \
    || fail "MODE +Z → esperado 472 (resp: $(echo "$resp" | tr '\r' ' '))"

# ═══════════════════════════════════════ INVITE ═════════════════════════════
# ── 16. INVITE sin registro → ERR_NOTREGISTERED (451) ────────────────────────
resp=$(irc_send 2 "INVITE bob #x")
echo "$resp" | grep -q "451" \
    && pass "INVITE sin registro → ERR_NOTREGISTERED (451)" \
    || fail "INVITE sin registro → esperado 451 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 17. INVITE a nick inexistente → ERR_NOSUCHNICK (401) y SIN crash ─────────
resp=$(irc_send 2 "$R" "NICK inv1" "USER inv1 0 * :I" "JOIN #invc" "INVITE ghost #invc")
echo "$resp" | grep -q "401" \
    && pass "INVITE a nick inexistente → ERR_NOSUCHNICK (401) [regresión de crash]" \
    || fail "INVITE a nick inexistente → esperado 401 (resp: $(echo "$resp" | tr '\r' ' '))"

# ── 18. INVITE efectivo → el invitado recibe ':... INVITE ...' ───────────────
(printf "%s\r\nNICK guest\r\nUSER guest 0 * :G\r\n" "$R"; sleep 6) \
    | timeout 7 nc -w 7 "$HOST" "$PORT" > /tmp/irc_guest_out 2>/dev/null &
BGI=$!
sleep 0.6
(printf "%s\r\nNICK host\r\nUSER host 0 * :H\r\nJOIN #party\r\nINVITE guest #party\r\n" "$R") \
    | timeout 2 nc -q 1 -w 2 "$HOST" "$PORT" &>/dev/null
sleep 0.5
kill "$BGI" 2>/dev/null; wait "$BGI" 2>/dev/null
if grep -q "INVITE guest" /tmp/irc_guest_out 2>/dev/null; then
    pass "INVITE efectivo → el invitado recibe el mensaje INVITE"
else
    fail "INVITE efectivo → el invitado no recibió INVITE (out: $(cat /tmp/irc_guest_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_guest_out

# ── 19. INVITE permite saltar +i → el invitado entra en canal invite-only ────
(printf "%s\r\nNICK gate\r\nUSER gate 0 * :G\r\nJOIN #vip\r\nMODE #vip +i\r\n" "$R"; \
 sleep 1; printf "INVITE friend #vip\r\n"; sleep 5) \
    | timeout 8 nc -w 8 "$HOST" "$PORT" &>/dev/null &
BGgate=$!
sleep 0.4
# 'friend' aún no existe cuando se envía el INVITE de arriba; lo registramos y esperamos
(printf "%s\r\nNICK friend\r\nUSER friend 0 * :F\r\n" "$R"; sleep 3; printf "JOIN #vip\r\n"; sleep 1) \
    | timeout 6 nc -w 6 "$HOST" "$PORT" > /tmp/irc_friend_out 2>/dev/null &
BGfriend=$!
sleep 5
kill "$BGgate" "$BGfriend" 2>/dev/null; wait "$BGgate" "$BGfriend" 2>/dev/null
if grep -q "JOIN" /tmp/irc_friend_out 2>/dev/null && ! grep -q "473" /tmp/irc_friend_out 2>/dev/null; then
    pass "INVITE permite entrar a canal +i → el invitado hace JOIN con éxito"
else
    fail "INVITE+i → el invitado no pudo entrar (out: $(cat /tmp/irc_friend_out 2>/dev/null | tr '\r' ' '))"
fi
rm -f /tmp/irc_friend_out

# ── 20. El servidor sigue vivo tras todas las operaciones ────────────────────
if kill -0 "$SERVER_PID" 2>/dev/null; then
    pass "El servidor sigue operativo tras KICK/MODE/INVITE (sin crash)"
else
    fail "El servidor murió durante las pruebas de operador"
fi

stop_server
print_summary
