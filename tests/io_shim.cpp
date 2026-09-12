// Test-only linker wrappers: force short writes and errors on real sockets.
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static fd_set readable;
static fd_set writable;

extern "C" int __real_select(int, fd_set *, fd_set *, fd_set *, timeval *);
extern "C" ssize_t __real_recv(int, void *, size_t, int);
extern "C" ssize_t __real_send(int, const void *, size_t, int);

static bool mode(const char *name)
{
	const char *value = std::getenv("IRC_TEST_MODE");
	return value && std::strcmp(value, name) == 0;
}

static void checkReady(int fd, fd_set &ready)
{
	if (fd >= FD_SETSIZE || !FD_ISSET(fd, &ready)
		|| !(fcntl(fd, F_GETFL) & O_NONBLOCK))
		_exit(90);
	FD_CLR(fd, &ready);
}

extern "C" int __wrap_select(int count, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, timeval *timeout)
{
	int result = __real_select(count, readfds, writefds, exceptfds, timeout);
	FD_ZERO(&readable);
	FD_ZERO(&writable);
	if (result > 0)
	{
		if (readfds)
			readable = *readfds;
		if (writefds)
			writable = *writefds;
	}
	return result;
}

extern "C" ssize_t __wrap_recv(int fd, void *buffer, size_t size, int flags)
{
	checkReady(fd, readable);
	static unsigned int calls = 0;
	++calls;
	if (mode("test_permanent_read_failure_disconnects"))
	{
		errno = ECONNRESET;
		return -1;
	}
	if (mode("test_temporary_failures_preserve_data") && calls == 1)
	{
		errno = EAGAIN;
		return -1;
	}
	return __real_recv(fd, buffer, size, flags);
}

extern "C" ssize_t __wrap_send(int fd, const void *buffer, size_t size, int flags)
{
	checkReady(fd, writable);
	static unsigned int calls = 0;
	++calls;
	if (mode("test_permanent_write_failure_disconnects"))
	{
		errno = EPIPE;
		return -1;
	}
	if (mode("test_temporary_failures_preserve_data") && calls % 2 == 1)
	{
		errno = EAGAIN;
		return -1;
	}
	return __real_send(fd, buffer, std::min(size, static_cast<size_t>(7)), flags);
}
