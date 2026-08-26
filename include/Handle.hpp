//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef HANDLE_HPP
#define HANDLE_HPP
#include <string>
#include <memory>
#include <functional>
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"
#include "glm/glm.hpp"
#include "WindowContext.hpp"

namespace Core{
    class Initializer;
}

namespace Window{
    struct InputState{
        std::unordered_map<int,bool> keyPressed;
        std::unordered_map<int,bool> keyJustPressed;
        std::unordered_map<int,bool> keyJustReleased;

        std::unordered_map<int,bool> mouseButtonPressed;
        std::unordered_map<int,bool> mouseButtonJustPressed;

        double mouseX=0.0,mouseY=0.0;
        double prevMouseX=0.0,prevMouseY=0.0;

        double scrollOffset=0.0;

        bool isKeyPressed(int key)const;
        bool isKeyJustPressed(int key)const;
        glm::vec2 getMousePosition()const;
        glm::vec2 getMouseDelta()const;
        bool isMouseButtonPressed(int button)const;
        bool isMouseButtonJustPressed(int button)const;
        double getScrollOffset()const;

        void clearFrameState();
        void updateMousePosition(double x,double y);
        void accumulateScroll(double offset);

        glm::vec2 getNDCPosition(int windowWidth,int windowHeight)const{
            return glm::vec2(
                (mouseX/(windowWidth/2.0f))-1.0f,
                ((mouseY/(windowHeight/2.0f))-1.0f)
            );
        }
    };
    class Handle{
        public:
            Handle(Core::Initializer& init,int width,int height,const std::string& title);
            ~Handle();
            Handle(const Handle&)=delete;
            Handle& operator=(const Handle&)=delete;
            Handle(Handle&&)=default;
            Handle& operator=(Handle&&)=default;

            void pollEvents()const;
            bool shouldClose()const;
            void setTitle(const std::string& title);

            int getWidth()const{return thiswidth;}
            int getHeight()const{return thisheight;}

            GLFWwindow* getNativeHandle()const{return thishandle;}

            void setRenderCallback(std::function<void(Render::Painter&)> callback);

            void renderFrame();

            void close();

            using KeyCallback=std::function<void(int key,int scancode,int action,int mods)>;
            using MouseButtonCallback=std::function<void(int button,int action,int mods)>;
            using CursorPosCallback=std::function<void(double x,double y)>;
            using ScrollCallback=std::function<void(double xoffset,double yoffset)>;

            void setKeyCallback(KeyCallback callback);
            void setMouseButtonCallback(MouseButtonCallback callback);
            void setCursorPosCallback(CursorPosCallback callback);
            void setScrollCallback(ScrollCallback callback);

            const InputState& getInputState()const{return thisinputState;}
            InputState& getInputState(){return thisinputState;}

            static void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods);
            static void mouseButtonCallback(GLFWwindow* window,int button,int action,int mods);
            static void cursorPosCallback(GLFWwindow* window,double xpos,double ypos);
            static void scrollCallback(GLFWwindow* window,double xoffset,double yoffset);

            void onKeyEvent(int key,int scancode,int action,int mods);
            void onMouseButtonEvent(int button,int action,int mods);
            void onCursorPosEvent(double x,double y);
            void onScrollEvent(double xoffset,double yoffset);

            Window::WindowContext* getContext()const{return thiscontext.get();}

            void updateInput();
        private:
            int thiswidth;
            int thisheight;
            std::string thistitle;
            GLFWwindow* thishandle=nullptr;
            std::unique_ptr<Window::WindowContext> thiscontext;
            void renderCallback(Render::Painter& painter);

            std::function<void(Render::Painter&)> thisrenderCallback;
            InputState thisinputState;

            KeyCallback thiskeyCallback;
            MouseButtonCallback thismouseButtonCallback;
            CursorPosCallback thiscursorPosCallback;
            ScrollCallback thisscrollCallback;
    };
    class WindowManager{
        public:
            static WindowManager& instance();

            std::shared_ptr<Handle> create(
                Core::Initializer& init,
                int width,
                int height,
                const std::string& title
            );

            void destroy(std::shared_ptr<Handle> handle);

            std::vector<std::shared_ptr<Handle>> getActiveWindows();

            size_t count()const;

            void closeAll();

            WindowManager(const WindowManager&)=delete;
            WindowManager& operator=(const WindowManager&)=delete;
            WindowManager(WindowManager&&)=delete;
            WindowManager& operator=(WindowManager&&)=delete;
            ~WindowManager()=default;
        private:
            WindowManager()=default;

            std::vector<std::weak_ptr<Handle>> handles;
    };
}

#endif