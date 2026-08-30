#include "Z-Vultiplier.hpp"
long long maxFPS=60;
std::string appname="test";
std::string luaScriptPath="lua/main.lua";
void luaPrint(const std::string& str){
    Core::globalLogger.traceLog(
        Core::logger::LOG_INFO,
        str,
        std::source_location::current()
    );
}
void UserLuaContact(){
    freopen("console.log","w",stderr);
    Game::ScriptEngine::instance().registerFunction("luaPrint",luaPrint);
}
void UserInit(){
    /*auto& e=Game::Engine::instance(appname);
    e.createWindow("title",800,800);
    auto ptr=e.getPtr("title");
    ptr->setRenderCallback([&](Render::Painter& painter){
        glm::mat4 proj=glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);
        glm::mat4 view=glm::mat4(1.0f);
        painter.setProjectionMatrix(proj);
        painter.setViewMatrix(view);

        painter.resetTransform();
        painter.drawTriangle({-0.6f,-0.5f},
                             {0.0f, 0.6f},
                             {0.6f,-0.5f},
                             {1.0f,0.0f,0.0f,1.0f},
                             {0.0f,1.0f,0.0f,1.0f},
                             {0.0f,0.0f,1.0f,1.0f});
    });*/
}
void UserUpdate(){
    return;
}