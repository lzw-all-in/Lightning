#include "Threadpool.h"

Task::Task(int (*func)(struct epoll_event*), struct epoll_event* arg) {
    _func = func;
    _arg = arg;
}

Task::~Task() {}

Threadpool::Threadpool(int threadnum, bool graceful) : _graceful(graceful), _thread_count(0), _shutdown(false), _started(0)
{
    printf("Threadpool init\n");
    if (threadnum <= 0) {
        exit(1);
    }
    _threads_id.reserve(threadnum);

    if (pthread_mutex_init(&_lock, NULL) < 0) {
        error_msg("threadpool mutex init");
        exit(1);
    }

    if (pthread_cond_init(&_cond, NULL) < 0) {
        error_msg("threadpool cond init");
        pthread_mutex_destroy(&_lock);
        exit(1);
    }

    pthread_t id;

    for (int i = _thread_count; i < threadnum; ++i) {
        
        if (pthread_create(&id, NULL, worker, static_cast<void*>(this)) != 0) {
            error_msg("thread creation");
        }
        else {
            _threads_id.push_back(id);
            printf("thread: %08x started\n", (uint32_t)_threads_id[i]);
            ++_thread_count;
            ++_started;
        }
    }

    if (_thread_count == 0) {
        error_msg("thread creation init");
        pthread_mutex_destroy(&_lock);
        pthread_cond_destroy(&_cond);
        exit(1);
    }
}

Threadpool::~Threadpool()
{
    // 发送shutdown状态
    pthread_mutex_lock(&_lock);
    _shutdown = true;
    // 这里需要唤醒所有线程，有可能所有任务都已经完成
    // 但是所有线程都在等待条件变量的情况
    pthread_cond_broadcast(&_cond);
    pthread_mutex_unlock(&_lock);

    for (int i = 0; i < _threads_id.size(); ++i) {
        if (pthread_join(_threads_id[i], NULL) != 0) {
            error_msg("thread join");
        }
        printf("thread %08x exit\n", (uint32_t)_threads_id[i]);
    }
    
    while (!_task_list.empty()) {
        delete _task_list.front();
        _task_list.pop_front();
    }
    
    pthread_mutex_destroy(&_lock);
    pthread_cond_destroy(&_cond);
}

int Threadpool::add_task(int (*func)(struct epoll_event*), struct epoll_event* arg) {
    pthread_mutex_lock(&_lock);

    if (!_shutdown) {
        printf("add task fd = %d\n", static_cast<Data*>(arg->data.ptr)->_fd);
        _task_list.push_back(new Task(func, arg));
        // 释放信号
        pthread_cond_signal(&_cond);    
    }
    pthread_mutex_unlock(&_lock);
    return 0;
}

// 由于要使用pthreadcreate所以只有将其设置为静态函数
// 由于类的静态函数不能访问类的非静态变量，所以需要将该对象作为参数传入
// 个人认为强制类型转换会比较亏时间, bug
void* Threadpool::worker(void* arg) {
    Task* task = nullptr;
    Threadpool *poll = static_cast<Threadpool*>(arg);
    pthread_t id = pthread_self();
    while (1) {
        pthread_mutex_lock(&poll->_lock);
        while (poll->_task_list.size() == 0 && !(poll->_shutdown)) {
            pthread_cond_wait(&poll->_cond, &poll->_lock);    
        }

        if (poll->_shutdown) {
            if (!poll->_graceful || (poll->_graceful && poll->_task_list.empty()))
                break;
        }
        task = poll->_task_list.front();
        poll->_task_list.pop_front();
        printf("fd %d from link list out\n", static_cast<Data*>(task->_arg->data.ptr)->_fd);
        pthread_mutex_unlock(&poll->_lock);
        
        (task->_func(task->_arg));
        delete task;
        task = nullptr;
    }
    pthread_mutex_unlock(&poll->_lock);
    pthread_exit(NULL);
    --poll->_started;
    return nullptr;
}

