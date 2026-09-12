"""Run with: make && python3 tests/test_network.py (Python standard library only)."""

import os
from pathlib import Path
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest


BINARY = Path(__file__).resolve().parents[1] / "ircserv"
RESPONSE = b":localhost CAP * LS :\r\n"


class NetworkFixture(unittest.TestCase):
    def setUp(self):
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            self.port = probe.getsockname()[1]
        self.server = subprocess.Popen(
            [str(getattr(self, "binary", BINARY)), str(self.port), "test"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            env=dict(os.environ, IRC_TEST_MODE=self._testMethodName),
        )
        self.clients = []
        self.connect()

    def tearDown(self):
        for client in self.clients:
            client.close()
        if self.server.poll() is None:
            self.server.terminate()
        try:
            self.server.wait(timeout=2)
        except subprocess.TimeoutExpired:
            self.server.kill()
            self.server.wait()
            self.fail("Server did not stop after SIGTERM")

    def connect(self):
        deadline = time.monotonic() + 2
        while True:
            try:
                client = socket.create_connection(("127.0.0.1", self.port), 0.2)
                client.settimeout(2)
                self.clients.append(client)
                return client
            except OSError:
                if time.monotonic() >= deadline or self.server.poll() is not None:
                    raise
                time.sleep(0.01)

    def receive_exact(self, client, size):
        data = b""
        while len(data) < size:
            chunk = client.recv(size - len(data))
            self.assertTrue(chunk, "Unexpected EOF")
            data += chunk
        return data

    def assert_quiet(self, client):
        client.settimeout(0.1)
        try:
            with self.assertRaises(socket.timeout):
                client.recv(1)
        finally:
            client.settimeout(2)


class NetworkTests(NetworkFixture):
    def test_fragmented_command_waits_for_newline(self):
        client = self.clients[0]
        for part in (b"CA", b"P LS 302", b"\r"):
            client.sendall(part)
            self.assert_quiet(client)
        client.sendall(b"\n")
        self.assertEqual(self.receive_exact(client, len(RESPONSE)), RESPONSE)

    def test_multiple_commands_and_no_substring_matches(self):
        client = self.clients[0]
        client.sendall(
            b"PRIVMSG bob :CAP LS\r\nXCAP LS\r\n"
            b"CAP LS\r\ncap ls 302\r\nCAP LS\n"
        )
        self.assertEqual(self.receive_exact(client, 3 * len(RESPONSE)), RESPONSE * 3)
        self.assert_quiet(client)

    def test_clients_have_separate_input_buffers(self):
        alice = self.clients[0]
        bob = self.connect()
        alice.sendall(b"CA")
        bob.sendall(b"CAP LS\r\n")
        self.assertEqual(self.receive_exact(bob, len(RESPONSE)), RESPONSE)
        self.assert_quiet(alice)
        alice.sendall(b"P LS\r\n")
        self.assertEqual(self.receive_exact(alice, len(RESPONSE)), RESPONSE)

    def test_half_close_drains_queued_replies(self):
        client = self.clients[0]
        client.sendall(b"CAP LS\r\n" * 30)
        client.shutdown(socket.SHUT_WR)
        self.assertEqual(self.receive_exact(client, 30 * len(RESPONSE)), RESPONSE * 30)
        self.assertEqual(client.recv(1), b"")

    def test_oversized_incomplete_line_disconnects_only_its_client(self):
        self.clients[0].sendall(b"X" * 512)
        self.assertEqual(self.clients[0].recv(1), b"")
        client = self.connect()
        client.sendall(b"CAP LS\r\n")
        self.assertEqual(self.receive_exact(client, len(RESPONSE)), RESPONSE)

    def test_slow_reader_does_not_block_other_clients(self):
        slow = self.clients[0]
        slow.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        try:
            slow.sendall(b"CAP LS\r\n" * 20000)
        except (BrokenPipeError, ConnectionResetError):
            pass  # The bounded output queue may disconnect this client.
        healthy = self.connect()
        healthy.sendall(b"CAP LS\r\n")
        self.assertEqual(self.receive_exact(healthy, len(RESPONSE)), RESPONSE)

    def test_reset_clients_do_not_kill_server(self):
        for _ in range(20):
            client = self.connect()
            client.sendall(b"CAP LS\r\n" * 5)
            client.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
            client.close()
        healthy = self.connect()
        healthy.sendall(b"CAP LS\r\n")
        self.assertEqual(self.receive_exact(healthy, len(RESPONSE)), RESPONSE)

    @unittest.skipUnless(Path("/proc/self/fdinfo").exists(), "Linux fd inspection")
    def test_listening_and_accepted_sockets_are_nonblocking(self):
        client = self.clients[0]
        client.sendall(b"CAP LS\r\n")
        self.receive_exact(client, len(RESPONSE))
        fd_dir = Path("/proc") / str(self.server.pid) / "fd"
        sockets = [entry for entry in fd_dir.iterdir()
                   if str(entry.readlink()).startswith("socket:")]
        self.assertGreaterEqual(len(sockets), 2)
        for entry in sockets:
            info = (fd_dir.parent / "fdinfo" / entry.name).read_text()
            flags = next(line.split()[1] for line in info.splitlines()
                         if line.startswith("flags:"))
            self.assertTrue(int(flags, 8) & os.O_NONBLOCK)

    def test_sigint_stops_with_live_clients(self):
        client = self.clients[0]
        client.sendall(b"CAP LS\r\n")
        self.receive_exact(client, len(RESPONSE))
        self.server.send_signal(signal.SIGINT)
        self.assertEqual(self.server.wait(timeout=2), 0)
        self.assertEqual(client.recv(1), b"")


@unittest.skipUnless(sys.platform.startswith("linux"), "GNU linker wrappers")
class InjectedIoTests(NetworkFixture):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="irc-io-tests-")
        cls.addClassCleanup(cls.directory.cleanup)
        cls.binary = Path(cls.directory.name) / "ircserv"
        root = BINARY.parent
        subprocess.run([
            "c++", "-std=c++98", "-Wall", "-Wextra", "-Werror", "-Iincludes",
            "srcs/main.cpp", "srcs/server.cpp", "srcs/client.cpp", "tests/io_shim.cpp",
            "-Wl,--wrap=select", "-Wl,--wrap=recv", "-Wl,--wrap=send",
            "-o", str(cls.binary),
        ], cwd=root, check=True)

    def test_partial_writes_preserve_order(self):
        client = self.clients[0]
        client.sendall(b"CAP LS\r\n" * 30)
        client.shutdown(socket.SHUT_WR)
        self.assertEqual(self.receive_exact(client, 30 * len(RESPONSE)), RESPONSE * 30)
        self.assertEqual(client.recv(1), b"")

    def test_temporary_failures_preserve_data(self):
        self.test_partial_writes_preserve_order()

    def assert_disconnected(self):
        client = self.clients[0]
        client.sendall(b"CAP LS\r\n")
        try:
            self.assertEqual(client.recv(1), b"")
        except ConnectionResetError:
            pass
        self.assertIsNone(self.server.poll())

    def test_permanent_read_failure_disconnects(self):
        self.assert_disconnected()

    def test_permanent_write_failure_disconnects(self):
        self.assert_disconnected()


if __name__ == "__main__":
    unittest.main(verbosity=2)
