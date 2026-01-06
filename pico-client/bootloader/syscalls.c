// bootloader/syscalls.c
#include <errno.h>
#include <sys/stat.h>

// Minimal syscalls
int _close(int file) { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _getpid(void) { return 1; }
int _isatty(int file) { (void)file; return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
off_t _lseek(int file, off_t ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
int _write(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return len; }
void _exit(int status) { while(1); }

// Heap allocation
void *_sbrk(int incr) {
    extern char _end;
    static char *heap_end = 0;
    char *prev_heap_end;
    
    if (heap_end == 0) heap_end = &_end;
    prev_heap_end = heap_end;
    
    // Simple check - adjust for your actual heap size
    if (heap_end + incr > (char*)0x20040000) return (void*)-1;
    
    heap_end += incr;
    return prev_heap_end;
}

// Assert handler
void hard_assertion_failure(void) {
    while(1);
}

// Runtime init
void runtime_init_default_alarm_pool(void) {}
