//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Json.hpp"
#include "Logger.hpp"
#include <fstream>

namespace Utils{
    bool Json::save(std::string path)const{
        std::ofstream file(path);
        if(!file.is_open()){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "File open failed.",
                std::source_location::current()
            );
            return false;
        }
        file<<this->j.dump(4);
        return true;
    }
    bool Json::read(std::string path){
        std::ifstream file(path);
        if(!file.is_open()){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "File open failed.",
                std::source_location::current()
            );
            return false;
        }
        file>>this->j;
        return true;
    }
}