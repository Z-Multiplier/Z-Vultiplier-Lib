//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Handle.hpp"
#include "Painter.hpp"
#include "Logger.hpp"

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <source_location>

bool Window::InputState::isKeyPressed(int key)const{
    auto it=keyPressed.find(key);
    return it!=keyPressed.end()&&it->second;
}

bool Window::InputState::isKeyJustPressed(int key)const{
    auto it=keyJustPressed.find(key);
    return it!=keyJustPressed.end()&&it->second;
}

glm::vec2 Window::InputState::getMousePosition()const{
    return glm::vec2(static_cast<float>(mouseX),static_cast<float>(mouseY));
}

glm::vec2 Window::InputState::getMouseDelta()const{
    return glm::vec2(
        static_cast<float>(mouseX-prevMouseX),
        static_cast<float>(mouseY-prevMouseY)
    );
}

bool Window::InputState::isMouseButtonPressed(int button)const{
    auto it=mouseButtonPressed.find(button);
    return it!=mouseButtonPressed.end()&&it->second;
}

bool Window::InputState::isMouseButtonJustPressed(int button)const{
    auto it=mouseButtonJustPressed.find(button);
    return it!=mouseButtonJustPressed.end()&&it->second;
}

double Window::InputState::getScrollOffset()const{
    return scrollOffset;
}

void Window::InputState::clearFrameState(){
    keyJustPressed.clear();
    keyJustReleased.clear();
    mouseButtonJustPressed.clear();
}

void Window::InputState::updateMousePosition(double x,double y){
    prevMouseX=mouseX;
    prevMouseY=mouseY;
    mouseX=x;
    mouseY=y;
}

void Window::InputState::accumulateScroll(double offset){
    scrollOffset+=offset;
}

namespace Window{
    Handle::Handle(Core::Initializer& init,int width,int height,const std::string& title)
        :thiswidth(width)
        ,thisheight(height)
        ,thistitle(title){

        glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
        thishandle=glfwCreateWindow(width,height,title.c_str(),nullptr,nullptr);
        if(!thishandle){
            throw std::runtime_error("Failed to create GLFW window!");
        }

        glfwSetWindowUserPointer(thishandle,this);

        glfwSetKeyCallback(thishandle,Handle::keyCallback);
        glfwSetMouseButtonCallback(thishandle,Handle::mouseButtonCallback);
        glfwSetCursorPosCallback(thishandle,Handle::cursorPosCallback);
        glfwSetScrollCallback(thishandle,Handle::scrollCallback);

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "GLFW window created:"+std::to_string(width)+"x"+std::to_string(height),
            std::source_location::current()
        );

