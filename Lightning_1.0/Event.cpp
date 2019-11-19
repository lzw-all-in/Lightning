#include "Event.h"

pthread_mutex_t Event::tq_lock = PTHREAD_MUTEX_INITIALIZER;

Event::Event(std::string& root, int listenfd, int threadnum, int maxEvents) 
:_root(root), _maxEvents(maxEvents), _threadpool(threadnum)
{
    // 创建epoll fd
    _epoll_create(0);
    // 初始化需要的数据
    data_ptr = nullptr;
    listen_data = new Data(listenfd);
    bzero(&_clientaddr, sizeof(_clientaddr));
    _clientlen = sizeof(_clientaddr);
    _events = (epoll_event*)malloc(sizeof(epoll_event) * _maxEvents);
}

Event::~Event()
{
    if (_events != nullptr) {
        free(_events);
    }
    if (listen_data != nullptr) {
        delete listen_data;
    }
}

// 由于被调用的任务函数需要是静态的，所以这些成员变量也需要定义为静态
int Event::_epfd = -1;
std::priority_queue<Timer*, std::vector<Timer*>, timerCmp> Event::timer_queue;

int Event::_epoll_create(int flags) {
    if ((_epfd = epoll_create1(flags)) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        error_msg("epoll create\n");
        return -1;
    }
    return 0;
}

// 这里注意一下，是从新创建一个
void Event::addEvent(Data* ptr, uint32_t flags) {
    epoll_event _event;
    _event.data.ptr = static_cast<void*>(ptr);
    _event.events = flags;
    // epoll_ctl 正常返回0 否则返回-1
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, ptr->_fd, &_event) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        error_msg("add Event\n");
    }
}

void Event::modEvent(Data* ptr, uint32_t flags) {
    epoll_event _event;
    _event.data.ptr = static_cast<void*>(ptr);
    _event.events = flags;
    if (epoll_ctl(_epfd, EPOLL_CTL_MOD, ptr->_fd, &_event) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        error_msg("mod Event\n");
    }
}

// 目前还是有线程安全的问题
void Event::deleteEvent(Data* ptr, uint32_t flags) {
    epoll_event _event;
    _event.data.ptr = static_cast<void*>(ptr);
    _event.events = flags;
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, ptr->_fd, &_event) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        error_msg("mod Event\n");
    }
}

int Event::_epoll_wait(int timeout) {
    int n = epoll_wait(_epfd, _events, _maxEvents, timeout);
    if (n < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        error_msg("epoll wait\n");
    }
    return n;
}

// 如果使用线程池到时候直接使用函数指针指过来就好了
int Event::_finish_events(struct epoll_event* event) {
    // 在进入开头进行timer的剥离可以修复bug
    Data *data = static_cast<Data*>(event->data.ptr);
    pthread_t id = pthread_self();
    printf("thread id %08x start a task on fd %d\n", id, data->_fd);
    int fd = data->_fd;
    uint32_t events = event->events;
    int n = -1;
    char buffer[BUFF_SIZE];
    int ret = 0;
    
    while ((n = read(fd, buffer, sizeof(buffer))) >= 0) {

         if (n <= 0) {
            if (n == 0) {
                // 连接关闭
                printf("Connection has been closed\n");
                // pthread_mutex_lock(&tq_lock);
                deleteEvent(data, events);
                // pthread_mutex_unlock(&tq_lock);
                delete data;
                free(event);
                return -1;
            }
            else if (errno == EINTR) {
                // 再试一次
                continue;
            }
            else if (errno == EAGAIN) {
                // 当前已经无数据可读，应该加入epoll wait等待下次数据到来
                break;
            }
            else {
                error_msg("socket read\n");
                // pthread_mutex_lock(&tq_lock);
                deleteEvent(data, events);
                // pthread_mutex_unlock(&tq_lock);
                delete data;
                free(event);
                return -1;
            }
        }
        else {
            // 如果当前读到了数据，则开始进行解析
            ret = data->parseHttp(buffer, n);

            // 表示还未解析完毕
            if (ret == HTTP_PARSE_AGAIN) {
                // 表示虽然数据处理完毕，但是还有数据没有读入，需要再次读入并解析
                printf("read data again\n");
                bzero(buffer, n);
                continue;
            }
            else if (ret == HTTP_LONG_CONNECTION) {
                // 代表发送成功并且建立长链接
                break;
            }
            else {
                // 表示消息发送成功但是是短连接或者出错状态
                printf("closed \n");
                // pthread_mutex_lock(&tq_lock);
                deleteEvent(data, events);
                // pthread_mutex_unlock(&tq_lock);
                delete data;
                free(event);
                return -1;
            }
        }
    }
    // printf("make long connection\n");
    Timer* timer = new Timer(data);
    pthread_mutex_lock(&tq_lock);
    timer_queue.push(timer);
    data->addTimer(timer);
    pthread_mutex_unlock(&tq_lock);
    // mod和add对epoll_ctl是没有太多影响，但是del就有很大的影响了
    printf("modEven fd %d\n", data->_fd);
    modEvent(data, EPOLLIN | EPOLLET | EPOLLONESHOT);
    free(event);
    return 0;
}

