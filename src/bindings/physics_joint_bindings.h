#pragma once

#include "gmMachine.h"
#include <vector>

class b2Body;

void RegisterPhysicsJointBindings(gmMachine& a_machine);

// Build the retail-style joint-list iterator used by PhysicsData.getJointsOnMe.
gmVariable MakePhysicsJointIterator(gmMachine& a_machine,
                                    const std::vector<b2Body*>& a_bodies);

// Box2D destroys connected joints when a body is destroyed. Clear the
// matching retail wrappers before that happens so no wrapper retains a
// dangling b2Joint pointer.
void RemovePhysicsJointsForBody(b2Body* a_body);