        thiscontext=std::make_unique<WindowContext>(init,thishandle,width,height);
    }
    Handle::~Handle(){
        thiscontext.reset();

        if(thishandle){
            glfwDestroyWindow(thishandle);
            thishandle=nullptr;
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Window handle destroyed.",
            std::source_location::current()
        );
    }
    void Handle::keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods){
        auto* handle=static_cast<Handle*>(glfwGetWindowUserPointer(window));
        if(handle){
            handle->onKeyEvent(key,scancode,action,mods);
        }
    }
    void Handle::mouseButtonCallback(GLFWwindow* window,int button,int action,int mods){
        auto* handle=static_cast<Handle*>(glfwGetWindowUserPointer(window));
        if(handle){
            handle->onMouseButtonEvent(button,action,mods);
        }
    }
    void Handle::cursorPosCallback(GLFWwindow* window,double xpos,double ypos){
        auto* handle=static_cast<Handle*>(glfwGetWindowUserPointer(window));
        if(handle){
            handle->onCursorPosEvent(xpos,ypos);
        }
    }
    void Handle::scrollCallback(GLFWwindow* window,double xoffset,double yoffset){
        auto* handle=static_cast<Handle*>(glfwGetWindowUserPointer(window));
        if(handle){
            handle->onScrollEvent(xoffset,yoffset);
        }
    }

    void Handle::updateInput(){
        thisinputState.clearFrameState();
    }

    void Handle::onKeyEvent(int key,int scancode,int action,int mods){
        if(action==GLFW_PRESS){
            thisinputState.keyPressed[key]=true;
            thisinputState.keyJustPressed[key]=true;
        }
        else if(action==GLFW_RELEASE){
            thisinputState.keyPressed[key]=false;
            thisinputState.keyJustReleased[key]=true;
        }

        if(thiskeyCallback){
            thiskeyCallback(key,scancode,action,mods);
        }
    }
    void Handle::setKeyCallback(KeyCallback callback){
        thiskeyCallback=std::move(callback);
    }
    void Handle::onMouseButtonEvent(int button,int action,int mods){
        if(action==GLFW_PRESS){
            thisinputState.mouseButtonPressed[button]=true;
            thisinputState.mouseButtonJustPressed[button]=true;
        }else if(action==GLFW_RELEASE){
            thisinputState.mouseButtonPressed[button]=false;
            thisinputState.mouseButtonJustPressed[button]=true;
        }

        if(thismouseButtonCallback){
            thismouseButtonCallback(button,action,mods);
        }
    }
    void Handle::setMouseButtonCallback(MouseButtonCallback callback){
        thismouseButtonCallback=std::move(callback);
    }
    void Handle::onCursorPosEvent(double xpos,double ypos){
        thisinputState.updateMousePosition(xpos,ypos);

        if(thiscursorPosCallback){
            thiscursorPosCallback(xpos,ypos);
        }
    }
    void Handle::setCursorPosCallback(CursorPosCallback callback){
        thiscursorPosCallback=std::move(callback);
    }
    void Handle::onScrollEvent(double xoffset,double yoffset){
        thisinputState.accumulateScroll(xoffset);

        if(thisscrollCallback){
            thisscrollCallback(xoffset,yoffset);
        }
    }
    void Handle::setScrollCallback(ScrollCallback callback){
        thisscrollCallback=std::move(callback);
    }

    void Handle::pollEvents()const{
        glfwPollEvents();
    }
    bool Handle::shouldClose()const{
        return glfwWindowShouldClose(thishandle);
    }
    void Handle::setTitle(const std::string& title){
        thistitle=title;
        glfwSetWindowTitle(thishandle,title.c_str());
    }
    void Handle::setRenderCallback(std::function<void(Render::Painter&)> callback){
        thisrenderCallback=std::move(callback);
    }
    void Handle::renderFrame(){
        if(!thiscontext) return;

        auto& painter=thiscontext->getPainter();
        if(!painter) return;

        painter->beginFrame();

        if(thisrenderCallback){
            thisrenderCallback(*painter);
        }

        painter->endFrame();

        thiscontext->drawFrame();
    }
    WindowManager& WindowManager::instance(){
        static WindowManager instance;
        return instance;
    }

    std::shared_ptr<Handle> WindowManager::create(
        Core::Initializer& init,
        int width,
        int height,
        const std::string& title){

        auto handle=std::make_shared<Handle>(init,width,height,title);

        handles.emplace_back(handle);

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "WindowManager: Window created and registered. Total: "+std::to_string(handles.size()),
            std::source_location::current()
        );

        return handle;
    }

    void WindowManager::destroy(std::shared_ptr<Handle> handle){
        if(!handle) return;

        auto it=std::find_if(handles.begin(),handles.end(),
            [&handle](const std::weak_ptr<Handle>& wp){
                auto sp=wp.lock();
                return sp==handle;
            });

        if(it!=handles.end()){
            handles.erase(it);
            Core::globalLogger.traceLog(
                Core::logger::LOG_INFO,
                "WindowManager: Window destroyed and unregistered.",
                std::source_location::current()
            );
        }
    }

    void Handle::close(){
        if(!thishandle) return;
        thiscontext.reset();

        glfwDestroyWindow(thishandle);
        thishandle=nullptr;

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Window closed.",
            std::source_location::current()
        );
    }

    std::vector<std::shared_ptr<Handle>> WindowManager::getActiveWindows(){
        auto it=handles.begin();
        while(it!=handles.end()){
            if(it->expired()){
                it=handles.erase(it);
            }
            else{
                ++it;
            }
        }

        std::vector<std::shared_ptr<Handle>> active;
        active.reserve(handles.size());
        for(auto& wp:handles){
            auto sp=wp.lock();
            if(sp){
                active.push_back(sp);
            }
        }
        return active;
    }

    size_t WindowManager::count()const{
        return handles.size();
    }

    void WindowManager::closeAll(){
        auto windows=getActiveWindows();

        for(auto& window:windows){
            if(window){
                glfwSetWindowShouldClose(window->getNativeHandle(),GLFW_TRUE);
            }
        }

        windows.clear();

        getActiveWindows();

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "WindowManager: All windows closed.",
            std::source_location::current()
        );
    }
}