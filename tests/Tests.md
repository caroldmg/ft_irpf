# Tests básicos y ejemplos de pruebas ft_irc

Guía de pruebas manuales para validar el servidor. Útil tanto para depurar durante el desarrollo como para preparar la eval. (Este archivo ha sido generado con un LMM)

## Preparación

Lanza el servidor:

```bash
./ircserv 6667 contra
```

Y conecta clientes con cualquiera de estas opciones:

- **HexChat** (recomendado, es lo que usan los evaluadores)
- **WeeChat** con `/server raw` para ver el tráfico crudo
- **netcat** para pruebas rápidas a pelo: `nc localhost 6667`
- Los helpers de `tests/integration/common.sh` (`irc_send`, `pass`, `fail`)

Para los tests que requieren dos o tres clientes, abre varias terminales o instancias del cliente en paralelo.

---

## 1. Tests de regresión (lo que ya funcionaba)

Comprueba que nada de lo previamente implementado se ha roto tras los merges.

### 1.1 Handshake de registro

```
PASS mipass
NICK alice
USER alice 0 * :Alice Doe
```

**Esperado:** códigos 001, 002, 003 y 004 (bienvenida). El cliente queda registrado.

### 1.2 Errores de registro

- `NICK alice` sin haber mandado `PASS` antes → error 464.
- `PASS malapass` → error 464.
- `PASS mipass` + `NICK alice` + (otro cliente) `NICK alice` → error 433 (nick en uso).
- Comando cualquiera antes de registrarse: `JOIN #x` → error 451 (not registered).

### 1.3 PING / PONG

```
PING :token123
```

**Esperado:** `PONG :token123` con el mismo token.

### 1.4 CAP (handshake de capabilities)

```
CAP LS
CAP END
```

**Esperado:** respuesta `CAP * LS :` (lista vacía) al `LS`, y `CAP END` se acepta sin error.

### 1.5 QUIT

```
QUIT :hasta luego
```

**Esperado:** desconexión limpia, y los demás miembros de los canales donde estaba reciben `:alice!alice@host QUIT :hasta luego`.

### 1.6 JOIN, PART y canal vacío

```
JOIN #general
PART #general :me voy
```

**Esperado:** al hacer JOIN llega broadcast a los demás miembros, el cliente recibe topic (331 si no hay) y NAMES (353+366). Al hacer PART, broadcast al resto. Si el canal queda vacío, se borra.

### 1.7 TOPIC

```
TOPIC #general
TOPIC #general :Nuevo topic
TOPIC #general
```

**Esperado:** la primera devuelve 331 (no topic) o 332 (topic actual). La segunda lo cambia. La tercera devuelve 332 con el nuevo.

### 1.8 PRIVMSG simple

Dos clientes (alice y bob) en `#general`:

```
# alice:
PRIVMSG #general :hola a todos
PRIVMSG bob :te mando un privado
```

**Esperado:** bob recibe ambos mensajes con prefijo `:alice!alice@host`. Alice **no** recibe el suyo de canal (broadcastExcept).

### 1.9 NOTICE

```
NOTICE bob :esto es un aviso
```

**Esperado:** bob recibe `:alice!alice@host NOTICE bob :esto es un aviso`. Si el destinatario no existe, **no** se genera error automático (esa es la diferencia con PRIVMSG).

---

## 2. Tests de los comandos nuevos

### 2.1 PRIVMSG multitarget 

Tres clientes: alice, bob, carol. Alice manda:

```
PRIVMSG bob,carol :hola a ambos
```

**Esperado:** bob Y carol reciben el mensaje. Sin error.

**Sospecha:** el código de Carol itera sobre `msg.params[]` cuando los destinos van separados por **coma** dentro de `params[0]`. Si esta prueba falla, hay que hacer split por coma en el primer parámetro.

Variante mezclando canal y usuario:

```
PRIVMSG #general,bob :prueba mixta
```

**Esperado:** todos los miembros de `#general` (menos alice) reciben el mensaje, y bob también.

### 2.2 KICK con razón 

Alice operadora de `#test`, bob dentro:

```
KICK #test bob :spam
```

