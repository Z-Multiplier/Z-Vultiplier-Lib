//MIT License

//Copyright (c) 2026 Z-Multiplier

#ifndef PARTICLE_HPP
#define PARTICLE_HPP
#include "Painter.hpp"
#include "Animation.hpp"
#include "Matrix.hpp"

namespace Render{
    struct Particle{
        Painter::Image* texture=nullptr;
        Animation* anim=nullptr;
        glm::vec4 tint={1,1,1,1};
        glm::vec2 size={0,0};
        glm::vec2 pos,vel,accel;
        float life=1.0f,maxLife=1.0f;
        float scale=1.0f,rotation=0.0f;
        bool alive=true;
        void update(float dt){
            if(!alive) return;

            life-=dt;
            if(life<=0.0f){
                alive=false;
                return;
            }
            vel+=accel*dt;
            pos+=vel*dt;
        }

        void render(Painter& painter)const{
            if(!alive||!texture) return;

            glm::mat4 model=Core::Matrix::translate(pos.x,pos.y)*
                               Core::Matrix::rotate(rotation)*
                               Core::Matrix::scale(scale,scale);
            painter.setTransform(model);

            painter.putImage(size*((-1.0f)/2.0f),size,*texture,tint);
            painter.resetTransform();
        }
    };

    class ParticlePool{
            std::vector<Particle> particles;
            std::vector<size_t> freeIndices;
        public:
            void init(size_t reserveCount){
                particles.reserve(reserveCount);
                freeIndices.reserve(reserveCount);
            }
            size_t emit(const Particle& p);
            void update(float dt);
            void render(Painter& painter);
            void clear(){
                particles.clear();
                freeIndices.clear();
            };
    };

    void spawnExplosion(ParticlePool& pool,glm::vec2 center,float power,size_t count,Painter::Image* tex,glm::vec2 size);
    void spawnWind(ParticlePool& pool,glm::vec2 start,glm::vec2 end,float power,float offset,size_t count,Painter::Image* tex,glm::vec2 size);
    void spawnFireJet(ParticlePool& pool,glm::vec2 center,float spread,float power,Painter::Image* tex,glm::vec2 size);
    void spawnCampfire(ParticlePool& pool,glm::vec2 base,float height,Painter::Image* tex,glm::vec2 size);
}
#endif