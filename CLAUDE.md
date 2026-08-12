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

### Phase 3 — Per-client buffering `[~]`  ← **WE ARE HERE**
Learn: TCP is a boundary-less stream — one `send` can arrive as `com` → `man` → `d`, and two
commands can arrive in a single `recv`. Message framing is **our** job, not TCP's.
Work: `Client::_buffer` + append-on-recv + cut-on-`\r\n`.
Verify: send one byte at a time through `nc`, the command still assembles correctly.

- `[x]` `Client` class shape: `_fd`, `_buffer`, `appendToBuffer(data, size)`, `getFd()`, destructor.
- `[x]` `Server` holds `std::map<int, Client>`; accept inserts, disconnect erases.
- `[~]` `extractCommand()` — currently a placeholder that returns the whole buffer and erases
  nothing. Still needs: find `\r\n`, return only the bytes before it, erase those bytes **and** the
  delimiter from `_buffer`, and report "no complete line yet" in a way an empty line can't fake.
- `[ ]` `Server` drains in a **loop** after each `recv` — one read can carry several commands.
- `[ ]` Verify with byte-at-a-time `nc`.

### Phase 4 — Parsing `[ ]`
Learn: the `:prefix CMD params :trailing` grammar.
Work: a **stateless** Parser turning one line into a `Message` struct + a dispatcher.
Verify: feed raw lines, get correct structs.

### Phase 5 — Registration `[ ]`
Learn: the PASS → NICK → USER state machine + numeric replies.
Work: handlers + `Client` state flags + numerics 001 / 433 / 464.
Verify: a real client (HexChat / irssi) fully registers.
**Milestone — proves the whole pipeline end to end.**

### Phase 6 — Channels + relay `[ ]`
Learn: a channel is a named member list; the first joiner is operator.
Work: `Channel` class, `JOIN`, `PRIVMSG` fan-out with prefix stamping.
Verify: two clients chat through the server.

### Phase 7 — Operators + MODE `[ ]`
One slice per command: `KICK`, `INVITE`, `TOPIC`, then MODE flags one at a time: `i`, `t`, `k`, `o`, `l`.
Work: permission checks + mode state on `Channel`.
Verify: each flag enforces correctly.

### Phase 8 — Robustness `[ ]`
Learn: the never-crash / no-leak discipline; **ownership order** — remove the Client pointer from
every Channel *before* the Server destroys it.
Work: disconnect cleanup, valgrind-clean.
Verify: abrupt kills, garbage input → no crash, no leaks.

---

## Known open bugs (not yet fixed — mine to fix)

- `Server.hpp`, disconnect path: `fds.erase(fds.begin() + i)` runs **before**
  `_clients.erase(fds[i].fd)`. After the vector erase, `fds[i]` is the *next* client, so the wrong
  map entry is erased — and if `i` was the last element, it reads past the end. Save the fd into a
  local before erasing from the vector.

## Design decisions already made (don't relitigate)

- **The `Client` destructor does not `close(_fd)`.** The Server `accept()`s the fd and the Server
  closes it — one owner. Closing in `~Client()` caused a real bug: copies into the map each closed
  the same fd, killing the socket before the first `recv()`.
- `appendToBuffer` takes `(const char *, size_t)`, not `std::string` — `recv()` data is not a C
  string and must not be truncated at a `\0`.
- `Client`'s default constructor sets `_fd = -1`, because `std::map::operator[]` default-constructs
  on a missing key and an uninitialized fd would later get `close()`d.