**Esperado:** bob recibe `:alice!alice@host KICK #test bob :spam` y queda fuera del canal. Todos los demás miembros también ven el broadcast.

**Sospecha:** el código de Carol puede usar un formato de broadcast no-RFC tipo `"KICK bob from #test"` en vez del estándar `"KICK #canal nick :razón"`. Verificar el formato exacto.

### 2.3 KICK sin razón 

```
KICK #test bob
```

**Esperado:** funciona, kick efectivo. La razón por defecto suele ser el nick del que expulsa.

**Sospecha:** el código pide 3 parámetros obligatorios cuando solo son 2. Si esta prueba devuelve "Not enough parameters", confirmado el bug.

### 2.4 KICK sin permisos

bob (no operador) intenta:

```
KICK #test alice :te echo
```

**Esperado:** error 482 (`CHANOPRIVSNEEDED`). Alice sigue en el canal.

### 2.5 KICK a usuario que no está en el canal

```
KICK #test pepito :fuera
```

**Esperado:** error 441 (`USERNOTINCHANNEL`).

### 2.6 INVITE básico 

Alice operadora de `#priv` con modo `+i`:

```
MODE #priv +i
INVITE bob #priv
```

**Esperado:**
- Bob recibe `:alice!alice@host INVITE bob :#priv` (con espacio antes de los dos puntos).
- Alice recibe `RPL_INVITING` (341).
- Bob puede entrar con `JOIN #priv` aunque esté en modo `+i`.

**Sospechas:**
- Falta espacio antes del `:` en el mensaje al invitado (`INVITE bob:#priv` en vez de `INVITE bob :#priv`).
- Falta enviar el `RPL_INVITING` 341 al que invita.
- Typo en el mensaje de error: pone "KICK :Not enough parameters" en vez de "INVITE".
- Si el nick del invitado no existe, no se hace check de NULL → posible segfault.

### 2.7 INVITE a alguien que ya está dentro

```
INVITE bob #priv
```

**Esperado:** error 443 (`USERONCHANNEL`).

### 2.8 INVITE sin ser operador en canal +i

bob (no operador) en `#priv` con `+i`:

```
INVITE carol #priv
```

**Esperado:** error 482 (`CHANOPRIVSNEEDED`).

---

## 3. Tests del comando MODE

### 3.1 MODE +i / -i (invite-only)

```
# alice (operadora):
MODE #test +i
```

Bob intenta `JOIN #test` → error 473 (`INVITEONLYCHAN`).

```
INVITE bob #test
```

Bob hace `JOIN #test` → entra.

```
MODE #test -i
```

Cualquier cliente puede entrar libremente.

### 3.2 MODE +t / -t (topic protegido)

```
MODE #test +t
```

bob (no operador) intenta `TOPIC #test :hack` → error 482.

```
MODE #test -t
```

bob puede cambiar el topic.

### 3.3 MODE +k / -k (clave de canal)

```
MODE #test +k claveSecreta
```

bob intenta `JOIN #test` (sin clave) → error 475 (`BADCHANNELKEY`).
bob intenta `JOIN #test claveSecreta` → entra.

```
MODE #test -k
```

`JOIN #test` (sin clave) funciona.

### 3.4 MODE +o / -o (operador)

```
MODE #test +o bob
```

Bob ahora puede usar KICK, INVITE, MODE, TOPIC con `+t` activo, etc.

```
MODE #test -o bob
```

Bob pierde los privilegios.

### 3.5 MODE +l / -l (límite de usuarios)

Con dos miembros ya dentro:

```
MODE #test +l 2
```

Un tercer cliente intenta `JOIN #test` → error 471 (`CHANNELISFULL`).

```
MODE #test -l
```

El tercer cliente entra.

### 3.6 MODE flags combinados

```
MODE #test +itk claveSecreta
```

**Esperado:** activa los tres modos. La clave se aplica solo al flag `k`.

```
MODE #test +ol 5 bob
```

**Esperado:** hace a bob operador y pone límite de 5. Cada flag consume su argumento en orden.

```
MODE #test -ik
```

**Esperado:** desactiva los tres modos a la vez.

### 3.7 MODE flag desconocido

```
MODE #test +z
```

