//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Animation.hpp"
#include "Logger.hpp"

namespace Render{
    glm::mat4 Animation::getMatrix(){
        if(keyframes.empty()){
            return glm::mat4(1.0f);
        }
        if(current>=duration){
            if(loop){
                current=std::fmod(current,duration);
            }
            else{
                const auto& last=keyframes.back();
                float x=0.0f,y=0.0f;
                float xsize=1.0f,ysize=1.0f;
                float rotation=0.0f;
                float xshear=0.0f,yshear=0.0f;
                for(const auto& [p,val,interp]:last.properties){
                    switch(p){
                        case Render::property::X_POS:x=val;break;
                        case Render::property::Y_POS:y=val;break;
                        case Render::property::X_SIZE:xsize=val;break;
                        case Render::property::Y_SIZE:ysize=val;break;
                        case Render::property::ROTATE:rotation=val;break;
                        case Render::property::X_SHEAR:xshear=val;break;
                        case Render::property::Y_SHEAR:yshear=val;break;
                    }
                }
                glm::mat4 ret=Core::Matrix::identity();
                ret*=Core::Matrix::model({x,y},rotation,{xsize,ysize});
                ret*=Core::Matrix::shear(xshear,yshear);
                return ret;
            }
        }
        auto it=std::upper_bound(keyframes.begin(),keyframes.end(),current);
        if(it==keyframes.begin()){
            const auto& first=keyframes.front();
            float x=0.0f,y=0.0f;
            float xsize=1.0f,ysize=1.0f;
            float rotation=0.0f;
            float xshear=0.0f,yshear=0.0f;
            for(const auto& [p,val,interp]:first.properties){
                switch(p){
                    case Render::property::X_POS:x=val;break;
                    case Render::property::Y_POS:y=val;break;
                    case Render::property::X_SIZE:xsize=val;break;
                    case Render::property::Y_SIZE:ysize=val;break;
                    case Render::property::ROTATE:rotation=val;break;
                    case Render::property::X_SHEAR:xshear=val;break;
                    case Render::property::Y_SHEAR:yshear=val;break;
                }
            }
            glm::mat4 ret=Core::Matrix::identity();
            ret*=Core::Matrix::model({x,y},rotation,{xsize,ysize});
            ret*=Core::Matrix::shear(xshear,yshear);
            return ret;
        }

        if(it==keyframes.end()){
            return keyframes.back().time>0?getMatrix():glm::mat4(1.0f);
        }

        auto prev=std::prev(it);
        float t=static_cast<float>((current-prev->time)/(it->time-prev->time));
        t=std::clamp(t,0.0f,1.0f);
        std::unordered_map<Render::property,float> targetVals;
        std::unordered_map<Render::property,std::function<float(float,float,float)>> interpFuncs;
        for(const auto& [p,val,interp]:it->properties){
            targetVals[p]=val;
            interpFuncs[p]=interp;
        }
        float x=0.0f,y=0.0f;
        float xsize=1.0f,ysize=1.0f;
        float rotation=0.0f;
        float xshear=0.0f,yshear=0.0f;
        for(const auto& [p,val,interp]:prev->properties){
            float targetVal=val;
            auto targetIt=targetVals.find(p);
            if(targetIt!=targetVals.end()){
                targetVal=targetIt->second;
            }
            auto interpIt=interpFuncs.find(p);
            std::function<float(float,float,float)> func=(interpIt!=interpFuncs.end())?interpIt->second:[](float a,float b,float t){return a+(b-a)*t;};
            float result=func(val,targetVal,t);
            switch(p){
                case Render::property::X_POS:x=result;break;
                case Render::property::Y_POS:y=result;break;
                case Render::property::X_SIZE:xsize=result;break;
                case Render::property::Y_SIZE:ysize=result;break;
                case Render::property::ROTATE:rotation=result;break;
                case Render::property::X_SHEAR:xshear=result;break;
                case Render::property::Y_SHEAR:yshear=result;break;
            }
        }
        for(const auto& [p,val,interp]:it->properties){
            bool handled=false;
            for(const auto& [pp,vv,ii]:prev->properties){
                if(pp==p){handled=true;break;}
            }
            if(!handled){
                switch(p){
                    case Render::property::X_POS:x=val;break;
                    case Render::property::Y_POS:y=val;break;
                    case Render::property::X_SIZE:xsize=val;break;
                    case Render::property::Y_SIZE:ysize=val;break;
                    case Render::property::ROTATE:rotation=val;break;
                    case Render::property::X_SHEAR:xshear=val;break;
                    case Render::property::Y_SHEAR:yshear=val;break;
                }
            }
        }
        glm::mat4 ret=Core::Matrix::identity();
        ret*=Core::Matrix::translate(x,y);
        ret*=Core::Matrix::rotate(rotation);
        ret*=Core::Matrix::scale(xsize,ysize);
        ret*=Core::Matrix::shear(xshear,yshear);
        return ret;
    }
}