#define QUICK_DEBUG
#include "Z-Vultiplier.hpp"
void drawBody(Render::Painter& painter,Physics::PhysicsBody& body,
              const glm::vec4& color,float pixelsPerMeter){
    auto vertices=body.getVertices();
    glm::vec2 pos=body.getPosition();
    float angle=body.getAngle();

    for(const auto& poly:vertices){
        std::vector<glm::vec2> screenVerts;
        screenVerts.reserve(poly.size());
        for(const auto& v:poly){
            screenVerts.push_back({
                v.x*pixelsPerMeter,
                v.y*pixelsPerMeter
            });
        }

        glm::vec2 screenPos={
            pos.x*pixelsPerMeter,
            pos.y*pixelsPerMeter
        };

        painter.setTransform(Core::Matrix::model(screenPos,-angle,{1.0f,1.0f}));
        painter.drawPolygon(screenVerts,color);
    }
}
int main(){
    freopen("console.log","w",stderr);

    try {
        Core::Initializer init("Physics + Rendering");
        auto& manager=Window::WindowManager::instance();
        auto window=manager.create(init,1280,800,"Physics + Rendering");

        Physics::PhysicsWorld world({0.0f,-9.8f});
        world.startup();

        std::vector<glm::vec2> groundPolygon={
            {-10.0f,-1.0f},
            {10.0f,-1.0f},
            {10.0f,0.0f},
            {-10.0f,0.0f}
        };
        Physics::PhysicsBody ground(
            world.getId(),
            {12.8f,1.0f},
            groundPolygon,
            0.0f,0.5f,
            true,
            true
        );

        float halfSize=0.5f;
        std::vector<glm::vec2> boxPolygon={
            {-halfSize,-halfSize},
            { halfSize,-halfSize},
            { halfSize, halfSize},
            {-halfSize, halfSize}
        };
        Physics::PhysicsBody box(
            world.getId(),
            {12.8f,7.0f},
            boxPolygon,
            1.0f,0.3f,
            false,
            true
        );

        window->setRenderCallback([&](Render::Painter& painter){
            int w=window->getWidth();
            int h=window->getHeight();

            painter.setProjectionMatrix(Core::Matrix::ortho(0.0f,(float)w,(float)h,0.0f));
            painter.setViewMatrix(Core::Matrix::identity());

            painter.resetTransform();
            painter.drawRect({0,0},{(float)w,(float)h},{0.15f,0.15f,0.2f,1.0f});

            drawBody(painter,ground,{0.4f,0.6f,0.8f,1.0f},Physics::pixelsPerMeter);

            drawBody(painter,box,{1.0f,0.4f,0.4f,0.8f},Physics::pixelsPerMeter);
        });
        window->setKeyCallback([&](int key,int scancode,int action,int mods){
            if(key==' '){
                box.applyLinearImpulseToCenter({0.0f,5.0f},true);
            }
        });

        Core::Clock clock(nullptr,60);
        while (clock){
            const float timeStep=1.0f/60.0f;
            const int subStepCount=4;
            float physicsAccumulator=0.0f;

            physicsAccumulator+=1.0f/60.0f;
            while (physicsAccumulator>=timeStep){
                world.step(timeStep,subStepCount);
                physicsAccumulator-=timeStep;
            }
            clock.run();
        }

    }catch(const std::exception& e){
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return -1;
    }
    return 0;
}