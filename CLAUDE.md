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

## Deliverables (subject v10.0, Chapter V — not yet done)

- **`README.md` is mandatory** and does not exist yet. Required contents:
  - first line, italicized, exactly: *This project has been created as part of the 42 curriculum by
    `<login>`* (add both logins if it stays a two-person project)
  - a **Description** section (goal + brief overview)
  - an **Instructions** section (compile, run)
  - a **Resources** section: classic references **and a description of how AI was used — which
    tasks, which parts of the project**
  - written in English
- Files to submit: `Makefile`, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp`, optional config file.
- Bonus (file transfer, bot) is only graded if the mandatory part is **perfect**. Don't start it.
- Defense includes a **live modification** request — a small behaviour change or data-structure
  tweak, to be done in a few minutes. Practice explaining every file out loud.

## Hard constraints (from the subject — violating these is an instant fail)

- **C++98 only.** No C++11: no `auto`, no `nullptr`, no range-for, no `std::to_string`, no move
  semantics. Remember there are no moves — a class put in a container gets **copied**, so any class
  holding an fd must not `close()` it in its destructor (rule of three).
- **Must compile with** `-Wall -Wextra -Werror`.
- **One and only one `poll()`** (or equivalent) for **read *and* write**, listening socket included.
- **Never `recv()` or `send()` on an fd unless `poll()` just said it is ready.** The subject states
  this outcome literally: doing so → **grade 0**. Sending therefore needs a per-client **out-buffer**
  plus `POLLOUT` in the same poll set — a handler may never call `send()` directly.
- **Everything non-blocking.** No blocking call may ever hang the server.
- **The server must never crash and must never quit unexpectedly** — "even when it runs out of
  memory". If it does, the project is non-functional and the grade is 0.
- No forking, no threads. Single process, single loop.
- **`fcntl` may be used *only* as `fcntl(fd, F_SETFL, O_NONBLOCK)`** — any other flag is forbidden,
  so do not add an `F_GETFL` read-modify-write. Current usage is compliant; leave it alone.
- Prefer C++ headers over C ones (`<cstring>`, not `<string.h>`). No external libs, no Boost.
- `sleep` is **not** in the allowed-function list. Allowed and useful later: `signal`/`sigaction`
  (clean shutdown, Phase 8), `getsockname`/`inet_ntoa`/`inet_ntop` (real host for the Phase 6
  prefix).
- **No server-to-server communication** — explicitly forbidden. Nothing will ever send us a prefix,
  so `parsePrefix` exists only to survive garbage input, not because clients use it.

## The subject's own Phase 3 test (run this one — it's what the evaluator types)

```sh
nc -C 127.0.0.1 6667
com^Dman^Dd          # ctrl+D after 'com', after 'man', then 'd\n'
```

`-C` makes nc send CRLF; `ctrl+D` flushes without a newline. Our byte-at-a-time test is stricter,
but this is the one in the PDF.

## Build

```sh
make        # -> ./ircserv
./ircserv <port> <password>
```

Header-only so far: `main.cpp` + `Server.hpp` + `Client.hpp`.

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
- `[x]` `bool extractCommand(std::string &line)` — erase-on-extract. Keys on `'\n'` (**not**
  `find_first_of("\r\n")`, which fired early when `\r` and `\n` arrived in different packets and
  emitted a phantom empty command), then strips a trailing `\r` off the cut line. Bare `\n` is
  accepted too, so interactive `nc` works.
- `[x]` `Server` drains in a **loop** after each `recv` — one read can carry several commands.
- `[x]` Verified: byte-at-a-time `nc` → 1 command; two commands in one write → 2; split
  mid-command across packets → 1. Server survives all three.

**Contract with Phase 4:** `extractCommand` hands out one line with `\r\n` already stripped,
possibly empty, otherwise untouched. The parser never sees a delimiter; `extractCommand` never
looks at a colon.

### Phase 4 — Parsing `[x]`
Learn: the `:prefix CMD params :trailing` grammar.
Work: a **stateless** Parser turning one line into a `Message` struct + a dispatcher.
Verify: feed raw lines, get correct structs — no sockets involved.

- `[x]` `t_command` struct in `parser.hpp` (`command`, `params`, `trailing` — no `prefix`, see below).
- `[x]` **1** `parse_command` — first word, uppercased via `toUpper`, leading spaces skipped.
- `[x]` **2** `parse_args` — params split on runs of spaces, first `:`-token opens the trailing,
  then the trailing is **also pushed as the last param** so `USER a b c :Real Name` is 4 params.
  The colon is wire syntax, not meaning: handlers must not care whether the sender used one.
- `[x]` **3** dispatcher on `Server`: `PASS NICK USER JOIN PRIVMSG MODE QUIT` + unknown fallback,
  guarded by an empty-command early return so a bare `\r\n` dispatches nothing.
- `[x]` **4** verified end to end through `nc` (table below).

Verified (8/8), server-side through `nc`:

| line | command | params | trailing |
|---|---|---|---|
| `PRIVMSG #general :hello there my friend` | PRIVMSG | `[#general]` | `hello there my friend` |
| `PRIVMSG   #general    :time is 10:30` | PRIVMSG | `[#general]` | `time is 10:30` |
| `USER a b c :Real Name` | USER | `[a, b, c]` | `Real Name` |
| `MODE #chan +o hen` | MODE | `[#chan, +o, hen]` | |
| `PRIVMSG #a :` | PRIVMSG | `[#a]` | |
| `JOIN` | JOIN | `[]` | |
| `pass hen` | PASS | `[hen]` | |
| `quit` | QUIT | `[]` | |

