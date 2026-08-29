//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef SCRIPT_HPP
#define SCRIPT_HPP

#include <string>
#include <vector>
#include "lua.hpp"
extern "C"{
    #include "lualib.h"
}
#include "LuaBridge.h"
#include "Handle.hpp"
#include "Logger.hpp"

namespace Game{
    class ScriptEngine{
        public:
            static ScriptEngine& instance();
            bool initialize();
            void loadScript(const std::string& path);
            void callFunction(const std::string& name);
            void callFunction(const std::string& name,float delta);
            template<typename Pred>
            void registerFunction(const std::string& name,Pred pred){
                if(!initialized||!L){
                    Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                        "ScriptEngine not initialized, cannot register function: "+name,
                        std::source_location::current());
                    return;
                }
                luabridge::getGlobalNamespace(L).addFunction(name.c_str(),pred);
            }
            void shutdown();
            lua_State* getState()const{return L;}
        private:
            ScriptEngine(){
                initialize();
            }
            ~ScriptEngine(){
                shutdown();
            }
            lua_State* L;
            bool initialized;
    };
    class Engine{
            std::unique_ptr<Core::Initializer> init;
            struct CallbackRefs{
                int renderRef=LUA_REFNIL;
                int keyRef=LUA_REFNIL;
                int mouseRef=LUA_REFNIL;
                int cursorRef=LUA_REFNIL;
                int scrollRef=LUA_REFNIL;
            };
            Engine(const std::string& appname){initialize(appname);}
            ~Engine()=default;
        public:
            int updateRef=LUA_REFNIL;
            std::map<std::string,std::pair<std::shared_ptr<Window::Handle>,CallbackRefs>> titleToHandle;
            static Engine& instance(const std::string& appname){
                static Engine e(appname);
                return e;
            }
            bool initialize(const std::string& appname);
            bool createWindow(const std::string& title,int width,int height);
            bool closeWindow(const std::string& title);
            
            void setUpdateCallback(int funcRef){
                if(updateRef!=LUA_REFNIL){
                    luaL_unref(Game::ScriptEngine::instance().getState(),LUA_REGISTRYINDEX,updateRef);
                }
                updateRef=funcRef;
            }
            void setRenderCallback(int funcRef,const std::string& title){
                if(titleToHandle[title].second.renderRef!=LUA_REFNIL){
                    luaL_unref(Game::ScriptEngine::instance().getState(),LUA_REGISTRYINDEX,titleToHandle[title].second.renderRef);
                }
                titleToHandle[title].second.renderRef=funcRef;
            }
            void setKeyCallback(int funcRef,const std::string& title){
                if(titleToHandle[title].second.keyRef!=LUA_REFNIL){
                    luaL_unref(Game::ScriptEngine::instance().getState(),LUA_REGISTRYINDEX,titleToHandle[title].second.keyRef);
                }
                titleToHandle[title].second.keyRef=funcRef;
            }
            void setMouseButtonCallback(int funcRef,const std::string& title){
                if(titleToHandle[title].second.mouseRef!=LUA_REFNIL){
                    luaL_unref(Game::ScriptEngine::instance().getState(),LUA_REGISTRYINDEX,titleToHandle[title].second.mouseRef);
                }
                titleToHandle[title].second.mouseRef=funcRef;
            }
            void setCursorPosCallback(int funcRef,const std::string& title){
                if(titleToHandle[title].second.cursorRef!=LUA_REFNIL){
                    luaL_unref(Game::ScriptEngine::instance().getState(),LUA_REGISTRYINDEX,titleToHandle[title].second.cursorRef);
                }
                titleToHandle[title].second.cursorRef=funcRef;
            }
            void setScrollCallback(int funcRef,const std::string& title){
                if(titleToHandle[title].second.scrollRef!=LUA_REFNIL){
                    luaL_unref(Game::ScriptEngine::instance().getState(),LUA_REGISTRYINDEX,titleToHandle[title].second.scrollRef);
                }
                titleToHandle[title].second.scrollRef=funcRef;
            }
            std::shared_ptr<Window::Handle> getPtr(const std::string& title)const{
                return titleToHandle.at(title).first;
            }
    };
}

#endif