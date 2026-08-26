//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Logger.hpp"
#include "Clock.hpp"
#include "Handle.hpp"
#include "Audio.hpp"
void Core::Clock::run(){
    this->fpsCounter.tick();
    if(!running){
        return;
    }
    auto now=std::chrono::steady_clock::now();
    
    auto& manager=Window::WindowManager::instance();
    const auto& windows=manager.getActiveWindows();

    for(const auto& window:windows){
        if(glfwGetWindowAttrib(window->getNativeHandle(),GLFW_ICONIFIED)){
            window->pollEvents();
            continue;
        }
        if(!window->shouldClose()){
            window->pollEvents();
        }
    }

    std::vector<std::shared_ptr<Window::Handle>> toClose;
    for(const auto& window:windows){
        if(window->shouldClose()){
            toClose.push_back(window);
        }
    }

    for(auto& window:toClose){
        window->close();
        manager.destroy(window);
    }

    const auto& newWindows=manager.getActiveWindows();

    bool anyOpen=false;
    for(const auto& window:newWindows){
        if(!window->shouldClose()){
            anyOpen=true;
            break;
        }
    }

    if(!anyOpen){
        running=false;
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Clock stopped: No open windows.",
            std::source_location::current()
        );
        return;
    }

    for(const auto& window:newWindows){
        if(glfwGetWindowAttrib(window->getNativeHandle(),GLFW_ICONIFIED)){
            continue;
        }
        if(!window->shouldClose()){
            window->updateInput();
            window->renderFrame();
        }
    }

    Audio::Engine::instance().update();

    if(loop){
        loop();
    }

    std::this_thread::sleep_until(nextFrame);
    Audio::Engine::instance().update();
}
Core::Clock::Clock(std::function<void()> loop,long long FPS):running(true),loop(loop){
    long long g=1e9/FPS;
    this->gap=std::chrono::nanoseconds(g);
    nextFrame=std::chrono::steady_clock::now()+gap;
}