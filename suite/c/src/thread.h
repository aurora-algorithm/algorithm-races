/* A minimal thread start/join: Win32 threads on Windows, POSIX threads elsewhere. C11 <threads.h>
 * is not shipped by macOS, so the kernels use this instead of it. */
#ifndef KERNELS_THREAD_H
#define KERNELS_THREAD_H

#include <stdbool.h>

typedef void (*ThreadFn)(void *arg);

typedef struct {
    ThreadFn fn;
    void *arg;
    void *handle;
} Thread;

/* Exits the process with a message when the thread cannot be started. */
void thread_start(Thread *t, ThreadFn fn, void *arg);
void thread_join(Thread *t);

#endif
