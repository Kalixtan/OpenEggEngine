#include "physics_joint_bindings.h"

#include "physics_body_bindings.h"
#include "vector2d_bindings.h"
#include "../core/physics_world.h"
#include "gmThread.h"
#include "gmUserObject.h"
#include <box2d/box2d.h>
#include <box2d/b2_relative_joint.h>
#include <algorithm>
#include <memory>
#include <unordered_map>

namespace {

constexpr int kRevolute = 1;
constexpr int kPrismatic = 2;
constexpr int kDistance = 3;
constexpr int kPin = 9;
constexpr int kRelative = 10;

struct JointData {
    int id = 0;
    int type = 0;
    b2Body* body1 = nullptr;
    b2Body* body2 = nullptr;
    b2Joint* joint = nullptr;
    bool bodiesCollidable = false;
    bool motorEnabled = false;
    float motorSpeed = 0.0f;
    float motorMax = 0.0f;
    bool angularLimitEnabled = false;
    float lowerAngle = 0.0f;
    float upperAngle = 0.0f;
    b2Vec2 prismaticAxis = b2Vec2(0.0f, 1.0f);
    bool hasPrismaticAxis = false;
    bool needsHighAccuracy = false;
    float angularSpringTension = 0.0f;
    float angularSpringDamping = 0.0f;
    float linearSpringTension = 0.0f;
    float linearSpringDamping = 0.0f;
    b2Vec2 relativeVelocity = b2Vec2_zero;
};

struct JointHandle {
    int id = 0;
};

struct JointIterData {
    std::vector<int> ids;
    size_t index = 0;
};

gmType g_jointType = -1;
gmType g_jointIterType = -1;
int g_nextJointId = 1;
std::unordered_map<int, std::unique_ptr<JointData>> g_joints;
b2Body* g_staticAnchorBody = nullptr;

JointHandle* ThisHandle(gmThread* a_thread) {
    return (JointHandle*)a_thread->ThisUserCheckType(g_jointType);
}

JointData* ResolveJoint(int a_id) {
    auto found = g_joints.find(a_id);
    return found == g_joints.end() ? nullptr : found->second.get();
}

JointData* ThisJoint(gmThread* a_thread) {
    JointHandle* handle = ThisHandle(a_thread);
    return handle ? ResolveJoint(handle->id) : nullptr;
}

gmVariable MakeJointHandle(gmMachine& a_machine, int a_id) {
    gmUserObject* obj = a_machine.AllocUserObject(new JointHandle{a_id}, g_jointType);
    return gmVariable(obj);
}

b2Body* StaticAnchorBody() {
    if (!g_staticAnchorBody) {
        b2BodyDef def;
        g_staticAnchorBody = GetPhysicsWorld().CreateBody(&def);
    }
    return g_staticAnchorBody;
}

void DestroyConstraint(JointData& a_data) {
    if (a_data.joint) {
        GetPhysicsWorld().DestroyJoint(a_data.joint);
        a_data.joint = nullptr;
    }
}

int GM_CDECL gmf_createJoint(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(type, 0, 0);
    std::unique_ptr<JointData> data(new JointData());
    data->id = g_nextJointId++;
    data->type = type;
    const int id = data->id;
    g_joints.emplace(id, std::move(data));
    a_thread->Push(MakeJointHandle(*a_thread->GetMachine(), id));
    return GM_OK;
}

int GM_CDECL gmf_deleteJoint(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    JointHandle* handle = (JointHandle*)a_thread->ParamUserCheckType(0, g_jointType);
    if (!handle) return GM_OK;
    auto found = g_joints.find(handle->id);
    if (found != g_joints.end()) {
        DestroyConstraint(*found->second);
        g_joints.erase(found);
    }
    return GM_OK;
}

int GM_CDECL gmf_deleteAllJoints(gmThread*) {
    for (auto& entry : g_joints) DestroyConstraint(*entry.second);
    g_joints.clear();
    return GM_OK;
}

int GM_CDECL gmf_getJoint(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(id, 0, 0);
    if (ResolveJoint(id)) a_thread->Push(MakeJointHandle(*a_thread->GetMachine(), id));
    else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_setupJoint(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    gmMachine& m = *a_thread->GetMachine();
    j->body1 = GetB2BodyFromVar(m, a_thread->Param(0));
    j->body2 = GetB2BodyFromVar(m, a_thread->Param(1));
    if (!j->body1 || !j->body2 || (j->type != kRevolute && j->type != kPrismatic)) {
        GM_EXCEPTION_MSG("PhysicsJoint.setupJoint: unsupported type or invalid bodies");
        return GM_EXCEPTION;
    }
    // Retail's setup stores the world anchor on the wrapper. Box2D's
    // Initialize helpers derive the matching body-local anchors from it.
    float x = 0.0f, y = 0.0f;
    if (!GetVector2D(m, a_thread->Param(2), x, y)) return GM_EXCEPTION;
    if (j->type == kRevolute) {
        b2RevoluteJointDef def;
        def.Initialize(j->body1, j->body2, b2Vec2(x, y));
        def.collideConnected = j->bodiesCollidable;
        def.enableLimit = j->angularLimitEnabled;
        def.lowerAngle = j->lowerAngle;
        def.upperAngle = j->upperAngle;
        def.enableMotor = j->motorEnabled;
        def.motorSpeed = j->motorSpeed;
        def.maxMotorTorque = j->motorMax;
        DestroyConstraint(*j);
        j->joint = GetPhysicsWorld().CreateJoint(&def);
    } else {
        b2PrismaticJointDef def;
        def.Initialize(j->body1, j->body2, b2Vec2(x, y), j->prismaticAxis);
        def.collideConnected = j->bodiesCollidable;
        def.enableMotor = j->motorEnabled;
        def.motorSpeed = j->motorSpeed;
        def.maxMotorForce = j->motorMax;
        DestroyConstraint(*j);
        j->joint = GetPhysicsWorld().CreateJoint(&def);
    }
    return GM_OK;
}

int GM_CDECL gmf_setupDistanceJoint(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(4);
    if (j->type != kDistance) {
        GM_EXCEPTION_MSG("PhysicsJoint.setupDistanceJoint: joint is not a distance joint");
        return GM_EXCEPTION;
    }
    gmMachine& m = *a_thread->GetMachine();
    j->body1 = GetB2BodyFromVar(m, a_thread->Param(0));
    j->body2 = GetB2BodyFromVar(m, a_thread->Param(1));
    float ax, ay, bx, by;
    if (!j->body1 || !j->body2 || !GetVector2D(m, a_thread->Param(2), ax, ay) ||
        !GetVector2D(m, a_thread->Param(3), bx, by)) return GM_EXCEPTION;
    b2DistanceJointDef def;
    def.Initialize(j->body1, j->body2, b2Vec2(ax, ay), b2Vec2(bx, by));
    def.collideConnected = j->bodiesCollidable;
    DestroyConstraint(*j);
    j->joint = GetPhysicsWorld().CreateJoint(&def);
    return GM_OK;
}

int GM_CDECL gmf_setupPinJoint(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    if (j->type != kPin) {
        GM_EXCEPTION_MSG("PhysicsJoint.setupPinJoint: joint is not a pin joint");
        return GM_EXCEPTION;
    }
    gmMachine& m = *a_thread->GetMachine();
    j->body1 = GetB2BodyFromVar(m, a_thread->Param(0));
    float x, y;
    if (!j->body1 || !GetVector2D(m, a_thread->Param(1), x, y)) return GM_EXCEPTION;
    b2RevoluteJointDef def;
    def.Initialize(StaticAnchorBody(), j->body1, b2Vec2(x, y));
    def.collideConnected = j->bodiesCollidable;
    def.enableLimit = j->angularLimitEnabled;
    def.lowerAngle = j->lowerAngle;
    def.upperAngle = j->upperAngle;
    def.enableMotor = j->motorEnabled;
    def.motorSpeed = j->motorSpeed;
    def.maxMotorTorque = j->motorMax;
    DestroyConstraint(*j);
    j->joint = GetPhysicsWorld().CreateJoint(&def);
    j->body2 = StaticAnchorBody();
    return GM_OK;
}

int GM_CDECL gmf_setupRelativeJoint(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    if (j->type != kRelative) {
        GM_EXCEPTION_MSG("PhysicsJoint.setupRelativeJoint: joint is not a relative joint");
        return GM_EXCEPTION;
    }
    gmMachine& m = *a_thread->GetMachine();
    j->body1 = GetB2BodyFromVar(m, a_thread->Param(0));
    j->body2 = GetB2BodyFromVar(m, a_thread->Param(1));
    if (!j->body1 || !j->body2) return GM_EXCEPTION;

    b2RelativeJointDef def;
    def.bodyA = j->body1;
    def.bodyB = j->body2;
    def.collideConnected = j->bodiesCollidable;
    def.relativeVelocity = j->relativeVelocity;
    DestroyConstraint(*j);
    j->joint = GetPhysicsWorld().CreateJoint(&def);
    return GM_OK;
}

int GM_CDECL gmf_setIsBodiesCollidable(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    j->bodiesCollidable = v != 0;
    if (j->joint) j->joint->GetBodyA()->SetAwake(true), j->joint->GetBodyB()->SetAwake(true);
    // Box2D cannot change collideConnected on a live joint. Recreate it
    // from the stored setup/configuration when that flag changes.
    if (j->joint) {
        b2Body* a = j->joint->GetBodyA();
        b2Body* b = j->joint->GetBodyB();
        if (j->type == kRevolute) {
            b2RevoluteJoint* old = (b2RevoluteJoint*)j->joint;
            b2RevoluteJointDef def;
            def.Initialize(a, b, old->GetAnchorA());
            def.collideConnected = j->bodiesCollidable;
            def.enableLimit = old->IsLimitEnabled();
            def.lowerAngle = old->GetLowerLimit(); def.upperAngle = old->GetUpperLimit();
            def.enableMotor = old->IsMotorEnabled();
            def.motorSpeed = old->GetMotorSpeed(); def.maxMotorTorque = old->GetMaxMotorTorque();
            DestroyConstraint(*j); j->joint = GetPhysicsWorld().CreateJoint(&def);
        } else if (j->type == kPin) {
            b2RevoluteJoint* old = (b2RevoluteJoint*)j->joint;
            b2RevoluteJointDef def;
            def.Initialize(a, b, old->GetAnchorA());
            def.collideConnected = j->bodiesCollidable;
            def.enableLimit = old->IsLimitEnabled();
            def.lowerAngle = old->GetLowerLimit(); def.upperAngle = old->GetUpperLimit();
            def.enableMotor = old->IsMotorEnabled();
            def.motorSpeed = old->GetMotorSpeed(); def.maxMotorTorque = old->GetMaxMotorTorque();
            DestroyConstraint(*j); j->joint = GetPhysicsWorld().CreateJoint(&def);
        } else if (j->type == kDistance) {
            b2DistanceJoint* old = (b2DistanceJoint*)j->joint;
            b2DistanceJointDef def;
            def.bodyA = a; def.bodyB = b;
            def.localAnchorA = old->GetLocalAnchorA(); def.localAnchorB = old->GetLocalAnchorB();
            def.length = old->GetLength(); def.minLength = old->GetMinLength(); def.maxLength = old->GetMaxLength();
            def.collideConnected = j->bodiesCollidable;
            DestroyConstraint(*j); j->joint = GetPhysicsWorld().CreateJoint(&def);
        } else if (j->type == kPrismatic) {
            b2PrismaticJoint* old = (b2PrismaticJoint*)j->joint;
            b2PrismaticJointDef def;
            def.bodyA = a; def.bodyB = b;
            def.localAnchorA = old->GetLocalAnchorA(); def.localAnchorB = old->GetLocalAnchorB();
            def.localAxisA = old->GetLocalAxisA(); def.referenceAngle = old->GetReferenceAngle();
            def.collideConnected = j->bodiesCollidable;
            def.enableLimit = old->IsLimitEnabled(); def.lowerTranslation = old->GetLowerLimit();
            def.upperTranslation = old->GetUpperLimit(); def.enableMotor = old->IsMotorEnabled();
            def.motorSpeed = old->GetMotorSpeed(); def.maxMotorForce = old->GetMaxMotorForce();
            DestroyConstraint(*j); j->joint = GetPhysicsWorld().CreateJoint(&def);
        } else if (j->type == kRelative) {
            b2RelativeJointDef def;
            def.bodyA = a; def.bodyB = b;
            def.relativeVelocity = j->relativeVelocity;
            def.collideConnected = j->bodiesCollidable;
            DestroyConstraint(*j); j->joint = GetPhysicsWorld().CreateJoint(&def);
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_isBodiesCollidable(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) { a_thread->PushInt(0); return GM_OK; }
    a_thread->PushInt(j->bodiesCollidable ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_setMotor(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(enabled, 0, 0);
    GM_FLOAT_OR_INT_PARAM(speed, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maximum, 2, 0.0f);
    if (j->type != kRevolute && j->type != kPrismatic && j->type != kPin) return GM_OK;
    j->motorEnabled = enabled != 0; j->motorSpeed = speed; j->motorMax = maximum;
    if (j->type == kRevolute || j->type == kPin) {
        if (b2RevoluteJoint* r = dynamic_cast<b2RevoluteJoint*>(j->joint)) {
            r->EnableMotor(j->motorEnabled);
            r->SetMotorSpeed(speed);
            r->SetMaxMotorTorque(maximum);
        }
    } else if (j->type == kPrismatic) {
        if (b2PrismaticJoint* p = dynamic_cast<b2PrismaticJoint*>(j->joint)) {
            p->EnableMotor(j->motorEnabled);
            p->SetMotorSpeed(speed);
            p->SetMaxMotorForce(maximum);
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_setPinMotor(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j || j->type != kPin) { GM_EXCEPTION_MSG("not a pin joint"); return GM_EXCEPTION; }
    return gmf_setMotor(a_thread);
}

int GM_CDECL gmf_setRevoluteMotor(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j || j->type != kRevolute) { GM_EXCEPTION_MSG("not a revolute joint"); return GM_EXCEPTION; }
    if (a_thread->GetNumParams() == 3) return gmf_setMotor(a_thread);
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(enabled, 0, 0);
    j->motorEnabled = enabled != 0; j->motorSpeed = 0.0f; j->motorMax = 0.0f;
    if (b2RevoluteJoint* r = dynamic_cast<b2RevoluteJoint*>(j->joint)) {
        r->EnableMotor(j->motorEnabled);
        r->SetMotorSpeed(0.0f);
        r->SetMaxMotorTorque(0.0f);
    }
    return GM_OK;
}

int GM_CDECL gmf_setRTMotorSpeed(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(speed, 0, 0.0f);
    // Retail's helper has implementations for revolute, prismatic and pin.
    if (j->type != kRevolute && j->type != kPrismatic && j->type != kPin) return GM_EXCEPTION;
    j->motorSpeed = speed;
    if (b2RevoluteJoint* r = dynamic_cast<b2RevoluteJoint*>(j->joint)) r->SetMotorSpeed(speed);
    else if (b2PrismaticJoint* p = dynamic_cast<b2PrismaticJoint*>(j->joint)) p->SetMotorSpeed(speed);
    return GM_OK;
}

int GM_CDECL gmf_setRTMaxMotorTorque(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(maximum, 0, 0.0f);
    // The retail helper only supports revolute and pin motors.
    if (j->type != kRevolute && j->type != kPin) return GM_EXCEPTION;
    j->motorMax = maximum;
    if (b2RevoluteJoint* r = dynamic_cast<b2RevoluteJoint*>(j->joint)) r->SetMaxMotorTorque(maximum);
    // Retail's RT torque setter handles revolute and pin joints only.
    return GM_OK;
}

int GM_CDECL gmf_getRTMotorSpeed(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    a_thread->PushFloat(j && j->type == kRevolute ? j->motorSpeed : 0.0f);
    return GM_OK;
}

int GM_CDECL gmf_getReactionForce(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(invDt, 0, 0.0f);
    b2Vec2 force(0.0f, 0.0f);
    if (j && j->joint) force = j->joint->GetReactionForce(invDt);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), force.x, force.y));
    return GM_OK;
}

int GM_CDECL gmf_getReactionTorque(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(invDt, 0, 0.0f);
    a_thread->PushFloat(j && j->joint ? j->joint->GetReactionTorque(invDt) : 0.0f);
    return GM_OK;
}

int GM_CDECL gmf_setRevoluteAngularConstraint(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    if (j->type != kRevolute) { GM_EXCEPTION_MSG("not a revolute joint"); return GM_EXCEPTION; }
    GM_CHECK_NUM_PARAMS(3); GM_INT_PARAM(enabled, 0, 0);
    GM_FLOAT_OR_INT_PARAM(lo, 1, 0.0f); GM_FLOAT_OR_INT_PARAM(hi, 2, 0.0f);
    j->angularLimitEnabled = enabled != 0; j->lowerAngle = lo; j->upperAngle = hi;
    if (b2RevoluteJoint* r = dynamic_cast<b2RevoluteJoint*>(j->joint)) {
        r->EnableLimit(j->angularLimitEnabled); r->SetLimits(lo, hi);
    }
    return GM_OK;
}

int GM_CDECL gmf_setPinAngularConstraint(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    if (j->type != kPin) { GM_EXCEPTION_MSG("not a pin joint"); return GM_EXCEPTION; }
    GM_CHECK_NUM_PARAMS(3); GM_INT_PARAM(enabled, 0, 0);
    GM_FLOAT_OR_INT_PARAM(lo, 1, 0.0f); GM_FLOAT_OR_INT_PARAM(hi, 2, 0.0f);
    j->angularLimitEnabled = enabled != 0; j->lowerAngle = lo; j->upperAngle = hi;
    if (b2RevoluteJoint* r = dynamic_cast<b2RevoluteJoint*>(j->joint)) {
        r->EnableLimit(j->angularLimitEnabled); r->SetLimits(lo, hi);
    }
    return GM_OK;
}

int GM_CDECL gmf_setPrismaticWorldAxis(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    if (j->type != kPrismatic) { GM_EXCEPTION_MSG("not a prismatic joint"); return GM_EXCEPTION; }
    GM_CHECK_NUM_PARAMS(1); float x, y;
    if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y)) return GM_EXCEPTION;
    j->prismaticAxis.Set(x, y); j->hasPrismaticAxis = true;
    return GM_OK;
}

int GM_CDECL gmf_setPrismaticTranslation(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    if (j->type != kPrismatic) { GM_EXCEPTION_MSG("not a prismatic joint"); return GM_EXCEPTION; }
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(speed, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(force, 1, 0.0f);
    j->motorEnabled = true; j->motorSpeed = speed; j->motorMax = force;
    if (b2PrismaticJoint* p = dynamic_cast<b2PrismaticJoint*>(j->joint)) {
        p->EnableMotor(true); p->SetMotorSpeed(speed); p->SetMaxMotorForce(force);
    }
    return GM_OK;
}

int GM_CDECL gmf_getBody1(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j || !j->body1) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(FindPhysicsBodyVarForB2Body(*a_thread->GetMachine(), j->body1)); return GM_OK;
}
int GM_CDECL gmf_getBody2(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j || !j->body2 || j->type == kPin) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(FindPhysicsBodyVarForB2Body(*a_thread->GetMachine(), j->body2)); return GM_OK;
}
int GM_CDECL gmf_getEntity1(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); a_thread->Push(j && j->body1 ? FindOwnerEntityForB2Body(j->body1) : gmVariable::s_null); return GM_OK;
}
int GM_CDECL gmf_getEntity2(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); a_thread->Push(j && j->body2 && j->type != kPin ? FindOwnerEntityForB2Body(j->body2) : gmVariable::s_null); return GM_OK;
}

