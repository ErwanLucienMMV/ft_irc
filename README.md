# ft_irc
ft_irc project

Build with `make`, then run `./ircserv <port> <password>`.

Run network tests with `python3 tests/test_network.py` after building.
The tests use Python's standard library. Linux-only checks also verify socket
flags and inject short writes and I/O failures into a temporary test executable.
