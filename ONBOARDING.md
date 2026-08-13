# ft_irc — shared context

Two-person 42 project, IRC server in C++98. This file is the handoff: where each branch stands,
what was verified, what is decided, and what is still broken. Read it before writing code.

## How we work

Phase by phase, slice by slice. Finish the current slice, verify it, commit, then move on. A slice
is **not done when it compiles** — it is done when its verify step passes. Don't write Phase 6 code
while Phase 5 is open; that is how the two branches ended up incompatible.

Comments in the code are in Darija/Arabizi on purpose. Don't delete or translate them.

## Branches

| repo | branch | state |
|---|---|---|
| `ft_irc` | `chajoui_cmd` | Phases 0–5 done. Channels/operators not started. |
| `henixirc` | `main` | Phases 0–4 done, Phase 5 not started. Has `parser.hpp` + `Client.cpp`. |

They diverged: both restructured `Server.hpp`/`Server.cpp` independently, and there are now two
command structs (`struct Command` in `Server.hpp` vs `t_command` in `parser.hpp`) and two
dispatchers (handler map vs if/else chain). **Agree who owns which file before writing more.**

## Verified status of `chajoui_cmd` (tested, not assumed)

| phase | status | how it was checked |
|---|---|---|
| 2 non-blocking + poll | ✓ | two clients registered concurrently |
| 3 framing | ✓ | byte-at-a-time, `\r`/`\n` split across packets, 4 commands in one write |
| 4 parsing | ✓ | 7/8 on the trailing table below |
| 5 registration | ✓ | 464 / 462 / 461 / 432 / 431 / 001 all correct |
| 6 channels | ✗ | `JOIN`, `PRIVMSG` → 421 |
| 7 operators | ✗ | `MODE`, `TOPIC`, `KICK`, `INVITE` → 421 |
| 8 robustness | partly | survives garbage, 100 KB no-newline, 20 rapid connects, abrupt kill |

## Still open on `chajoui_cmd`

1. **`sendToClient` calls `send()` directly.** The subject: *"if you attempt to read/recv or
   write/send in any file descriptor without using poll(), your grade will be 0."* Everything
   funnels through that one function, so the fix is contained: append to a per-client write buffer,
   set `POLLOUT` in that client's `events` only when the buffer is non-empty, and on `POLLOUT`
   `send()` and erase **only the bytes send() actually accepted** (it can take fewer than you give
   it). All 18 call sites are then fixed without being touched.
2. **Numerics show an empty nick before registration** — `:ircserv 464  :Password incorrect`.
   Should be `*` when no nick is set. Clients use that field to know the reply is theirs.
3. **`QUIT` does nothing.** Verified: after `QUIT`, the server still answers `PING`.
4. **Unknown-command check runs before the registration gate**, so `JOIN` from an unregistered
   client gets 421 instead of 451. Self-resolves once `JOIN` exists.
5. **No cap on `_buffer`** — a client that never sends `\n` grows it forever. Phase 8.

## Bugs already found and fixed — do not reintroduce

**Framing must never use `std::getline`.** `getline` returns *success* on end-of-stream, so `"com"`
and `"com\n"` produce identical output — the one fact you need (was there a delimiter?) is
destroyed, and a partial line gets emitted as a complete command. That is the subject's own
`com^Dman^Dd` test failing.

Second half of the same bug: `_buffer = _buffer.substr(pos + 2)` mixed coordinate systems (`pos`
indexed into `line`, applied to `_buffer`) and hardcoded a 2-byte delimiter. When only the `\r` had
arrived, `substr(3)` on a 2-byte string threw `std::out_of_range` → `terminate()` → **server
died**. Reproduce with `printf 'A\r' | nc localhost 6667`.

The shape that works — find `'\n'` in `_buffer`, compute everything in `_buffer`'s coordinates,
erase `pos + 1`, treat `\r` as optional:

```cpp
while ((pos = _buffer.find('\n')) != std::string::npos)
{
    std::string line = _buffer.substr(0, pos);
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    _buffer.erase(0, pos + 1);
    cmds.push_back(line);
}
```

**Disconnect order.** `fds.erase(fds.begin() + i)` before `_clients.erase(fds[i].fd)` erases the
*wrong* map entry, and aborts outright when the disconnecting client is the last element. Copy the
fd into a local first, then erase from both containers.

**A class holding an fd must not `close()` it in its destructor.** C++98 has no moves, so putting
one in a container copies it, and every copy's destructor closes the same fd. The Server accepts
the fd and the Server closes it — one owner.

