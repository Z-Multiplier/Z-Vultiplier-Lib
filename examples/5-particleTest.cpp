#include "Z-Vultiplier.hpp"
int main(){
    Core::Initializer init("Quick Start");
    auto& manager=Window::WindowManager::instance();
    auto window=manager.create(init,800,800,"Title");
    Render::ParticlePool pp;
    Render::Painter::Image* pimg;
    pp.init(1024);
    window->setRenderCallback([&](Render::Painter& painter){
        glm::mat4 proj=glm::ortho(0.0f,800.0f,800.0f,0.0f,-1.0f,1.0f);
        glm::mat4 view=glm::mat4(1.0f);

        static Render::Painter::Image& img=painter.createImage("./assets/icon1.png");
        pimg=&img;

        painter.setProjectionMatrix(proj);
        painter.setViewMatrix(view);

        pp.render(painter);
    });

    window->setKeyCallback([&](int key,int scancode,int action,int mods){
        if(key==' '&&pimg!=nullptr){
            Render::spawnWind(pp,{400,400},{600,500},0.02f,30.0f,100,pimg,{50,50});
        }
    });

    Core::Clock clock([&](){
        pp.update(1.0/60);
        return;
    },60);

    while(clock){
        clock.run();
    }
    return 0;
}