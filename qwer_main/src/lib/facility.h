#ifndef FACILITY_H
#define FACILITY_H 

#include "config.h"
#include <time.h>
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
#include <map>
#include <unordered_map>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <string>
#include <cassert>
#include <functional>
#include <algorithm>
#include <tuple>
#include <memory>
#include <utility>

/*
 * This file is implement for some facility like logging, debugging, error
 */

namespace my_http {

#define exit_if(r, ...) \
    if (r) {\
        printf(__VA_ARGS__);\
        printf("%s : %d, error no : %d %s\n", __FILE__, __LINE__, errno, strerror(errno));\
        exit(1);\
    }

#define log_if_level(level, ...) \
    if (static_cast<uint32_t>(level) < static_cast<uint32_t>(Logger::get_logger().get_log_level())) { \
        Logger::get_logger().log_v(static_cast<uint32_t>(level), __FILE__, std::to_string(__LINE__).c_str(), __TIME__, __VA_ARGS__); \
    }

#define LOG_ERROR(...) log_if_level(Logger::LogLevel::ERROR, __VA_ARGS__)
#define LOG_WARN(...) log_if_level(Logger::LogLevel::WARN, __VA_ARGS__)
#define LOG_DEBUG(...) log_if_level(Logger::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_if_level(Logger::LogLevel::INFO, __VA_ARGS__)
#define LOG_SET_FILE(file) Logger::get_logger().repare(file)
#define LOG_SET_LEVEL(level) Logger::get_logger().set_log_level(level)
#define ABORT(...) \
    while (0) {\
        log_if_level(Logger::LogLevel::ERROR, __VA_ARGS__);\
        std::abort();\
    }
#define NOTDONE() ABORT("not implement yet!")

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::map;
using std::unique_ptr;
using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;
using std::weak_ptr;
using std::pair;
using std::tuple;
using std::stoi;
using std::make_pair;
using std::unordered_map;
using std::to_string;
using CallBack = std::function<void()>;
using unique_id_t = uint32_t;

    int MyFacilityTest(); 

    // return log file fd
    int DeployLogFile(const char* file);

    void WriteToFile(int fd, const char* format, ...); 

    class noncopyable {
        public :
        noncopyable() {}
        noncopyable(const noncopyable& c) = delete;
        noncopyable& operator==(const noncopyable& c) = delete;
    };

    // TODO implement a LogStream to use support iostream like << for logging
    struct LogStream {

    };

    class Logger : private noncopyable {
        public :
            enum class LogLevel {ERROR = 0, WARN = 1, DEBUG = 2, INFO = 3};
            Logger();
            ~Logger();
            static Logger& get_logger();
            Logger& repare(const char* file);
            void set_log_level(string level);
            int log(const char* format, ...);
            int log_v(int level, const char* filename, const char* line, const char* time, const char* format, ...);
            LogLevel get_log_level();
        private :
            string filename_;
            int fd_;
            LogLevel cur_level_ = LogLevel::WARN;
            const unordered_map<int, string> level_m_ = {
                {0, "ERROR"},
                {1, "WARN"},
                {2, "DEBUG"},
                {3, "INFO"}
            };
    };

} /* my_http */ 

#endif /* ifndef FACILITY_H */
