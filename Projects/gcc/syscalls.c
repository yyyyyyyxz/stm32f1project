#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

extern char _end;
extern char _estack;

void *_sbrk(int incr)
{
    static char *heap_end;
    char *prev;
    char *stack_limit = (char *)((uintptr_t)&_estack - 0x400U);

    if (heap_end == 0)
    {
        heap_end = &_end;
    }

    if (heap_end + incr > stack_limit)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    prev = heap_end;
    heap_end += incr;
    return prev;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}
