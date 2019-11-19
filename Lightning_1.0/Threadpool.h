#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <list>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include "Timer.h"
#include "Util.h"

#define GRACEFUL_SHUTDOWN 1
#define IMMEDIATE_SHUTDOWN 2

struct Task{
public:
    int (*_func)(struct epoll_event*);
    struct epoll_event *_arg;
    Task(int (*func)(struct epoll_event*), struct epoll_event* arg);
    ~Task();
};

class Threadpool
{
private:
    pthread_mutex_t _lock;
    pthread_cond_t _cond;
    u_int8_t _thread_count;
    std::list<Task*> _task_list;
    std::vector<pthread_t> _threads_id;
    bool _shutdown;
    bool _graceful;
    int _started;
public:
    Threadpool(int threadnum, bool graceful = true);
    static void* worker(void* arg);
    int add_task(int (*func)(struct epoll_event*), struct epoll_event* arg);
    ~Threadpool();
};


#endif