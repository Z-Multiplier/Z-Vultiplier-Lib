//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Physics.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace Physics{
    float pixelsPerMeter=50.0f;
    void PhysicsWorld::startup(){
        thisid=b2CreateWorld(&thisDef);
        if(!b2World_IsValid(thisid)){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,"Failed to create physics world!",std::source_location::current());
            throw std::runtime_error("Physics world creation failed");
        }
    }
    void PhysicsWorld::step(float dt,float subStepCnt){
        b2World_Step(thisid,dt,subStepCnt);
    }
    PhysicsWorld::~PhysicsWorld(){
        b2DestroyWorld(thisid);
    }
    PhysicsBody::PhysicsBody(b2WorldId id,
                            glm::vec2 position,
                            std::vector<glm::vec2> polygon,
                            float density,
                            float friction,
                            bool isStatic,
                            bool triangulate){
        b2SetLengthUnitsPerMeter(1.0f);
        
        b2BodyDef bodyDef=b2DefaultBodyDef();
        bodyDef.type=isStatic?b2_staticBody:b2_dynamicBody;
        bodyDef.position={position.x,position.y};
        thisid=b2CreateBody(id,&bodyDef);
        if(!b2Body_IsValid(thisid)){
            throw std::runtime_error("Failed to create body!");
        }

        b2ShapeDef shapeDef=b2DefaultShapeDef();
        shapeDef.density=density;
        shapeDef.material.friction=friction;

        if(triangulate){
            std::vector<b2Vec2> verts;
            verts.reserve(polygon.size());
            for(const auto& v:polygon){
                verts.push_back({v.x,v.y});
            }
            b2Hull hull=b2ComputeHull(verts.data(),(int32_t)verts.size());
            if(hull.count<3){
                throw std::runtime_error("Invalid polygon for physics body!");
            }
            b2Polygon poly=b2MakePolygon(&hull,0.0f);
            b2CreatePolygonShape(thisid,&shapeDef,&poly);
        }
        else{
            auto triangles=Utils::earClipTriangulate(polygon);
            if(triangles.empty()){
                throw std::runtime_error("Failed to triangulate polygon!");
            }
            for(const auto& tri:triangles){
                glm::vec2 v0=tri[0],v1=tri[1],v2=tri[2];
                float area=(v0.x*(v1.y-v2.y)+v1.x*(v2.y-v0.y)+v2.x*(v0.y-v1.y))*0.5f;
                std::array<glm::vec2,3> ordered=tri;
                if(area<0){
                    std::swap(ordered[1],ordered[2]);
                }
                std::vector<b2Vec2> triVerts;
                triVerts.reserve(3);
                for(const auto& v:tri){
                    triVerts.push_back({v.x,v.y});
                }
                b2Hull hull=b2ComputeHull(triVerts.data(),(int32_t)triVerts.size());
                b2Polygon poly=b2MakePolygon(&hull,0.0f);
                b2CreatePolygonShape(thisid,&shapeDef,&poly);
            }
        }

        if(!b2Body_IsValid(thisid)){
            throw std::runtime_error("Failed to create physics body!");
        }
    }
    PhysicsBody::~PhysicsBody(){
        b2DestroyBody(thisid);
    }
    void PhysicsBody::applyForce(glm::vec2 force,glm::vec2 point,bool wake){
        b2Body_ApplyForce(thisid,{force.x,force.y},{point.x,point.y},wake);
    }
    void PhysicsBody::applyForceToCenter(glm::vec2 force,bool wake){
        b2Body_ApplyForceToCenter(thisid,{force.x,force.y},wake);
    }
    void PhysicsBody::applyTorque(float torque,bool wake){
        b2Body_ApplyTorque(thisid,torque,wake);
    }
    void PhysicsBody::applyLinearImpulse(glm::vec2 impulse,glm::vec2 point,bool wake){
        b2Body_ApplyLinearImpulse(thisid,{impulse.x,impulse.y},{point.x,point.y},wake);
    }
    void PhysicsBody::applyLinearImpulseToCenter(glm::vec2 impulse,bool wake){
        b2Body_ApplyLinearImpulseToCenter(thisid,{impulse.x,impulse.y},wake);
    }
    void PhysicsBody::applyAngularImpulse(float impulse,bool wake){
        b2Body_ApplyAngularImpulse(thisid,impulse,wake);
    }
    std::vector<std::vector<glm::vec2>> PhysicsBody::getVertices(){
        int32_t count=b2Body_GetShapeCount(thisid);
        std::vector<b2ShapeId> vecshapes(count);
        b2Body_GetShapes(thisid,vecshapes.data(),count);
        std::vector<b2Polygon> vecpolygons;
        vecpolygons.reserve(vecshapes.size());
        std::transform(vecshapes.begin(),vecshapes.end(),std::back_inserter(vecpolygons),[](b2ShapeId id){return b2Shape_GetPolygon(id);});
        std::vector<std::vector<glm::vec2>> vecvertices;
        vecvertices.reserve(vecpolygons.size());
        std::transform(vecpolygons.begin(),vecpolygons.end(),std::back_inserter(vecvertices),[](b2Polygon p){
            std::vector<glm::vec2> ret;
            for(int i=0;i<p.count;i++){
                ret.push_back({p.vertices[i].x,p.vertices[i].y});
            }
            return ret;
        });
        return vecvertices;
    }
    RevoluteJoint::RevoluteJoint(PhysicsWorld& world,const Def& def):PhysicsJoint(def.bodyA,def.bodyB,def.collideConnected){
        b2Vec2 worldAnchor={def.anchor.x,def.anchor.y};

        thisDef=b2DefaultRevoluteJointDef();
        thisDef.bodyIdA=def.bodyA->getId();
        thisDef.bodyIdB=def.bodyB->getId();
        thisDef.collideConnected=def.collideConnected;
        thisDef.localAnchorA=b2Body_GetLocalPoint(thisDef.bodyIdA,worldAnchor);
        thisDef.localAnchorB=b2Body_GetLocalPoint(thisDef.bodyIdB,worldAnchor);

        thisDef.enableLimit=def.enableLimit;
        thisDef.lowerAngle=def.lowerAngle;
        thisDef.upperAngle=def.upperAngle;

        thisDef.enableMotor=def.enableMotor;
        thisDef.motorSpeed=def.motorSpeed;
        thisDef.maxMotorTorque=def.maxMotorTorque;

        thisid=b2CreateRevoluteJoint(world.getId(),&thisDef);
    }
    float RevoluteJoint::getJointAngle()const{
        return b2RevoluteJoint_GetAngle(thisid);
    }
    PrismaticJoint::PrismaticJoint(PhysicsWorld& world,const Def& def):PhysicsJoint(def.bodyA,def.bodyB,def.collideConnected){
        b2Vec2 worldAnchor={def.anchor.x,def.anchor.y};

        thisDef=b2DefaultPrismaticJointDef();
        thisDef.bodyIdA=def.bodyA->getId();
        thisDef.bodyIdB=def.bodyB->getId();
        thisDef.collideConnected=def.collideConnected;
        thisDef.localAnchorA=b2Body_GetLocalPoint(thisDef.bodyIdA,worldAnchor);
        thisDef.localAnchorB=b2Body_GetLocalPoint(thisDef.bodyIdB,worldAnchor);

        thisDef.localAxisA={def.localAxisA.x,def.localAxisA.y};

        thisDef.enableLimit=def.enableLimit;
        thisDef.lowerTranslation=def.lowerTranslation;
        thisDef.upperTranslation=def.upperTranslation;

        thisDef.enableMotor=def.enableMotor;
        thisDef.motorSpeed=def.motorSpeed;
        thisDef.maxMotorForce=def.maxMotorForce;

        thisid=b2CreatePrismaticJoint(world.getId(),&thisDef);
    }
    DistanceJoint::DistanceJoint(PhysicsWorld& world,const Def& def):PhysicsJoint(def.bodyA,def.bodyB,def.collideConnected){
        b2Vec2 worldAnchor={def.anchor.x,def.anchor.y};

        thisDef=b2DefaultDistanceJointDef();
        thisDef.bodyIdA=def.bodyA->getId();
        thisDef.bodyIdB=def.bodyB->getId();
        thisDef.collideConnected=def.collideConnected;
        thisDef.localAnchorA=b2Body_GetLocalPoint(thisDef.bodyIdA,worldAnchor);
        thisDef.localAnchorB=b2Body_GetLocalPoint(thisDef.bodyIdB,worldAnchor);

        thisDef.length=def.length;
        thisDef.hertz=def.hertz;
        thisDef.dampingRatio=def.dampingRatio;

        thisid=b2CreateDistanceJoint(world.getId(),&thisDef);
    }
    WeldJoint::WeldJoint(PhysicsWorld& world,const Def& def):PhysicsJoint(def.bodyA,def.bodyB,def.collideConnected){
        b2Vec2 worldAnchor={def.anchor.x,def.anchor.y};

        thisDef=b2DefaultWeldJointDef();
        thisDef.bodyIdA=def.bodyA->getId();
        thisDef.bodyIdB=def.bodyB->getId();
        thisDef.collideConnected=def.collideConnected;
        thisDef.localAnchorA=b2Body_GetLocalPoint(thisDef.bodyIdA,worldAnchor);
        thisDef.localAnchorB=b2Body_GetLocalPoint(thisDef.bodyIdB,worldAnchor);

        thisDef.referenceAngle=def.referenceAngle;

        thisid=b2CreateWeldJoint(world.getId(),&thisDef);
    }
    WheelJoint::WheelJoint(PhysicsWorld& world,const Def& def):PhysicsJoint(def.bodyA,def.bodyB,def.collideConnected){
        b2Vec2 worldAnchor={def.anchor.x,def.anchor.y};

        thisDef=b2DefaultWheelJointDef();
        thisDef.bodyIdA=def.bodyA->getId();
        thisDef.bodyIdB=def.bodyB->getId();
        thisDef.collideConnected=def.collideConnected;
        thisDef.localAnchorA=b2Body_GetLocalPoint(thisDef.bodyIdA,worldAnchor);
        thisDef.localAnchorB=b2Body_GetLocalPoint(thisDef.bodyIdB,worldAnchor);

        thisDef.localAxisA={def.localAxisA.x,def.localAxisA.y};
        thisDef.enableSpring=def.enableSpring;

        thisDef.lowerTranslation=def.lowerTranslation;
        thisDef.upperTranslation=def.upperTranslation;

        thisDef.enableMotor=def.enableMotor;
        thisDef.motorSpeed=def.motorSpeed;
        thisDef.maxMotorTorque=def.maxMotorTorque;
        thisDef.hertz=def.hertz;
        thisDef.dampingRatio=def.dampingRatio;

        thisid=b2CreateWheelJoint(world.getId(),&thisDef);
    }
    MotorJoint::MotorJoint(PhysicsWorld& world,const Def& def):PhysicsJoint(def.bodyA,def.bodyB,def.collideConnected){
        thisDef=b2DefaultMotorJointDef();
        thisDef.bodyIdA=def.bodyA->getId();
        thisDef.bodyIdB=def.bodyB->getId();
        thisDef.collideConnected=def.collideConnected;
        thisDef.angularOffset=def.angularOffset;
        thisDef.correctionFactor=def.correctionFactor;
        thisDef.linearOffset={def.linearOffset.x,def.linearOffset.y};
        thisDef.maxForce=def.maxForce;
        thisDef.maxTorque=def.maxTorque;

        thisid=b2CreateMotorJoint(world.getId(),&thisDef);
    }
}