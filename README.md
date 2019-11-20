# Web 服务器项目

> 说明：该项目目前应该会一直保持更新，不过更新频率不一定很高

**测试页面** **[Link](<http://129.211.28.16:8002/>)**

**服务器详情页面** [**Link**](http://129.211.28.16:8002/serverInfo.html)

目前完成了最初稿和改进版本，后续准备添加日志系统。

## 前置课程

书籍：《C++Primer》，《APUE》，《UNP》，《图解HTTP》

源码学习：

1. [**Tinyhttpd  作者：J. David Blackstone**](<https://github.com/EZLippi/Tinyhttpd>)
2. [ **简易http服务器  作者：zouxiaohang**](<https://github.com/zouxiaohang/webserver>)
3. [**Zaver  作者：zyearn**](<https://github.com/zyearn/zaver>)
4. [**A C++ High Performance Web Server.(原始版本) 作者：linyacool**](<https://github.com/linyacool/WebServer>)

## 如何使用

1. conf_path为自己写的的配置文件的路径，如果未改动路径则用默认值即可
2. 端口号默认8002 如有需要请改动Lightning.conf文件

```cmake
./make
./myServer -c conf_path  or ./myServer
```

## 主要模块

1. HTTP解析 模块
2. Reactor模块
3. 线程池模块
4. 小根堆模块

