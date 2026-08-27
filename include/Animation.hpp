//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include "Matrix.hpp"

namespace Render{
    enum class property{
        X_POS=1,
        Y_POS=2,
        X_SIZE=3,
        Y_SIZE=4,
        ROTATE=5,
        X_SHEAR=6,
        Y_SHEAR=7,
    };//ALL absolute value
    inline bool operator<(property a,property b){
        return static_cast<uint32_t>(a)<static_cast<uint32_t>(b);
    }
    struct Keyframe{
        double time;
        std::vector<std::tuple<property,float,std::function<float(float,float,float)>>> properties;
        std::unordered_map<property,bool> added;
        Keyframe()=delete;
        Keyframe(double time):time(time){};
        inline bool operator<(const Keyframe& other)const{
            return this->time<other.time;
        }
        inline bool operator<(const double& other)const{
            return this->time<other;
        }
        void addProperty(property p,float val,std::function<float(float,float,float)> interp){
            if(added[p]) return;
            properties.emplace_back(p,val,interp);
            added[p]=true;
        }
    };
    inline bool operator<(const double& a,const Keyframe& b){
        return a<b.time;
    }
    struct Animation{
        std::vector<Keyframe> keyframes;
        double current=0;
        bool loop=true;
        double duration=0;
        void step(double len){
            current+=len;
            if(loop&&current>duration){
                current=std::fmod(current,duration);
            }
        }
        void addKeyframe(Keyframe kf){
            keyframes.push_back(kf);
            std::sort(keyframes.begin(),keyframes.end(),
                [](const Keyframe& a,const Keyframe& b){return a.time<b.time;});
            if(!keyframes.empty()){
                duration=keyframes.back().time;
            }
        }
        glm::mat4 getMatrix();
    };
    namespace Easing{
        inline float linear(float a,float b,float t){
            return a+(b-a)*t;
        }
        inline float easeIn(float a,float b,float t){
            float p=t*t;
            return a+(b-a)*p;
        }
        inline float easeInCubic(float a,float b,float t){
            float p=t*t*t;
            return a+(b-a)*p;
        }
        inline float easeInQuartic(float a,float b,float t){
            float p=t*t*t*t;
            return a+(b-a)*p;
        }
        inline float easeOut(float a,float b,float t){
            float p=1.0f-(1.0f-t)*(1.0f-t);
            return a+(b-a)*p;
        }
        inline float easeOutCubic(float a,float b,float t){
            float p=1.0f-(1.0f-t)*(1.0f-t)*(1.0f-t);
            return a+(b-a)*p;
        }
        inline float easeOutQuartic(float a,float b,float t){
            float p=1.0f-(1.0f-t)*(1.0f-t)*(1.0f-t)*(1.0f-t);
            return a+(b-a)*p;
        }
        inline float easeInOut(float a,float b,float t){
            if(t<0.5f){
                float p=2.0f*t*t;
                return a+(b-a)*p*0.5f;
            }
            else{
                float p=1.0f-(1.0f-2.0f*(t-0.5f))*(1.0f-2.0f*(t-0.5f));
                return a+(b-a)*(0.5f+p*0.5f);
            }
        }
        inline float easeInOutCubic(float a,float b,float t){
            if(t<0.5f){
                float p=4.0f*t*t*t;
                return a+(b-a)*p*0.5f;
            }
            else{
                float p=1.0f-(1.0f-2.0f*(t-0.5f))*(1.0f-2.0f*(t-0.5f))*(1.0f-2.0f*(t-0.5f));
                return a+(b-a)*(0.5f+p*0.5f);
            }
        }
        inline float easeOutElastic(float a,float b,float t){
            if(t>=1.0f) return b;
            float p=t*t;
            float p2=sinf(t*20.0f)*(1.0f-t);
            float result=a+(b-a)*(p+p2*0.3f);
            return result;
        }
        inline float easeOutBounce(float a,float b,float t){
            if(t>=1.0f) return b;
            float p;
            if(t<1.0f/2.75f){
                p=7.5625f*t*t;
            }
            else if(t<2.0f/2.75f){
                t-=1.5f/2.75f;
                p=7.5625f*t*t+0.75f;
            }
            else if(t<2.5f/2.75f){
                t-=2.25f/2.75f;
                p=7.5625f*t*t+0.9375f;
            }
            else{
                t-=2.625f/2.75f;
                p=7.5625f*t*t+0.984375f;
            }
            return a+(b-a)*p;
        }
        inline float easeOutBack(float a,float b,float t){
            float c1=1.70158f;
            float c3=c1+1.0f;
            float p=1.0f+c3*(1.0f-t)*(1.0f-t)*(1.0f-t)-c1*(1.0f-t)*(1.0f-t);
            return a+(b-a)*p;
        }
    }
}

#endif