Two bugs fixed on the way, both the same shape — `start = space_pos + 1` lands *on* the next space
when spaces repeat, pushing empty params. Use `find_first_not_of(' ', pos)` to move to the next
real token. Free functions in a header need `inline`, or the linker reports multiple definition
(member functions defined in a class body are implicitly inline; free functions are not).

**`toUpper` must cast to `unsigned char`** before calling `std::toupper` — plain `char` is signed
on Linux, and a byte ≥ 0x80 (any accented character) becomes a negative int, which is UB.

**Decision — no `CAP` handling.** Real clients send `CAP LS 302` as their first line; it currently
falls through to the unknown-command branch, which is harmless. Do not raise this again unless a
reference client actually stalls during registration.

**Decision — no prefix parsing.** Clients never send a prefix (the server already knows which
socket the bytes came from), and server-to-server is explicitly forbidden by the subject, so no
line we ever receive will carry one. A `:`-leading garbage line just produces a command that
matches no handler — harmless. The `prefix` field stays in the struct but is only ever filled when
we *generate* replies. Do not reopen this.

Prefix **generation** is still mandatory in Phase 6: relayed messages must be stamped
`:nick!user@host PRIVMSG #chan :text` or the reference client shows no author. That is string
concatenation, not parsing.

The Parser is stateless, so it lives in `parser.hpp` as free functions — not on `Client`
(per-client state), not inlined in `Server`.

Parked until this phase lands: `parse_cmd` (commented out in `Server.hpp`) and all of
`Authentication.cpp` — both are Phase 5 code written early, and both hand-roll their own parsing
(`cmd.substr(4)`). They get rebuilt on top of `Message`.

### Phase 5 — Registration `[~]`  ← **WE ARE HERE**
Learn: the PASS → NICK → USER state machine + numeric replies.
Work: handlers + `Client` state flags + numerics 001 / 433 / 464.
Verify: a real client (HexChat / irssi) fully registers.
**Milestone — proves the whole pipeline end to end.**

- `[ ]` **1 — the write path, first.** Nothing else in this phase can be verified without it. A
  handler may **never** call `send()` (grade 0). Instead: a `_writeBuffer` on `Client`; handlers
  append to it; `Run()` sets `POLLOUT` in that client's `events` **only when the buffer is
  non-empty**; on `POLLOUT`, `send()` and erase **only the bytes actually sent** — `send` can write
  fewer than you gave it. (This is the one idea worth taking from the `bircd` skeleton: its
  `init_fd` does `if (strlen(buf_write) > 0) FD_SET(i, &fd_write);`.)
- `[ ]` **2** `PASS` → wrong password gets `464 ERR_PASSWDMISMATCH`.
- `[ ]` **3** `NICK` → uniqueness check across `_clients`; `433 ERR_NICKNAMEINUSE`,
  `431 ERR_NONICKNAMEGIVEN`, `432 ERR_ERRONEUSNICKNAME`.
