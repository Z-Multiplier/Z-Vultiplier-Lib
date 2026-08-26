//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef CLOCK_HPP
#define CLOCK_HPP
#include <vector>
#include <chrono>
#include <thread>
#include <functional>
namespace Core{
    struct Clock{
        struct FPSCounter{
            private:
                int frameCount=0;
                double fps=0;
                std::chrono::steady_clock::time_point lastTime;
            public:
                void tick(){
                    frameCount++;
                    auto now=std::chrono::steady_clock::now();
                    double elapsed=std::chrono::duration<double>(now-lastTime).count();
                    if(elapsed>=1.0){
                        fps=frameCount/elapsed;
                        frameCount=0;
                        lastTime=now;
                    }
                }
                double getFPS()const{return fps;}
        }fpsCounter;
        bool running;
        std::chrono::steady_clock::time_point nextFrame;
        std::chrono::nanoseconds gap;
        std::function<void()> loop;
        Clock()=delete;
        Clock(std::function<void()> loop,long long FPS=60);
        void run();
        operator bool()const{
            return running;
        }
        double fps(){return this->fpsCounter.getFPS();}
        void stop(){this->running=false;}
        Clock(Clock& other)=delete;
        Clock(Clock&& other)=delete;
        Clock operator=(Clock& other)=delete;
        Clock operator=(Clock&& other)=delete;
        ~Clock()=default;
    };
}
#endif