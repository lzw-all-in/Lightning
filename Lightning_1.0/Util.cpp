#include "Util.h"

int Configure::read_conf_file(std::string& filename) {
    std::fstream in(filename, std::ios::in);
    char buff[256];
    if (in.is_open()) {
        while (!in.eof()) {
            in >> buff;

            if (strcmp(buff, "root") == 0) {
                if (!in.eof()) {
                    in >> root_path;
                }
                else {
                    error_msg("configure information\n");
                }
            }

            if (strcmp(buff, "port") == 0) {
                if (!in.eof()) {
                    in >> port;
                }
                else {
                    error_msg("configure information\n");
                }
            }

            if (strcmp(buff, "threadnum") == 0) {
                if (!in.eof()) {
                    in >> threadnum;
                }
                else {
                    error_msg("configure information\n");
                }
            }
        }
    }
    else {
        // 读取文件失败
        in.close();
        error_msg("open configure file\n");
        return -1;
    }
    in.close();
    return 0;
}

int handle_SIGPIPE() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    // 忽略该信号
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL)) {
        error_msg("install sigal handler for SIGPIPE failed");
        return -1;
    }
    return 0;
}

int make_socket_non_blocking(int fd) {
    int flags, s;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        error_msg("fcntl\n");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        error_msg("fcntl\n");
        return -1;
    }

    return 0;
}

int get_listenfd(int port) {
    int listenfd;
    int on = 1;
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_msg("socket\n");
        return -1;
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0) {
        error_msg("setsockopt\n");
        return -1;
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    if (bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        error_msg("bind\n");
        return -1;
    }

    if (listen(listenfd, LISTEN_QUE) < 0) {
        error_msg("listen\n");
        return -1;
    }
    return listenfd;
}

ssize_t writen(int fd, char *msg, size_t n) {
    size_t nleft = n;
    ssize_t nwrite = 0;
    ssize_t writeSum = 0;
    const char *ptr = msg;
    while (nleft > 0) {

        if ((nwrite = write(fd, ptr, nleft)) < 0) {
            // 我这里只是简单的让它忙等，等到有空间可以写入数据
            // 不过感觉这样的方式比较占用CPU
            // 这里的一种改进是将读写数据的位置记录下来，然后将该任务
            // 重新放入线程池末尾，该线程从线程池头部再取出一个任务
            if (errno == EINTR || errno == EAGAIN) {
                nwrite = 0;
                // printf("the buffer is full\n");
                // sleep(1);
                continue;
            }
            else {
                // error_msg("write msg to fd ");
                printf("write msg to fd %d\n", fd);
                return -1;
            }
        }
        nleft -= nwrite;
        ptr += nwrite;
        writeSum += nwrite;
    }
    return writeSum;
}

ssize_t readn(int fd, char *msg, size_t n) {
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char *ptr = msg;
    while (nleft > 0) {

        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
                continue;
            }
            else if (errno == EAGAIN) {
                return readSum;
            }
            else {
                error_msg("read msg to fd \n" );
                return -1;
            }
        }
        else if (nread == 0) 
            break;
        nleft -= nread;
        ptr += nread;
        readSum += nread;
    }
    return readSum;
}


