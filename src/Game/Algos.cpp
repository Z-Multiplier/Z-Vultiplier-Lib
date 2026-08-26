//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Algos.hpp"
#include "Painter.hpp"
#include "GameObject.hpp"
#include <vector>
#include <algorithm>
#include <unordered_map>

struct Vec2Hash{
    std::size_t operator()(const glm::vec2& v)const{
        std::size_t h1=std::hash<float>()(v.x);
        std::size_t h2=std::hash<float>()(v.y);

        h1^=h2+0x9e3779b9+(h1<<6)+(h1>>2);
        return h1;
    }
};
struct Vec2Equal{
    bool operator()(const glm::vec2& a,const glm::vec2& b)const{
        return a.x==b.x&&a.y==b.y;
    }
};

namespace Game{
    std::vector<Point> Astar(Point start,Point goal,
                             const Terrain& terrain,
                             ExpandMode mode,
                             std::function<float(Point,Point)> actualCost,
                             std::function<float(Point,Point)> heuristic){
        if(terrain(start.x,start.y)>=Unpassable||
            terrain(goal.x,goal.y)>=Unpassable){
            return {};
        }
        struct Node{
            Point p;
            float g;
            float f;
            bool operator>(const Node& other)const{return f>other.f;}
        };
        std::priority_queue<Node,std::vector<Node>,std::greater<Node>> open;
        std::unordered_map<Point,float,Vec2Hash,Vec2Equal> bestG;
        std::unordered_map<Point,Point,Vec2Hash,Vec2Equal> cameFrom;
        bestG[start]=0.0f;
        open.push({start,0.0f,heuristic(start,goal)});
        auto getNeighbors=[&](const Point& p)->std::vector<Point> {
            std::vector<Point> neighbors;
            const std::vector<Point> fourDirOffsets={{1,0},{-1,0},{0,1},{0,-1}};
            const std::vector<Point> diagOffsets={{1,1},{1,-1},{-1,1},{-1,-1}};
            const std::vector<Point> knightOffsets={
                {2,1},{2,-1},{-2,1},{-2,-1},
                {1,2},{1,-2},{-1,2},{-1,-2}
            };
            auto addFixed=[&](const std::vector<Point>& offsets){
                for(auto& off:offsets){
                    Point n{p.x+off.x,p.y+off.y};
                    if(terrain(n.x,n.y)<Unpassable)
                        neighbors.push_back(n);
                }
            };
            if((mode&ExpandMode::FOURDIR)!=0) addFixed(fourDirOffsets);
            if((mode&ExpandMode::DIAGONAL)!=0) addFixed(diagOffsets);
            if((mode&ExpandMode::KNIGHT)!=0) addFixed(knightOffsets);
            auto addRay=[&](int dx,int dy){
                Point cur{p.x+dx,p.y+dy};
                while(terrain(cur.x,cur.y)<Unpassable){
                    if(dx!=0&&terrain(p.x+(cur.x>p.x?dx:0),p.y)>=Unpassable) break;
                    if(dy!=0&&terrain(p.x,p.y+(cur.y>p.y?dy:0))>=Unpassable) break;
                    neighbors.push_back(cur);
                    cur.x+=dx;
                    cur.y+=dy;
                }
            };
            if((mode&ExpandMode::INFFOURDIR)!=0){
                addRay(1,0);addRay(-1,0);
                addRay(0,1);addRay(0,-1);
            }
            if((mode&ExpandMode::INFDIAGONAL)!=0){
                addRay(1,1);addRay(1,-1);
                addRay(-1,1);addRay(-1,-1);
            }
            if((mode&ExpandMode::INFKNIGHT)!=0){
                for(auto& off:knightOffsets){
                    Point cur{p.x+off.x,p.y+off.y};
                    while(terrain(cur.x,cur.y)<Unpassable){
                        neighbors.push_back(cur);
                        cur.x+=off.x;
                        cur.y+=off.y;
                    }
                }
            }
            return neighbors;
        };
        while(!open.empty()){
            Node current=open.top();
            open.pop();
            auto it=bestG.find(current.p);
            if(it==bestG.end()||current.g>it->second)
                continue;
            if(current.p==goal){
                std::vector<Point> path;
                Point trace=goal;
                while(!(trace==start)){
                    path.push_back(trace);
                    trace=cameFrom[trace];
                }
                path.push_back(start);
                std::reverse(path.begin(),path.end());
                return path;
            }
            for(Point& neighbor:getNeighbors(current.p)){
                float moveCost=actualCost(current.p,neighbor);
                if(moveCost>=Unpassable) continue;
                float tentativeG=current.g+moveCost;
                auto it=bestG.find(neighbor);
                if(it==bestG.end()||tentativeG<it->second){
                    bestG[neighbor]=tentativeG;
                    cameFrom[neighbor]=current.p;
                    float h=heuristic(neighbor,goal);
                    open.push({neighbor,tentativeG,tentativeG+h});
                }
            }
        }
        return {};
    }
}