void Event::_handle_events(int num) {

    for (int i = 0; i < num; ++i) {
        
        data_ptr = static_cast<Data*>(_events[i].data.ptr);
        if (data_ptr->_fd == listen_data->_fd) {
            if ((_clientfd = accept(listen_data->_fd, (sockaddr*)&_clientaddr, &_clientlen)) < 0) {
                error_msg("accept new client\n");
                continue;
            }

            if (make_socket_non_blocking(_clientfd) < 0) {
                continue;
            }

            Data *client_data = new Data(_clientfd, _root);
            Timer* timer = new Timer(client_data);

            pthread_mutex_lock(&tq_lock);
            timer_queue.push(timer);
            client_data->addTimer(timer);
            // 设置oneshot防止在多线程+线程池模式下，同时有2个线程服务该客户
            // 这样每次线程服务客户时不需要先将其删除掉, 线程池情况下需要进行设定
            // printf("addEvent fd %d\n", _clientfd);
            addEvent(client_data, EPOLLIN | EPOLLET | EPOLLONESHOT);
            pthread_mutex_unlock(&tq_lock);
            // 输出调试信息
            // inet_ntop(AF_INET, &_clientaddr, _ip_addr, sizeof(_ip_addr));
            // printf("a new client come in port is %d ip is %s fd is %d\n", 
            //         _clientaddr.sin_port,
            //         _ip_addr,
            //         _clientfd);
        }
        else {
            // 先分离定时器
            pthread_mutex_lock(&tq_lock);
            data_ptr->separateTimer();
            pthread_mutex_unlock(&tq_lock);
            // 排除错误事件
            if ((_events[i].events & EPOLLERR) || 
                (_events[i].events & EPOLLHUP) || 
                !(_events[i].events & EPOLLIN)) {
                // EPOLLHUP代表对方已经关闭连接
                error_msg("EPOLLERR or EPOLLHUP at another side\n");
                delete data_ptr;
                continue;
            }

            // 这里需要拷贝的原因就是当我把任务加入线程池的时候，参数传递的是指针
            // 而我主线程又使用_events[]数组去进行监听，而这个监听又回重新更新数组里面的值，
            // 那么线程池里的指针所指向的值就变了
            // 必须动态分配，不然的话函数一结束，t的内存就已经释放了
            struct epoll_event* t = (struct epoll_event*)malloc(sizeof(struct epoll_event));
            t->data.ptr = data_ptr;
            _threadpool.add_task(_finish_events, t);
            // printf("---------------------------\n");
        }
    }
}

void Event::_handle_out_of_time() {
    // 目前出错的问题在于删除了同一个fd两次
    pthread_mutex_lock(&tq_lock);
    while (!timer_queue.empty()&& !timer_queue.top()->is_valid()) {
        printf("delete a timer\n");
        Timer* timer = timer_queue.top();
        Data* data = timer->get_data();
        if (data != nullptr) {
            // printf("delete a out of time event and the fd is %d\n", data->_fd);
            // 实际上如果是EPOLL_CTL_DEL模式则flags字段会被忽略
            // 这里是处理超时的情况
            // printf("deleteEvent fd %d", data->_fd);
            deleteEvent(data, EPOLLIN | EPOLLET | EPOLLONESHOT);
            delete data;
        }
        delete timer;
        timer_queue.pop();
    }
    pthread_mutex_unlock(&tq_lock);
}

void Event::eventLoop() {

    // 对于监听套接字我希望它默认水平触发
    addEvent(listen_data, EPOLLIN);
    int n;
    while (1) {
        n = _epoll_wait(-1);
        // printf("There are %d Events coming\n", n);
        if (n > 0) {
            _handle_events(n);
        }
        _handle_out_of_time();
    }
}