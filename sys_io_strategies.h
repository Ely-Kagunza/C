#ifndef SYS_IO_STRATEGIES_H
#define SYS_IO_STRATEGIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
#else  
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

// ============== ERROR HANDLING MACROS (Phase 11 Part 3) ==============

#define SYS_CALL(call) \
    do { \
        if ((call) < 0) { \
            perror("System call failed: "#call); \
            return -1; \
        } \
    } while (0)

#define HANDLE_OPEN_ERROR(fd) \
    do { \
        if ((fd) < 0) { \
            printf("[ERROR] Failed to open file\n"); \
            return NULL; \
        } \
    } while (0)

#define HANDLE_READ_ERROR(bytes) \
    do { \
        if ((bytes) < 0) { \
            perror("Read failed"); \
            return -1; \
        } \
    } while (0)

#define HANDLE_WRITE_ERROR(bytes) \
    do { \
        if ((bytes) < 0) { \
            perror("Write failed"); \
            return -1; \
        } \
    } while (0)

// ============== I/O CALLBACKS (Phase 11 Part 1) ==============

// Event callbacks for I/O operations
typedef void (*OnFileOpenCallback)(int fd, const char *filepath, void *user_data);
typedef void (*OnFileReadCallback)(int fd, size_t bytes_read, void *user_data);
typedef void (*OnFileWriteCallback)(int fd, size_t bytes_written, void *user_data);
typedef void (*OnFileCloseCallback)(int fd, void *user_data);

typedef struct {
    OnFileOpenCallback on_open;
    OnFileReadCallback on_read;
    OnFileWriteCallback on_write;
    OnFileCloseCallback on_close;
    void *user_data;
} IOCallbacks;

// ============== I/O STRATEGY VTABLE (Phase 11 Part 2) ==============

typedef struct IOStrategy {
    // Function pointers (the vtable)
    int(*open)(void *self, const char *path, int flags);
    int(*read)(void *self, int fd, void *buf, size_t count);
    int(*write)(void *self, int fd, const void *buf, size_t count);
    int(*close)(void *self, int fd);
    int(*seek)(void *self, int fd, long offset);
    void(*display_stats)(void *self);
    void(*free_strategy)(void *self);

    // Metadata
    const char *name;
    IOCallbacks callbacks;
    void *strategy_data;
} IOStrategy;

// ============== STRATEGY IMPLEMENTATIONS ==============

// Strategy 1: Direct system calls (standard I/O)
IOStrategy* io_standard_create(IOCallbacks callbacks);

// Strategy 2: Buffered I/O (internal buffering)
IOStrategy* io_buffered_create(IOCallbacks callbacks);

// Strategy 3: Memory-mapped I/O (advanced - linux only)
IOStrategy* io_mmap_create(IOCallbacks callbacks);

// ============== GENERIC HELPER FUNCTIONS ==============

int io_run(IOStrategy *strategy, const char *filepath, int flags);
void io_display_all_stats(IOStrategy *strategies[], int count);

#endif // SYS_IO_STRATEGIES_H