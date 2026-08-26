//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef MUSIC_HPP
#define MUSIC_HPP

#include <string>
#include <vector>

namespace Audio{
    float stringToRate(std::string name);
    struct Notation{
        std::string oneEquals="1=C4";
        std::string music;
        Notation(std::string oe,std::string music):oneEquals(oe),music(music){};
        std::vector<std::pair<float,float>> toRates(float noteDuration);
    };
}

#endif