int GM_CDECL gmf_getAnchorPos(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    b2Vec2 p(0.0f, 0.0f);
    if (j && j->joint) p = j->joint->GetAnchorA();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), p.x, p.y));
    return GM_OK;
}

int GM_CDECL gmf_setRelativeVel(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    if (j->type != kRelative) {
        GM_EXCEPTION_MSG("Error: Calling setRelativeVel on a non Relative Joint!!");
        return GM_EXCEPTION;
    }
    float x, y;
    if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y)) return GM_EXCEPTION;
    j->relativeVelocity.Set(x, y);
    if (b2RelativeJoint* relative = dynamic_cast<b2RelativeJoint*>(j->joint)) {
        relative->SetRelativeVelocity(j->relativeVelocity);
    }
    return GM_OK;
}

int GM_CDECL gmf_getObjectID(gmThread* a_thread) {
    JointHandle* handle = ThisHandle(a_thread);
    a_thread->PushInt(handle && ResolveJoint(handle->id) ? handle->id : 0);
    return GM_OK;
}

int GM_CDECL gmf_getType(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread);
    a_thread->PushInt(j ? j->type : 0);
    return GM_OK;
}

int GM_CDECL gmf_setIsNeedsHighAccuracy(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0); j->needsHighAccuracy = v != 0; return GM_OK;
}

