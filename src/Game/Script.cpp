//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Script.hpp"
#include <stack>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "Logger.hpp"
#include "Painter.hpp"
#include "Audio.hpp"
#include "Utils.hpp"
#include "Matrix.hpp"
#include <glm/glm.hpp>
namespace Game{
    ScriptEngine& ScriptEngine::instance(){
        static ScriptEngine engine;
        return engine;
    }
    bool ScriptEngine::initialize(){
        if(initialized){
            Core::globalLogger.traceLog(Core::logger::LOG_WARNING,
                "ScriptEngine already initialized.",std::source_location::current());
            return true;
        }

        L=luaL_newstate();
        if(!L){
            Core::globalLogger.traceLog(Core::logger::LOG_FATAL,
                "Failed to create lua_State.",std::source_location::current());
            return false;
        }

        luaL_openlibs(L);

        lua_atpanic(L,[](lua_State* L)->int {
            Core::globalLogger.traceLog(Core::logger::LOG_FATAL,
                "Lua panic: "+std::string(lua_tostring(L,-1)),std::source_location::current());
            return 0;
        });

        luabridge::getGlobalNamespace(L)
        .beginClass<glm::vec2>("Vec2")
            .addConstructor<void(*)(float,float)>()
            .addProperty("x",&glm::vec2::x)
            .addProperty("y",&glm::vec2::y)
            .addFunction("length",[](const glm::vec2& v){
                return glm::length(v);
            })
            .addFunction("dot",[](const glm::vec2& a,const glm::vec2& b){
                return glm::dot(a,b);
            })
            .addFunction("normalize",[](const glm::vec2& v){
                return glm::normalize(v);
            })
        .endClass()
        .beginClass<glm::vec4>("Vec4")
            .addConstructor<void(*)(float,float,float,float)>()
            .addProperty("r",&glm::vec4::r)
            .addProperty("g",&glm::vec4::g)
            .addProperty("b",&glm::vec4::b)
            .addProperty("a",&glm::vec4::a)
            .addProperty("x",&glm::vec4::x)
            .addProperty("y",&glm::vec4::y)
            .addProperty("z",&glm::vec4::z)
            .addProperty("w",&glm::vec4::w)
        .endClass()
        .beginClass<glm::mat4>("Mat4")
            .addConstructor<void(*)()>()
            .addFunction("translate",[](glm::mat4 m,float x,float y){
                return m*Core::Matrix::translate(x,y);
            })
            .addFunction("rotate",[](glm::mat4 m,float angle){
                return m*Core::Matrix::rotate(angle);
            })
            .addFunction("scale",[](glm::mat4 m,float sx,float sy){
                return m*Core::Matrix::scale(sx,sy);
            })
            .addFunction("mul",[](const glm::mat4& a,const glm::mat4& b){
                return a*b;
            })
        .endClass()
        .beginClass<Engine>("Engine")
            .addStaticFunction("instance",Engine::instance)
            .addFunction("createWindow",Engine::createWindow)
            .addFunction("closeWindow",Engine::closeWindow)
            .addFunction("setUpdateCallback",[](Engine* self,lua_State* L){
                if(lua_isfunction(L,-1)){
                    int ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self->setUpdateCallback(ref);
                }
                else{
                    luaL_error(L,"setUpdateCallback expects a function");
                }
            })
            .addFunction("setRenderCallback",[](Engine* self,lua_State* L){
                const char* title=lua_tostring(L,-1);
                if(lua_isfunction(L,-2)){
                    lua_pushvalue(L,-2);
                    int ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self->setRenderCallback(ref,title);
                }
                else{
                    luaL_error(L,"setRenderCallback expects a function and a string");
                }
                lua_pop(L,2);
            })
            .addFunction("setKeyCallback",[](Engine* self,lua_State* L){
                const char* title=lua_tostring(L,-1);
                if(lua_isfunction(L,-2)){
                    lua_pushvalue(L,-2);
                    int ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self->setKeyCallback(ref,title);
                }
                else{
                    luaL_error(L,"setKeyCallback expects a function and a string");
                }
                lua_pop(L,2);
            })
            .addFunction("setMouseCallback",[](Engine* self,lua_State* L){
                const char* title=lua_tostring(L,-1);
                if(lua_isfunction(L,-2)){
                    lua_pushvalue(L,-2);
                    int ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self->setMouseButtonCallback(ref,title);
                }
                else{
                    luaL_error(L,"setMouseCallback expects a function and a string");
                }
                lua_pop(L,2);
            })
            .addFunction("setCursorCallback",[](Engine* self,lua_State* L){
                const char* title=lua_tostring(L,-1);
                if(lua_isfunction(L,-2)){
                    lua_pushvalue(L,-2);
                    int ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self->setCursorPosCallback(ref,title);
                }
                else{
                    luaL_error(L,"setCursorCallback expects a function and a string");
                }
                lua_pop(L,2);
            })
            .addFunction("setScrollCallback",[](Engine* self,lua_State* L){
                const char* title=lua_tostring(L,-1);
                if(lua_isfunction(L,-2)){
                    lua_pushvalue(L,-2);
                    int ref=luaL_ref(L,LUA_REGISTRYINDEX);
                    self->setScrollCallback(ref,title);
                }
                else{
                    luaL_error(L,"setScrollCallback expects a function and a string");
                }
                lua_pop(L,2);
            })
        .endClass()
        .beginClass<Render::Painter>("Painter")
            .addFunction("drawLine",Render::Painter::drawLine)
            .addFunction("drawTriangle",[](Render::Painter* self,glm::vec2 p1,glm::vec2 p2,glm::vec2 p3,glm::vec4 color){self->drawTriangle(p1,p2,p3,color);})
            .addFunction("drawRect",Render::Painter::drawRect)
            .addFunction("drawCircle",Render::Painter::drawCircle)
            .addFunction("drawPolygon",Render::Painter::drawPolygon)
            .addFunction("drawBezier",Render::Painter::drawBezier)
            .addFunction("drawEllipse",Render::Painter::drawEllipse)
            .addFunction("drawRegularPolygon",Render::Painter::drawRegularPolygon)
            .addFunction("drawRoundedRect",Render::Painter::drawRoundedRect)
            .addFunction("putImage",Render::Painter::putImage)
            .addFunction("drawText",Render::Painter::drawText)
            .addFunction("loadImage",Render::Painter::createImage)
            .addFunction("loadFont",Render::Painter::loadFont)
            .addFunction("resetTransform",Render::Painter::resetTransform)
            .addFunction("setTransform",Render::Painter::setTransform)
            .addFunction("setProjectionMatrix",Render::Painter::setProjectionMatrix)
            .addFunction("setViewMatrix",Render::Painter::setViewMatrix)
            .addFunction("enablePostProcess",Render::Painter::enablePostProcess)
            .addFunction("unablePostProcess",Render::Painter::unablePostProcess)
        .endClass()
        .beginClass<Render::Painter::Image>("Image")
            .addFunction("getWidth",Render::Painter::Image::getWidth)
            .addFunction("getHeight",Render::Painter::Image::getHeight)
        .endClass()
        .beginClass<Render::Painter::Font>("Font")
            .addFunction("valid",Render::Painter::Font::isValid)
        .endClass()
        .beginClass<Audio::Engine>("AudioEngine")
            .addStaticFunction("instance",&Audio::Engine::instance)
            .addStaticFunction("playSound",[](const std::string& file){Audio::Engine::instance().playSound(Audio::Sound(file));})
        .endClass()
        .beginClass<Utils::Random>("Random")
            .addConstructor<void(*)()>()
            .addFunction("range",Utils::Random::range)
            .addFunction("real",Utils::Random::real)
        .endClass()
        .beginClass<Utils::Timer>("Timer")
            .addConstructor<void(*)()>()
            .addFunction("reset",Utils::Timer::reset)
            .addFunction("reached",Utils::Timer::reached)
            .addFunction("elapsed",Utils::Timer::elapsed)
        .endClass()
        .addFunction("ortho",[](float left,float right,float bottom,float top){
            return glm::ortho(left,right,bottom,top,-1.0f,1.0f);
        })
        .addFunction("identity",[](){
            return glm::mat4(1.0f);
        })
        .addFunction("mat4",[](){
            return glm::mat4(1.0f);
        });

        lua_pushstring(L,"1.0");
        lua_setglobal(L,"ENGINE_VERSION");

        initialized=true;
        Core::globalLogger.traceLog(Core::logger::LOG_INFO,
            "ScriptEngine initialized successfully.",std::source_location::current());
        return true;
    }
    void ScriptEngine::loadScript(const std::string& path){
        if(!initialized){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "ScriptEngine not initialized before loading script.",
                std::source_location::current());
            return;
        }
        if(luaL_dofile(L,path.c_str())!=0){
            std::string err=lua_tostring(L,-1);
            lua_pop(L,1);
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Failed to load script: "+path+" - "+err,
                std::source_location::current());
            return;
        }
        Core::globalLogger.traceLog(Core::logger::LOG_INFO,
            "Script loaded: "+path,
            std::source_location::current());
    }
    void ScriptEngine::callFunction(const std::string& name){
        if(!initialized) return;
        lua_getglobal(L,name.c_str());

        if(!lua_isfunction(L,-1)){
            Core::globalLogger.traceLog(Core::logger::LOG_WARNING,
                "Global function '"+name+"' is not a function.",
                std::source_location::current());
            lua_pop(L,1);
            return;
        }

        if(lua_pcall(L,0,0,0)!=0){
            std::string err=lua_tostring(L,-1);
            lua_pop(L,1);
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Error calling function '"+name+"': "+err,
                std::source_location::current());
        }
    }
    void ScriptEngine::callFunction(const std::string& name,float delta){
        if(!initialized) return;

        lua_getglobal(L,name.c_str());
        if(!lua_isfunction(L,-1)){
            Core::globalLogger.traceLog(Core::logger::LOG_WARNING,
                "Global function '"+name+"' is not a function.",
                std::source_location::current());
            lua_pop(L,1);
            return;
        }

        lua_pushnumber(L,delta);

        if(lua_pcall(L,1,0,0)!=0){
            std::string err=lua_tostring(L,-1);
            lua_pop(L,1);
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Error calling function '"+name+"': "+err,
                std::source_location::current());
        }
    }
    void ScriptEngine::shutdown(){
        if(L){
            lua_close(L);
            L=nullptr;
        }
        initialized=false;
        Core::globalLogger.traceLog(Core::logger::LOG_INFO,
            "ScriptEngine shut down.",
            std::source_location::current());
    }
    bool Engine::initialize(const std::string& appname){
        if(init){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Engine initialized before.",
                std::source_location::current());
            return false;
        }
        init=std::make_unique<Core::Initializer>(appname);
        return true;
    }
    bool Engine::createWindow(const std::string& title,int width,int height){
        if(!init){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Engine not initialized before.",
                std::source_location::current());
            return false;
        }
        auto& manager=Window::WindowManager::instance();
        auto window=manager.create(*init,width,height,title);
        if(!window){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Failed to create window: "+title,
                std::source_location::current());
            return false;
        }
        titleToHandle[title]=std::make_pair(std::move(window),Game::Engine::CallbackRefs());
        return true;
    }
    bool Engine::closeWindow(const std::string& title){
        if(!init){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,
                "Engine not initialized before.",
                std::source_location::current());
            return false;
        }
        titleToHandle[title].first->close();
        Window::WindowManager::instance().destroy(titleToHandle[title].first);
        titleToHandle.erase(title);
        return true;
    }
}