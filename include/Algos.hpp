//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef ALGOS_HPP
#define ALGOS_HPP

#include "Painter.hpp"
#include "GameObject.hpp"
#include <vector>
#include <queue>

namespace Game{
    using Point=glm::vec2;
    enum class ExpandMode{
        NONE=0,
        FOURDIR=1,
        DIAGONAL=2,
        KNIGHT=4,
        INFFOURDIR=8,
        INFDIAGONAL=16,
        INFKNIGHT=32,
        EIGHTDIR=1|2,
        QUEEN=8|16,
        AMAZON=8|16|4,
    };
    inline constexpr ExpandMode operator|(ExpandMode a,ExpandMode b){
        return static_cast<ExpandMode>(static_cast<uint32_t>(a)|static_cast<uint32_t>(b));
    }
    inline constexpr ExpandMode operator&(ExpandMode a,ExpandMode b){
        return static_cast<ExpandMode>(static_cast<uint32_t>(a)&static_cast<uint32_t>(b));
    }
    inline constexpr bool operator!=(ExpandMode t,int val){
        return static_cast<uint32_t>(t)!=static_cast<uint32_t>(val);
    }
    inline constexpr bool operator==(ExpandMode t,int val){
        return static_cast<uint32_t>(t)==static_cast<uint32_t>(val);
    }
    std::vector<Point> Astar(Point start,Point goal,
                             const Terrain& terrain,
                             ExpandMode mode,
                             std::function<float(Point,Point)> actualCost,
                             std::function<float(Point,Point)> heuristic);
}

#endif