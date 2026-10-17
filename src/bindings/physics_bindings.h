#pragma once
#include "gmMachine.h"
#include <box2d/box2d.h>
#include <vector>
#include <memory>

// Exposed for physics_body_bindings.cpp's appendShapes(sd) -- consumes the
// fixture-def list a PHYSICSSHAPECOLLECTIONDEF built up.
struct ShapeDefCollection {
    std::vector<std::unique_ptr<b2Shape>> shapes;
    std::vector<b2FixtureDef> fixtureDefs;
    // Retail collection fields at +0x1c..+0x32. The append path copies
    // these values to every shape; setters act on the collection object.
    float friction = 0.2f;
    float playerFriction = 0.3f;
    float restitution = 0.0f;
    float density = 0.0f;
    bool isSensor = false;
    uint16 category = 1;
    uint16 mask = 0xffff;

    void Reset() {
        shapes.clear();
        fixtureDefs.clear();
        friction = 0.2f;
        playerFriction = 0.3f;
        restitution = 0.0f;
        density = 0.0f;
        isSensor = false;
        category = 1;
        mask = 0xffff;
    }

    void AddFixture(b2Shape* a_shape) {
        shapes.emplace_back(a_shape);
        b2FixtureDef def;
        def.shape = a_shape;
        def.friction = friction;
        def.restitution = restitution;
        def.density = density;
        def.isSensor = isSensor;
        def.filter.categoryBits = category;
        def.filter.maskBits = mask;
        fixtureDefs.push_back(def);
    }
};
extern gmType g_physicsShapeDefType;

void RegisterPhysicsBindings(gmMachine& a_machine);
