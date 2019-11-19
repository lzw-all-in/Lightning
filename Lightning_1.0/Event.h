#ifndef _EVENT_H_
#define _EVENT_H_

#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <queue>
#include "Util.h"
#include "Timer.h"
#include "Threadpool.h"

class Event
{
private:
    static int _epfd;
    int _maxEvents;
    int _clientfd;
    char _ip_addr[20];
    std::string& _root;
    Data *listen_data;
    Data *data_ptr;
    // 用于epoll_wait的返回事件
    struct epoll_event *_events;
    // 用于epoll加入单个事件
    // struct epoll_event _event;
    struct sockaddr_in _clientaddr;
    // 小根堆管理定时器
    static std::priority_queue<Timer*, std::vector<Timer*>, timerCmp> timer_queue;
    // 线程池
    Threadpool _threadpool;
    socklen_t _clientlen;
    int _epoll_create(int flags);
    int _epoll_wait(int timeout);
    void _handle_events(int num);
    void _handle_out_of_time();
    static int _finish_events(struct epoll_event* event);
    static void run_task(Event*);

public:
    static pthread_mutex_t tq_lock;
    static void addEvent(Data* ptr, uint32_t flags);
    static void modEvent(Data* ptr, uint32_t flags);
    static void deleteEvent(Data* ptr, uint32_t flags);
    void eventLoop();

    Event(std::string& root, int listenfd, int threadnum, int maxEvents = 1024);
    ~Event();
};

#endif