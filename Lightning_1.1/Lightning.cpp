#include "Lightning.h"

Lightning::Lightning(std::string conf_path): 
    _conf()
{
    if (_conf.read_conf_file(conf_path) < 0) {
        exit(1);
    }

    if ((_listenfd = get_listenfd(_conf.get_port())) < 0) {
        exit(1);
    }

    if (make_socket_non_blocking(_listenfd) < 0) {
        exit(1);
    }

    if (handle_SIGPIPE() < 0) {
        exit(1);
    }
    _myEvent = std::unique_ptr<Event>(new Event(_conf.get_root_path(), _listenfd, _conf.get_threadnum()));
}

Lightning::~Lightning()
{
    
}

void Lightning::serve_start() {
    _myEvent->eventLoop();
}