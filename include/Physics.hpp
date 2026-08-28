//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef PHYSICS_HPP
#define PHYSICS_HPP


#include "box2d.h"
#include "glm/glm.hpp"
#include <optional>

namespace Physics{
    extern float pixelsPerMeter;
    struct Meter{
        float val;
        operator float(){
            return val*pixelsPerMeter;
        }
        Meter(float val){
            this->val=val;
        }
    };
    inline Meter operator""_meter(long double val){
        return Meter(val);
    }
    struct PhysicsWorld{
        private:
            glm::vec2 gravity={0.0f,-9.8f};
            b2WorldId thisid;
            b2WorldDef thisDef;
        public:
            PhysicsWorld(glm::vec2 G={0.0f,-9.8f}){
                b2SetLengthUnitsPerMeter(1.0f);
                gravity=G;
                thisDef=b2DefaultWorldDef();
                thisDef.gravity={gravity.x,gravity.y};
            }
            ~PhysicsWorld();
            b2WorldDef& getWorldDef(){return thisDef;}
            void startup();
            void step(float dt,float subStepCnt);
            b2WorldId getId(){return thisid;}
    };
    struct PhysicsBody{
        public:
            PhysicsBody(
                b2WorldId id,
                glm::vec2 position,
                std::vector<glm::vec2> polygon,
                float density,
                float friction,
                bool isStatic,
                bool unableTriangulate
            );
            ~PhysicsBody();
            std::vector<std::vector<glm::vec2>> getVertices();
            b2BodyId getId(){return thisid;}
            glm::vec2 getPosition()const{
                b2Vec2 pos=b2Body_GetPosition(thisid);
                return {pos.x,pos.y};
            }
            float getAngle()const{
                auto [c,s]=b2Body_GetRotation(thisid);
                return std::atan2(s,c);
            }
            void applyForce(glm::vec2 force,glm::vec2 point,bool wake=true);
            void applyForceToCenter(glm::vec2 force,bool wake=true);
            void applyTorque(float torque,bool wake=true);
            void applyLinearImpulse(glm::vec2 impulse,glm::vec2 point,bool wake=true);
            void applyLinearImpulseToCenter(glm::vec2 impulse,bool wake=true);
            void applyAngularImpulse(float impulse,bool wake=true);
        private:
            b2BodyId thisid;
            b2BodyDef thisDef;
    };
    struct PhysicsJoint{
        b2JointId thisid;
        PhysicsBody *A,*B;
        bool enableCollide;
        virtual ~PhysicsJoint()=default;
        protected:
            PhysicsJoint(PhysicsBody* bodyA,PhysicsBody* bodyB,bool collideConnected=false)
                :A(bodyA),B(bodyB),enableCollide(collideConnected){}
    };
    struct RevoluteJoint:public PhysicsJoint{
        struct Def{
            PhysicsBody* bodyA;
            PhysicsBody* bodyB;
            glm::vec2 anchor;
            bool collideConnected=false;

            bool enableLimit=false;
            float lowerAngle=0.0f;
            float upperAngle=0.0f;

            bool enableMotor=false;
            float motorSpeed=0.0f;
            float maxMotorTorque=0.0f;
        };

        RevoluteJoint(PhysicsWorld& world,const Def& def);
        ~RevoluteJoint() override=default;

        void setAngleLimit(float lower,float upper){
            b2RevoluteJoint_SetLimits(thisid,lower,upper);
        }
        void setMotorSpeed(float speed){
            b2RevoluteJoint_SetMotorSpeed(thisid,speed);
        }
        float getJointAngle()const;
        private:
            b2RevoluteJointDef thisDef;
    };
    using Hinge=RevoluteJoint;
    struct PrismaticJoint:public PhysicsJoint{
        struct Def{
            PhysicsBody* bodyA;
            PhysicsBody* bodyB;
            glm::vec2 anchor;
            bool collideConnected=false;

            glm::vec2 localAxisA;
            bool enableLimit=false;
            float lowerTranslation;
            float upperTranslation;

            bool enableMotor;
            float motorSpeed;
            float maxMotorForce;
        };

        PrismaticJoint(PhysicsWorld& world,const Def& def);
        ~PrismaticJoint() override=default;

        float getJointTranslation()const{return b2PrismaticJoint_GetTranslation(thisid);}
        float getJointSpeed()const{return b2PrismaticJoint_GetSpeed(thisid);}
        void setTranslationLimit(float lower,float upper){b2PrismaticJoint_SetLimits(thisid,lower,upper);}
        void setMotorSpeed(float speed){b2PrismaticJoint_SetMotorSpeed(thisid,speed);}
        private:
            b2PrismaticJointDef thisDef;
    };
    using Slider=PrismaticJoint;
    struct DistanceJoint:public PhysicsJoint{
        struct Def{
            PhysicsBody* bodyA;
            PhysicsBody* bodyB;
            glm::vec2 anchor;
            bool collideConnected=false;

            float length;
            float hertz;
            float dampingRatio;
        };

        DistanceJoint(PhysicsWorld& world,const Def& def);
        ~DistanceJoint() override=default;

        float getJointLength()const{return b2DistanceJoint_GetLength(thisid);}
        void setJointLength(float len)const{b2DistanceJoint_SetLength(thisid,len);}
        private:
            b2DistanceJointDef thisDef;
    };
    using Rope=DistanceJoint;
    struct WeldJoint:public PhysicsJoint{
        struct Def{
            PhysicsBody* bodyA;
            PhysicsBody* bodyB;
            glm::vec2 anchor;
            bool collideConnected=false;

            float referenceAngle=0.0f;
        };

        WeldJoint(PhysicsWorld& world,const Def& def);
        ~WeldJoint() override=default;
        private:
            b2WeldJointDef thisDef;
    };
    using Glue=WeldJoint;
    struct WheelJoint:public PhysicsJoint{
        struct Def{
            PhysicsBody* bodyA;
            PhysicsBody* bodyB;
            glm::vec2 anchor;
            bool collideConnected=false;

            glm::vec2 localAxisA;
            bool enableSpring;
            float lowerTranslation;
            float upperTranslation;

            bool enableMotor;
            float motorSpeed;
            float maxMotorTorque;
            float hertz;
            float dampingRatio;
        };

        WheelJoint(PhysicsWorld& world,const Def& def);
        ~WheelJoint() override=default;
        void setLimits(float lower,float upper){b2WheelJoint_SetLimits(thisid,lower,upper);}
        private:
            b2WheelJointDef thisDef;
    };
    using Hanging=WheelJoint;
    struct MotorJoint:public PhysicsJoint{
        struct Def{
            PhysicsBody* bodyA;
            PhysicsBody* bodyB;
            bool collideConnected=false;

            glm::vec2 linearOffset;
            float angularOffset;
            float maxForce;
            float maxTorque;
            float correctionFactor;
        };

        MotorJoint(PhysicsWorld& world,const Def& def);
        ~MotorJoint() override=default;
        void setLinearOffset(glm::vec2 offset){b2MotorJoint_SetLinearOffset(thisid,{offset.x,offset.y});}
        void setAngularOffset(float offset){b2MotorJoint_SetAngularOffset(thisid,offset);}
        glm::vec2 getLinearOffset(){b2Vec2 vec=b2MotorJoint_GetLinearOffset(thisid);return {vec.x,vec.y};}
        float getAngularOffset(){return b2MotorJoint_GetAngularOffset(thisid);}
        private:
            b2MotorJointDef thisDef;
    };
    using Motor=MotorJoint;
}

#endif