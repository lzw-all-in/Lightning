#ifndef _TIMER_H_
#define _TIMER_H_
#include "Http.h"
// #include "Event.h"
#include <sys/time.h>
#include <stdio.h>
#define TIMEOUT 2000

class Timer;

class Data {
private:
    // _myHttp设置为指针的一个原因就是因为监听套接字不需要这个指针可以设置为空
    Http* _myHttp;
    Timer* _timer;

public:
    
    int _fd;
    void do_error(char *errnum, char *reason);
    std::string& getMethod();
    std::unordered_map<std::string, std::string>& getKeyValue();
    std::string& getPath();
    std::string& getVersion();
    int parseHttp(char* buff, size_t len);
    void addTimer(Timer* timer);
    void separateTimer();
    void _printf();
    Data(int fd, std::string& root);
    Data(int fd);
    ~Data();
};

class Timer
{
private:
    bool _is_delete;
    size_t _expired_time;
    Data* _data;
public:
    bool is_delete();
    bool is_valid();
    size_t expired_time() const;
    void separateData();
    bool has_data();
    Data* get_data();
    Timer(Data* data, size_t timeout = TIMEOUT);
    ~Timer();
};

struct timerCmp
{
    bool operator()(const Timer* a, const Timer* b) const;
};

#endif