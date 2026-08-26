//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Music.hpp"
#include <cmath>
#include <string>
#include <sstream>

namespace Audio{
    float stringToRate(std::string name){
        if(name.size()==2){
            name.insert(name.begin()+1,' ');
        }
        if(name.size()!=3) return 0.0f;
        if(name[2]<'0'||name[2]>'9') return 0.0f;
        long long halves=0;
        halves+=(long long)(name[2]-'4')*12;
        long long noteOffset=0;
        switch(name[0]){
            case 'C':case 'c':noteOffset=0;break;
            case 'D':case 'd':noteOffset=2;break;
            case 'E':case 'e':noteOffset=4;break;
            case 'F':case 'f':noteOffset=5;break;
            case 'G':case 'g':noteOffset=7;break;
            case 'A':case 'a':noteOffset=9;break;
            case 'B':case 'b':noteOffset=11;break;
            default:return 0.0f;
        }
        
        long long accidentalOffset=0;
        if(name[1]=='#'){
            accidentalOffset=1;
        }
        else if(name[1]=='b'){
            accidentalOffset=-1;
        }
        else if(name[1]==' '){
            accidentalOffset=0;
        }
        else{
            return 0.0f;
        }
        
        long long totalHalves=halves+noteOffset+accidentalOffset;

        float baseFreq=261.63f;
        return baseFreq*std::pow(2.0f,totalHalves/12.0f);
    }
    std::vector<std::pair<float,float>> Notation::toRates(float noteDuration){
        std::vector<std::pair<float,float>> result;
        
        size_t equalPos=oneEquals.find('=');
        if(equalPos==std::string::npos) return result;
        std::string baseNoteName=oneEquals.substr(equalPos+1);
        float baseFreq=stringToRate(baseNoteName);
        if(baseFreq==0.0f) return result;
        
        int scaleOffsets[8]={-1,0,2,4,5,7,9,11};
        
        std::istringstream iss(music);
        std::string token;
        float lastValidFreq=0.0f;

        while(iss>>token){
            if(token=="-"){
                result.back().second+=noteDuration;
                continue;
            }
            
            char noteChar=token[0];
            if(noteChar<'0'||noteChar>'7') continue;
            
            int scaleIndex=noteChar-'0';
            int octaveShift=0;
            int extraHalves=0;

            if(token=="0"){
                result.emplace_back(0.0f,noteDuration);
                lastValidFreq=0.0f;
                continue;
            }
            
            float duration=noteDuration;
            if(token.size()>=2){
                while(token.size()>=2){
                    char c=token.back();
                    token.pop_back();
                    switch(c){
                        case '+':octaveShift++;break;
                        case '-':octaveShift--;break;
                        case '_':duration/=2;break;
                        case '#':extraHalves++;break;
                        case 'b':extraHalves--;break;
                        default:break;
                    }
                }
            }
            if(scaleOffsets[scaleIndex]==-1){
                result.emplace_back(0.0f,duration);
                lastValidFreq=0.0f;
            }
            else{
                int totalHalves=scaleOffsets[scaleIndex]+octaveShift*12+extraHalves;
                
                float freq=baseFreq*std::pow(2.0f,totalHalves/12.0f);
                result.emplace_back(freq,duration);
                lastValidFreq=freq;
            }
        }
        
        return result;
    }
}