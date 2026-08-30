//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <unordered_map>
#include "Logger.hpp"
#include <any>
#include <unordered_set>
#include <functional>
#include <numeric>

namespace Game{
    extern Core::logger gameLogger;
    constexpr float Unpassable=std::numeric_limits<float>::infinity();
    struct Terrain{
        std::vector<std::vector<std::string>> storage;
        std::unordered_map<std::string,float> costMap; 
        std::vector<std::vector<float>> getCost()const;
        float operator()(int x,int y)const;
        Terrain()=delete;
        Terrain(const Terrain& other)=default;
        Terrain& operator=(const Terrain& other)=default;
        Terrain(Terrain&& other)=default;
        Terrain& operator=(Terrain&& other)=default;
        Terrain(const std::vector<std::vector<std::string>>& terrain,const std::unordered_map<std::string,float>& costs)
        :storage(terrain),costMap(costs){};
        Terrain(const std::string& init,size_t width,size_t height,const std::unordered_map<std::string,float>& costs):costMap(costs){
            storage.assign(height,std::vector<std::string>(width,init));
        };
    };
    struct AIBehavior{
        std::vector<std::function<bool(const std::unordered_map<std::string,std::any>&,const std::unordered_set<std::string>&)>> conditions;
        std::function<void(std::unordered_map<std::string,std::any>&,std::unordered_set<std::string>&)> behavior;
        bool check(const std::unordered_map<std::string,std::any>&,const std::unordered_set<std::string>&)const;
    };
    struct BasicLife{
        private:
        std::unordered_map<std::string,std::any> attributes;
        std::vector<AIBehavior> behaviors;
        public:
        bool alive=true;
        std::function<void(std::unordered_map<std::string,std::any>&,std::unordered_set<std::string>&)> updateFunction;
        std::unordered_set<std::string> status;
        template<typename T>
        T getAttribute(const std::string& key,T defaultValue=T())const{
            if(!attributes.count(key)){return defaultValue;}
            try{
                T res=std::any_cast<T>(attributes.at(key));
                return res;
            }catch(std::bad_any_cast bac){
                gameLogger.traceLog(Core::logger::LOG_ERROR,"Type error,maybe you stored something different before.",std::source_location::current());
                gameLogger.traceLog(Core::logger::LOG_NOTE,bac.what(),std::source_location::current());
                return defaultValue;
            }
        }
        template<typename T>
        bool setAttribute(const std::string& key,T val){
            try{
                attributes[key]=val;
                return true;
            }catch(std::exception e){
                gameLogger.traceLog(Core::logger::LOG_ERROR,"Something happened",std::source_location::current());
                gameLogger.formatLog(Core::logger::LOG_NOTE,"what():%s",std::source_location::current(),e.what());
                return false;
            }
        }
        void pushBehavior(const AIBehavior& beh);
        void update();
        BasicLife()=delete;
        BasicLife(std::unordered_map<std::string,std::any> attr,std::vector<AIBehavior> behs,
                  std::function<void(std::unordered_map<std::string,std::any>&,std::unordered_set<std::string>&)> updateFunc,
                  std::unordered_set<std::string> status):attributes(std::move(attr)),behaviors(std::move(behs)),
                                                          updateFunction(std::move(updateFunc)),status(std::move(status)){};
        ~BasicLife()=default;
    };
    struct LifePool{
        std::vector<std::shared_ptr<BasicLife>> pool;
        std::vector<size_t> freeIndices;
        LifePool(size_t reserveCnt){
            pool.reserve(reserveCnt);
            freeIndices.reserve(reserveCnt);
        }
        ~LifePool(){
            pool.clear();
            freeIndices.clear();
        }
        std::pair<std::weak_ptr<BasicLife>,size_t> spawn(BasicLife&& l){
            if(freeIndices.empty()){
                pool.emplace_back(std::make_shared<BasicLife>(std::move(l)));
                return std::make_pair(pool.back(),pool.size()-1);
            }
            size_t index=freeIndices.back();
            freeIndices.pop_back();
            pool[index].reset();
            pool[index]=std::make_shared<BasicLife>(std::move(l));
            return std::make_pair(pool[index],index);
        }
        void despawn(size_t index){
            pool[index]->alive=false;
            freeIndices.push_back(index);
        }
    };
}

#endif