//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef UTILS_HPP
#define UTILS_HPP

#include "glm/glm.hpp"
#include <vector>
#include <array>
#include <random>
#include <chrono>

namespace Utils{
    using glm::vec2;
    using glm::vec3;
    float cross(const vec2& a,const vec2& b);
    bool pointInTriangle(const vec2& p,const vec2& a,const vec2& b,const vec2& c);
    std::vector<std::array<vec2,3>> earClipTriangulate(const std::vector<vec2>& verts);
    class Random{
            std::random_device dev;
            std::mt19937 gen;
        public:
            Random():gen(dev()){}
            int range(int min,int max){
                std::uniform_int_distribution<int> r(min,max);
                return r(gen);
            }
            float real(float min,float max){
                std::uniform_real_distribution<float> r(min,max);
                return r(gen);
            }
    };
    class Timer{
            std::chrono::steady_clock::time_point start;
        public:
            Timer(){reset();}
            void reset(){start=std::chrono::steady_clock::now();}
            double elapsed()const{
                return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            }
            bool reached(double seconds)const{return elapsed()>=seconds;}
    };
    static inline std::string basicRoman(int n){
        static const std::vector<std::pair<int,std::string>> valToSym={
            {1000,"M"},{900,"CM"},{500,"D"},{400,"CD"},
            {100,"C"},{90,"XC"},{50,"L"},{40,"XL"},
            {10,"X"},{9,"IX"},{5,"V"},{4,"IV"},
            {1,"I"}
        };
        std::string result;
        for(const auto& [val,sym]:valToSym){
            while(n>=val){
                result+=sym;
                n-=val;
            }
        }
        return result;
    }
    inline std::string intToRoman(int num){
        if(num==0) return "NULLA";
        std::vector<int> groups;
        while(num>0){
            groups.push_back(num%1000);
            num/=1000;
        }
        std::string result;
        for(int i=groups.size()-1;i>=0;i--){
            int val=groups[i];
            if(val==0) continue;
            std::string romanGroup=basicRoman(val);
            if(i>0){
                std::string overlined;
                for(char ch:romanGroup){
                    overlined+=ch;
                    for(int k=0;k<i;++k){
                        overlined+="\u0305";
                    }
                }
                romanGroup=overlined;
            }
            result+=romanGroup;
        }
        return result;
    }
}

#endif