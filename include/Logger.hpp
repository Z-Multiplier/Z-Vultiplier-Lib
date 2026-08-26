//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <mutex>
#include <memory>
#include <bitset>
#include <chrono>
#include <source_location>

#ifdef QUICK_DEBUG
    #define BREAKPOINT do{\
        Core::globalLogger.traceLog(Core::logger::LOG_DEBUG,"Breakpoint triggered!",std::source_location::current());\
        std::cin.get();\
    }while(false)
    #define MESSAGE(msg) do{\
        Core::globalLogger.traceLog(Core::logger::LOG_DEBUG,msg,std::source_location::current());\
    }while(false)
#else
    #define BREAKPOINT (void)0
    #define MESSAGE(msg) (void)0
#endif


namespace Core{
    extern std::bitset<6> ignores;
    struct logger{
        private:
            std::ostream *out;
            std::mutex safeLock;
            std::chrono::steady_clock::time_point startTime;
        public:
            enum LogLevel{
                LOG_DEBUG=0,
                LOG_INFO=2,
                LOG_WARNING=3,
                LOG_ERROR=4,
                LOG_NOTE=1,
                LOG_FATAL=5
            };
            logger():out(&std::clog){};
            logger(std::ostream &output):out(&output){};
            void traceLog(enum LogLevel level,std::string message,std::source_location sl){
                std::lock_guard<std::mutex> lock(safeLock);
                if(ignores.test(level)) return;
                auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                (*out)<<"Log from file:"<<sl.file_name()<<" in function:"<<sl.function_name()<<";at line:"<<sl.line()<<" column:"<<sl.column()<<std::endl;
                switch(level){
                    case LOG_DEBUG:
                        (*out)<<std::ctime(&now)<<"[DEBUG]: "<<message<<std::endl;
                        break;
                    case LOG_INFO:
                        (*out)<<std::ctime(&now)<<"[INFO]: "<<message<<std::endl;
                        break;
                    case LOG_WARNING:
                        (*out)<<std::ctime(&now)<<"[WARNING]: "<<message<<std::endl;
                        break;
                    case LOG_ERROR:
                        (*out)<<std::ctime(&now)<<"[ERROR]: "<<message<<std::endl;
                        break;
                    case LOG_NOTE:
                        (*out)<<std::ctime(&now)<<"[NOTE]: "<<message<<std::endl;
                        break;
                    case LOG_FATAL:
                        (*out)<<std::ctime(&now)<<"[FATAL]: "<<message<<std::endl;
                        break;
                    default:
                        (*out)<<std::ctime(&now)<<"[UNKNOWN]: "<<message<<std::endl;
                        break;
                }
            }
            template<typename T>
            void varLog(enum LogLevel level,std::string varName,T varValue,std::source_location sl){
                std::lock_guard<std::mutex> lock(safeLock);
                if(ignores.test(level)) return;
                auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                (*out)<<"Log from file:"<<sl.file_name()<<" in function:"<<sl.function_name()<<";at line:"<<sl.line()<<" column:"<<sl.column()<<std::endl;
                switch(level){
                    case LOG_DEBUG:
                        (*out)<<std::ctime(&now)<<"[DEBUG]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                    case LOG_INFO:
                        (*out)<<std::ctime(&now)<<"[INFO]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                    case LOG_WARNING:
                        (*out)<<std::ctime(&now)<<"[WARNING]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                    case LOG_ERROR:
                        (*out)<<std::ctime(&now)<<"[ERROR]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                    case LOG_NOTE:
                        (*out)<<std::ctime(&now)<<"[NOTE]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                    case LOG_FATAL:
                        (*out)<<std::ctime(&now)<<"[FATAL]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                    default:
                        (*out)<<std::ctime(&now)<<"[UNKNOWN]: "<<varName<<" = "<<varValue<<std::endl;
                        break;
                }
            }
            template<typename... Args>
            void formatLog(enum LogLevel level,std::string format,std::source_location sl,Args... args){
                std::lock_guard<std::mutex> lock(safeLock);
                if(ignores.test(level)) return;
                size_t size=snprintf(nullptr,0,format.c_str(),args...)+1;
                std::unique_ptr<char[]> buf(new char[size]);
                snprintf(buf.get(),size,format.c_str(),args...);
                auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                (*out)<<"Log from file:"<<sl.file_name()<<" in function:"<<sl.function_name()<<";at line:"<<sl.line()<<" column:"<<sl.column()<<std::endl;
                switch(level){
                    case LOG_DEBUG:
                        (*out)<<std::ctime(&now)<<"[DEBUG]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                    case LOG_INFO:
                        (*out)<<std::ctime(&now)<<"[INFO]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                    case LOG_WARNING:
                        (*out)<<std::ctime(&now)<<"[WARNING]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                    case LOG_ERROR:
                        (*out)<<std::ctime(&now)<<"[ERROR]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                    case LOG_NOTE:
                        (*out)<<std::ctime(&now)<<"[NOTE]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                    case LOG_FATAL:
                        (*out)<<std::ctime(&now)<<"[FATAL]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                    default:
                        (*out)<<std::ctime(&now)<<"[UNKNOWN]: "<<std::string(buf.get(),buf.get()+size-1)<<std::endl;
                        break;
                }
            }
    };
    extern logger globalLogger;
}
#endif // LOGGER_HPP