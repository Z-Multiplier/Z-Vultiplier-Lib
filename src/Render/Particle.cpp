//MIT License

//Copyright (c) 2026 Z-Multiplier

#include "Particle.hpp"
#include "Utils.hpp"

namespace Render{
    size_t ParticlePool::emit(const Particle& p){
        if(!freeIndices.empty()){
            size_t idx=freeIndices.back();
            freeIndices.pop_back();
            particles[idx]=p;
            particles[idx].alive=true;
            return idx;
        } 
        else{
            particles.push_back(p);
            particles.back().alive=true;
            return particles.size()-1;
        }
    }
    void ParticlePool::update(float dt){
        for(size_t i=0;i<particles.size();++i){
            auto& p=particles[i];
            if(!p.alive) continue;

            p.update(dt);

            if(!p.alive){
                freeIndices.push_back(i);
            }
        }
    }
    void ParticlePool::render(Painter& p){
        for(size_t i=0;i<particles.size();++i){
            if(particles[i].alive){
                particles[i].render(p);
            }
        }
    }
    void spawnExplosion(ParticlePool& pool,glm::vec2 center,float power,size_t count,Painter::Image* tex,glm::vec2 size){
        Utils::Random rng;
        for(size_t i=0;i<count;i++){
            Particle p;
            p.texture=tex;
            p.pos=center;
            p.size=size;
            float angle=rng.real(0.0f,2.0f*glm::pi<float>());
            float speed=rng.real(power,power+2.0f);
            p.vel=glm::vec2({cos(angle),sin(angle)})*speed;
            p.accel=glm::vec2(0.0f,-2.0f);
            p.maxLife=rng.real(0.5f,1.5f);
            p.life=p.maxLife;
            p.scale=rng.real(0.3f,1.0f);
            pool.emit(p);
        }
    }
    void spawnWind(ParticlePool& pool,glm::vec2 start,glm::vec2 end,float power,float offset,size_t count,Painter::Image* tex,glm::vec2 size){
        Utils::Random rng;
        for(size_t i=0;i<count;i++){
            Particle p;
            p.texture=tex;
            p.pos=start;
            p.size=size;
            glm::vec2 dir=end-start;
            float factor=rng.real(power,power*1.5f);
            p.vel=dir*factor;
            float angle=rng.real(0.0f,2.0f*glm::pi<float>());
            p.accel=dir+glm::vec2(cos(angle)*offset,sin(angle)*offset);
            p.maxLife=rng.real(0.5f,1.5f);
            p.life=p.maxLife;
            p.scale=rng.real(0.3f,1.0f);
            pool.emit(p);
        }
    }
    void spawnFireJet(ParticlePool& pool,glm::vec2 center,float spread,float power,Painter::Image* tex,glm::vec2 size){
        static Utils::Random rng;
        for(int i=0;i<30;++i){
            Particle p;
            p.texture=tex;
            p.pos=center;
            p.size=size;
            float dir=rng.real(-1.0f,1.0f);
            p.vel=glm::vec2(dir*spread,rng.real(power,power*1.5f));
            p.accel=glm::vec2(-dir*spread*0.8f,-2.0f);
            p.maxLife=rng.real(1.2f,2.4f);
            p.life=p.maxLife;
            p.scale=rng.real(0.2f,0.6f);
            pool.emit(p);
        }
    }
    void spawnCampfire(ParticlePool& pool,glm::vec2 base,float height,Painter::Image* tex,glm::vec2 size){
        static Utils::Random rng;
        Particle p;
        p.texture=tex;
        p.size=size;
        p.pos=base+glm::vec2(rng.real(-5.0f,5.0f),0.0f);
        p.vel=glm::vec2(rng.real(-2.0f,2.0f),rng.real(2.0f,height));
        p.accel=glm::vec2(-p.vel.x*0.5f,-1.0f);
        p.maxLife=rng.real(1.0f,2.0f);
        p.life=p.maxLife;
        pool.emit(p);
    }
}