- `[ ]` **4** `USER` → store username + realname.
- `[ ]` **5** registration gate: once PASS+NICK+USER are all done, send `001 RPL_WELCOME`.
  Commands before that get `451 ERR_NOTREGISTERED` — **except** the allowlist below.
- `[ ]` **6** verify with a real client (HexChat / irssi), not just `nc`.

**Numeric reply format — first param is always the recipient's nick:**

```
:localhost 001 hen :Welcome to the IRC network
:localhost 433 hen newnick :Nickname is already in use
:localhost 464 hen :Password incorrect
```

Leaving out the nick is the classic mistake; clients use it to confirm the reply is addressed to
them.

**Allowed before registration completes** (rejecting these breaks the reference client):

```
CAP   PASS   NICK   USER   PING   PONG   QUIT
```

`PING`/`PONG` are not in the subject's command list but are not optional — irssi and HexChat ping
the server and disconnect if nothing answers.

### Phase 6 — Channels + relay `[ ]`
Learn: a channel is a named member list; the first joiner is operator.
Work: `Channel` class, `JOIN`, `PRIVMSG` fan-out with prefix stamping.
Verify: two clients chat through the server.

### Phase 7 — Operators + MODE `[ ]`
One slice per command: `KICK`, `INVITE`, `TOPIC`, then MODE flags one at a time: `i`, `t`, `k`, `o`, `l`.
Work: permission checks + mode state on `Channel`.
Verify: each flag enforces correctly.

**Known parser limitation to fix here — empty trailing vs. no trailing.** `parse_args` pushes the
trailing into `params` only `if (!cmd.trailing.empty())`, so these two collapse to the same struct:

```
TOPIC #chan          -> "show me the topic"
TOPIC #chan :        -> "clear the topic"      <- currently indistinguishable
```

Fix: add a `bool has_trailing` to `t_command`, set it when `parse_args` hits the `:`, and push
unconditionally on that flag instead of on `!empty()`. Harmless everywhere else — `PRIVMSG #a :`
with an empty message is an error case either way (412 ERR_NOTEXTTOSEND).

### Phase 8 — Robustness `[ ]`
Learn: the never-crash / no-leak discipline; **ownership order** — remove the Client pointer from
every Channel *before* the Server destroys it.
Work: disconnect cleanup, valgrind-clean.
Verify: abrupt kills, garbage input → no crash, no leaks.

---

## Known open bugs (not yet fixed — mine to fix)

- No `.gitignore`. Build artifacts are **tracked**: `ircserv`, `a.out`, `compile_commands.json`,
  `.idea/`, `.cache/clangd/`. `*.o` / `*.d` are untracked but a `git add -A` would sweep them in.
- `Authentication.cpp` is fully commented out but listed in `SRCS`. When it comes back:
  `disconnect` is defined **twice**, and `authenticated(fd)` should be `isAuthenticated(fd)`.
- Its `send()` calls fire without `poll()` having reported `POLLOUT` on that fd — subject rule
  violation, and a partial write / `EAGAIN` waiting to happen. Needs a per-client out-buffer
  (Phase 8 territory, but do not forget it).

**Fixed:** disconnect path used to `fds.erase(fds.begin() + i)` *before* `_clients.erase(fds[i].fd)`,
so it erased the wrong map entry — and crashed with a `_GLIBCXX_ASSERTIONS` abort when the
disconnecting client was the last element. Now the fd is saved into `dead_fd` first.

## Design decisions already made (don't relitigate)

- **The `Client` destructor does not `close(_fd)`.** The Server `accept()`s the fd and the Server
  closes it — one owner. Closing in `~Client()` caused a real bug: copies into the map each closed
  the same fd, killing the socket before the first `recv()`.
- `appendToBuffer` takes `(const char *, size_t)`, not `std::string` — `recv()` data is not a C
  string and must not be truncated at a `\0`.
- `Client`'s default constructor sets `_fd = -1`, because `std::map::operator[]` default-constructs
  on a missing key and an uninitialized fd would later get `close()`d.
