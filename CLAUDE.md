# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# ft_irc — project context for Claude

An IRC server in C++98 (42 school project). The author is **learning the material while building it**.
Read the "How to work with me" section before writing any code.

---

## How to work with me — READ THIS FIRST

**You are a tutor and a helper, not an autopilot.**

1. **We build phase by phase, slice by slice. Never push ahead.**
   If we are on Phase 3, do not write Phase 4 code, do not "prepare" Phase 5 fields, do not add a
   Parser class because it will be needed later. Finish the current slice, stop, and say what the
   next slice is. Scope creep is the failure mode here.

2. **Explain before you implement.** When I ask for a slice, say what the concept is and why it
   works that way (what TCP actually guarantees, what `poll` returns, why the fd owner matters)
   — then write the code. If I ask "which phase are we on" or "check the issue", answer only that:
   do not start coding.

3. **Point out my bugs, don't silently fix them.** If you spot a bug outside the current slice,
   name it, show the line, explain the failure — then let me decide. Do not rewrite my code.

4. **Keep my style.** My comments are in Darija/Arabizi and they are how I remember things.
   Never delete or translate them. Match the surrounding formatting (tabs, brace placement).

5. **One slice = one commit.** After a slice is verified, remind me to commit.

6. **Verification is part of the slice.** A slice is not done when it compiles; it is done when the
   verify step in the phase list below passes. Actually run it when you can.

---

## Hard constraints (from the subject — violating these is an instant fail)

- **C++98 only.** No C++11: no `auto`, no `nullptr`, no range-for, no `std::to_string`, no move
  semantics. Remember there are no moves — a class put in a container gets **copied**, so any class
  holding an fd must not `close()` it in its destructor (rule of three).
- **Must compile with** `-Wall -Wextra -Werror`.
- **One and only one `poll()`** (or equivalent) for all I/O, listening socket included.
- **Never `recv()` or `send()` on an fd unless `poll()` just said it is ready.** Non-negotiable.
- **Everything non-blocking.** No blocking call may ever hang the server.
- **The server must never crash and never leak** — not on abrupt disconnect, not on garbage input.
- No forking, no threads. Single process, single loop.

## Build

```sh
make        # -> ./ircserv
./ircserv <port> <password>
```

**`make` links clean** with `-Wall -Wextra -Werror -std=c++98`.

Layout:

```
includes/        ft_irc.hpp  Server.hpp  Client.hpp  Channel.hpp  Command.hpp
srcs/
  main.cpp
  core/          Server.cpp  Client.cpp  Channel.cpp
  commands/      Authentication.cpp  join.cpp  PRIVMSG.cpp  kick.cpp
                 INVITE.cpp  TOPIC.cpp  MODE.cpp
docs/            roadmap.txt, vscode-mcp-setup.md, reference/bircd.tar.gz — not source
```

Headers are found through `-I includes`, so sources keep plain `#include "Server.hpp"`.
`.o` files are built **beside their `.cpp`** (`srcs/core/Server.o`) and are gitignored.
When you add a `.cpp`, add it to `SRCS` with its full path from the repo root.

**No dependency tracking.** `-MMD -MP` and the `.d` files were removed on purpose, so make only
knows `foo.o` depends on `foo.cpp` — **not** on any header. Edit a `.hpp` and `make` will say
`Nothing to be done for 'all'` and happily link stale objects. **After touching any header, run
`make re`.**

There is no test suite. Verification is manual, per the *Verify* line of the current slice
(`nc localhost <port>`, `ss -tlnp | grep <port>`, later a real IRC client).
Note: `nc` is **not installed** on this machine — use a short python3 socket script instead.

---

## The architecture — phases and slices

Status legend: `[x]` done · `[~]` in progress · `[ ]` not started.
**Keep this table updated as we finish slices — it is the single source of truth for where we are.**

### Phase 0 — Skeleton that compiles `[x]`
Learn: Makefile rules, project layout, the empty Server class shape.
Work: a repo that builds `ircserv` and prints "started".
Verify: `make` produces the binary, runs clean.
Why first: you always have something that compiles; every later slice extends a green build.

### Phase 1 — The socket chain (one slice per syscall) `[x]`
- `[x]` **1** argv: validate port + password → `main` + `Server` constructor storing them. *Verify: bad args exit cleanly.*
- `[x]` **2** `socket()`: create the fd → `Server::setup()`. *Verify: no error, fd ≥ 0.*
- `[x]` **3** `setsockopt()`: reusable port. *Verify: restart twice, no "address in use".*
- `[x]` **4** `bind()`: fill `sockaddr_in`, claim the port. *Verify: it binds.*
- `[x]` **5** `listen()`: LISTEN state. *Verify: `ss -tlnp | grep <port>` shows LISTEN.*
- `[x]` **6** `accept()` (blocking): one client fd. *Verify: `nc` connects, prints "client connected".*

