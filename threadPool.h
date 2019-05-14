#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdbool.h>
#include "osqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


typedef struct {
    void (*function)(void *);
    void *param;
} Tasks;

typedef struct thread_pool
{
    OSQueue* queue;
    int maxThreads;
    int shouldFinishTasks;
    bool destroyStarted;
    Tasks *task;

    pthread_mutex_t *mutex;
    pthread_cond_t *conditionMutex;

    pthread_t **threads;

}ThreadPool;


typedef enum {
    invalid = -1, failed_insert_task= -1, malloc_failed = -3, success = 0
} exit_values;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
