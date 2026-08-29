//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Z-Vultiplier.hpp"
//IoC!
extern void UserLuaContact();
extern void UserInit();
extern void UserUpdate();
extern long long maxFPS;
extern std::string appname;
extern std::string luaScriptPath;
long long FPSnow;
int main(int argc,char** argv){
    UserLuaContact();
    Game::ScriptEngine& se=Game::ScriptEngine::instance();
    se.loadScript(luaScriptPath.c_str());
    Core::Clock c(UserUpdate,maxFPS);
    for(auto& [t,w]:Game::Engine::instance(appname).titleToHandle){
        auto& [win,ref]=w;
        win->setRenderCallback([&](Render::Painter& p){
            if(ref.renderRef==LUA_REFNIL) return;
            lua_State* L=Game::ScriptEngine::instance().getState();
            lua_rawgeti(L,LUA_REGISTRYINDEX,ref.renderRef);
            auto res=luabridge::push(L,&p);
            if(lua_pcall(L,1,0,0)!=0){
                std::string err=lua_tostring(L,-1);
                lua_pop(L,1);
                Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                    "Render callback error: "+err,
                    std::source_location::current());
            }
        });
        win->setKeyCallback([&](int key,int scancode,int action,int mods){
            if(ref.keyRef==LUA_REFNIL) return;
            lua_State* L=Game::ScriptEngine::instance().getState();
            lua_rawgeti(L,LUA_REGISTRYINDEX,ref.keyRef);
            auto res=luabridge::push(L,&key);
            res=luabridge::push(L,&scancode);
            res=luabridge::push(L,&action);
            res=luabridge::push(L,&mods);
            if(lua_pcall(L,4,0,0)!=0){
                std::string err=lua_tostring(L,-1);
                lua_pop(L,1);
                Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                    "Key callback error: "+err,
                    std::source_location::current());
            }
        });
        win->setMouseButtonCallback([&](int button,int action,int mods){
            if(ref.mouseRef==LUA_REFNIL) return;
            lua_State* L=Game::ScriptEngine::instance().getState();
            lua_rawgeti(L,LUA_REGISTRYINDEX,ref.mouseRef);
            auto res=luabridge::push(L,&button);
            res=luabridge::push(L,&action);
            res=luabridge::push(L,&mods);
            if(lua_pcall(L,3,0,0)!=0){
                std::string err=lua_tostring(L,-1);
                lua_pop(L,1);
                Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                    "MouseButton callback error: "+err,
                    std::source_location::current());
            }
        });
        win->setCursorPosCallback([&](double x,double y){
            if(ref.cursorRef==LUA_REFNIL) return;
            lua_State* L=Game::ScriptEngine::instance().getState();
            lua_rawgeti(L,LUA_REGISTRYINDEX,ref.cursorRef);
            auto res=luabridge::push(L,&x);
            res=luabridge::push(L,&y);
            if(lua_pcall(L,2,0,0)!=0){
                std::string err=lua_tostring(L,-1);
                lua_pop(L,1);
                Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                    "CursorPos callback error: "+err,
                    std::source_location::current());
            }
        });
        win->setScrollCallback([&](double xoffset,double yoffset){
            if(ref.scrollRef==LUA_REFNIL) return;
            lua_State* L=Game::ScriptEngine::instance().getState();
            lua_rawgeti(L,LUA_REGISTRYINDEX,ref.scrollRef);
            auto res=luabridge::push(L,&xoffset);
            res=luabridge::push(L,&yoffset);
            if(lua_pcall(L,2,0,0)!=0){
                std::string err=lua_tostring(L,-1);
                lua_pop(L,1);
                Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                    "Scroll callback error: "+err,
                    std::source_location::current());
            }
        });
    }
    UserInit();
    while(c.running){
        lua_State* L=Game::ScriptEngine::instance().getState();
        if(Game::Engine::instance(appname).updateRef!=LUA_REFNIL){
        lua_rawgeti(L,LUA_REGISTRYINDEX,Game::Engine::instance(appname).updateRef);
            if(lua_pcall(L,0,0,0)!=0){
                std::string err=lua_tostring(L,-1);
                lua_pop(L,1);
                Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                    "Scroll callback error: "+err,
                    std::source_location::current());
            }
        }
        c.run();
        FPSnow=c.fps();
    }
    return 0;
}