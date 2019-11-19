#ifndef _LIGHTNING_H_
#define _LIGHTNING_H_
#include <string>
#include <iostream>
// #include <memory>
#include "Util.h"
#include "Event.h"

class Lightning
{
private:
    int _threadnum;
    int _listenfd;
    Event* _myEvent;
    Configure _conf;
public:
    void serve_start();
    Lightning(std::string conf_path);
    ~Lightning();
};

#endif