### Phase 2 — Non-blocking + one `poll()` `[x]`
- `[x]` **1** `fcntl(O_NONBLOCK)` on the listening socket. *Verify: accept no longer freezes.*
- `[x]` **2** the `poll()` set + main loop watching the listening fd. *Verify: loop runs, accepts on ready.*
- `[x]` **3** accept adds each client fd to the poll set → many clients. *Verify: two `nc` sessions at once.*

This became `Server::Run()` — the heart of the project.

### Phase 3 — Per-client buffering `[x]`
Learn: TCP is a boundary-less stream — one `send` can arrive as `com` → `man` → `d`, and two
commands can arrive in a single `recv`. Message framing is **our** job, not TCP's.
Work: `Client::_buffer` + append-on-recv + cut-on-`\r\n`.
Verify: send one byte at a time through `nc`, the command still assembles correctly.

- `[x]` `Client` class shape: `_fd`, `_buffer`, `appendToBuffer(data, size)`, `getFd()`, destructor.
- `[x]` `Server` holds `std::map<int, Client>`; accept inserts, disconnect erases.
- `[x]` framing — `Client::splitBuffer()` (`srcs/core/Client.cpp`) loops on `find('\n')`, strips a
  trailing `\r`, and **erases the consumed bytes** with `_buffer.erase(0, pos + 1)`. An
  unterminated tail stays in `_buffer` until its delimiter arrives. This was the open slice.
- `[x]` `Server` drains in a loop after each `recv` — `Run()` iterates `cmds` and calls `parse_cmd`
  per line.
- `[ ]` Byte-at-a-time verify still never actually run. The code reads correct; the *evidence* is
  missing. Worth doing once in Phase 8 with a python socket script (no `nc` here).

### Phase 4 — Parsing `[x]`
Learn: the `:prefix CMD params :trailing` grammar.
Work: a **stateless** Parser turning one line into a struct + a dispatcher.
Verify: feed raw lines, get correct structs.

- `Command` struct (`Server.hpp`): `command`, `args`, `trailing`, `hasTrailing`.
  `Server::buildCommand` fills it, `Server::split_cmd` tokenizes, `Server::parse_cmd` dispatches.
- The if/else chain is gone: dispatch is `std::map<std::string, CmdHandler>` where
  `typedef void (Server::*CmdHandler)(const Command &cmds, int fd)` — a **pointer-to-member**,
  so every handler shares one signature. This is what the old `Server.hpp` comment was asking for.
- `Server::needsRegistration` gates commands that require a registered client.

### Phase 5 — Registration `[x]`
Learn: the PASS → NICK → USER state machine + numeric replies.
Work: handlers + `Client` state flags + numerics 001 / 433 / 464.
Verify: a real client (HexChat / irssi) fully registers.
**Milestone — proves the whole pipeline end to end.**

- `srcs/commands/Authentication.cpp`: `checkPass`, `setNickname`, `setUsername`, `registerClient`,
  `quit`, `disconnect` all implemented. `Client` carries `_pass` / `_registered` with accessors.
- Replies are real numerics from the macro set in `includes/Command.hpp` (001, 431/432/433,
  451, 461, 462, 464, …).
- Verified: `PASS`/`NICK`/`USER` over a socket returns
  `:ircserv 001 bob :Welcome to the IRC Network bob!b@127.0.0.1`.

### Phase 6 — Channels + relay `[x]`
Learn: a channel is a named member list; the first joiner is operator.
Work: `Channel` class, `JOIN`, `PRIVMSG` fan-out with prefix stamping.
Verify: two clients chat through the server.

- `Channel.hpp` / `Channel.cpp`, `commands/join.cpp`, `commands/PRIVMSG.cpp`.
- `Server::broadcastMessage` (two overloads — with and without an fd to skip),
  `sendToChannel`, `toClient`, `getMembersList`, `joinAlert` (353/366).
- Writes go through `sendToClient` → per-client `_writeBuffer`, flushed on `POLLOUT`
  (`Server.cpp:302` arms it, `:315` drains it). The old "send on a POLLIN-only fd" violation is gone.

### Phase 7 — Operators + MODE `[x]`
One slice per command: `KICK`, `INVITE`, `TOPIC`, then MODE flags one at a time: `i`, `t`, `k`, `o`, `l`.
Work: permission checks + mode state on `Channel`.
Verify: each flag enforces correctly.

- `commands/kick.cpp`, `commands/INVITE.cpp`, `commands/TOPIC.cpp`, `commands/MODE.cpp`.
- All five flags handled in the `switch` in `MODE.cpp` (`i` `t` `k` `l` `o`), with `+`/`-` tracking
  and 324/472/482 replies.
- `[ ]` Per-flag enforcement never verified with two live clients — that verify is still owed.

### Phase 8 — Robustness `[~]`  ← **WE ARE HERE**
Learn: the never-crash / no-leak discipline; **ownership order** — remove the Client pointer from
every Channel *before* the Server destroys it.
Work: disconnect cleanup, valgrind-clean.
Verify: abrupt kills, garbage input → no crash, no leaks.

