//
// Created by wangfrank on 17-8-9.
//

#ifndef ONLINE_EXAMPLE_WEBSERVICE_H
#define ONLINE_EXAMPLE_WEBSERVICE_H

#include "facility.h"
#include <fstream>
#include <tuple>
#include "../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

namespace my_http {

    class MiddleWareInterface {
    public:
        MiddleWareInterface();
        virtual ~MiddleWareInterface();
    };

    class OneMiddle : public MiddleWareInterface {
    public:
        OneMiddle ();
        virtual ~OneMiddle ();
    
    private:
        
    };

    template<typename ... Middlewares>
    class MiddleWareSequence {
    public:
        MiddleWareSequence();
        MiddleWareSequence(shared_ptr<Middlewares> ... middleware_p);
        virtual ~MiddleWareSequence();
        using type_ = MiddleWareSequence<Middlewares...>;
        void tocb();
    };

    class DBholder {
    public:
        DBholder(string dbpath = DBholder::get_default_db_path());
        virtual ~DBholder();
        static string get_default_db_path();
        SQLite::Column SingleQuery(string query_str);
    private:
        SQLite::Database db;
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
