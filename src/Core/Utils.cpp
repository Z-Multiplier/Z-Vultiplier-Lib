//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Utils.hpp"

#include <array>
#include <numeric>
#include <vector>
#include <algorithm>

float Utils::cross(const glm::vec2& a,const glm::vec2& b){return a.x*b.y-a.y*b.x;}

bool Utils::pointInTriangle(const glm::vec2& p,const glm::vec2& a,const glm::vec2& b,const glm::vec2& c){
    float d1=cross(p-a,b-a);
    float d2=cross(p-b,c-b);
    float d3=cross(p-c,a-c);
    bool hasNeg=(d1<0)||(d2<0)||(d3<0);
    bool hasPos=(d1>0)||(d2>0)||(d3>0);
    return !(hasNeg&&hasPos);
}

std::vector<std::array<glm::vec2,3>> Utils::earClipTriangulate(const std::vector<glm::vec2>& verts){
        std::vector<std::array<glm::vec2,3>> triangles;
        if(verts.size()<3) return triangles;

        std::vector<int> indices(verts.size());
        std::iota(indices.begin(),indices.end(),0);

        float area=0.0f;
        for(size_t i=0;i<verts.size();++i){
            size_t j=(i+1)%verts.size();
            area+=verts[i].x*verts[j].y-verts[j].x*verts[i].y;
        }
        if(area<0){
            std::reverse(indices.begin()+1,indices.end());
        }

        while(indices.size()>3){
            bool earFound=false;
            for(size_t i=0;i<indices.size();++i){
                size_t prevIdx=(i-1+indices.size())%indices.size();
                size_t nextIdx=(i+1)%indices.size();

                int a=indices[prevIdx];
                int b=indices[i];
                int c=indices[nextIdx];

                const glm::vec2& A=verts[a];
                const glm::vec2& B=verts[b];
                const glm::vec2& C=verts[c];

                if(cross(B-A,C-A)<=0.0f) continue;

                bool hasInternal=false;
                for(int p:indices){
                    if(p==a||p==b||p==c) continue;
                    if(pointInTriangle(verts[p],A,B,C)){
                        hasInternal=true;
                        break;
                    }
                }
                if(hasInternal) continue;

                triangles.push_back({A,B,C});
                indices.erase(indices.begin()+i);
                earFound=true;
                break;
            }
            if(!earFound) break;
        }

        if(indices.size()==3){
            triangles.push_back({
                verts[indices[0]],
                verts[indices[1]],
                verts[indices[2]]
            });
        }
        return triangles;
    }
