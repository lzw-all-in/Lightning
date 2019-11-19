#include "Timer.h"

Data::Data(int fd, std::string& root) : _fd(fd), _timer(nullptr)
{
    _myHttp = new Http(root);
}

// 对于监听套接字而言不需要_myHttp这个对象实体,所以自己设置为指针
Data::Data(int fd) :_myHttp(nullptr), _fd(fd), _timer(nullptr)
{
}

Data::~Data()
{
    // 确保Data和Timer分开了
    // 可能存在超时的情况，然后Timer并没有显示和Data分开
    separateTimer();
    if (_myHttp != nullptr) {
        delete _myHttp;
    }
    close(_fd);
}

void Data::addTimer(Timer* timer) {
    _timer = timer;
}

void Data::separateTimer() {
    if (_timer != nullptr) {
        _timer->separateData();
        _timer = nullptr;
    }
}

void Data::_printf() {
    printf("I'm alive \n");
}

int Data::parseHttp(char* buff, size_t len) {
    return _myHttp->parseHttp(_fd, buff, len);
}

void Data::do_error(char *errnum, char *reason) {
    return _myHttp->do_error(_fd, errnum, reason);
}

std::string& Data::getMethod() {
    return _myHttp->getMethod();
}

std::unordered_map<std::string, std::string>& Data::getKeyValue() {
    return _myHttp->getKeyValue();
}

std::string& Data::getPath() {
    return _myHttp->getPath();
}

std::string& Data::getVersion() {
    return _myHttp->getVersion();
}


Timer::Timer(Data* data, size_t timeout): _is_delete(false), _data(data)
{
    struct timeval t;
    gettimeofday(&t, nullptr);
    _expired_time = t.tv_sec * 1000 + t.tv_usec / 1000 + timeout;
}

Timer::~Timer()
{
}

bool Timer::is_valid() {
    if (_is_delete) return false;
    else {
        struct timeval t;
        gettimeofday(&t, nullptr);
        size_t num = t.tv_sec * 1000 + t.tv_usec / 1000;
        if (_expired_time < num) {
            return false;
        }
        else return true;
    }
}

bool Timer::has_data() {
    return _data != nullptr;
}

Data* Timer::get_data() {
    return _data;
}

void Timer::separateData() {
    _data = nullptr;
    _is_delete = true;
}

bool Timer::is_delete() {
    return _is_delete;
}

size_t Timer::expired_time() const {
    return _expired_time;
}

bool timerCmp::operator()(const Timer* a, const Timer* b) const {
    return a->expired_time() > b->expired_time();
}