- `[x]` `Server::removeClientFromChannels(fd, quitMessage)` exists and is called from `disconnect`,
  so the Channel membership goes before the Client does — the ownership order this phase is about.
- `[x]` **signals** — `main.cpp` sets `SIGPIPE` to `SIG_IGN` (a `send()` to a dead peer now returns
  `-1/EPIPE` instead of killing the process) and catches `SIGINT`/`SIGTERM` into
  `volatile sig_atomic_t g_stop`. `Run()` loops on `while (!g_stop)`, so `~Server()` finally runs
  and closes every fd. The `EINTR` branch on `poll()` is what wakes the loop to see the flag.
  Verified: `/proc/<pid>/status` shows SIGPIPE ignored, SIGINT+SIGTERM caught; Ctrl-C exits 0 and
  prints "Server shutting down..." (before, it died on signal 130 and never ran the destructor).
- `[x]` **abrupt-kill pass** — 25 rounds of `SO_LINGER{1,0}` RST disconnects with 400 bytes still
  queued for the dead client: server survived every round, `00:00:00` CPU time (no spin), clean
  exit afterwards.
- `[ ]` valgrind never run — **valgrind is not installed on this machine** (`pacman -S valgrind`).
- `[ ]` garbage-input pass never run.
- `[ ]` the two owed verifies inherited from Phases 3 and 7 (byte-at-a-time framing, per-flag MODE).

---

## Known open bugs (not yet fixed — mine to fix)

Ordered by what blocks first. **Do not fix these unsolicited** — rule 3 applies: name it,
show the line, let me decide.

*Logic:*

- `includes/Server.hpp:58` — `// std::vector<int> _client_fds;` is a commented-out dead member.
  `Run()` keeps its poll set in `_fds`. Just noise now; delete when you next touch the header.
- `Server::Run()` never tests `POLLERR` or `POLLHUP`, only `POLLIN`/`POLLOUT`. A socket in an error
  state is noticed indirectly (via `recv` returning `<= 0`), which works but is not explicit.

*Owed verifications (not bugs — missing evidence):*

- byte-at-a-time framing (Phase 3 gate), per-flag MODE enforcement (Phase 7), valgrind,
  abrupt-kill / garbage-input survival. All four are Phase 8 work.

*Fixed (kept for the lesson):*

- ~~disconnect path erased from `fds` before `_clients`, so the wrong map entry was erased~~ —
  `dead_fd` is saved into a local first.
- ~~`setAuthenticated()` was defined on `Server` but assigned `Client::_authenticated`~~ — the
  whole `_authenticated` flag is gone, replaced by `Client::_pass` / `_registered` with accessors.
  Lesson: a setter lives on the class that owns the field.
- ~~`Server::disconnect` defined twice, identically~~ — one definition, `Authentication.cpp:129`.
- ~~call to `authenticated(fd)`, declared nowhere~~ — gone.
- ~~`Makefile` built `main.cpp` only, so every out-of-line method was an undefined reference~~ —
  `SRCS` now lists all eleven sources.
- ~~`splitBuffer()` never erased consumed bytes and returned an unterminated tail as a command~~ —
  fixed; see Phase 3. **This was the real lesson of the project: framing is the application's job.**
- ~~`send()` called on an fd `poll()` only reported `POLLIN`-ready~~ — there is now a per-client
  `_writeBuffer` + `hasPendingWrite()` + `POLLOUT`, which is the instant-fail constraint satisfied.
- ~~`Client.hpp` used `std::cout`/`std::vector` and `Server.hpp` used `std::istringstream` without
  their includes, compiling only by include-order luck~~ — both headers include what they use.
  Lesson: a header must be self-sufficient.

## Design decisions already made (don't relitigate)

- **The `Client` destructor does not `close(_fd)`.** The Server `accept()`s the fd and the Server
  closes it — one owner. Closing in `~Client()` caused a real bug: copies into the map each closed
  the same fd, killing the socket before the first `recv()`.
- `appendToBuffer` takes `(const char *, size_t)`, not `std::string` — `recv()` data is not a C
  string and must not be truncated at a `\0`.
- `Client`'s default constructor sets `_fd = -1`, because `std::map::operator[]` default-constructs
  on a missing key and an uninitialized fd would later get `close()`d.
- **On a fatal `send()` error, `flushClient` clears the write buffer — it does not `disconnect()`.**
  Deliberate. `flushClient` is called from inside `Run()`'s loop over `_fds`, and `disconnect()`
  erases from `_fds`; erasing the element being iterated would shift the vector under the loop and
  read `_fds[i]` out of range when `i` was the last index. Clearing the buffer instead drops
  `hasPendingWrite()` to false, `POLLOUT` stops being armed (no spin), and the dead socket is
  reaped normally on the next `recv() <= 0`. `EAGAIN`/`EWOULDBLOCK` are excluded — those are not
  errors, and clearing on them would silently discard real output.
