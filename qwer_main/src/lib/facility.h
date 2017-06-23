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
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>
#include <error.h>
#include <string.h>
#include <stdio.h>
#include <map>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <functional>
#include <algorithm>
#include <tuple>
#include <memory>
#include <utility>
#include <cstdint>

/*
 * This file is implement for some facility like logging, debugging, error
 */

namespace my_http {

inline std::string get_time_of_now();

#define exit_if(r, ...) \
    if (r) {\
        printf(__VA_ARGS__);\
        printf("%s : %d, error no : %d %s\n", __FILE__, __LINE__, errno, strerror(errno));\
        exit(1);\
    }

#define log_if_level(level, ...) \
    if (static_cast<uint32_t>(level) - 1 < static_cast<uint32_t>(Logger::get_logger().get_log_level())) { \
        Logger::get_logger().log_v(static_cast<uint32_t>(level), __FILE__, std::to_string(__LINE__).c_str(), my_http::get_time_of_now().c_str(), __VA_ARGS__); \
    }

#define LOG_ERROR(...) log_if_level(Logger::LogLevel::ERROR, __VA_ARGS__)
#define LOG_WARN(...) log_if_level(Logger::LogLevel::WARN, __VA_ARGS__)
#define LOG_DEBUG(...) log_if_level(Logger::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_if_level(Logger::LogLevel::INFO, __VA_ARGS__)
#define LOG_SET_FILE(file) Logger::get_logger().repare(file).set_buffer_active(true)
#define LOG_SET_FILE_P(file, flag) Logger::get_logger().repare(file).set_buffer_active(flag)        // set flag to true would let it be thread safe log
#define LOG_SET_LEVEL(level) Logger::get_logger().set_log_level(level)
#define ABORT(...) \
    do {\
        Logger::get_logger().set_buffer_active(false);\
        log_if_level(Logger::LogLevel::ERROR, __VA_ARGS__);\
        Logger::get_logger().set_buffer_active(true);\
        throw std::runtime_error("my abort");\
    } while (0)
 // by doing so, I can get error log even a unhandled signal is rised
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
using std::get;
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
        TimeStamp();
        TimeStamp(struct timespec spec);
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
    std::ostream& operator<<(std::ostream& os, const TimeStamp& rhs);

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

    void set_nonblock(int fd);

    class noncopyable {
        public :
        noncopyable() {}
        noncopyable(const noncopyable& c) = delete;
        noncopyable& operator==(const noncopyable& c) = delete;
    };

    class Logger : private noncopyable {
        public :
            enum class LogLevel {ERROR = 1, WARN = 2, DEBUG = 3, INFO = 4};
            Logger();
            ~Logger();
            static Logger& get_logger();
            Logger& repare(const char* file);
            void set_log_level(string level);
            int log(const char* format, ...);
            int log_v(int level, const char* filename, const char* line, const char* time, const char* format, ...);
            LogLevel get_log_level();
            void set_buffer_active(bool ac);
        private :
            void change_or_quit();
            void buffer_repare();
            void force_write();
            int fd_;
            bool buffer_active_ = false;
            const size_t x_size_ = 204800;
            char* x_buffer_ = nullptr;
            size_t  x_buf_sign_ = 0;
            LogLevel cur_level_ = LogLevel::WARN;
            const unordered_map<int, string> level_m_ = {
                {1, "ERROR"},
                {2, "WARN"},
                {3, "DEBUG"},
                {4, "INFO"}
            };
    };

    enum class ChannelType {TCP, TIMER, UDP, FD};
    // name it as 'channel', which wraps the operation of fd that can be epoll or poll
    class Channel : private noncopyable {
        public:
            Channel (int fd);
            virtual ~Channel ();
            int get_fd();
            void shutdown();
            bool is_shutdown();
            void close();
            bool is_closed();
            //struct epoll_event* get_epoll_event_p(); this is not good, because if you return a heap pointer, the caller would be responsible to delete it.
            // following would be more good
            uint32_t get_events();
            static uint32_t get_readonly_event_flag();
            static uint32_t get_writeonly_event_flag();
            static uint32_t get_edge_trigger_flag();
            static uint32_t get_peer_shutdown_flag();
            void add_event(uint32_t para_event);
        private:
            int fd_;
            enum ChannelType ct_;
            uint32_t event_;
            bool is_closed_ = false;
            bool is_shutdown_ = false;
            /* data */
    };

} /* my_http */ 

#endif /* ifndef FACILITY_H */
