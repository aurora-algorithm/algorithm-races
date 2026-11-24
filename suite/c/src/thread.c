#include "thread.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <windows.h>

static unsigned __stdcall trampoline(void *p) {
    Thread *t = p;
    t->fn(t->arg);
    return 0;
}

void thread_start(Thread *t, ThreadFn fn, void *arg) {
    t->fn = fn;
    t->arg = arg;
    uintptr_t h = _beginthreadex(NULL, 0, trampoline, t, 0, NULL);
    if (h == 0) {
        fprintf(stderr, "error: cannot start a thread\n");
        exit(EXIT_FAILURE);
    }
    t->handle = (void *)h;
}

void thread_join(Thread *t) {
    WaitForSingleObject((HANDLE)t->handle, INFINITE);
    CloseHandle((HANDLE)t->handle);
}
#else
#include <pthread.h>

static void *trampoline(void *p) {
    Thread *t = p;
    t->fn(t->arg);
    return NULL;
}

void thread_start(Thread *t, ThreadFn fn, void *arg) {
    t->fn = fn;
    t->arg = arg;
    pthread_t *id = malloc(sizeof *id);
    if (id == NULL || pthread_create(id, NULL, trampoline, t) != 0) {
        fprintf(stderr, "error: cannot start a thread\n");
        exit(EXIT_FAILURE);
    }
    t->handle = id;
}

void thread_join(Thread *t) {
    pthread_t *id = t->handle;
    pthread_join(*id, NULL);
    free(id);
}
#endif
