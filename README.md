*This project has been created as part of the 42 curriculum by hcombaud, emaigne.*

# ft_irc

## Description

`ft_irc` is a small IRC server written in C++98. It accepts multiple TCP/IP
clients in a single non-blocking `select()` loop. Connected users can register,
exchange direct messages, join channels, talk in channels, and use the channel
operator commands required by the project.

The server keeps its state in memory. Users, channels and messages are not
preserved after it stops. Irssi 1.4.5 is the reference IRC client.

## Instructions

Compile the server:

```sh
make
```

Start it with a TCP port and an optional connection password:

```sh
./ircserv <port> [password]
```

For example:

```sh
./ircserv 6667 secret
```

The subject's invocation `./ircserv <port> <password>` requires clients to
authenticate with that password. For local testing, `./ircserv 6667` also
works: clients can register with `NICK` and `USER` without sending `PASS`.
An explicitly supplied empty password is rejected; omit the argument instead.

Connect with Irssi:

```sh
irssi -c 127.0.0.1 -p 6667 -w secret -n alice
```

Omit `-w secret` when connecting to a server started without a password.

The implemented channel modes are `i` (invite-only), `t` (operator-only topic),
`k` (channel key), `o` (channel operator) and `l` (user limit).

Usernames are limited to 12 characters. If memory allocation fails while the
server is running, it closes existing client connections and clears channels
to recover without allocating memory. The listening socket remains open;
clients can reconnect once memory is available again.

Clean generated files with `make clean` or `make fclean`. Use `make re` for a
complete rebuild.

## Resources

- [RFC 2812 - Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
- [Modern IRC Client Protocol](https://modern.ircdocs.horse/)
- Linux manual pages for `socket`, `fcntl`, `select`, `recv` and `send`

AI assistance was used to explain networking and IRC concepts, review design
choices, help implement and refactor the parser, server, client and channel
logic, and prepare local automated tests. Every generated change was reviewed,
compiled and tested incrementally by the project authors.
