
## 📑 Índice
 
- [Sobre el proyecto](#-sobre-el-proyecto)
- [El protocolo IRC](#-el-protocolo-irc)
- [Parte obligatoria](#-parte-obligatoria)
- [Comandos soportados](#-comandos-soportados)
- [Modos de canal](#-modos-de-canal)
- [Bonus](#-bonus-ircbot)
- [Estructura del proyecto](#-estructura-del-proyecto)
- [Compilación](#️-compilación)
- [Uso](#-uso)
- [Ejemplo de sesión](#-ejemplo-de-sesión)
- [Tests](#-tests)
- [Recursos](#-recursos)
---
 
## 📖 Sobre el proyecto
 
`ft_irc` consiste en implementar un **servidor IRC** desde cero en **C++98**, capaz de
atender a múltiples clientes de forma simultánea sin nunca bloquearse. La comunicación
se gestiona con un único `poll()` (I/O multiplexado y no bloqueante) sobre sockets TCP/IP,
respetando el subconjunto del estándar IRC necesario para conectarse con un cliente real
como **HexChat**, **WeeChat** o **irssi**.
 
El objetivo es entender cómo funciona la programación de red de bajo nivel: sockets,
gestión de eventos, troceado de mensajes por `\r\n`, registro de usuarios, canales,
operadores y difusión de mensajes.
 
> ⚠️ Todo el I/O pasa por **un solo** `poll()`. No hay forking, ni hilos, ni lecturas/escrituras
> bloqueantes: el servidor jamás debe quedarse colgado esperando a un cliente.
 
---
 
## 🌐 El protocolo IRC
 
IRC funciona en modo texto sobre TCP. Cada mensaje termina en `\r\n` y sigue la forma:
 
```
[:prefix] COMMAND [params] [:trailing]
```
 
El flujo de una conexión es siempre el mismo:
 
1. El cliente envía la contraseña con `PASS`.
2. Se identifica con `NICK` (apodo) y `USER` (usuario + nombre real).
3. Una vez completado el *handshake*, el servidor responde con la bienvenida
   (códigos `001`, `002`, `003`, `004`) y el cliente ya puede unirse a canales y hablar.
---
 
## ✅ Parte obligatoria
 
| Requisito | Estado |
|-----------|:------:|
| Servidor TCP/IP no bloqueante con un único `poll()` | ✔️ |
| Atención simultánea a múltiples clientes | ✔️ |
| Autenticación por contraseña (`PASS`) | ✔️ |
| Registro de usuario: `NICK` / `USER` | ✔️ |
| Creación y gestión de canales | ✔️ |
| Mensajería privada y a canales (`PRIVMSG`) | ✔️ |
| Operadores de canal y comandos de operador | ✔️ |
| Gestión correcta de desconexiones y señales | ✔️ |
| Compatibilidad con un cliente IRC de referencia | ✔️ |
| `C++98`, sin librerías externas, flags `-Wall -Wextra -Werror` | ✔️ |
 
Características técnicas destacadas:
 
- **Un solo `poll()`** para todas las conexiones (servidor + clientes).
- **Buffer por cliente**: los mensajes parciales se acumulan y se procesan solo cuando
  llega una línea completa terminada en `\r\n` (manejo de TCP fragmentado).
- Gestión de **señales** (`SIGINT`, `SIGTERM`) para un apagado limpio, e ignorado de `SIGPIPE`.
- Validación de argumentos: puerto en rango `1–65535` y contraseña no vacía.
---
 
## 💬 Comandos soportados
 
### Registro y conexión
 
| Comando | Descripción |
|---------|-------------|
| `PASS`  | Envía la contraseña del servidor |
| `NICK`  | Establece o cambia el apodo |
| `USER`  | Registra usuario y nombre real |
| `CAP`   | Negociación de *capabilities* (handshake con clientes modernos) |
| `PING` / `PONG` | Comprobación de conexión (keep-alive) |
| `QUIT`  | Cierra la sesión y desconecta al cliente |
 
### Canales y mensajería
 
| Comando | Descripción |
|---------|-------------|
| `JOIN`    | Unirse (o crear) un canal |
| `PART`    | Abandonar un canal |
| `PRIVMSG` | Enviar mensaje a un usuario o canal |
| `NOTICE`  | Enviar aviso (sin respuesta automática de error) |
| `TOPIC`   | Consultar o cambiar el tema del canal |
| `KICK`    | Expulsar a un usuario del canal (operador) |
| `INVITE`  | Invitar a un usuario a un canal (operador) |
| `MODE`    | Cambiar los modos del canal (operador) |
 
---
 
## 🔧 Modos de canal
 
El comando `MODE` implementa los modos exigidos por el subject:
 
| Modo | Efecto |
|:----:|--------|
| `+i` / `-i` | Canal solo por invitación (*invite-only*) |
| `+t` / `-t` | Solo los operadores pueden cambiar el `TOPIC` |
| `+k` / `-k` | Establece o elimina una contraseña de canal (*key*) |
| `+o` / `-o` | Concede o retira el privilegio de operador a un usuario |
| `+l` / `-l` | Limita o elimina el límite de usuarios del canal |
 
---
 
## 🤖 Bonus: `ircbot`
 
Como parte bonus se incluye **PPBot**, un cliente IRC independiente que se conecta al
servidor (o a cualquier servidor IRC de referencia) y responde de forma automática:
 
- Reacciona al comando `ppsize` tanto en **PRIVMSG directos** como en **mensajes de canal**.
- Si lo **invitan** a un canal (`INVITE`), se une automáticamente.
- Responde a los `PING` del servidor con `PONG` para mantener viva la conexión.
```bash
make bonus
./ircbot <host> <port> <password> [nick]
```
 
---
 
## 🗂 Estructura del proyecto
 
```
ft_irpf/
├── inc/                     # Cabeceras del servidor
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   ├── IrcMessage.hpp
│   └── Replies.hpp
├── src/
│   ├── main.cpp             # Parseo de argumentos, señales y arranque
│   ├── core/                # Lógica de dominio
│   │   ├── Server.cpp
│   │   ├── Client.cpp
│   │   └── Channel.cpp
│   ├── network/             # Capa de red
│   │   ├── Server_connect.cpp
│   │   ├── Server_recv.cpp
│   │   └── IrcMessage.cpp   # Parser del protocolo
│   └── commands/            # Implementación de comandos
│       ├── Cmd_register.cpp
│       └── Cmd_channel.cpp
├── bonus/                   # Bot IRC (binario independiente)
│   ├── inc/bot.hpp
│   └── src/{bot.cpp,main.cpp}
├── tests/                   # Suite de pruebas (unit + integration)
│   ├── unit/test_parser.cpp
│   ├── integration/*.sh
│   ├── run_all.sh
│   └── Tests.md
└── Makefile
```
 
---
 
## ⚙️ Compilación
 
| Regla | Acción |
|-------|--------|
| `make` / `make all` | Compila el servidor `ircserv` |
| `make bonus`        | Compila el bot `ircbot` |
| `make clean`        | Elimina los objetos (`obj/`) |
| `make fclean`       | Elimina objetos y binarios |
| `make re`           | `fclean` + `all` |
| `make test`         | Ejecuta toda la suite de tests |
| `make test_unit`    | Solo tests unitarios del parser |
| `make test_integration` | Solo tests de integración |
 
```bash
git clone https://github.com/caroldmg/ft_irpf.git
cd ft_irpf
make
```
 
---
 
## 🚀 Uso
 
```bash
./ircserv <port> <password>
```
 
- `<port>`: puerto de escucha (entero entre 1 y 65535).
- `<password>`: contraseña que deberán enviar los clientes para conectarse.
Ejemplo:
 
```bash
./ircserv 6667 contra
```
 
Después, conéctate con tu cliente favorito (por ejemplo HexChat) apuntando a
`localhost:6667` y usando esa misma contraseña, o prueba rápidamente con `netcat`:
 
```bash
nc localhost 6667
```
 
---
 
## 🧪 Ejemplo de sesión
 
```text
PASS contra
NICK alice
USER alice 0 * :Alice Doe
JOIN #42
TOPIC #42 :Bienvenidos al canal
PRIVMSG #42 :Hola a todos!
MODE #42 +it
INVITE bob #42
```
 
El servidor responde con la bienvenida (`001`–`004`), confirma el `JOIN`, difunde el
`TOPIC` y el `PRIVMSG`, y aplica los modos del canal.
 
---
 
## 🔬 Tests
 
El proyecto incluye una batería de pruebas propia para validar tanto el parser como el
comportamiento del servidor de extremo a extremo:
 
```bash
make test              # suite completa (unit + integration)
make test_unit         # solo el parser de mensajes IRC
make test_integration  # handshake, errores, JOIN/PART/PRIVMSG, modos, KICK...
```
 
Los tests de integración cubren, entre otros: el *handshake* de registro, los errores
(`464`, `433`, `451`), `PING/PONG`, `CAP`, `JOIN`/`PART`/`PRIVMSG`, `NOTICE`/`TOPIC`,
y `KICK`/`MODE`/`INVITE`. Consulta [`tests/Tests.md`](tests/Tests.md) para la guía de
pruebas manuales con clientes reales.
 
---