int GM_CDECL gmf_setAngularSpring(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(d, 1, 0.0f);
    // Retail explicitly warns springs are disabled, then only stores these.
    j->angularSpringTension = t; j->angularSpringDamping = d; return GM_OK;
}
int GM_CDECL gmf_setLinearSpring(gmThread* a_thread) {
    JointData* j = ThisJoint(a_thread); if (!j) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(d, 1, 0.0f);
    j->linearSpringTension = t; j->linearSpringDamping = d; return GM_OK;
}

gmFunctionEntry s_jointTypeLib[] = {
    {"setupJoint", gmf_setupJoint, nullptr},
    {"setupDistanceJoint", gmf_setupDistanceJoint, nullptr},
    {"setupPinJoint", gmf_setupPinJoint, nullptr},
    {"setupRelativeJoint", gmf_setupRelativeJoint, nullptr},
    {"getType", gmf_getType, nullptr},
    {"setIsBodiesCollidable", gmf_setIsBodiesCollidable, nullptr},
    {"isBodiesCollidable", gmf_isBodiesCollidable, nullptr},
    {"setMotor", gmf_setMotor, nullptr},
    {"getReactionForce", gmf_getReactionForce, nullptr},
    {"getReactionTorque", gmf_getReactionTorque, nullptr},
    {"setRevoluteAngularConstraint", gmf_setRevoluteAngularConstraint, nullptr},
    {"setPinAngularConstraint", gmf_setPinAngularConstraint, nullptr},
    {"setPrismaticWorldAxis", gmf_setPrismaticWorldAxis, nullptr},
    {"setPrismaticTranslation", gmf_setPrismaticTranslation, nullptr},
    {"setRevoluteMotor", gmf_setRevoluteMotor, nullptr},
    {"setPinMotor", gmf_setPinMotor, nullptr},
    {"setRTMotorSpeed", gmf_setRTMotorSpeed, nullptr},
    {"setRTMaxMotorTorque", gmf_setRTMaxMotorTorque, nullptr},
    {"getRTMotorSpeed", gmf_getRTMotorSpeed, nullptr},
    {"getBody1", gmf_getBody1, nullptr}, {"getBody2", gmf_getBody2, nullptr},
    {"getEntity1", gmf_getEntity1, nullptr}, {"getEntity2", gmf_getEntity2, nullptr},
    {"getAnchorPos", gmf_getAnchorPos, nullptr},
    {"setRelativeVel", gmf_setRelativeVel, nullptr},
    {"getObjectID", gmf_getObjectID, nullptr},
    {"setIsNeedsHighAccuracy", gmf_setIsNeedsHighAccuracy, nullptr},
    {"setAngularSpring", gmf_setAngularSpring, nullptr},
    {"setLinearSpring", gmf_setLinearSpring, nullptr},
};

