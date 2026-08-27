//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef SCENE_HPP
#define SCENE_HPP
#include <functional>
#include <stack>

namespace Game{
    struct Scene{
        Scene()=default;
        Scene(std::function<void()> enter,std::function<void()> exit,std::function<void(float)> update)
        :onEnter(enter),onExit(exit),onUpdate(update){};
        ~Scene()=default;
        std::function<void()> onEnter;
        std::function<void()> onExit;
        std::function<void(float)> onUpdate;
    };
    struct SceneManager{
        public:
            SceneManager()=default;
            ~SceneManager()=default;
            void enter(Scene s);
            void exit();
            Scene getScene();
            void update(float delta);
        private:
            std::stack<Scene> scenes;
    };
}

#endif