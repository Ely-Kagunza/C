#include "sys_io_strategies.h"

// ============== STRATEGY 1: STANDARD I/O (Direct system calls) ==============

typedef struct {
    int total_opens;
    int total_reads;
    int total_writes;
    int total_closes;
    size_t total_bytes_read;
    size_t total_bytes_written;
} StandardIOData;

int standard_open(void *self, const char *path, int flags)
{
    IOStrategy *strategy = (IOStrategy *)self;
    StandardIOData *data = (StandardIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int fd = _open(path, flags, 0666);
    #else
        int fd = open(path, flags, 0666);
    #endif

    if (fd >= 0)
    {
        data->total_opens++;
        printf("[STANDARD] Opened: %s (fd=%d)\n", path, fd);
        if (strategy->callbacks.on_open)
            strategy->callbacks.on_open(fd, path, strategy->callbacks.user_data);
    }
    else
    {
        printf("[STANDARD] Failed to open: %s\n", path);
    }

    return fd;
}

int standard_read(void *self, int fd, void *buf, size_t count)
{
    IOStrategy *strategy = (IOStrategy *)self;
    StandardIOData *data = (StandardIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int bytes = _read(fd, buf, count);
    #else
        int bytes = read(fd, buf, count);
    #endif

    if (bytes >= 0)
    {
        data->total_reads++;
        data->total_bytes_read += bytes;
        printf("[STANDARD] Read: %d bytes from fd=%d\n", bytes, fd);
        if (strategy->callbacks.on_read)
            strategy->callbacks.on_read(fd, bytes, strategy->callbacks.user_data);
    }
    else
    {
        printf("[STANDARD] Read error on fd=%d\n", fd);
    }

    return bytes;
}

int standard_write(void *self, int fd, const void *buf, size_t count)
{
    IOStrategy *strategy = (IOStrategy *)self;
    StandardIOData *data = (StandardIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int bytes = _write(fd, buf, count);
    #else
        int bytes = write(fd, buf, count);
    #endif

    if (bytes >= 0)
    {
        data->total_writes++;
        data->total_bytes_written += bytes;
        printf("[STANDARD] Wrote: %d bytes to fd=%d\n", bytes, fd);
        if (strategy->callbacks.on_write)
            strategy->callbacks.on_write(fd, bytes, strategy->callbacks.user_data);
    }
    else
    {
        printf("[STANDARD] Write error on fd=%d\n", fd);
    }

    return bytes;
}

int standard_close(void *self, int fd)
{
    IOStrategy *strategy = (IOStrategy *)self;
    StandardIOData *data = (StandardIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int result = _close(fd);
    #else
        int result = close(fd);
    #endif

    if (result == 0)
    {
        data->total_closes++;
        printf("[STANDARD] Closed fd=%d\n", fd);
        if (strategy->callbacks.on_close)
            strategy->callbacks.on_close(fd, strategy->callbacks.user_data);
    }

    return result;
}

int standard_seek(void *self, int fd, long offset)
{
    #ifdef _WIN32
        return _lseek(fd, offset, SEEK_SET);
    #else
        return lseek(fd, offset, SEEK_SET);
    #endif
}

void standard_display_stats(void *self)
{
    IOStrategy *strategy = (IOStrategy *)self;
    StandardIOData *data = (StandardIOData *)strategy->strategy_data;

    printf("\n=== Standard I/O Strategy Statistics ===\n");
    printf("  Opens:             %d\n", data->total_opens);
    printf("  Reads:             %d\n", data->total_reads);
    printf("  Writes:            %d\n", data->total_writes);
    printf("  Closes:            %d\n", data->total_closes);
    printf("  Total bytes read:  %zu\n", data->total_bytes_read);
    printf("  Total bytes written: %zu\n\n", data->total_bytes_written);
}

void standard_free(void *self)
{
    IOStrategy *strategy = (IOStrategy *)self;
    free(strategy->strategy_data);
    free(strategy);
}

IOStrategy* io_standard_create(IOCallbacks callbacks)
{
    IOStrategy *strategy = malloc(sizeof(IOStrategy));
    StandardIOData *data = malloc(sizeof(StandardIOData));

    memset(data, 0, sizeof(StandardIOData));

    strategy->open = standard_open;
    strategy->read = standard_read;
    strategy->write = standard_write;
    strategy->close = standard_close;
    strategy->seek = standard_seek;
    strategy->display_stats = standard_display_stats;
    strategy->free_strategy = standard_free;
    strategy->name = "Standard";
    strategy->callbacks = callbacks;
    strategy->strategy_data = data;

    return strategy;
}

// ============== STRATEGY 2: BUFFERED I/O (Internal Buffering) ==============

typedef struct {
    char *buffer;
    int buffer_size;
    int buffer_pos;
    int total_flushes;
    int total_reads;
    int total_writes;
    size_t total_bytes_read;
    size_t total_bytes_written;
} BufferedIOData;

int buffered_open(void *self, const char *path, int flags)
{
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int fd = _open(path, flags, 0666);
    #else
        int fd = open(path, flags, 0666);
    #endif

    if (fd >= 0)
    {
        data->buffer = malloc(4096);
        data->buffer_size = 4096;
        data->buffer_pos = 0;
        printf("[BUFFERED] Opened: %s (fd=%d, buffer=4KB)\n", path, fd);
        if (strategy->callbacks.on_open)
            strategy->callbacks.on_open(fd, path, strategy->callbacks.user_data);
    }

    return fd;
}

int buffered_read(void *self, int fd, void *buf, size_t count)
{
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int bytes = _read(fd, buf, count);
    #else
        int bytes = read(fd, buf, count);
    #endif

    if (bytes >= 0)
    {
        data->total_reads++;
        data->total_bytes_read += bytes;
        printf("[BUFFERED] Read %d bytes via buffering (fd=%d)\n", bytes, fd);
        if (strategy->callbacks.on_read)
            strategy->callbacks.on_read(fd, bytes, strategy->callbacks.user_data);
    }

    return bytes;
}

int buffered_write(void *self, int fd, const void *buf, size_t count)
{
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;

    // Copy to buffer
    if (data->buffer_pos + count > data->buffer_size)
    {
        // Flush buffer
        #ifdef _WIN32
            _write(fd, data->buffer, data->buffer_pos);
        #else
            write(fd, data->buffer, data->buffer_pos);
        #endif
        data->total_flushes++;
        data->buffer_pos = 0;
    }

    memcpy(data->buffer + data->buffer_pos, buf, count);
    data->buffer_pos += count;
    data->total_writes++;
    data->total_bytes_written += count;

    printf("[BUFFERED] Buffered %zu bytes (fd=%d, buffer usage: %d/%d)\n",
           count, fd, data->buffer_pos, data->buffer_size);

    if (strategy->callbacks.on_write)
        strategy->callbacks.on_write(fd, count, strategy->callbacks.user_data);

    return count;
}

int buffered_close(void *self, int fd)
{
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;

    // Flush remaining data
    if (data->buffer_pos > 0)
    {
        #ifdef _WIN32
            _write(fd, data->buffer, data->buffer_pos);
        #else
            write(fd, data->buffer, data->buffer_pos);
        #endif
        data->total_flushes++;
    }

    free(data->buffer);

    #ifdef _WIN32
        int result = _close(fd);
    #else
        int result = close(fd);
    #endif

    printf("[BUFFERED] Closed fd=%d (flushed %d bytes)\n", fd, data->total_flushes);

    if (strategy->callbacks.on_close)
        strategy->callbacks.on_close(fd, strategy->callbacks.user_data);

    return result;
}

int buffered_seek(void *self, int fd, long offset)
{
    // Flush before seeking
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;

    if (data->buffer_pos > 0)
    {
        #ifdef _WIN32
            _write(fd, data->buffer, data->buffer_pos);
        #else
            write(fd, data->buffer, data->buffer_pos);
        #endif
        data->buffer_pos = 0;
    }

    #ifdef _WIN32
        return _lseek(fd, offset, SEEK_SET);
    #else
        return lseek(fd, offset, SEEK_SET);
    #endif
}

void buffered_display_stats(void *self)
{
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;
    
    printf("\n=== Buffered I/O Strategy Statistics ===\n");
    printf("  Logical reads:     %d\n", data->total_reads);
    printf("  Logical writes:    %d\n", data->total_writes);
    printf("  Buffer flushes:    %d\n", data->total_flushes);
    printf("  Total bytes read:  %zu\n", data->total_bytes_read);
    printf("  Total bytes written: %zu\n", data->total_bytes_written);
    printf("  Efficiency:        %.1f%% (flushes vs writes)\n\n",
           (float)data->total_flushes / data->total_writes * 100);
}

void buffered_free(void *self)
{
    IOStrategy *strategy = (IOStrategy *)self;
    BufferedIOData *data = (BufferedIOData *)strategy->strategy_data;
    if (data->buffer)
        free(data->buffer);
    free(strategy->strategy_data);
    free(strategy);
}

IOStrategy* io_buffered_create(IOCallbacks callbacks)
{
    IOStrategy *strategy = malloc(sizeof(IOStrategy));
    BufferedIOData *data = malloc(sizeof(BufferedIOData));

    memset(data, 0, sizeof(BufferedIOData));

    strategy->open = buffered_open;
    strategy->read = buffered_read;
    strategy->write = buffered_write;
    strategy->close = buffered_close;
    strategy->seek = buffered_seek;
    strategy->display_stats = buffered_display_stats;
    strategy->free_strategy = buffered_free;
    strategy->name = "Buffered";
    strategy->callbacks = callbacks;
    strategy->strategy_data = data;

    return strategy;
}

// ============== STRATEGY 3: MEMORY-MAPPED I/O (Advanced - Linux only) ==============

typedef struct {
    int file_opens;
    int file_closes;
    size_t total_bytes;
} MmapIOData;

int mmap_open(void *self, const char *path, int flags)
{
    IOStrategy *strategy = (IOStrategy *)self;
    MmapIOData *data = (MmapIOData *)strategy->strategy_data;

    #ifdef _WIN32
        int fd = _open(path, flags, 0666);
    #else
        int fd = open(path, flags, 0666);
    #endif

    if (fd >= 0)
    {
        data->file_opens++;
        printf("[MMAP] Opened with memory mapping: %s (fd=%d)\n", path, fd);
        printf("[MMAP] Note: Full mmap implimentation requires platform-specific code\n");
        if (strategy->callbacks.on_open)
            strategy->callbacks.on_open(fd, path, strategy->callbacks.user_data);
    }

    return fd;
}

int mmap_read(void *self, int fd, void *buf, size_t count)
{
    // Simplified: just do regular read from demo
    // Real implimentation would use mmap to map file into memory
    printf("[MMAP] Would map %zu bytes into virtual memory\n", count);

    #ifdef _WIN32
        int bytes = _read(fd, buf, count);
    #else
        int bytes = read(fd, buf, count);
    #endif

    IOStrategy *strategy = (IOStrategy *)self;
    if (bytes >= 0 && strategy->callbacks.on_read)
        strategy->callbacks.on_read(fd, bytes, strategy->callbacks.user_data);

    return bytes;
}

int mmap_write(void *self, int fd, const void *buf, size_t count)
{
    printf("[MMAP] Would write %zu bytes through memory-mapped region\n", count);

    #ifdef _WIN32
        int bytes = _write(fd, buf, count);
    #else
        int bytes = write(fd, buf, count);
    #endif

    IOStrategy *strategy = (IOStrategy *)self;
    if (bytes >= 0 && strategy->callbacks.on_write)
        strategy->callbacks.on_write(fd, bytes, strategy->callbacks.user_data);

    return bytes;
}

int mmap_close(void *self, int fd)
{
    IOStrategy *strategy = (IOStrategy *)self;
    MmapIOData *data = (MmapIOData *)strategy->strategy_data;

    data->file_closes++;
    printf("[MMAP] Closed fd=%d (unmapped memory region)\n", fd);

    #ifdef _WIN32
        return _close(fd);
    #else
        return close(fd);
    #endif
}

int mmap_seek(void *self, int fd, long offset)
{
    printf("[MMAP] Seeking to offset %ld in mapped region\n", offset);
    #ifdef _WIN32
        return _lseek(fd, offset, SEEK_SET);
    #else
        return lseek(fd, offset, SEEK_SET);
    #endif
}

void mmap_display_stats(void *self)
{
    IOStrategy *strategy = (IOStrategy *)self;
    MmapIOData *data = (MmapIOData *)strategy->strategy_data;

    printf("\n=== Memory-Mapped I/O Strategy Statistics ===\n");
    printf("  Files opened:     %d\n", data->file_opens);
    printf("  Files closed:     %d\n", data->file_closes);
    printf("  Efficiency:       Excellent for sequential access\n");
    printf("  Use case:         Large files, database pages\n\n");
}

void mmap_free(void *self)
{
    IOStrategy *strategy = (IOStrategy *)self;
    free(strategy->strategy_data);
    free(strategy);
}

IOStrategy* io_mmap_create(IOCallbacks callbacks)
{
    IOStrategy *strategy = malloc(sizeof(IOStrategy));
    MmapIOData *data = malloc(sizeof(MmapIOData));

    memset(data, 0, sizeof(MmapIOData));

    strategy->open = mmap_open;
    strategy->read = mmap_read;
    strategy->write = mmap_write;
    strategy->close = mmap_close;
    strategy->seek = mmap_seek;
    strategy->display_stats = mmap_display_stats;
    strategy->free_strategy = mmap_free;
    strategy->name = "Memory-Mapped";
    strategy->callbacks = callbacks;
    strategy->strategy_data = data;

    return strategy;
}

// ============== GENERIC HELPER FUNCTIONS ==============

int io_run(IOStrategy *strategy, const char *filepath, int flags)
{
    if (strategy == NULL)
    {
        printf("Error: Strategy is NULL\n");
        return -1;
    }

    printf("\n═══════════════════════════════════════════════════════\n");
    printf("Using [%s] I/O Strategy\n", strategy->name);
    printf("═══════════════════════════════════════════════════════\n\n");

    return strategy->open(strategy, filepath, flags);
}

void io_display_all_stats(IOStrategy *strategies[], int count)
{
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("I/O Strategy Comparison\n");
    printf("═══════════════════════════════════════════════════════\n");

    for (int i = 0; i < count; i++)
    {
        if (strategies[i])
            strategies[i]->display_stats(strategies[i]);
    }
}