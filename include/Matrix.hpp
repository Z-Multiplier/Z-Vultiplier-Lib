//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Core{
    namespace Matrix{
        inline glm::mat4 identity(){
            return glm::mat4(1.0f);
        }
        inline glm::mat4 translate(float x,float y){
            return glm::translate(glm::mat4(1.0f),glm::vec3(x,y,0.0f));
        }
        inline glm::mat4 rotate(float angle){
            return glm::rotate(glm::mat4(1.0f),angle,glm::vec3(0.0f,0.0f,1.0f));
        }
        inline glm::mat4 scale(float sx,float sy){
            return glm::scale(glm::mat4(1.0f),glm::vec3(sx,sy,1.0f));
        }

        inline glm::mat4 ortho(float left,float right,float bottom,float top){
            return glm::ortho(left,right,bottom,top,-1.0f,1.0f);
        }

        inline glm::mat4 model(const glm::vec2& translation,float rotation,const glm::vec2& scaling){
            return translate(translation.x,translation.y)*
                   rotate(rotation)*
                   scale(scaling.x,scaling.y);
        }

        inline glm::mat4 model(float tx,float ty,float rot,float sx,float sy){
            return translate(tx,ty)*rotate(rot)*scale(sx,sy);
        }
    }
}

#endif