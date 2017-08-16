//
// Created by wangfrank on 17-8-9.
//
#include "webservice.h"

namespace my_http {
    FileReader::FileReader(string filename) : filename_(filename) { }

    FileReader::~FileReader() {}

    string FileReader::to_string() {
        std::fstream s(filename_, s.binary | s.trunc | s.in);
        if (!s.is_open()) {
            SLOG_WARN("failed to open " << filename_ << '\n');
        } else {
            // read
            std::string str;
            while (!s.eof()) {
                s >> str;
            }
        }
    }

    string DBholder::get_default_db_path() {
        return string() + GET_MY_COMPILE_ROOT_PATH + "/stuff/defaultdb";
    }

    DBholder::DBholder(string dbpath) : 
        db(dbpath, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE) {

    }

    SQLite::Column DBholder::SingleQuery(string qstr) {
         return db.execAndGet(qstr.c_str());
    }

    DBholder::~DBholder() {

    }
};

