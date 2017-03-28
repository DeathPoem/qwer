#ifndef FACILITY_CPP
#define FACILITY_CPP 
#include "facility.h"

using namespace my_http;

int my_http::MyFacilityTest() {
    int fd = my_http::DeployLogFile();
    my_http::WriteToFile(fd, "Open log file");
    close(fd);
    return 0;
}

int my_http::DeployLogFile() {
    char filename[100];
    int fd;
    sprintf(filename, "%s/stuff/log.txt", GET_MY_ROOT_PATH);
    fd = open(filename, O_WRONLY | O_TRUNC | O_APPEND);
    if (fd == -1) {
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH);
    } else {
        cout << "log exist!!!" << endl;
    }
    exit_if(fd<0, "error in open log file");
    char buff[100];
    sprintf(buff, "======== new log =======\n");
    write(fd, buff, strlen(buff));
    return fd;
}

void my_http::WriteToFile(int fd, const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);

    char buff[500];
    sprintf(buff, "Time : %s. ", __TIME__);
    vsprintf(buff, format, arglist);
    sprintf(buff, "\n");
    write(fd, buff, strlen(buff));
    va_end(arglist);
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

Logger& Logger::repare() {
    int fd = DeployLogFile();
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

int Logger::log(const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);

    char buf[100];
    sprintf(buf, "log : ");
    write(fd_, buf, strlen(buf));

    char buff[500];
    sprintf(buff, "Time : %s. ", __TIME__);
    vsprintf(buff, format, arglist);
    sprintf(buff, "\n");

    write(fd_, buff, strlen(buff));
    va_end(arglist);
    return strlen(buff);
}

#endif /* ifndef FACILITY_CPP */
