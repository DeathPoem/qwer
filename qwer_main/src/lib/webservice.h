//
// Created by wangfrank on 17-8-9.
//

#ifndef ONLINE_EXAMPLE_WEBSERVICE_H
#define ONLINE_EXAMPLE_WEBSERVICE_H

#include "facility.h"
#include <fstream>
#include <tuple>

namespace my_http {

    class MiddleWareInterface {
    public:
        MiddleWareInterface();
        virtual ~MiddleWareInterface();
    };

    template<typename ... Middlewares>
    class MiddleWareSequence {
    public:
        MiddleWareSequence(tuple<Middlewares ...>* middleware_p);
        virtual ~MiddleWareSequence();
    };

    class DBholder {
    public:
        DBholder(string dbpath = DBholder::get_default_db_path());
        virtual ~DBholder();
        static string get_default_db_path();
    };

    //! @brief thread safe
    class FileReader {
    public:
        FileReader(string filename);
        virtual ~FileReader();
        string to_string();

    private:
        string filename_;
    };
};

#endif //ONLINE_EXAMPLE_WEBSERVICE_H
