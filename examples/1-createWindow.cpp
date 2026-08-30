#include "Z-Vultiplier.hpp"

int main(){
    Core::Initializer init("Quick Start");//Initializer.
    auto& manager=Window::WindowManager::instance();//Manager instance.
    auto window=manager.create(init,800,800,"Title");//w&h

    //Setup stage done.

    Core::Clock clock([&](){
        return;
    },60);//loop,FPS

    while(clock){
        clock.run();
    }
    //done!
    return 0;
}