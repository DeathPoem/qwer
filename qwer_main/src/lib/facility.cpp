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
        sprintf(c_str, "TimeStamp: %ld.%03ld seconds since the Epoch.",
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
        uint64_t s = para_t / 1000;
        uint64_t remain = para_t % 1000;
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
            sprintf(filename, "%s/stuff/log.txt", GET_MY_COMPILE_ROOT_PATH);
        }
        fd = open(filename, O_WRONLY | O_TRUNC | O_APPEND);
        if (fd == -1) {
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH);
        }
        if (fd < 0) {
            char debug_buf[1000];
            sprintf(debug_buf, "can't open file, filename=%s, root =%s", filename, GET_MY_COMPILE_ROOT_PATH);
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

    inline string get_time_of_now() {
        return TimeStamp().init_stamp_of_now().tostring();
    }

    Logger::Logger() {
        fd_ = -1;
    }

    Logger::~Logger() {
        cout << "logger destruct" << endl;
        change_or_quit();
    }

    static std::mutex m_logger_;

    void Logger::change_or_quit() {
        std::lock_guard<std::mutex> lk(m_logger_);
        if (fd_ != -1) {
            if (buffer_active_) {
                if (x_buffer_ != nullptr) {
                    force_write();
                    delete[] x_buffer_;
                    x_buffer_ = nullptr;
                }
            }
            close(fd_);
        }
    }

    void Logger::force_write() {
        auto check = write(fd_, x_buffer_, x_buf_sign_);
        if (check < 0) {
            throw std::runtime_error(strerror(errno));
        }
        x_buf_sign_ -= check;
        assert(x_buf_sign_ == 0);
    }

    Logger& Logger::get_logger() {
        static Logger lg;
        return lg;
    }

    Logger& Logger::repare(const char* file) {
        change_or_quit();
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
        sprintf(buff, "\nfilename:%s, line:%s, time:%s \n level:%s <====> log:", filename, line, time, levelstr.c_str());
        vsprintf(buff + strlen(buff), format, arglist);

        if (fd_ < 0) {
            throw std::runtime_error("no fd_, please LOG_SETFILE()");
        }
        // thread safe log and file write order safe
        if (!buffer_active_) {
            write(fd_, buff, strlen(buff));   // original
        } else {
            std::lock_guard<std::mutex> lk(m_logger_);
            buffer_repare();
            memcpy(x_buffer_ + x_buf_sign_, buff, strlen(buff));
            x_buf_sign_ += strlen(buff);
            assert(x_buf_sign_ < x_size_);
            if (x_buf_sign_ > x_size_ - 1024) {
                force_write();
                buffer_repare();
            }
        }
        va_end(arglist);
        return (int) strlen(buff);
    }

    void Logger::buffer_repare() {
        if (x_buffer_ == nullptr) {
            x_buffer_ = new char[x_size_];
            memset(x_buffer_, 0, x_size_);
            x_buf_sign_ = 0;
        } else if (x_buffer_ != nullptr && x_buf_sign_ == 0) {
            memset(x_buffer_, 0, x_size_);
        }
    }

    void Logger::set_buffer_active(bool ac) {
        buffer_active_ = ac;
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

    Channel::Channel(int fd) : fd_(fd) {
        event_ = 0;
    }

    Channel::~Channel() {
        close();
    }

    int Channel::get_fd() {return fd_;}

    void Channel::shutdown() {
        if (!is_shutdown_) {
            ::shutdown(fd_, SHUT_WR);
            is_shutdown_ = true;
        }
    }

    void Channel::close() {
        if (!is_closed_) {
            ::close(fd_);
            is_closed_ = true;
        }
    }

    uint32_t Channel::get_events() {return event_;}

    void Channel::delete_event(uint32_t para_event) {
        event_ &= ~para_event;
    }

    void Channel::add_event(uint32_t para_event) {event_ |= para_event;}

    constexpr uint32_t Channel::get_readonly_event_flag() {return EPOLLIN;}

    constexpr uint32_t Channel::get_writeonly_event_flag() {return EPOLLOUT;}

    constexpr uint32_t Channel::get_peer_shutdown_flag(){return EPOLLRDHUP;}

    constexpr uint32_t Channel::get_edge_trigger_flag() { return EPOLLET;}

    constexpr uint32_t Channel::get_err_flag() { return  EPOLLERR;}

    void Channel::add_cb(uint32_t event, CallBack&& cb) {
        if (event == Channel::get_readonly_event_flag()) {
            cbvec_[0] = cb;
        } else if (event == Channel::get_writeonly_event_flag()) {
            cbvec_[1] = cb;
        } else if (event == Channel::get_peer_shutdown_flag()) {
            cbvec_[2] = cb;
        } else if (event == Channel::get_err_flag()) {
            cbvec_[3] = cb;
        } else {
            NOTDONE();
        }
    }

    void Channel::act(EventEnum event) {
        auto event_s = enum2uint(event);
        if (event_s == Channel::get_readonly_event_flag()) {
            cbvec_[0]();
        } else if (event_s == Channel::get_writeonly_event_flag()) {
            cbvec_[1]();
        } else if (event_s == Channel::get_peer_shutdown_flag()) {
            cbvec_[2]();
        } else if (event_s == Channel::get_err_flag()) {
            cbvec_[3]();
        } else {
            NOTDONE();
        }
    }

    void set_nonblock(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) {
            NOTDONE();
        }
        auto check = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (check < 0) {
            NOTDONE();
        }
    }

    uint32_t enum2uint(EventEnum eventenum) {
        uint32_t event;

        if (eventenum == EventEnum::IORead) {
            event = Channel::get_readonly_event_flag();
        } else if (eventenum == EventEnum::IOWrite) {
            event = Channel::get_writeonly_event_flag();
        } else if (eventenum == EventEnum::PeerShutDown) {
            event = Channel::get_peer_shutdown_flag();
        } else if (eventenum == EventEnum::Error) {
            event = Channel::get_err_flag();
        } else {
            NOTDONE();
        }
        return event;
    }

    EventEnum uint2enum(uint32_t event) {
        EventEnum para_enum;
        if (event == Channel::get_readonly_event_flag()) {
            para_enum = EventEnum::IORead;
        } else if (event == Channel::get_writeonly_event_flag()) {
            para_enum = EventEnum::IOWrite;
        } else if (event == Channel::get_peer_shutdown_flag()) {
            para_enum = EventEnum::PeerShutDown;
        } else if (event == Channel::get_err_flag()) {
            para_enum = EventEnum ::Error;
        } else {
            NOTDONE();
        }
        return para_enum;
    }

    std::ostream& operator<<(std::ostream& os, EventEnum e) {
        if (e == EventEnum::IORead) {
            os << "IORead";
        } else if (e == EventEnum::IOWrite) {
            os << "IOWrite";
        } else if (e == EventEnum::PeerShutDown) {
            os << "PeerShutDown";
        } else if (e == EventEnum::Error) {
            os << "Error";
        } else {
            NOTDONE();
        }
    }


} /* my_http */ 

#endif /* ifndef FACILITY_CPP */