**`std::toupper` needs `static_cast<unsigned char>`.** Plain `char` is signed on Linux; any byte
≥ 0x80 (an accented character) becomes negative, which is undefined behaviour.

## Parsing rules

A `:` opens the trailing **only at the start of a parameter** — not anywhere in the line, not only
at the first parameter. Then it takes the rest of the line verbatim, spaces and further colons
included, as the last parameter.

| line | params | trailing |
|---|---|---|
| `PRIVMSG #a :hello world` | `[#a]` | `hello world` |
| `PRIVMSG #a hello` | `[#a, hello]` | — |
| `PRIVMSG #a :time is 10:30` | `[#a]` | `time is 10:30` |
| `PRIVMSG #a :a :b :c` | `[#a]` | `a :b :c` |
| `USER us:er 0 * :Real Name` | `[us:er, 0, *]` | `Real Name` |
| `NICK he:n` | `[he:n]` | — |
| `PRIVMSG   #a    :hi` | `[#a]` | `hi` |

**Known gap (Phase 7):** an empty trailing collapses to no trailing, so `TOPIC #a` (query) and
`TOPIC #a :` (clear) are indistinguishable. Fix with a `bool has_trailing` set when the `:` is
seen, and push on that flag instead of on `!empty()`.

## Decisions — settled, don't relitigate

- **No prefix parsing.** Clients never send a prefix and server-to-server is forbidden by the
  subject, so no incoming line will carry one. Prefix *generation* is still mandatory in Phase 6:
  relayed messages must be stamped `:nick!user@host PRIVMSG #chan :text` or the reference client
  shows no author. That is string concatenation, not parsing.
- **No `CAP` handling.** Real clients open with `CAP LS 302`; falling through to unknown-command is
  harmless. Revisit only if a reference client actually stalls.
- **Trailing goes into `params` as the last element.** Handlers must not care whether the sender
  used a colon — `USER a b c :Real Name` and `USER a b c Real` are both 4 parameters.
- **Parsing is stateless** and lives outside `Client` (per-client state) and outside `Server`
  (needs no server state), so it can be tested without opening a socket.

## Hard constraints (instant fail)

- C++98 only. No `auto`, `nullptr`, range-for, `std::to_string`, no moves.
- Compiles with `-Wall -Wextra -Werror`.
- **One** `poll()` for read *and* write, listening socket included.
- Never `recv()`/`send()` on an fd unless `poll()` just said it is ready → **grade 0**.
- Never crashes, never quits unexpectedly — "even when it runs out of memory".
- No fork, no threads.
- `fcntl` only as `fcntl(fd, F_SETFL, O_NONBLOCK)`. Any other flag is forbidden, so no
  `F_GETFL` read-modify-write.
- `README.md` is a **mandatory deliverable** and does not exist yet. First line italicised, exactly:
  *This project has been created as part of the 42 curriculum by `<login1>, <login2>`*. Needs
  Description, Instructions, and Resources sections — and Resources must describe **how AI was
  used, for which tasks and which parts of the project**.

## Test commands

```sh
make && ./ircserv 6667 hen

# happy path
printf 'PASS hen\r\nNICK bob\r\nUSER u 0 * :Real Name\r\n' | nc -q1 127.0.0.1 6667

# the subject's own framing test — must produce ONE command
nc -C 127.0.0.1 6667
com^Dman^Dd

# same thing scripted
(printf 'com'; sleep .4; printf 'man'; sleep .4; printf 'd\r\n'; sleep .4) | nc -q1 127.0.0.1 6667

# the crash case — server must stay alive
printf 'A\r' | nc -q1 127.0.0.1 6667

# bare \n (plain nc, terminal Enter)
printf 'PASS hen\nNICK b2\nUSER u 0 * :R\n' | nc -q1 127.0.0.1 6667

# robustness
printf '\x01\x02\xff\xfe garbage :\r\n' | nc -q1 127.0.0.1 6667
(head -c 100000 /dev/zero | tr '\0' 'A') | nc -q1 127.0.0.1 6667
```

## Next

**Phase 6 — channels + relay.** A channel is a named member list; the first joiner is operator.
`Channel` class, `JOIN`, then `PRIVMSG` fan-out: walk the member list and append to every *other*
member's write buffer, stamped with the sender's prefix. Verify with two clients chatting.

Do the write path (`POLLOUT`) first — fan-out has nowhere to go without it.
