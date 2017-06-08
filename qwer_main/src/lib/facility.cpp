#ifndef FACILITY_CPP
#define FACILITY_CPP 
#include "facility.h"

namespace my_http {

    string TimeStamp::tostring() const {
        long ms;
        time_t s;
        s = spec_.tv_sec;
        ms = round(spec_.tv_nsec / 1.0e6);
        char c_str[400];
        sprintf(c_str, "TimeStamp: %ld.%03ld seconds since the Epoch",
                (intmax_t)s, ms);
        return string(c_str);
    }

    TimeStamp& TimeStamp::init_stamp_of_now() {
        memset(&spec_, 0, sizeof(spec_));
        assert(spec_.tv_sec == 0);
        ::clock_gettime(CLOCK_MONOTONIC_COARSE, &spec_);
        assert(spec_.tv_sec != 0);
        return *this;
    }

    void TimeStamp::add_stamp_by_mill(time_ms_t para_t) {
        uint64_t s = 2300 / 1000;
        uint64_t remain = 2300 % 1000;
        uint64_t addup = remain + (spec_.tv_nsec / 1.0e6);
        s += addup / 1000;
        remain = addup % 1000;
        spec_.tv_nsec = remain * 1000000;
        spec_.tv_sec += s;
    }

    struct timespec TimeStamp::get_spec() const {
        return spec_;
    }

    TimeStamp::TimeStamp(struct timespec spec) {
        spec_ = spec;
    }

    TimeStamp::TimeStamp() {

    }

    bool operator==(const TimeStamp& lhs, const TimeStamp& rhs) {
        return lhs.get_spec().tv_nsec == rhs.get_spec().tv_nsec
            && lhs.get_spec().tv_sec == rhs.get_spec().tv_sec;
    }

    bool operator<(const TimeStamp& lhs, const TimeStamp& rhs) {
        auto dis = lhs.get_spec().tv_nsec - rhs.get_spec().tv_nsec;
        return lhs.get_spec().tv_sec < rhs.get_spec().tv_sec
            // for some unknown case, wake up time is always short than expected, so fix this precise problem
            || (lhs.get_spec().tv_sec == rhs.get_spec().tv_sec 
                        && (dis < 20 * 1000 * 1000));
    }

    bool operator>(const TimeStamp& lhs, const TimeStamp& rhs) {
        return rhs < lhs;
    }

    bool operator==(const TimerId& lhs, const TimerId& rhs) {
        return lhs.alarm_time_ == rhs.alarm_time_
            && lhs.seqno_ == rhs.seqno_;
    }

    bool operator<(const TimerId& lhs, const TimerId& rhs) {
        return lhs.alarm_time_ < rhs.alarm_time_;
    }

    bool operator>(const TimerId& lhs, const TimerId& rhs) {
        return rhs < lhs;
    }


    int MyFacilityTest() {
        int fd = my_http::DeployLogFile("");
        my_http::WriteToFile(fd, "Open log file");
        close(fd);
        return 0;
    }

    void Logger::set_log_level(string level) {
        if (level == "ERROR") {
            cur_level_ = LogLevel::ERROR;
        } else if (level == "WARN") {
            cur_level_ = LogLevel::WARN;
        } else if (level == "DEBUG") {
            cur_level_ = LogLevel::DEBUG;
        } else if (level == "INFO") {
            cur_level_ = LogLevel::INFO;
        } else {
            exit_if(true, "can't find loglevel");
        }
    }

    Logger::LogLevel Logger::get_log_level() {
        return cur_level_;
    }

    int DeployLogFile(const char* file) {
        char filename[100];
        memset(filename, 0, sizeof(filename));
        int fd;
        if (strcmp(file, "")) {
            sprintf(filename, "%s", file);
        } else {
            sprintf(filename, "%s/stuff/log.txt", GET_MY_ROOT_PATH);
        }
        fd = open(filename, O_WRONLY | O_TRUNC | O_APPEND);
        if (fd == -1) {
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH);
        }
        if (fd < 0) {
            char debug_buf[1000];
            sprintf(debug_buf, "can't open file, filename=%s, root =%s", filename, GET_MY_ROOT_PATH);
            string tx = string(debug_buf);
            cout << tx << endl;
            throw std::runtime_error("no file opened");
        }
        char buff[100];
        sprintf(buff, "======== new log =======\n");
        write(fd, buff, strlen(buff));
        return fd;
    }

    void WriteToFile(int fd, const char* format, ...) {
        va_list arglist;
        va_start(arglist, format);

        char buff[500];
        sprintf(buff, "\nTime : %s. ", __TIME__);
        vsprintf(buff, format, arglist);
        write(fd, buff, strlen(buff));
        va_end(arglist);
    }

    std::ostream &operator<<(std::ostream &os, const TimeStamp &rhs) {
        return os << rhs.tostring();
    }

    Logger::Logger() {
        fd_ = -1;
    }

    Logger::~Logger() {
        if (fd_ != -1) {
            close(fd_);
        }
    }

    Logger& Logger::get_logger() {
        static Logger lg;
        return lg;
    }

    Logger& Logger::repare(const char* file) {
        int fd;
        fd = DeployLogFile(file);
        if (fd == -1) {
            perror("deploy log fail");
        } else {
            fd_ = fd;
        }
        char buff[100];
        sprintf(buff, "repare.");
        write(fd, buff, strlen(buff));
        return *this;
    }

    int Logger::log_v(int level, const char* filename, const char* line, const char* time, const char* format, ...) {

        va_list arglist;
        va_start(arglist, format);

        string levelstr = level_m_.at(level);
        char buff[500];
        sprintf(buff, "\nfilename:%s, line:%s, time:%s, level:%s <====> log:", filename, line, time, levelstr.c_str());
        vsprintf(buff + strlen(buff), format, arglist);

        if (fd_ < 0) {
            throw std::runtime_error("no fd_, please LOG_SETFILE()");
        }
        write(fd_, buff, strlen(buff));
        va_end(arglist);
        return strlen(buff);
    }

    int Logger::log(const char* format, ...) {
        va_list arglist;
        va_start(arglist, format);

        char buf[100];
        sprintf(buf, "log : ");
        write(fd_, buf, strlen(buf));

        char buff[500];
        vsprintf(buff, format, arglist);

        write(fd_, buff, strlen(buff));
        va_end(arglist);
        return strlen(buff);
    }   

    Channel::Channel(int fd) : fd_(fd) {}

    Channel::~Channel() {}

    int Channel::get_fd() {return fd_;}

    bool Channel::is_closed() {return is_closed_;}

    void Channel::close() {::close(fd_);}

    uint32_t Channel::get_events() {return event_;}

    void Channel::set_events(uint32_t para_event) {event_ = para_event;}

    uint32_t Channel::get_readonly_event_flag() {return EPOLLIN;}

    uint32_t Channel::get_writeonly_event_flag() {return EPOLLOUT;}

    uint32_t Channel::get_no_wr_event_flag() {return EPOLLERR;}

    uint32_t Channel::get_wr_event_flag() {return EPOLLIN | EPOLLOUT;}

} /* my_http */ 

#endif /* ifndef FACILITY_CPP */