#include "threadPool.h"

#define ERROR_TYPE "Error in system call\n"
#define ERROR_SIZE 255

void printError(){
    write(2, ERROR_TYPE, ERROR_SIZE);
}

void * startThreads(void *threads){
    ThreadPool *pool = threads;

    while (true){

        //entering critical section - need to lock mutex
        if (pthread_mutex_lock(pool->mutex)){
            printError();
        }

        //as long as there are no tasks - wait
        while (osIsQueueEmpty(pool->queue)){
            //destroy started - we will not add more tasks so no need to wait
            if (pool->destroyStarted){
                break;
            } else {
                //wait for tasks to be added
                if (pthread_cond_wait(pool->conditionMutex, pool->mutex) != 0){
                    printError();
                }
            }
        }

        if (pool->destroyStarted){
            //if we need to finish the tasks entered, break only if all tasks are finished
                if ((pool->shouldFinishTasks) && (osIsQueueEmpty(pool->queue))){
                    break;
                } else if (pool->shouldFinishTasks == 0){
                    break;
                }
            }

        //get a task to preform
        Tasks *task = osDequeue(pool->queue);

        //end of critical section
        if(pthread_mutex_unlock(pool->mutex) != 0){
            printError();
        }

        //preform the task
        (*(task->function))(task->param);
    }

    //if it wasn't unlocked in the 'while'
    if(pthread_mutex_unlock(pool->mutex) != 0){
        printError();
    }

    pthread_exit(NULL);
}

ThreadPool* tpCreate(int numOfThreads){

    //Initialize//
    ThreadPool* threadPool = malloc(sizeof(ThreadPool));
    if (threadPool == NULL){
        //malloc failed
        printError();
        return NULL;
    }

    threadPool->conditionMutex = malloc(sizeof(pthread_cond_t));
    if (threadPool->conditionMutex == NULL){
        //malloc failed
        printError();
        return NULL;
    }

    threadPool->mutex = malloc(sizeof(pthread_mutex_t));
    if (threadPool->mutex == NULL){
        //malloc failed
        printError();
        return NULL;
    }

    threadPool->task = malloc(sizeof(Tasks));
    if (threadPool->task == NULL){
        //malloc failed
        printError();
        return NULL;
    }

    threadPool->threads = malloc(sizeof(pthread_t)*numOfThreads);
    if (threadPool->threads == NULL){
        //malloc failed
        printError();
        return NULL;
    }

    threadPool->queue = osCreateQueue();
    threadPool->destroyStarted = false;
    threadPool->maxThreads = numOfThreads;
    threadPool->shouldFinishTasks = -1;
    pthread_cond_init(threadPool->conditionMutex, NULL);
    pthread_mutex_init(threadPool->mutex, NULL);
    //end of Initialize//

    //creating threads
    int i = 0;
    for (; i < numOfThreads; i++){
        threadPool->threads[i] = malloc(sizeof(pthread_t));
        if (threadPool->threads[i] == NULL) {
            //malloc failed
            printError();
            return NULL;
        }
        pthread_create(threadPool->threads[i], NULL, startThreads, threadPool);
    }

    return threadPool;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    if (threadPool == NULL){
        return failed_insert_task;
    }

    if(threadPool->destroyStarted){
        return failed_insert_task;
    }

    if (computeFunc == NULL){
        return failed_insert_task;
    }

    //Initialize task//
    threadPool->task->function = computeFunc;
    threadPool->task->param = param;

    //entering critical section
    if (pthread_mutex_lock(threadPool->mutex) != 0){
        printError();
        return failed_insert_task;
    }

    //enter task to queue
    osEnqueue(threadPool->queue, threadPool->task);

    //notify new task available
    if (pthread_cond_signal(threadPool->conditionMutex) != 0){
        printError();
        return failed_insert_task;
    }

    //end of critical section
    if (pthread_mutex_unlock(threadPool->mutex) != 0){
        printError();
        return failed_insert_task;
    }

    return success;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){

    //tpDestroy is allowed to be called only once
    if(threadPool->destroyStarted){
        return;
    }

    //entering critical section
    if (pthread_mutex_lock(threadPool->mutex) != 0){
        printError();
    }

    threadPool->destroyStarted = true;
    threadPool->shouldFinishTasks = shouldWaitForTasks;

    //wake up all thrreads
    if (pthread_cond_broadcast(threadPool->conditionMutex) != 0){
        printError();
    }

    //end of critical section
    if (pthread_mutex_unlock(threadPool->mutex) != 0){
        printError();
    }

    int i = 0;
    for (; i < threadPool->maxThreads; i++){
        if (pthread_join(*(threadPool->threads[i]), NULL) != 0){
            printError();
        }
    }

    //free resources
    int j = 0;
    for(; j < threadPool->maxThreads; j++){
        if(threadPool->threads[j] != NULL){
            free(threadPool->threads[j]);
        }
    }

    free(threadPool->threads);
    osDestroyQueue(threadPool->queue);
    pthread_mutex_destroy(threadPool->mutex);
    pthread_cond_destroy(threadPool->conditionMutex);
    free(threadPool->mutex);
    free(threadPool->conditionMutex);
    free(threadPool->task);
    free(threadPool);
}