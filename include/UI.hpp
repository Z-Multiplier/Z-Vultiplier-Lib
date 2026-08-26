//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef UI_HPP
#define UI_HPP

#include "Color.hpp"
#include "Painter.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace UI{
    struct Widget{
        Core::Color bodyColor={(unsigned char)0,0,0,0};
        Core::Color borderColor={(unsigned char)0,0,0,0};
        float borderThickness=0.0f;
        float borderRoundness=0.0f;
        float x=0.0f;
        float y=0.0f;
        float width=0.0f;
        float height=0.0f;
        float fontScale=0.0f;
        std::string text="";
        Core::Color textColor={(unsigned char)0,0,0,0};
        Render::Painter::Font* font=nullptr;
        std::vector<std::unique_ptr<Widget>> children;
        Widget* parent=nullptr;
        void render(Render::Painter& p)const;
        std::vector<const Widget*> collect()const;
        enum class MouseEvent{
            NOEVENT,
            DOWN,
            RELEASE
        };
        void onMouseEvent(float x,float y,MouseEvent e);
        std::function<void(float,float)> onHover;
        std::function<void(float,float)> onClick;
        std::function<void(float,float)> onRelease;

        Widget()=default;
        ~Widget()=default;
        Widget(const Widget&)=default;
        Widget(Widget&&)=default;
        Widget& operator=(const Widget&)=default;
        Widget& operator=(Widget&&)=default;
    };
}

#endif