//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "UI.hpp"

namespace UI{
    void Widget::render(Render::Painter& p)const{
        p.drawRoundedRect({x,y},{width,height},borderRoundness,borderColor);
        p.drawRoundedRect({x+borderThickness,y+borderThickness},
                        {width-2*borderThickness,height-2*borderThickness},
                        borderRoundness,bodyColor);
        auto textSize=p.measureText(*font,text,fontScale);
        float textX=x+(width-textSize.x)/2.0f;
        float textY=y+(height+textSize.y)/2.0f;
        p.drawText(*font,text,{textX,textY},textColor,fontScale);
        for(const auto& child:children){
            child->render(p);
        }
    }
    std::vector<const Widget*> Widget::collect()const{
        std::vector<const Widget*> ret;
        for(const auto& child:children){
            std::vector<const Widget*> tgt=child->collect();
            ret.insert(ret.end(),tgt.begin(),tgt.end());
        }
        ret.push_back(this);
        return ret;
    }
    void Widget::onMouseEvent(float x,float y,Widget::MouseEvent e){
        for(auto& child:children){
            child->onMouseEvent(x,y,e);
        }
        switch(e){
            case Widget::MouseEvent::NOEVENT:if(this->onHover)this->onHover(x,y);break;
            case Widget::MouseEvent::DOWN:if(this->onClick)this->onClick(x,y);break;
            case Widget::MouseEvent::RELEASE:if(this->onRelease)this->onRelease(x,y);break;
        }
    }
}