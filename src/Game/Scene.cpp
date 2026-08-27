//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Scene.hpp"

namespace Game{
    void SceneManager::enter(Scene s){
        if(s.onEnter){
            s.onEnter();
        }
        scenes.push(s);
    }
    void SceneManager::exit(){
        Scene s=scenes.top();
        if(s.onExit){
            s.onExit();
        }
        scenes.pop();
    }
    Scene SceneManager::getScene(){
        return scenes.top();
    }
    void SceneManager::update(float delta){
        Scene s=scenes.top();
        if(s.onUpdate){
            s.onUpdate(delta);
        }
    }
}