//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef JSON_HPP
#define JSON_HPP
#include "nlohmann/json.hpp"
#include <string>

namespace Utils{
    struct Json{
        private:
            nlohmann::json j;
            static void mergeRecursive(nlohmann::json& base,const nlohmann::json& override){
                for(auto& [key,value]:override.items()){
                    if(base.contains(key)&&base[key].is_object()&&value.is_object()){
                        mergeRecursive(base[key],value);
                    }
                    else{
                        base[key]=value;
                    }
                }
            }
        public:
            Json(nlohmann::json data={}):j(data){};
            bool read(std::string path);
            bool save(std::string path)const;
            const nlohmann::json& getJson()const{return j;}
            nlohmann::json& getJson(){return j;}
            Json operator+(const Json& other)const{
                nlohmann::json js;
                js=this->j;
                mergeRecursive(js,other.getJson());
                return js;
            };
            Json& operator+=(const Json& other){
                mergeRecursive(this->j,other.getJson());
                return *this;
            }
            Json& operator<<(const std::string& path){
                read(path);
                return *this;
            }
            Json& operator>>(const std::string& path){
                save(path);
                return *this;
            }
    };
}

#endif