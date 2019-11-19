#include "Http.h"

Http::Http(std::string& root): _state(HTTP_PARSE_STATE::start), _root(root), 
_keep_alive(false), _data(nullptr), _n_left(0), _fd(-1)
{  
}

Http::~Http()
{
    if (_data != nullptr) {
        free(_data);
    }
}

pthread_mutex_t Http::timer_lock = PTHREAD_MUTEX_INITIALIZER;
// 目前只对该数据进行读，并不存在写的问题，所以可以取消掉互斥锁
std::unordered_map<std::string, std::string> Http::mime{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {"NULL" ,"text/plain"}
};


std::unordered_set<std::string> Http::supported_version {
    "0.9", "1.0", "1.1"
};

std::string& Http::getMethod() {
    return _method;
}

std::unordered_map<std::string, std::string>& Http::getKeyValue() {
    return _key_value;
}

std::string& Http::getPath() {
    return _path;
}

std::string& Http::getVersion() {
    return _version;
}

bool Http::getConnection() {
    return _keep_alive;
}

int Http::_parseHttpHeader(char* buff, size_t len) {
    std::string key, value;

    for (size_t i = 0; i < len; ++i) {
        char ch = buff[i];

        switch (_state)
        {
        case HTTP_PARSE_STATE::start:

            if (ch == '\r' || ch == '\n') {
                break;
            }
            
            if ((ch < 'A' || ch > 'Z') && ch != '_') {
                return HTTP_BAD_REQUEST;
            }

            _method += ch;
            _state = HTTP_PARSE_STATE::method;
            break;

        case HTTP_PARSE_STATE::method:

            if ((ch >= 'A' && ch <= 'Z') || ch == '_') {
                _method += ch;
            }
            else if (ch == ' ') {
                _state = HTTP_PARSE_STATE::spaces_before_uri;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::spaces_before_uri:

            if (ch == '/') {
                _path += ch;
                _state = HTTP_PARSE_STATE::uri;
            }
            else if (ch != ' ') {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::uri:

            if (ch == ' ') {
                _state = HTTP_PARSE_STATE::http;
            }
            else {
                _path += ch;
            }
            break;

        case HTTP_PARSE_STATE::http:

            if (ch == 'H') {
                _state = HTTP_PARSE_STATE::http_H;
            }
            else if (ch != ' ') {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::http_H:

            if (ch == 'T') {
                _state = HTTP_PARSE_STATE::http_HT;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::http_HT:

            if (ch == 'T') {
                _state = HTTP_PARSE_STATE::http_HTT;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::http_HTT:

            if (ch == 'P') {
                _state = HTTP_PARSE_STATE::http_HTTP;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::http_HTTP:

            if (ch == '/') {
                _state = HTTP_PARSE_STATE::http_HTTP_slash;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::http_HTTP_slash:

            if (ch >= '0' && ch <= '9') {
                _version += ch;
                _state = HTTP_PARSE_STATE::http_version;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;
        
        case HTTP_PARSE_STATE::http_version:

            if (ch == '\r' || ch == '\n') {
                if (!supported_version.count(_version)) {
                    return HTTP_VERSION_UNSUPPORT;
                }
                else {
                    _state = HTTP_PARSE_STATE::request_header_end;
                }
            }
            // 其实这里可以解析的更加严格一些
            else if (ch >= '0' || ch <= '9' || ch == '.') {
                _version += ch;
            }
            else {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::request_header_end:

            if (ch != '\r' && ch != '\n') {
                _state = HTTP_PARSE_STATE::http_key;
                key += ch;
            }
            break;

        case HTTP_PARSE_STATE::http_key:

            if (ch == '\r' || ch == '\n') {
                return HTTP_BAD_REQUEST;
            }
            else if (ch == ':') {
                _state = HTTP_PARSE_STATE::http_space_after_colon;
            }
            else if (ch == ' ') {
                _state = HTTP_PARSE_STATE::http_space_before_colon;
            }
            else {
                key += ch;
            }
            break;
        
        case HTTP_PARSE_STATE::http_space_before_colon:

            if (ch == ':') {
                _state = HTTP_PARSE_STATE::http_space_after_colon;
            }
            else if (ch != ' ') {
                return HTTP_BAD_REQUEST;
            }
            break;

        case HTTP_PARSE_STATE::http_space_after_colon:

            if (ch != ' ') {
                value += ch;
                _state = HTTP_PARSE_STATE::http_value;
            }
            break;

        case HTTP_PARSE_STATE::http_value:   

            if (ch == '\r' || ch == '\n') {
                _key_value.insert({key, value});
                key.clear();
                value.clear();
                _state = HTTP_PARSE_STATE::line_end;
            }
            else {
                value += ch;
            }
            break;

        case HTTP_PARSE_STATE::line_end:

            if (ch == '\r') {
                _state = HTTP_PARSE_STATE::header_end;
            }
            else if (ch != '\n'){
                _state = HTTP_PARSE_STATE::http_key;
                key += ch;
            }
            break;

        case HTTP_PARSE_STATE::header_end:

            if (ch != '\n') {
                return HTTP_BAD_REQUEST;
            }
            // 返回已经解析的字符个数
            _state = HTTP_PARSE_STATE::http_end;
            return i + 1;

        // 之前已经解析完毕了
        case HTTP_PARSE_STATE::http_end:
            return 0;
        }
    }
    // 数据还没解析完整
    return HTTP_PARSE_AGAIN;
}

void Http::_clear_state() {
    if (_keep_alive) {
        _state = HTTP_PARSE_STATE::start;
        _key_value.clear();
        _method.clear();
        _path.clear();
        _query.clear();
        _version.clear();
        _keep_alive = false;
        _data = nullptr;
        _n_left = 0;
        _fd = -1;
    }
}

int Http::parseHttp(int fd, char* buff, size_t len) {
    // ret代表解析的数据长度
    int ret =  _parseHttpHeader(buff, len);
    int flag = 0;
    if (ret < 0) {
        switch (ret)
        {
        // 开始解析头部, again代表之前读取的数据都已经解析完毕，start不用改变
        case HTTP_PARSE_AGAIN:
            printf("You need to read data again\n");
            break;
        case HTTP_VERSION_UNSUPPORT:
            do_error(fd, "501", "Http Version Unsupport");
            break;
        // case HTTP_METHOD_NOT_IMPLEMENTED:
        //     do_error(fd, "501", "Method Not Implemented");
        //     break;
        case HTTP_BAD_REQUEST:
            do_error(fd, "400", "Bad Request");
            break;
        default:
            do_error(fd, "400", "Bad Request");
            break;
        }
        return ret;
    }
    // 将路径修正，这块可以根据你数据所放的位置来决定
    _modify_path();

    if (_method == "GET") {
        return GET(fd);
    }
    else if (_method == "POST") {
        // todo
        do_error(fd, "501", "Method Not Implemented");
        return HTTP_METHOD_NOT_IMPLEMENTED;
    }
    else if (_method == "HEAD"){
        return HEAD(fd);
    }
    else {
        do_error(fd, "501", "Method Not Implemented");
        return HTTP_METHOD_NOT_IMPLEMENTED;
    }
}

void Http::do_error(int fd, std::string errnum, std::string reason) {
    char body[BUFF_SIZE], header[BUFF_SIZE];
    const char* errnum_cstr = errnum.c_str();
    const char* reason_cstr = reason.c_str();

    sprintf(body, "<html><title>%s : %s</title>", errnum_cstr, reason_cstr);
    sprintf(body, "<H1>%s : %s</H1>", errnum_cstr, reason_cstr);
    sprintf(body, "%s<hr><em>Lightning web server</em>\n</html>", body);

    sprintf(header, "HTTP/%s %s %s\r\n", _version.c_str(), errnum_cstr, reason_cstr);
    sprintf(header, "%sServer: Lightning\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));

    writen(fd, header, strlen(header));
    writen(fd, body, strlen(body));
}

void Http::send_header(int fd, std::string& content_type, std::string& connection, int content_length) {
    char header[BUFF_SIZE];
    sprintf(header, "HTTP/%s 200 OK\r\n", _version.c_str());
    sprintf(header, "%sServer: Lightning\r\n", header);
    sprintf(header, "%sContent-type: %s\r\n", header, content_type.c_str());
    sprintf(header, "%sConnection: %s\r\n", header, connection.c_str());
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, content_length);

    writen(fd, header, strlen(header));
}

std::string Http::_getfile_type() {
    size_t pos = _path.find_last_of('.');
    std::string suffix = _path.substr(pos, _path.size() - pos);
    if (mime.count(suffix)) {
        return mime[suffix];
    }
    else return "";
}

int Http::GET(int fd) {

    std::string file_type(_getfile_type());
    if (file_type.empty()) {
        do_error(fd, "501", "Not Avilable File Type");
        return -1;
    }

    std::string connection("close");
    if (_key_value.count("Connection")) {
        connection = _key_value["Connection"];
        _keep_alive = true;
    }

    // 发送数据出错，那么断开连接
    if (send_file(fd, file_type, connection) < 0) {
        return -1;
    }
    return HTTP_PARSE_OK;
}

int Http::send_file(int fd, std::string& file_type, std::string& connection) {
    std::ifstream is(_path.c_str(), std::ifstream::in);
    // seekg第一个参数是偏移量，第二个参数是基地址。
    // 它将修改当前文件指针所指位置
    if (is.is_open()) {
        is.seekg(0, is.end);
        // 返回当前指针所在位置，如果在文件末尾则返回文件长度
        int flength = is.tellg();
        // 再将文件指针定位到开头位置
        is.seekg(0, is.beg);


        char* buffer = (char*)malloc(flength);
        if (buffer == NULL) {
            error_msg("memory allocate");
        }

        is.read(buffer, flength);

        // 先发送header
        send_header(fd, file_type, connection, flength);

        // 再将响应写入发送缓冲区，并发送
        _data = buffer;
        _n_left = flength;
        _fd = fd;
        // ret = writen(fd, buffer, flength);       
        // delete buffer;
    }
    else {
        do_error(fd, "404", "Not Found");
        return -1;
    }
    // 清理
    is.close();
    return HTTP_PARSE_OK;
}

int Http::send_data() {
    int nwrite = 0;
    while (_n_left) {
        if ((nwrite = write(_fd, _data, _n_left)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            else if (errno == EAGAIN) {
                break;
            }
            else {
                return -1;
            }
        }
        _n_left -= nwrite;
        _data += nwrite;
    }
    if (_n_left == 0 && _keep_alive) {
        // 如果是长链接需要清除状态机的状态
        _clear_state();
    }
    
    return _n_left;
}

// HEAD方法用于查看文件是否存在
int Http::HEAD(int fd) {
    const char* filename = _path.c_str();
    // 对于head方法来说应该不需要建立长连接
    std::string connection("close");
    _keep_alive = false;
    struct stat st;

    if (stat(filename, &st) < 0) {
            do_error(fd, "404", "Not Found");
            return -1;
    }

    std::string file_type = _getfile_type();
    if (file_type.empty()) {
        do_error(fd, "501", "Not Avilable File Type");
        return -1;
    }
    
    send_header(fd, file_type, connection, st.st_size);
    return HTTP_PARSE_OK;
}

void Http::_modify_path() {
    // printf("source path = %s\n", _path.c_str());
    // 设置为相对路径
    if (_path[0] == '/') {
        _path = _root + _path;
    }

    if (_path.back() == '/') {
        _path += "index.html";
    }
    // printf("path = %s\n", _path.c_str());
}