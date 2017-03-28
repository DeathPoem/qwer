#ifndef FACILITY_H
#define FACILITY_H 

#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <tuple>
#include <utility>

/*
 * This file is implement for some facility like logging, debugging, error
 */

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::pair;
using std::tuple;

#define exit_if(r, ...) \
    if (r) {\
        printf(__VA_ARGS__);\
        printf("%s : %d, error no : %d %s\n", __FILE__, __LINE__, errno, strerror(errno));\
        exit(1);\
    }

namespace my_http {

    int MyFacilityTest(); 

    // return log file fd
    int DeployLogFile();

    void WriteToFile(int fd, const char* format, ...); 

    class noncopyable {
        public :
        noncopyable() {}
        noncopyable(const noncopyable& c) = delete;
        noncopyable& operator==(const noncopyable& c) = delete;
    };

    class Logger : private noncopyable {
        public :
            Logger();
            ~Logger();
            static Logger& get_logger();
            Logger& repare();
            int log(const char* format, ...);
        private :
            string filename_;
            int fd_;
    };

} /* my_http */ 

#endif /* ifndef FACILITY_H */
