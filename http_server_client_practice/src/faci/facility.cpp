#ifndef FACILITY_CPP
#define FACILITY_CPP 
#include "facility.h"

int my_http::MyFacilityTest() {
    int fd = my_http::DeployLogFile();
    my_http::WriteToLog(fd, "Open log file");
    close(fd);
    return 0;
}

int my_http::DeployLogFile() {
    char filename[100];
    sprintf(filename, "%s/stuff/log.txt", GET_MY_ROOT_PATH);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH);
    exit_if(fd<0, "error in open log file")
        return fd;
}

void my_http::WriteToLog(int fd, const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);

    char buff[500];
    sprintf(buff, "Time : %s. ", __TIME__);
    vsprintf(buff, format, arglist);
    sprintf(buff, "\n");
    write(fd, buff, strlen(buff));
    va_end(arglist);
}

#endif /* ifndef FACILITY_CPP */
