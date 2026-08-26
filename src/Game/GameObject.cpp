//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "GameObject.hpp"
namespace Game{
   Core::logger gameLogger;
}
bool Game::AIBehavior::check(const std::unordered_map<std::string,std::any>& attributes,const std::unordered_set<std::string>& status)const{
    for(const auto& f:this->conditions){
        if(!f(attributes,status)){
            return false;
        }
    }
    return true;
}
void Game::BasicLife::pushBehavior(const AIBehavior& beh){
    this->behaviors.emplace_back(beh);
}
void Game::BasicLife::update(){
    this->updateFunction(attributes,status);
    for(const auto& beh:this->behaviors){
        if(beh.check(attributes,status)){
            beh.behavior(attributes,status);
            return;
        }
    }
}
std::vector<std::vector<float>> Game::Terrain::getCost()const{
    if(storage.empty()||storage[0].empty()) return{};
    std::vector<std::vector<float>> ret(storage.size(),std::vector<float>(storage[0].size(),Unpassable));
    for(size_t y=0;y<storage.size();y++){
        for(size_t x=0;x<storage[y].size();x++){
            ret[y][x]=costMap.at(storage[y][x]);
        }
    }
    return ret;
}
float Game::Terrain::operator()(int x,int y)const{
    if(storage.empty()||storage[0].empty())
        return Unpassable;
    if(x<0||static_cast<size_t>(x)>=storage[0].size()||
       y<0||static_cast<size_t>(y)>=storage.size())
        return Unpassable;
    auto it=costMap.find(storage[y][x]);
    return (it!=costMap.end())?it->second:Unpassable;
}