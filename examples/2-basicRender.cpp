#include "Z-Vultiplier.hpp"

int main(){
    Core::Initializer init("Quick Start");//Initializer.
    auto& manager=Window::WindowManager::instance();//Manager instance.
    auto window=manager.create(init,800,800,"Title");//w&h

    //Setup stage done.

    window->setRenderCallback([&](Render::Painter& painter){
        //Render callback function
        //1. Setup projection & view matrix
        glm::mat4 proj=glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);//NDC
        glm::mat4 view=glm::mat4(1.0f);
        painter.setProjectionMatrix(proj);
        painter.setViewMatrix(view);

        //2. Drawing
        painter.resetTransform();//Remember this!
        painter.drawTriangle({-0.6f,-0.5f},
                             {0.0f, 0.6f},
                             {0.6f,-0.5f},
                             {1.0f,0.0f,0.0f,1.0f},
                             {0.0f,1.0f,0.0f,1.0f},
                             {0.0f,0.0f,1.0f,1.0f});
        //pos1,pos2,pos3,color1,color2,color3
        //Either Core::Color & glm::vec4 is avaliable
    });
    //Window setting done.

    Core::Clock clock([&](){
        return;
    },60);//loop,FPS

    while(clock){
        clock.run();
    }
    //done!
    return 0;
}