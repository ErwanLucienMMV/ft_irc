*This project has been created as part of the 42 curriculum by hcombaud, emaigne.*

# ft_irc

## Description

A C++98 IRC server supporting multiple clients, private messages, channels
and operator commands (`KICK`, `INVITE`, `TOPIC`, `MODE i/t/k/o/l`).
Reference client: **Irssi 1.4.5**.

## Instructions

```sh
make
./ircserv <port> [password]
```

Example:

```sh
./ircserv 6667 secret
```

In another terminal:

```sh
irssi -c 127.0.0.1 -p 6667 -w secret -n alice
```

For password-free testing, omit both `secret` and `-w secret`.
Use a password for the subject's required invocation.

`make clean` removes object files, `make fclean` also removes the executable,
and `make re` rebuilds the project.

## Resources

- [RFC 2812 - Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
- [Modern IRC Client Protocol](https://modern.ircdocs.horse/)
- Linux manual pages for `socket`, `fcntl`, `select`, `recv` and `send`

AI was used for networking explanations, design reviews, implementation and
refactoring of the parser, server, clients and channels, and local test creation.