gmFunctionEntry s_jointGlobalLib[] = {
    {"createJoint", gmf_createJoint, nullptr},
    {"deleteJoint", gmf_deleteJoint, nullptr},
    {"deleteAllJoints", gmf_deleteAllJoints, nullptr},
    {"getJoint", gmf_getJoint, nullptr},
};

void JointGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (JointHandle*)a_object->m_user;
}
void JointIterGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (JointIterData*)a_object->m_user;
}
int GM_CDECL gmf_jointIterValid(gmThread* a_thread) {
    JointIterData* it = (JointIterData*)a_thread->ThisUserCheckType(g_jointIterType);
    a_thread->PushInt(it && it->index < it->ids.size() ? 1 : 0); return GM_OK;
}
int GM_CDECL gmf_jointIterNext(gmThread* a_thread) {
    JointIterData* it = (JointIterData*)a_thread->ThisUserCheckType(g_jointIterType);
    if (it && it->index < it->ids.size()) ++it->index; return GM_OK;
}
int GM_CDECL gmf_jointIterGet(gmThread* a_thread) {
    JointIterData* it = (JointIterData*)a_thread->ThisUserCheckType(g_jointIterType);
    if (it && it->index < it->ids.size() && ResolveJoint(it->ids[it->index]))
        a_thread->Push(MakeJointHandle(*a_thread->GetMachine(), it->ids[it->index]));
    else a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_jointIterSize(gmThread* a_thread) {
    JointIterData* it = (JointIterData*)a_thread->ThisUserCheckType(g_jointIterType);
    a_thread->PushInt(it ? (gmint)it->ids.size() : 0);
    return GM_OK;
}
gmFunctionEntry s_jointIterLib[] = {
    {"isValid", gmf_jointIterValid, nullptr},
    {"goToNext", gmf_jointIterNext, nullptr},
    {"get", gmf_jointIterGet, nullptr},
    {"size", gmf_jointIterSize, nullptr},
};

} // namespace

