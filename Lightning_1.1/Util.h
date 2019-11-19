#ifndef _UTIL_H_
#define _UTIL_H_
#include <string>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

// 监听队列的可以监听的数量
#define LISTEN_QUE 1024

ssize_t writen(int fd, char *msg, size_t n);
ssize_t readn(int fd, char *msg, size_t n);

struct Configure {
private:
    std::string root_path;
    int threadnum;
    int port;

public:
    int read_conf_file(std::string& file_path);
    int get_threadnum() {return threadnum;};
    int get_port() {return port;};
    std::string& get_root_path() {return root_path;};
};

#define error_msg(msg) fprintf(stderr, "error %s:%d  info : %s\n", __FILE__, __LINE__, msg);
int get_listenfd(int port);
int make_socket_non_blocking(int fd);
int handle_SIGPIPE();
std::string read_image(const std::string& image_path);
int send_image(int & fd, std::string& image);


#endif