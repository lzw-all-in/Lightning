#ifndef _HTTP_H_
#define _HTTP_H_
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <iostream>
// #include <memory>
#include "Util.h"

#define BUFF_SIZE 2048

#define HTTP_UNKNOWN                     0x0001
#define HTTP_GET                         0x0002
#define HTTP_HEAD                        0x0004
#define HTTP_POST                        0x0008

#define HTTP_VERSION_UNSUPPORT -5
#define HTTP_METHOD_NOT_IMPLEMENTED -4
#define HTTP_PARSE_AGAIN -3
#define HTTP_BAD_REQUEST -2
#define HTTP_PARSE_OK -1
#define HTTP_SHORT_CONNECTION HTTP_PARSE_OK
#define HTTP_LONG_CONNECTION 0

enum HTTP_PARSE_STATE {
    start = 0,
    method,
    spaces_before_uri,
    uri,
    http,
    http_H,
    http_HT,
    http_HTT,
    http_HTTP,
    http_HTTP_slash,
    http_version,
    request_header_end,
    http_key,
    http_space_before_colon,
    http_space_after_colon,
    http_value,
    line_end,
    header_end,
    http_end
};

class Http
{
private:
    /* data */
    static pthread_mutex_t timer_lock;
    std::string _root;
    static std::unordered_map<std::string, std::string> mime;
    // static std::unordered_set<std::string> implemented_method;
    static std::unordered_set<std::string> supported_version;
    std::unordered_map<std::string, std::string> _key_value;
    std::string _method;
    std::string _path;
    std::string _query;
    std::string _version;
    bool _keep_alive;
    int _parseHttpHeader(char* buff, size_t len);
    std::string _getfile_type();
    void _modify_path();
    void _clear_state();
public:
    HTTP_PARSE_STATE _state;
    
    int parseHttp(int fd, char* buff, size_t len);
    void do_error(int fd, std::string errnum, std::string reason);
    void send_header(int fd, std::string& content_type, std::string& connection, int content_length);
    int send_file(int fd, std::string& file_type, std::string& connection);
    void GET(int fd);
    void HEAD(int fd);
    std::string& getMethod();
    std::unordered_map<std::string, std::string>& getKeyValue();
    std::string& getPath();
    std::string& getVersion();
    Http(std::string& root);
    ~Http();
};

#endif