**Esperado:** error 472 (`UNKNOWNMODE`).

### 3.8 MODE sin ser operador

bob (no operador) intenta:

```
MODE #test +t
```

**Esperado:** error 482.

---

## 4. Tests de robustez

### 4.1 Cliente que envía línea muy larga

Mandar una línea de más de 512 bytes desde netcat:

```bash
python3 -c "print('PRIVMSG #test :' + 'A'*600)" | nc -q1 localhost 6667
```

**Esperado:** el servidor no se cae. Idealmente truncar o descartar la línea.

### 4.2 Cliente que cierra abruptamente

```bash
nc localhost 6667
# Hacer PASS+NICK+USER+JOIN #general, después Ctrl+C
```

**Esperado:** los demás miembros de `#general` reciben el QUIT del cliente. El servidor sigue funcionando.

### 4.3 Mensajes partidos por el socket

Usando un script en Python que mande `NICK ali` y luego (tras un sleep) `ce\r\n`:

```python
import socket, time
s = socket.socket()
s.connect(('localhost', 6667))
s.send(b'PASS mipass\r\n')
s.send(b'NICK ali')
time.sleep(1)
s.send(b'ce\r\nUSER alice 0 * :Alice\r\n')
```

**Esperado:** el cliente se registra correctamente. El buffer reensambla la línea.

### 4.4 Múltiples comandos en un solo envío

```bash
printf "PASS mipass\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n" | nc localhost 6667
```

**Esperado:** procesa los tres comandos secuencialmente y manda la bienvenida.

### 4.5 Estrés con varios clientes a la vez

Lanzar 3+ clientes simultáneamente con `nc` y un script. Que todos hagan JOIN al mismo canal y manden PRIVMSG.

**Esperado:** todos reciben los mensajes de todos, sin pérdidas. El `poll()` aguanta.

### 4.6 Test específico del subject (`nc` con pipe)

```bash
nc -C localhost 6667 < comandos.txt
```

Donde `comandos.txt` tiene varios comandos. **Esperado:** el servidor procesa todo aunque la entrada llegue de golpe y el cliente cierre la escritura inmediatamente.

---

## 5. Sesión completa de demostración

Útil para preparar la defensa. Tres clientes (HexChat o WeeChat).

```
# alice:
PASS mipass
NICK alice
USER alice 0 * :Alice
JOIN #lab
TOPIC #lab :Sala de pruebas ft_irc

# bob:
PASS mipass
NICK bob
USER bob 0 * :Bob
JOIN #lab
PRIVMSG #lab :buenas, soy bob

# carol:
PASS mipass
NICK carol
USER carol 0 * :Carol
JOIN #lab
PRIVMSG alice :te mando un privado

# alice se hace operador y configura el canal:
MODE #lab +itk secreta
INVITE bob #lab
KICK #lab carol :prueba de kick

# carol reintenta entrar sin la clave:
JOIN #lab   → 475 BADCHANNELKEY
JOIN #lab secreta   → 473 INVITEONLYCHAN

# alice la invita y entra:
# (alice) INVITE carol #lab
# (carol) JOIN #lab secreta   → entra
```

---

## 6. Tests con cliente real (HexChat o WeeChat)

Estos no se pueden automatizar pero son los que más se acercan a lo que evaluará el corrector.

### 6.1 HexChat conecta y se registra solo

Abrir HexChat → añadir red local (host `127.0.0.1`, puerto `6667`, password `contra`, SSL desactivado) → conectar.

**Esperado:** HexChat envía `CAP LS 302` + `PASS` + `NICK` + `USER`, recibe la bienvenida, y queda conectado sin colgarse.

### 6.2 HexChat /join, /msg, /topic

Probar los comandos desde la UI de HexChat con `/join`, `/msg`, `/topic`, `/kick`, `/invite`, `/mode +o`.

**Esperado:** todo funciona como en un servidor IRC real.

### 6.3 WeeChat con /server raw

Activar el modo raw para ver los bytes exactos que mandan y reciben:

```
/server raw
```

Útil para confirmar que los broadcasts (KICK, MODE, PRIVMSG) salen con el formato RFC correcto.

---