gmVariable MakePhysicsJointIterator(gmMachine& a_machine, const std::vector<b2Body*>& a_bodies) {
    std::unique_ptr<JointIterData> iter(new JointIterData());
    for (const auto& entry : g_joints) {
        JointData& j = *entry.second;
        if (!j.joint) continue;
        bool attached = false;
        for (b2Body* body : a_bodies) if (body && (body == j.body1 || body == j.body2)) { attached = true; break; }
        if (attached) iter->ids.push_back(j.id);
    }
    std::sort(iter->ids.begin(), iter->ids.end());
    return gmVariable(a_machine.AllocUserObject(iter.release(), g_jointIterType));
}

void RemovePhysicsJointsForBody(b2Body* a_body) {
    if (!a_body) return;
    for (auto& entry : g_joints) {
        JointData& j = *entry.second;
        if (j.body1 == a_body || j.body2 == a_body) {
            DestroyConstraint(j);
            j.body1 = j.body2 = nullptr;
        }
    }
}

void RegisterPhysicsJointBindings(gmMachine& a_machine) {
    g_jointType = a_machine.CreateUserType("PHYSICSJOINT");
    a_machine.RegisterTypeLibrary(g_jointType, s_jointTypeLib,
                                  sizeof(s_jointTypeLib) / sizeof(s_jointTypeLib[0]));
    a_machine.RegisterUserCallbacks(g_jointType, nullptr, JointGCDestruct, nullptr);
    g_jointIterType = a_machine.CreateUserType("PHYSICSJOINTLISTREFCONTAINER");
    a_machine.RegisterTypeLibrary(g_jointIterType, s_jointIterLib,
                                  sizeof(s_jointIterLib) / sizeof(s_jointIterLib[0]));
    a_machine.RegisterUserCallbacks(g_jointIterType, nullptr, JointIterGCDestruct, nullptr);
    a_machine.RegisterLibrary(s_jointGlobalLib,
                              sizeof(s_jointGlobalLib) / sizeof(s_jointGlobalLib[0]), "Physics");
}
