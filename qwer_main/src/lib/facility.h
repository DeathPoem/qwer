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
#include <sstream>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
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
    if (static_cast<uint32_t>(level) <= static_cast<uint32_t>(Logger::get_logger().get_log_level())) { \
        Logger::get_logger().log_v(static_cast<uint32_t>(level), __FILE__, std::to_string(__LINE__).c_str(), __TIME__, __VA_ARGS__); \
    }

#define LOG_ERROR(...) log_if_level(Logger::LogLevel::ERROR, __VA_ARGS__)
#define LOG_WARN(...) log_if_level(Logger::LogLevel::WARN, __VA_ARGS__)
#define LOG_DEBUG(...) log_if_level(Logger::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_if_level(Logger::LogLevel::INFO, __VA_ARGS__)
#define LOG_SET_FILE(file) Logger::get_logger().repare(file)
#define LOG_SET_LEVEL(level) Logger::get_logger().set_log_level(level)
#define ABORT(...) \
    do {\
        log_if_level(Logger::LogLevel::ERROR, __VA_ARGS__);\
        std::abort();\
    } while (0)

#define NOTDONE() ABORT("not implement yet!")
#define SLOG_ERROR(msg) \
    do {\
        std::stringstream tmp;\
        tmp << msg << std::endl;\
        log_if_level(Logger::LogLevel::ERROR, tmp.str().c_str());\
    } while (0)
#define SLOG_WARN(msg) \
    do {\
        std::stringstream tmp;\
        tmp << msg << std::endl;\
        log_if_level(Logger::LogLevel::WARN, tmp.str().c_str());\
    } while (0)
#define SLOG_DEBUG(msg) \
    do {\
        std::stringstream tmp;\
        tmp << msg << std::endl;\
        log_if_level(Logger::LogLevel::DEBUG, tmp.str().c_str());\
    } while (0)
#define SLOG_INFO(msg) \
    do {\
        std::stringstream tmp;\
        tmp << msg << std::endl;\
        log_if_level(Logger::LogLevel::INFO, tmp.str().c_str());\
    } while (0)

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::map;
using std::bind;
using std::function;
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
using time_ms_t = int;

    struct TimeStamp {
        string tostring() const;
        TimeStamp& init_stamp_of_now();
        void add_stamp_by_mill(time_ms_t para_t);
        struct timespec get_spec() const;
        private :
        struct timespec spec_;
    };

    bool operator==(const TimeStamp& lhs, const TimeStamp& rhs);
    bool operator<(const TimeStamp& lhs, const TimeStamp& rhs);
    bool operator>(const TimeStamp& lhs, const TimeStamp& rhs);

    struct TimerId {
        TimeStamp alarm_time_;
        unique_id_t seqno_;
    };

    bool operator==(const TimerId& lhs, const TimerId& rhs);
    bool operator<(const TimerId& lhs, const TimerId& rhs);
    bool operator>(const TimerId& lhs, const TimerId& rhs);


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

    // name it as 'channel', which wraps the operation of fd that can be epoll or poll
    class Channel : private noncopyable {
        public:
            Channel (int fd);
            virtual ~Channel ();
            int get_fd();
            void close();
            bool is_closed();
            //struct epoll_event* get_epoll_event_p(); this is not good, because if you return a heap pointer, the caller would be responsible to delete it.
            // following would be more good
            uint32_t get_events();
            static uint32_t get_readonly_event_flag();
            static uint32_t get_writeonly_event_flag();
            static uint32_t get_wr_event_flag();
            static uint32_t get_no_wr_event_flag();
            void set_events(uint32_t para_event);
        private:
            int fd_;
            uint32_t event_;
            bool is_closed_;
            /* data */
    };

} /* my_http */ 

#endif /* ifndef FACILITY_H */
