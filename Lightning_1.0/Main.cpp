#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include "Lightning.h"
#define CONF_PATH "./Lightning.conf"

// #include "Util.h"

static const option long_options[] =
{
    {"help", no_argument, nullptr, '?'},
    {"conf", required_argument, nullptr, 'c'}
};

static void usage() {
   fprintf(stderr,
	"Lightning [option]... \n"
	"  -c|--conf <config file>  Specify config file. Default ./Lightning.conf.\n"
	"  -?|-h|--help             This information.\n"
	);
}

int main(int argc, char* argv[]) {
    std::string conf_path = CONF_PATH;
    int opt;
    int listenfd;
    // Configure conf;

    while ((opt = getopt_long(argc, argv, "c:?h", long_options, nullptr)) != EOF) {
        switch (opt)
        {
        case 'c':
            conf_path = optarg;
            break;
        case '?':
        case 'h':
        case ':':
            usage();
            return 0;
        }
    }
    // 查看路径的调试信息
    std::cout << "Your configure path is " << conf_path << std::endl;
    
    // 解析非选项字段
    if (optind < argc) {
        std::cerr << "This is non-options argments : " << std::endl;
        while (optind < argc) {
            std::cerr << "\t" << argv[optind++] << std::endl;
        }
        return 0;
    }

    // if (conf.read_conf_file(conf_path) < 0) {
    //     return -1;
    // }

    Lightning Server(conf_path);
    Server.serve_start();
    
    return 0;
}