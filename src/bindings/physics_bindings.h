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

    void Reset() {
        shapes.clear();
        fixtureDefs.clear();
    }

    void AddFixture(b2Shape* a_shape) {
        shapes.emplace_back(a_shape);
        b2FixtureDef def;
        def.shape = a_shape;
        // Box2D's own b2FixtureDef default is categoryBits=0x0001 -- the
        // exact same bit constants_bindings.cpp assigned
        // PLAYER_COLLISION_CATEGORY (1<<0), confirmed real usage
        // (Platformer/Player.gm explicitly calls
        // `pb.setCollisionCategory(PLAYER_COLLISION_CATEGORY)`). Any shape
        // whose own script never calls setCollisionCategory would
        // otherwise incidentally collide with that bit, making
        // category-based "is this the player" checks (e.g. the render
        // camera's player-tracking, draw_list.cpp) unreliable -- explicit
        // 0 here means "no category set" is unambiguous.
        def.filter.categoryBits = 0;
        fixtureDefs.push_back(def);
    }
};
extern gmType g_physicsShapeDefType;

// confirms addBox/addPolygon/addCircle/addEdges/setFrictionForPlayer as
// real methods on it). Real scripts build one of these, add shape(s), tune
// a couple of fixture properties, then hand it to a body
// (`pb.appendShapes(sd)`, confirmed in game_data/Platformer/EntityPacks/
// GoAroundGuy.gm) -- i.e. it's a Box2D b2FixtureDef list, not a live
// physics object. Backed by the real Box2D 2.4.1 (vendor/box2d), per
// "just import the real lib, no need to overcomplicate" -- this only
// covers building the fixture-def list; PhysicsData/PhysicsBody
// (createBody, appendShapes, simulation) are a separate, larger binding
// surface not implemented yet.
void RegisterPhysicsBindings(gmMachine& a_machine);
