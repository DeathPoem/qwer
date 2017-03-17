#ifndef FACILITY_H
#define FACILITY_H 

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
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

    /*
     * return log file fd
     */
    int DeployLogFile();

    /*
     * write to a df log file
     */
    void WriteToLog(int fd, const char* format, ...); 

    // wrapper
    
    /*
     * a wrapper for create + bind + listen socket
     */

    /*
     * create + connect socket
     */

} /* my_http */ 


#endif /* ifndef FACILITY_H */
