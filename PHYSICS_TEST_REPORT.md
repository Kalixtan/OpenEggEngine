# Physics test report

Generated: 2026-10-20 19:43 UTC

Matched: **1906/2290 declared readings (83%)**. This measures captured cases, not total physics parity.

The current query failures share one observed cause: OpenEgg recreates overlap records after a filter is restored while retail keeps them absent for the remaining sampled ticks.

Functions with readings: **56/293**. The inventory comes from the local API reference and includes OpenEgg-only methods.

## Sample groups

Scenes sample motion; other groups cover callbacks, removal, queries, and rays.

| Group | Matched | Cases |
| --- | ---: | ---: |
| scene.free | 48 | 48 |
| scene.fast | 48 | 48 |
| scene.solid | 72 | 72 |
| scene.sensor | 60 | 60 |
| contact.solid | 108 | 108 |
| contact.sensor | 108 | 108 |
| removal.body | 98 | 98 |
| query.solidA | 286 | 406 |
| query.solidB | 286 | 406 |
| query.sensorA | 334 | 406 |
| query.sensorB | 334 | 406 |
| ray.solid | 9 | 9 |
| ray.sensors | 9 | 9 |
| ray.inside | 9 | 9 |
| ray.miss | 9 | 9 |
| ray.bodyA | 11 | 11 |
| ray.bodyB | 11 | 11 |
| ray.bodyMiss | 11 | 11 |

## Per function

| Function | Matched | Cases | Status |
| --- | ---: | ---: | --- |
| `Entity.getPhysicsData` | 1 | 1 | PASS |
| `EntityManager.createEntity` | 1 | 1 | PASS |
| `Global.PHYSICSSHAPECOLLECTIONDEF` | 1 | 1 | PASS |
| `Physics.createJoint` | 0 | 0 | untested |
| `Physics.deleteAllJoints` | 0 | 0 | untested |
| `Physics.deleteJoint` | 0 | 0 | untested |
| `Physics.flush` | 0 | 0 | untested |
| `Physics.getBodyFromShapeID` | 0 | 0 | untested |
| `Physics.getEntityFromShapeID` | 0 | 0 | untested |
| `Physics.getJoint` | 0 | 0 | untested |
| `Physics.getNumFramesToIgnoreCollisions` | 3 | 3 | PASS |
| `Physics.getShapeByID` | 0 | 0 | untested |
| `Physics.getShapesNear` | 0 | 0 | untested |
| `Physics.getShapesRoughlyWithin` | 0 | 0 | untested |
| `Physics.getShapesWithin` | 0 | 0 | untested |
| `Physics.isShapeSensor` | 0 | 0 | untested |
| `Physics.raycast` | 8 | 8 | PASS |
| `Physics.raycastOne` | 24 | 24 | PASS |
| `Physics.raycastShapes` | 0 | 0 | untested |
| `Physics.raycastSimple` | 4 | 4 | PASS |
| `Physics.setNumFramesToIgnoreCollisions` | 0 | 0 | untested |
| `collisionInfo.getCollidingShapeIDList` | 0 | 0 | untested |
| `collisionInfo.getContactNormalOfMyCollidingShape` | 80 | 112 | FAIL |
| `collisionInfo.getEntity` | 40 | 56 | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | 40 | 56 | FAIL |
| `collisionInfo.getNumCollidingShapes` | 0 | 0 | untested |
| `collisionInfo.getShapeCollisionInfo` | 80 | 112 | FAIL |
| `collisionInfo.isAllSensors` | 40 | 56 | FAIL |
| `collisionInfo.isBodyColliding` | 40 | 56 | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | 40 | 56 | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | 40 | 56 | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | 40 | 56 | FAIL |
| `collisionInfo.isShapeColliding` | 40 | 56 | FAIL |
| `intIter.get` | 40 | 56 | FAIL |
| `intIter.goToNext` | 0 | 0 | untested |
| `intIter.isValid` | 0 | 0 | untested |
| `intIter.reset` | 0 | 0 | untested |
| `intIter.size` | 40 | 56 | FAIL |
| `jointIter.get` | 0 | 0 | untested |
| `jointIter.goToNext` | 0 | 0 | untested |
| `jointIter.isValid` | 0 | 0 | untested |
| `jointIter.size` | 0 | 0 | untested |
| `jointType.getAnchorPos` | 0 | 0 | untested |
| `jointType.getBody1` | 0 | 0 | untested |
| `jointType.getBody2` | 0 | 0 | untested |
| `jointType.getEntity1` | 0 | 0 | untested |
| `jointType.getEntity2` | 0 | 0 | untested |
| `jointType.getObjectID` | 0 | 0 | untested |
| `jointType.getRTMotorSpeed` | 0 | 0 | untested |
| `jointType.getReactionForce` | 0 | 0 | untested |
| `jointType.getReactionTorque` | 0 | 0 | untested |
| `jointType.getType` | 0 | 0 | untested |
| `jointType.isBodiesCollidable` | 0 | 0 | untested |
| `jointType.setAngularSpring` | 0 | 0 | untested |
| `jointType.setIsBodiesCollidable` | 0 | 0 | untested |
| `jointType.setIsNeedsHighAccuracy` | 0 | 0 | untested |
| `jointType.setLinearSpring` | 0 | 0 | untested |
| `jointType.setMotor` | 0 | 0 | untested |
| `jointType.setPinAngularConstraint` | 0 | 0 | untested |
| `jointType.setPinMotor` | 0 | 0 | untested |
| `jointType.setPrismaticTranslation` | 0 | 0 | untested |
| `jointType.setPrismaticWorldAxis` | 0 | 0 | untested |
| `jointType.setRTMaxMotorTorque` | 0 | 0 | untested |
| `jointType.setRTMotorSpeed` | 0 | 0 | untested |
| `jointType.setRelativeVel` | 0 | 0 | untested |
| `jointType.setRevoluteAngularConstraint` | 0 | 0 | untested |
| `jointType.setRevoluteMotor` | 0 | 0 | untested |
| `jointType.setupDistanceJoint` | 0 | 0 | untested |
| `jointType.setupJoint` | 0 | 0 | untested |
| `jointType.setupPinJoint` | 0 | 0 | untested |
| `jointType.setupRelativeJoint` | 0 | 0 | untested |
| `physicsBody.appendShapes` | 2 | 2 | PASS |
| `physicsBody.applyForce` | 0 | 0 | untested |
| `physicsBody.brake` | 0 | 0 | untested |
| `physicsBody.computeAngularVel` | 0 | 0 | untested |
| `physicsBody.disableRestitutionThisFrame` | 0 | 0 | untested |
| `physicsBody.doesRotationResponse` | 0 | 0 | untested |
| `physicsBody.dummy` | 0 | 0 | untested |
| `physicsBody.getAABB` | 0 | 0 | untested |
| `physicsBody.getAngularVel` | 0 | 0 | untested |
| `physicsBody.getBBox` | 0 | 0 | untested |
| `physicsBody.getBrakeAcc` | 0 | 0 | untested |
| `physicsBody.getCategoriesToCollideWith` | 58 | 58 | PASS |
| `physicsBody.getCenterOfMass` | 0 | 0 | untested |
| `physicsBody.getCollisionCategory` | 58 | 58 | PASS |
| `physicsBody.getCollisionGroup` | 56 | 56 | PASS |
| `physicsBody.getDir` | 0 | 0 | untested |
| `physicsBody.getDirRef` | 0 | 0 | untested |
| `physicsBody.getDirty` | 0 | 0 | untested |
| `physicsBody.getEntity` | 0 | 0 | untested |
| `physicsBody.getFriction` | 0 | 0 | untested |
| `physicsBody.getHeight` | 0 | 0 | untested |
| `physicsBody.getMass` | 0 | 0 | untested |
| `physicsBody.getMaxPos` | 0 | 0 | untested |
| `physicsBody.getMaxRotateSpeed` | 0 | 0 | untested |
| `physicsBody.getMaxSpeed` | 0 | 0 | untested |
| `physicsBody.getMinPos` | 0 | 0 | untested |
| `physicsBody.getNumShapeVerts` | 0 | 0 | untested |
| `physicsBody.getNumShapes` | 2 | 2 | PASS |
| `physicsBody.getObjectID` | 0 | 0 | untested |
| `physicsBody.getPos` | 100 | 100 | PASS |
| `physicsBody.getPosRef` | 0 | 0 | untested |
| `physicsBody.getPosVec` | 0 | 0 | untested |
| `physicsBody.getRadius` | 1 | 1 | PASS |
| `physicsBody.getRestitution` | 0 | 0 | untested |
| `physicsBody.getRotateAcc` | 0 | 0 | untested |
| `physicsBody.getRotateVel` | 0 | 0 | untested |
| `physicsBody.getShape` | 0 | 0 | untested |
| `physicsBody.getShapeCategoriesToCollideWith` | 0 | 0 | untested |
| `physicsBody.getShapeHeight` | 1 | 1 | PASS |
| `physicsBody.getShapeIndex` | 0 | 0 | untested |
| `physicsBody.getShapeObjectID` | 0 | 0 | untested |
| `physicsBody.getShapeOffset` | 0 | 0 | untested |
| `physicsBody.getShapeRadius` | 1 | 1 | PASS |
| `physicsBody.getShapeType` | 1 | 1 | PASS |
| `physicsBody.getShapeVert` | 0 | 0 | untested |
| `physicsBody.getShapeVerts` | 0 | 0 | untested |
| `physicsBody.getShapeWidth` | 1 | 1 | PASS |
| `physicsBody.getShapeWorldCenterPos` | 0 | 0 | untested |
| `physicsBody.getShapeWorldVert` | 0 | 0 | untested |
| `physicsBody.getSpeed` | 1 | 1 | PASS |
| `physicsBody.getThrustAcc` | 0 | 0 | untested |
| `physicsBody.getVel` | 98 | 98 | PASS |
| `physicsBody.getVelRef` | 0 | 0 | untested |
| `physicsBody.getWidth` | 0 | 0 | untested |
| `physicsBody.getWorldDir` | 0 | 0 | untested |
| `physicsBody.getWorldPos` | 0 | 0 | untested |
| `physicsBody.isBullet` | 12 | 12 | PASS |
| `physicsBody.isCircle` | 0 | 0 | untested |
| `physicsBody.isCollidable` | 56 | 56 | PASS |
| `physicsBody.isContainsShape` | 0 | 0 | untested |
| `physicsBody.isPointWithin` | 2 | 2 | PASS |
| `physicsBody.isPointWithinShape` | 0 | 0 | untested |
| `physicsBody.isRect` | 0 | 0 | untested |
| `physicsBody.isSensor` | 7 | 7 | PASS |
| `physicsBody.isShapeSensor` | 0 | 0 | untested |
| `physicsBody.isSleeping` | 0 | 0 | untested |
| `physicsBody.putToSleep` | 0 | 0 | untested |
| `physicsBody.raycast` | 12 | 12 | PASS |
| `physicsBody.raycastShapesOne` | 9 | 9 | PASS |
| `physicsBody.rebuildBodyMass` | 0 | 0 | untested |
| `physicsBody.removeShape` | 0 | 0 | untested |
| `physicsBody.reverseThrust` | 0 | 0 | untested |
| `physicsBody.rotate` | 0 | 0 | untested |
| `physicsBody.rotateLeft` | 0 | 0 | untested |
| `physicsBody.rotateRight` | 0 | 0 | untested |
| `physicsBody.rotateTo` | 0 | 0 | untested |
| `physicsBody.setAllowSleeping` | 0 | 0 | untested |
| `physicsBody.setAngularVel` | 0 | 0 | untested |
| `physicsBody.setAsCircle` | 0 | 0 | untested |
| `physicsBody.setAsRect` | 0 | 0 | untested |
| `physicsBody.setBrakeAcc` | 0 | 0 | untested |
| `physicsBody.setCategoriesToCollideWith` | 0 | 0 | untested |
| `physicsBody.setCollisionCategory` | 0 | 0 | untested |
| `physicsBody.setCollisionGroup` | 0 | 0 | untested |
| `physicsBody.setDir` | 0 | 0 | untested |
| `physicsBody.setDoesRotationResponse` | 0 | 0 | untested |
| `physicsBody.setEdgeShapeVertex` | 0 | 0 | untested |
| `physicsBody.setFriction` | 0 | 0 | untested |
| `physicsBody.setIsBullet` | 0 | 0 | untested |
| `physicsBody.setIsCollidable` | 0 | 0 | untested |
| `physicsBody.setIsSensor` | 0 | 0 | untested |
| `physicsBody.setIsShapeSensor` | 0 | 0 | untested |
| `physicsBody.setIsUsingContinuousPhysics` | 0 | 0 | untested |
| `physicsBody.setMass` | 0 | 0 | untested |
| `physicsBody.setMaxRotateSpeed` | 0 | 0 | untested |
| `physicsBody.setMaxRotationDecayRate` | 0 | 0 | untested |
| `physicsBody.setMaxSpeed` | 0 | 0 | untested |
| `physicsBody.setMaxVelocityDecayRate` | 0 | 0 | untested |
| `physicsBody.setPolygonShapeVertices` | 0 | 0 | untested |
| `physicsBody.setPos` | 0 | 0 | untested |
| `physicsBody.setRestitution` | 0 | 0 | untested |
| `physicsBody.setRotateAcc` | 0 | 0 | untested |
| `physicsBody.setRotateVel` | 0 | 0 | untested |
| `physicsBody.setRotationDecayRate` | 0 | 0 | untested |
| `physicsBody.setShapeCategoriesToCollideWith` | 0 | 0 | untested |
| `physicsBody.setShapeDensity` | 0 | 0 | untested |
| `physicsBody.setSpeed` | 0 | 0 | untested |
| `physicsBody.setThrustAcc` | 0 | 0 | untested |
| `physicsBody.setVel` | 0 | 0 | untested |
| `physicsBody.setVelocityDecayRate` | 0 | 0 | untested |
| `physicsBody.stopRotatingTo` | 0 | 0 | untested |
| `physicsBody.thrust` | 0 | 0 | untested |
| `physicsData.appendShapes` | 0 | 0 | untested |
| `physicsData.brake` | 0 | 0 | untested |
| `physicsData.createBody` | 2 | 2 | PASS |
| `physicsData.doesRotationResponse` | 0 | 0 | untested |
| `physicsData.getAABB` | 0 | 0 | untested |
| `physicsData.getBody` | 1 | 1 | PASS |
| `physicsData.getBrakeAcc` | 0 | 0 | untested |
| `physicsData.getCategoriesToCollideWith` | 0 | 0 | untested |
| `physicsData.getCollidingEntities` | 0 | 0 | untested |
| `physicsData.getCollisionCategory` | 0 | 0 | untested |
| `physicsData.getCollisionGroup` | 0 | 0 | untested |
| `physicsData.getCollisionInfo` | 40 | 56 | FAIL |
| `physicsData.getDir` | 0 | 0 | untested |
| `physicsData.getHeight` | 0 | 0 | untested |
| `physicsData.getJointsOnMe` | 0 | 0 | untested |
| `physicsData.getMass` | 0 | 0 | untested |
| `physicsData.getMaxPos` | 0 | 0 | untested |
| `physicsData.getMaxRotateSpeed` | 0 | 0 | untested |
| `physicsData.getMaxSpeed` | 0 | 0 | untested |
| `physicsData.getMinPos` | 0 | 0 | untested |
| `physicsData.getNumBodies` | 17 | 17 | PASS |
| `physicsData.getPos` | 0 | 0 | untested |
| `physicsData.getPosRef` | 0 | 0 | untested |
| `physicsData.getRadius` | 0 | 0 | untested |
| `physicsData.getRotateAcc` | 0 | 0 | untested |
| `physicsData.getShapeByID` | 0 | 0 | untested |
| `physicsData.getShapeRadius` | 0 | 0 | untested |
| `physicsData.getSpeed` | 0 | 0 | untested |
| `physicsData.getThrustAcc` | 0 | 0 | untested |
| `physicsData.getType` | 0 | 0 | untested |
| `physicsData.getVel` | 0 | 0 | untested |
| `physicsData.getWidth` | 0 | 0 | untested |
| `physicsData.isCircle` | 0 | 0 | untested |
| `physicsData.isCollidable` | 0 | 0 | untested |
| `physicsData.isColliding` | 129 | 161 | FAIL |
| `physicsData.isPointWithin` | 0 | 0 | untested |
| `physicsData.isRect` | 0 | 0 | untested |
| `physicsData.isSensor` | 7 | 7 | PASS |
| `physicsData.isSleeping` | 0 | 0 | untested |
| `physicsData.putToSleep` | 0 | 0 | untested |
| `physicsData.raycast` | 6 | 6 | PASS |
| `physicsData.raycastShapesOne` | 6 | 6 | PASS |
| `physicsData.removeBody` | 0 | 0 | untested |
| `physicsData.replaceBody` | 0 | 0 | untested |
| `physicsData.reverseThrust` | 0 | 0 | untested |
| `physicsData.rotateLeft` | 0 | 0 | untested |
| `physicsData.rotateRight` | 0 | 0 | untested |
| `physicsData.rotateTo` | 0 | 0 | untested |
| `physicsData.setAllowSleeping` | 0 | 0 | untested |
| `physicsData.setAsCircle` | 0 | 0 | untested |
| `physicsData.setAsRect` | 0 | 0 | untested |
| `physicsData.setBrakeAcc` | 0 | 0 | untested |
| `physicsData.setCategoriesToCollideWith` | 0 | 0 | untested |
| `physicsData.setCollisionCategory` | 0 | 0 | untested |
| `physicsData.setCollisionGroup` | 0 | 0 | untested |
| `physicsData.setDir` | 0 | 0 | untested |
| `physicsData.setDoesRotationResponse` | 0 | 0 | untested |
| `physicsData.setIsCollidable` | 0 | 0 | untested |
| `physicsData.setIsSensor` | 0 | 0 | untested |
| `physicsData.setMass` | 0 | 0 | untested |
| `physicsData.setMaxRotateSpeed` | 0 | 0 | untested |
| `physicsData.setMaxRotationDecayRate` | 0 | 0 | untested |
| `physicsData.setMaxSpeed` | 0 | 0 | untested |
| `physicsData.setMaxVelocityDecayRate` | 0 | 0 | untested |
| `physicsData.setPos` | 0 | 0 | untested |
| `physicsData.setRestitution` | 0 | 0 | untested |
| `physicsData.setRotateAcc` | 0 | 0 | untested |
| `physicsData.setRotationDecayRate` | 0 | 0 | untested |
| `physicsData.setSpeed` | 0 | 0 | untested |
| `physicsData.setThrustAcc` | 0 | 0 | untested |
| `physicsData.setVel` | 0 | 0 | untested |
| `physicsData.setVelocityDecayRate` | 0 | 0 | untested |
| `physicsData.setVisibilityAABBScale` | 0 | 0 | untested |
| `physicsData.stopRotatingTo` | 0 | 0 | untested |
| `physicsData.thrust` | 0 | 0 | untested |
| `physicsData.wakeUp` | 0 | 0 | untested |
| `physicsShape.dummy` | 0 | 0 | untested |
| `physicsShape.getIsSensor` | 0 | 0 | untested |
| `physicsShape.getType` | 0 | 0 | untested |
| `physicsShape.getVertexCount` | 0 | 0 | untested |
| `physicsShape.getVertexWorldPos` | 0 | 0 | untested |
| `physicsShape.isSensor` | 0 | 0 | untested |
| `physicsShape.setCategoriesToCollideWith` | 0 | 0 | untested |
| `physicsShape.setCollisionCategory` | 0 | 0 | untested |
| `physicsShape.setFrictionForPlayer` | 0 | 0 | untested |
| `physicsShape.setIsSensor` | 0 | 0 | untested |
| `shapeCollisionInfo.disableContact` | 0 | 0 | untested |
| `shapeCollisionInfo.getContactNormal` | 80 | 112 | FAIL |
| `shapeCollisionInfo.getContactPos` | 80 | 112 | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | 96 | 112 | FAIL |
| `shapeCollisionInfo.getNumContacts` | 40 | 56 | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | 40 | 56 | FAIL |
| `shapeDef.addBox` | 0 | 0 | untested |
| `shapeDef.addCircle` | 1 | 1 | PASS |
| `shapeDef.addEdges` | 0 | 0 | untested |
| `shapeDef.addPolygon` | 0 | 0 | untested |
| `shapeDef.getCategoriesToCollideWith` | 1 | 1 | PASS |
| `shapeDef.getCollisionCategory` | 2 | 2 | PASS |
| `shapeDef.getFriction` | 0 | 0 | untested |
| `shapeDef.getMass` | 2 | 2 | PASS |
| `shapeDef.getRestitution` | 0 | 0 | untested |
| `shapeDef.isSensor` | 3 | 3 | PASS |
| `shapeDef.reset` | 0 | 0 | untested |
| `shapeDef.setCategoriesToCollideWith` | 0 | 0 | untested |
| `shapeDef.setCollisionCategory` | 0 | 0 | untested |
| `shapeDef.setFriction` | 0 | 0 | untested |
| `shapeDef.setFrictionForPlayer` | 0 | 0 | untested |
| `shapeDef.setIsSensor` | 0 | 0 | untested |
| `shapeDef.setMass` | 0 | 0 | untested |
| `shapeDef.setRestitution` | 0 | 0 | untested |

## Contact callbacks

| Hook | Matched | Cases |
| --- | ---: | ---: |
| `Contact callback argument` | 50 | 50 |
| `Contact callback order` | 25 | 25 |
| `Entity.onEntityCollide` | 50 | 50 |
| `Entity.onEntityCollidePreSolve` | 50 | 50 |
| `Entity.onEntitySeparate` | 50 | 50 |
| `Entity.onEntityTouch` | 50 | 50 |

## Readings

| Function | Case | Retail | OpenEgg | Result |
| --- | --- | --- | --- | --- |
| `EntityManager.createEntity` | EntityManager.createEntity.exists | `1` | `1` | PASS |
| `Entity.getPhysicsData` | Entity.getPhysicsData.exists | `1` | `1` | PASS |
| `physicsData.getNumBodies` | PhysicsData.getNumBodies.initial | `0` | `0` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.noBodies | `1` | `1` | PASS |
| `physicsData.getNumBodies` | PhysicsData.getNumBodies.afterSensorQuery | `0` | `0` | PASS |
| `physicsData.createBody` | PhysicsData.createBody.index | `0` | `0` | PASS |
| `physicsData.getNumBodies` | PhysicsData.getNumBodies.created | `1` | `1` | PASS |
| `physicsData.getBody` | PhysicsData.getBody.exists | `1` | `1` | PASS |
| `physicsBody.getNumShapes` | PhysicsBody.getNumShapes.initial | `0` | `0` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.empty | `1` | `1` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.empty | `1` | `1` | PASS |
| `physicsBody.getPos` | PhysicsBody.getPos.x.initial | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | PhysicsBody.getPos.y.initial | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | PhysicsBody.getPos.x.set | `12.000000` | `12.000000` | PASS |
| `physicsBody.getPos` | PhysicsBody.getPos.y.set | `-7.000000` | `-7.000000` | PASS |
| `physicsBody.getVel` | PhysicsBody.getVel.x.set | `0.125000` | `0.125000` | PASS |
| `physicsBody.getVel` | PhysicsBody.getVel.y.set | `-0.250000` | `-0.250000` | PASS |
| `physicsBody.getSpeed` | PhysicsBody.getSpeed.set | `0.279509` | `0.279509` | PASS |
| `shapeDef.getMass` | ShapeDef.getMass.initial | `0.000000` | `0.000000` | PASS |
| `shapeDef.getMass` | ShapeDef.getMass.set | `0.000000` | `0.000000` | PASS |
| `shapeDef.isSensor` | ShapeDef.isSensor.initial | `0` | `0` | PASS |
| `shapeDef.isSensor` | ShapeDef.isSensor.set | `1` | `1` | PASS |
| `shapeDef.isSensor` | ShapeDef.isSensor.reset | `0` | `0` | PASS |
| `shapeDef.getCollisionCategory` | ShapeDef.getCollisionCategory.initial | `1` | `1` | PASS |
| `shapeDef.getCollisionCategory` | ShapeDef.getCollisionCategory.set | `8` | `8` | PASS |
| `shapeDef.getCategoriesToCollideWith` | ShapeDef.getCategoriesToCollideWith.set | `11` | `11` | PASS |
| `physicsBody.appendShapes` | PhysicsBody.appendShapes.index | `0` | `0` | PASS |
| `physicsBody.getNumShapes` | PhysicsBody.getNumShapes.circle | `1` | `1` | PASS |
| `physicsBody.getRadius` | PhysicsBody.getRadius.circle | `5.000000` | `5.000000` | PASS |
| `physicsBody.getShapeRadius` | PhysicsBody.getShapeRadius.circle | `5.000000` | `5.000000` | PASS |
| `physicsBody.getShapeType` | PhysicsBody.getShapeType.circle | `0` | `0` | PASS |
| `physicsBody.getShapeWidth` | PhysicsBody.getShapeWidth.circle | `10.000000` | `10.000000` | PASS |
| `physicsBody.getShapeHeight` | PhysicsBody.getShapeHeight.circle | `10.000000` | `10.000000` | PASS |
| `physicsBody.getCollisionCategory` | PhysicsBody.getCollisionCategory.fromDef | `8` | `8` | PASS |
| `physicsBody.getCategoriesToCollideWith` | PhysicsBody.getCategoriesToCollideWith.fromDef | `11` | `11` | PASS |
| `physicsBody.getCollisionCategory` | PhysicsBody.getCollisionCategory.set | `4` | `4` | PASS |
| `physicsBody.getCategoriesToCollideWith` | PhysicsBody.getCategoriesToCollideWith.set | `7` | `7` | PASS |
| `physicsBody.isPointWithin` | PhysicsBody.isPointWithin.inside | `1` | `1` | PASS |
| `physicsBody.isPointWithin` | PhysicsBody.isPointWithin.outside | `0` | `0` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.set | `1` | `1` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.set | `1` | `1` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.reset | `0` | `0` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.reset | `0` | `0` | PASS |
| `physicsData.createBody` | PhysicsData.createBody.second | `1` | `1` | PASS |
| `physicsBody.appendShapes` | PhysicsBody.appendShapes.second | `0` | `0` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.mixed | `0` | `0` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.firstAfterDataSet | `1` | `1` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.secondAfterDataSet | `1` | `1` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.allSet | `1` | `1` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.firstAfterDataReset | `0` | `0` | PASS |
| `physicsBody.isSensor` | PhysicsBody.isSensor.secondAfterDataReset | `0` | `0` | PASS |
| `physicsData.isSensor` | PhysicsData.isSensor.allReset | `0` | `0` | PASS |
| `Physics.getNumFramesToIgnoreCollisions` | Physics.getNumFramesToIgnoreCollisions.initial | `0` | `0` | PASS |
| `Physics.getNumFramesToIgnoreCollisions` | Physics.getNumFramesToIgnoreCollisions.set | `3` | `3` | PASS |
| `Physics.getNumFramesToIgnoreCollisions` | Physics.getNumFramesToIgnoreCollisions.reset | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t0.x | `-100.000000` | `-100.000000` | PASS |
| `physicsBody.getPos` | scene.free.t0.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | scene.free.t0.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t0.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t0.x | `-500.000000` | `-500.000000` | PASS |
| `physicsBody.getPos` | scene.fast.t0.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t0.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t0.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t0.y | `20.000000` | `20.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t0.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t0.vy | `-0.200000` | `-0.200000` | PASS |
| `physicsData.isColliding` | scene.solid.t0.colliding | `0` | `0` | PASS |
| `physicsBody.isBullet` | scene.solid.t0.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t0.x | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t0.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t0.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t0.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t1.x | `-98.333336` | `-98.333336` | PASS |
| `physicsBody.getPos` | scene.free.t1.y | `100.833336` | `100.833336` | PASS |
| `physicsBody.getVel` | scene.free.t1.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t1.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t1.x | `-466.666656` | `-466.666656` | PASS |
| `physicsBody.getPos` | scene.fast.t1.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t1.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t1.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t1.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t1.y | `16.666666` | `16.666666` | PASS |
| `physicsBody.getVel` | scene.solid.t1.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t1.vy | `-0.200000` | `-0.200000` | PASS |
| `physicsData.isColliding` | scene.solid.t1.colliding | `0` | `0` | PASS |
| `physicsBody.isBullet` | scene.solid.t1.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t1.x | `33.333332` | `33.333332` | PASS |
| `physicsBody.getPos` | scene.sensor.t1.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t1.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t1.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t1.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t2.x | `-96.666672` | `-96.666672` | PASS |
| `physicsBody.getPos` | scene.free.t2.y | `101.666672` | `101.666672` | PASS |
| `physicsBody.getVel` | scene.free.t2.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t2.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t2.x | `-433.333313` | `-433.333313` | PASS |
| `physicsBody.getPos` | scene.fast.t2.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t2.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t2.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t2.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t2.y | `13.333333` | `13.333333` | PASS |
| `physicsBody.getVel` | scene.solid.t2.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t2.vy | `-0.200000` | `-0.200000` | PASS |
| `physicsData.isColliding` | scene.solid.t2.colliding | `0` | `0` | PASS |
| `physicsBody.isBullet` | scene.solid.t2.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t2.x | `36.666664` | `36.666664` | PASS |
| `physicsBody.getPos` | scene.sensor.t2.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t2.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t2.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t2.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t3.x | `-95.000008` | `-95.000008` | PASS |
| `physicsBody.getPos` | scene.free.t3.y | `102.500008` | `102.500008` | PASS |
| `physicsBody.getVel` | scene.free.t3.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t3.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t3.x | `-399.999969` | `-399.999969` | PASS |
| `physicsBody.getPos` | scene.fast.t3.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t3.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t3.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t3.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t3.y | `10.000000` | `10.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t3.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t3.vy | `-0.200000` | `-0.200000` | PASS |
| `physicsData.isColliding` | scene.solid.t3.colliding | `0` | `0` | PASS |
| `physicsBody.isBullet` | scene.solid.t3.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t3.x | `39.999996` | `39.999996` | PASS |
| `physicsBody.getPos` | scene.sensor.t3.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t3.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t3.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t3.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t4.x | `-93.333344` | `-93.333344` | PASS |
| `physicsBody.getPos` | scene.free.t4.y | `103.333344` | `103.333344` | PASS |
| `physicsBody.getVel` | scene.free.t4.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t4.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t4.x | `-366.666626` | `-366.666626` | PASS |
| `physicsBody.getPos` | scene.fast.t4.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t4.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t4.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t4.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t4.y | `6.666667` | `6.666667` | PASS |
| `physicsBody.getVel` | scene.solid.t4.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t4.vy | `-0.200000` | `-0.200000` | PASS |
| `physicsData.isColliding` | scene.solid.t4.colliding | `0` | `0` | PASS |
| `physicsBody.isBullet` | scene.solid.t4.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t4.x | `43.333328` | `43.333328` | PASS |
| `physicsBody.getPos` | scene.sensor.t4.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t4.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t4.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t4.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t5.x | `-91.666679` | `-91.666679` | PASS |
| `physicsBody.getPos` | scene.free.t5.y | `104.166679` | `104.166679` | PASS |
| `physicsBody.getVel` | scene.free.t5.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t5.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t5.x | `-333.333282` | `-333.333282` | PASS |
| `physicsBody.getPos` | scene.fast.t5.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t5.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t5.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t5.y | `3.333333` | `3.333333` | PASS |
| `physicsBody.getVel` | scene.solid.t5.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t5.vy | `-0.200000` | `-0.200000` | PASS |
| `physicsData.isColliding` | scene.solid.t5.colliding | `0` | `0` | PASS |
| `physicsBody.isBullet` | scene.solid.t5.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t5.x | `46.666660` | `46.666660` | PASS |
| `physicsBody.getPos` | scene.sensor.t5.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t5.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t5.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t6.x | `-90.000015` | `-90.000015` | PASS |
| `physicsBody.getPos` | scene.free.t6.y | `105.000015` | `105.000015` | PASS |
| `physicsBody.getVel` | scene.free.t6.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t6.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t6.x | `-299.999939` | `-299.999939` | PASS |
| `physicsBody.getPos` | scene.fast.t6.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t6.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t6.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t6.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t6.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t6.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t6.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t6.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t6.x | `49.999992` | `49.999992` | PASS |
| `physicsBody.getPos` | scene.sensor.t6.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t6.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t6.colliding | `1` | `1` | PASS |
| `physicsBody.getPos` | scene.free.t7.x | `-88.333351` | `-88.333351` | PASS |
| `physicsBody.getPos` | scene.free.t7.y | `105.833351` | `105.833351` | PASS |
| `physicsBody.getVel` | scene.free.t7.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t7.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t7.x | `-266.666595` | `-266.666595` | PASS |
| `physicsBody.getPos` | scene.fast.t7.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t7.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t7.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t7.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t7.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t7.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t7.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t7.x | `53.333324` | `53.333324` | PASS |
| `physicsBody.getPos` | scene.sensor.t7.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t7.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t7.colliding | `1` | `1` | PASS |
| `physicsBody.getPos` | scene.free.t8.x | `-86.666687` | `-86.666687` | PASS |
| `physicsBody.getPos` | scene.free.t8.y | `106.666687` | `106.666687` | PASS |
| `physicsBody.getVel` | scene.free.t8.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t8.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t8.x | `-233.333267` | `-233.333267` | PASS |
| `physicsBody.getPos` | scene.fast.t8.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t8.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t8.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t8.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t8.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t8.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t8.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t8.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t8.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t8.x | `56.666656` | `56.666656` | PASS |
| `physicsBody.getPos` | scene.sensor.t8.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t8.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t8.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t8.colliding | `1` | `1` | PASS |
| `physicsBody.getPos` | scene.free.t9.x | `-85.000023` | `-85.000023` | PASS |
| `physicsBody.getPos` | scene.free.t9.y | `107.500023` | `107.500023` | PASS |
| `physicsBody.getVel` | scene.free.t9.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t9.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t9.x | `-199.999939` | `-199.999939` | PASS |
| `physicsBody.getPos` | scene.fast.t9.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t9.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t9.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t9.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t9.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t9.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t9.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t9.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t9.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t9.x | `59.999989` | `59.999989` | PASS |
| `physicsBody.getPos` | scene.sensor.t9.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t9.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t9.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t9.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t10.x | `-83.333359` | `-83.333359` | PASS |
| `physicsBody.getPos` | scene.free.t10.y | `108.333359` | `108.333359` | PASS |
| `physicsBody.getVel` | scene.free.t10.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t10.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t10.x | `-166.666611` | `-166.666611` | PASS |
| `physicsBody.getPos` | scene.fast.t10.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t10.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t10.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t10.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t10.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t10.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t10.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t10.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t10.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t10.x | `63.333321` | `63.333321` | PASS |
| `physicsBody.getPos` | scene.sensor.t10.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t10.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t10.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t10.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t11.x | `-81.666695` | `-81.666695` | PASS |
| `physicsBody.getPos` | scene.free.t11.y | `109.166695` | `109.166695` | PASS |
| `physicsBody.getVel` | scene.free.t11.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t11.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t11.x | `-133.333282` | `-133.333282` | PASS |
| `physicsBody.getPos` | scene.fast.t11.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t11.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t11.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t11.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t11.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t11.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t11.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t11.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t11.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t11.x | `66.666656` | `66.666656` | PASS |
| `physicsBody.getPos` | scene.sensor.t11.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t11.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t11.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t11.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.solid.t0.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t0.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.solid.t0.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.solid.t0.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t0.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t0.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.solid.t0.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.solid.t0.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t0.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.solid.t0.aArg | `0` | `0` | PASS |
| `Contact callback argument` | contact.solid.t0.bArg | `0` | `0` | PASS |
| `physicsData.isColliding` | contact.solid.t0.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.sensor.t0.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t0.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t0.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t0.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t0.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t0.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t0.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t0.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t0.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.sensor.t0.aArg | `0` | `0` | PASS |
| `Contact callback argument` | contact.sensor.t0.bArg | `0` | `0` | PASS |
| `physicsData.isColliding` | contact.sensor.t0.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.solid.t1.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t1.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.solid.t1.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.solid.t1.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t1.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t1.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.solid.t1.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.solid.t1.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t1.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.solid.t1.aArg | `0` | `0` | PASS |
| `Contact callback argument` | contact.solid.t1.bArg | `0` | `0` | PASS |
| `physicsData.isColliding` | contact.solid.t1.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.sensor.t1.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t1.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t1.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t1.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t1.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t1.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t1.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t1.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t1.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.sensor.t1.aArg | `0` | `0` | PASS |
| `Contact callback argument` | contact.sensor.t1.bArg | `0` | `0` | PASS |
| `physicsData.isColliding` | contact.sensor.t1.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.solid.t2.trace | `aPbPaCbC` | `aPbPaCbC` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t2.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t2.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t2.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t2.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t2.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t2.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t2.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t2.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.solid.t2.aArg | `2` | `2` | PASS |
| `Contact callback argument` | contact.solid.t2.bArg | `2` | `2` | PASS |
| `physicsData.isColliding` | contact.solid.t2.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.sensor.t2.trace | `aPbPaCbC` | `aPbPaCbC` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t2.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t2.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t2.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t2.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t2.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t2.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t2.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t2.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.sensor.t2.aArg | `2` | `2` | PASS |
| `Contact callback argument` | contact.sensor.t2.bArg | `2` | `2` | PASS |
| `physicsData.isColliding` | contact.sensor.t2.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.solid.t3.trace | `aPbPaCbCaTbT` | `aPbPaCbCaTbT` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t3.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t3.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t3.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t3.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t3.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t3.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t3.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t3.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.solid.t3.aArg | `3` | `3` | PASS |
| `Contact callback argument` | contact.solid.t3.bArg | `3` | `3` | PASS |
| `physicsData.isColliding` | contact.solid.t3.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.sensor.t3.trace | `aPbPaCbCaTbT` | `aPbPaCbCaTbT` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t3.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t3.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t3.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t3.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t3.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t3.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t3.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t3.bS | `0` | `0` | PASS |
| `Contact callback argument` | contact.sensor.t3.aArg | `3` | `3` | PASS |
| `Contact callback argument` | contact.sensor.t3.bArg | `3` | `3` | PASS |
| `physicsData.isColliding` | contact.sensor.t3.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.solid.t4.trace | `aPbPaCbCaTbTaSbS` | `aPbPaCbCaTbTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t4.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t4.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t4.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t4.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t4.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t4.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t4.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t4.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.solid.t4.aArg | `4` | `4` | PASS |
| `Contact callback argument` | contact.solid.t4.bArg | `4` | `4` | PASS |
| `physicsData.isColliding` | contact.solid.t4.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.sensor.t4.trace | `aPbPaCbCaTbTaSbS` | `aPbPaCbCaTbTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t4.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t4.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t4.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t4.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t4.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t4.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t4.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t4.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.sensor.t4.aArg | `4` | `4` | PASS |
| `Contact callback argument` | contact.sensor.t4.bArg | `4` | `4` | PASS |
| `physicsData.isColliding` | contact.sensor.t4.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.solid.t5.trace | `aPbPaCbCaTbTaSbS` | `aPbPaCbCaTbTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t5.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t5.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t5.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t5.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t5.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.solid.t5.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.solid.t5.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t5.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.solid.t5.aArg | `4` | `4` | PASS |
| `Contact callback argument` | contact.solid.t5.bArg | `4` | `4` | PASS |
| `physicsData.isColliding` | contact.solid.t5.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.sensor.t5.trace | `aPbPaCbCaTbTaSbS` | `aPbPaCbCaTbTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t5.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t5.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t5.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t5.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t5.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t5.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t5.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t5.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.sensor.t5.aArg | `4` | `4` | PASS |
| `Contact callback argument` | contact.sensor.t5.bArg | `4` | `4` | PASS |
| `physicsData.isColliding` | contact.sensor.t5.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.solid.t6.trace | `aPbPaCbCaTbTaSbSaPbPaCbC` | `aPbPaCbCaTbTaSbSaPbPaCbC` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t6.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.solid.t6.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.solid.t6.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t6.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t6.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.solid.t6.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.solid.t6.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t6.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.solid.t6.aArg | `6` | `6` | PASS |
| `Contact callback argument` | contact.solid.t6.bArg | `6` | `6` | PASS |
| `physicsData.isColliding` | contact.solid.t6.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.sensor.t6.trace | `aPbPaCbCaTbTaSbSaPbPaCbC` | `aPbPaCbCaTbTaSbSaPbPaCbC` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t6.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t6.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t6.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t6.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t6.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t6.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t6.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t6.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.sensor.t6.aArg | `6` | `6` | PASS |
| `Contact callback argument` | contact.sensor.t6.bArg | `6` | `6` | PASS |
| `physicsData.isColliding` | contact.sensor.t6.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.solid.t7.trace | `aPbPaCbCaTbTaSbSaPbPaCbCaTbT` | `aPbPaCbCaTbTaSbSaPbPaCbCaTbT` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t7.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.solid.t7.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.solid.t7.aT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t7.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t7.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.solid.t7.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.solid.t7.bT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t7.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.solid.t7.aArg | `7` | `7` | PASS |
| `Contact callback argument` | contact.solid.t7.bArg | `7` | `7` | PASS |
| `physicsData.isColliding` | contact.solid.t7.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.sensor.t7.trace | `aPbPaCbCaTbTaSbSaPbPaCbCaTbT` | `aPbPaCbCaTbTaSbSaPbPaCbCaTbT` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t7.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t7.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t7.aT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t7.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t7.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t7.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t7.bT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t7.bS | `1` | `1` | PASS |
| `Contact callback argument` | contact.sensor.t7.aArg | `7` | `7` | PASS |
| `Contact callback argument` | contact.sensor.t7.bArg | `7` | `7` | PASS |
| `physicsData.isColliding` | contact.sensor.t7.colliding | `1` | `1` | PASS |
| `Contact callback order` | contact.solid.t8.trace | `aPbPaCbCaTbTaSbSaPbPaCbCaTbTaSbS` | `aPbPaCbCaTbTaSbSaPbPaCbCaTbTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t8.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.solid.t8.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.solid.t8.aT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t8.aS | `2` | `2` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.solid.t8.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.solid.t8.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.solid.t8.bT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.solid.t8.bS | `2` | `2` | PASS |
| `Contact callback argument` | contact.solid.t8.aArg | `8` | `8` | PASS |
| `Contact callback argument` | contact.solid.t8.bArg | `8` | `8` | PASS |
| `physicsData.isColliding` | contact.solid.t8.colliding | `0` | `0` | PASS |
| `Contact callback order` | contact.sensor.t8.trace | `aPbPaCbCaTbTaSbSaPbPaCbCaTbTaSbS` | `aPbPaCbCaTbTaSbSaPbPaCbCaTbTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t8.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t8.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t8.aT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t8.aS | `2` | `2` | PASS |
| `Entity.onEntityCollidePreSolve` | contact.sensor.t8.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | contact.sensor.t8.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | contact.sensor.t8.bT | `2` | `2` | PASS |
| `Entity.onEntitySeparate` | contact.sensor.t8.bS | `2` | `2` | PASS |
| `Contact callback argument` | contact.sensor.t8.aArg | `8` | `8` | PASS |
| `Contact callback argument` | contact.sensor.t8.bArg | `8` | `8` | PASS |
| `physicsData.isColliding` | contact.sensor.t8.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t0.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t0.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | removal.body.t0.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | removal.body.t0.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t0.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t0.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | removal.body.t0.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | removal.body.t0.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t0.bS | `0` | `0` | PASS |
| `Contact callback argument` | removal.body.t0.aArg | `0` | `0` | PASS |
| `Contact callback argument` | removal.body.t0.bArg | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t0.aBodies | `1` | `1` | PASS |
| `physicsData.getNumBodies` | removal.body.t0.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t0.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t1.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t1.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | removal.body.t1.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | removal.body.t1.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t1.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t1.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | removal.body.t1.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | removal.body.t1.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t1.bS | `0` | `0` | PASS |
| `Contact callback argument` | removal.body.t1.aArg | `0` | `0` | PASS |
| `Contact callback argument` | removal.body.t1.bArg | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t1.aBodies | `1` | `1` | PASS |
| `physicsData.getNumBodies` | removal.body.t1.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t1.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t2.trace | `aPbPaCaSbSbC` | `aPbPaCaSbSbC` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t2.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t2.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t2.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t2.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t2.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t2.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t2.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t2.bS | `1` | `1` | PASS |
| `Contact callback argument` | removal.body.t2.aArg | `3` | `3` | PASS |
| `Contact callback argument` | removal.body.t2.bArg | `3` | `3` | PASS |
| `physicsData.getNumBodies` | removal.body.t2.aBodies | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t2.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t2.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t3.trace | `aPbPaCaSbSbC` | `aPbPaCaSbSbC` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t3.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t3.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t3.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t3.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t3.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t3.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t3.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t3.bS | `1` | `1` | PASS |
| `Contact callback argument` | removal.body.t3.aArg | `3` | `3` | PASS |
| `Contact callback argument` | removal.body.t3.bArg | `3` | `3` | PASS |
| `physicsData.getNumBodies` | removal.body.t3.aBodies | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t3.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t3.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t4.trace | `aPbPaCaSbSbC` | `aPbPaCaSbSbC` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t4.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t4.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t4.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t4.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t4.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t4.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t4.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t4.bS | `1` | `1` | PASS |
| `Contact callback argument` | removal.body.t4.aArg | `3` | `3` | PASS |
| `Contact callback argument` | removal.body.t4.bArg | `3` | `3` | PASS |
| `physicsData.getNumBodies` | removal.body.t4.aBodies | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t4.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t4.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t5.trace | `aPbPaCaSbSbC` | `aPbPaCaSbSbC` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t5.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t5.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t5.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t5.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t5.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t5.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t5.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t5.bS | `1` | `1` | PASS |
| `Contact callback argument` | removal.body.t5.aArg | `3` | `3` | PASS |
| `Contact callback argument` | removal.body.t5.bArg | `3` | `3` | PASS |
| `physicsData.getNumBodies` | removal.body.t5.aBodies | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t5.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t5.colliding | `0` | `0` | PASS |
| `Contact callback order` | removal.body.t6.trace | `aPbPaCaSbSbC` | `aPbPaCaSbSbC` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t6.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t6.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t6.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t6.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | removal.body.t6.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | removal.body.t6.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | removal.body.t6.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | removal.body.t6.bS | `1` | `1` | PASS |
| `Contact callback argument` | removal.body.t6.aArg | `3` | `3` | PASS |
| `Contact callback argument` | removal.body.t6.bArg | `3` | `3` | PASS |
| `physicsData.getNumBodies` | removal.body.t6.aBodies | `0` | `0` | PASS |
| `physicsData.getNumBodies` | removal.body.t6.bBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | removal.body.t6.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t0.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t0.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t0.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t0.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t0.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t0.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t0.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t0.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t0.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t0.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t0.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t0.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t0.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t0.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t0.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t0.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t0.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t0.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t0.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t0.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t0.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t0.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t0.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t0.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t0.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t0.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t0.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t0.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t0.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t0.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t0.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t0.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t0.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t0.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t0.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t0.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t0.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t0.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t0.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t0.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t0.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t0.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t0.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t0.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t0.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t0.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t0.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t0.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t0.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t0.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t0.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t0.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t0.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t0.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t0.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t0.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t0.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t0.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t0.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t0.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t0.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t0.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t0.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t0.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t0.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t0.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t0.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t0.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t0.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t0.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t0.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t0.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t0.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t0.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t0.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t0.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t0.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t0.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t0.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t0.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t0.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t0.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t0.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t0.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t0.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t0.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t0.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t0.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t0.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t0.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t0.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t0.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t0.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t0.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t0.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t0.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t0.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t0.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t0.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t0.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t0.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t0.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t0.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t0.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t0.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t0.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t0.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t0.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t0.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t0.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t0.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t0.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t0.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t0.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t0.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t0.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidA.t1.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t1.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t1.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t1.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t1.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t1.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t1.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t1.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t1.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t1.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t1.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t1.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t1.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t1.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t1.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t1.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t1.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t1.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t1.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t1.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t1.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t1.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t1.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t1.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t1.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t1.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t1.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t1.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t1.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t1.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t1.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t1.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t1.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t1.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t1.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t1.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t1.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t1.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t1.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t1.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t1.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t1.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t1.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t1.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t1.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t1.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t1.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t1.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t1.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t1.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t1.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t1.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t1.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t1.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t1.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t1.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t1.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t1.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t1.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t1.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t1.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t1.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t1.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t1.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t1.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t1.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t1.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t1.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t1.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t1.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t1.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t1.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t1.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t1.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t1.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t1.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t1.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t1.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t1.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t1.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t1.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t1.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t1.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t1.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t1.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t1.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t1.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t1.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t1.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t1.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t1.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t1.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t1.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t1.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t1.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t1.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t1.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t1.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t1.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t1.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t1.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t1.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t1.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t1.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t1.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t1.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t1.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t1.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t1.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t1.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t1.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t1.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t1.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t1.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t1.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t1.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidA.t2.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.solidA.t2.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t2.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t2.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t2.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t2.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t2.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t2.listSize | `1` | `1` | PASS |
| `intIter.get` | query.solidA.t2.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t2.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t2.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t2.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t2.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t2.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t2.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t2.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t2.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t2.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t2.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t2.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t2.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t2.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t2.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t2.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t2.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t2.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t2.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t2.contactX | `1.320900` | `1.320900` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t2.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidB.t2.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.solidB.t2.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t2.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t2.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t2.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t2.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t2.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t2.listSize | `1` | `1` | PASS |
| `intIter.get` | query.solidB.t2.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t2.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t2.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t2.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t2.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t2.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t2.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t2.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t2.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t2.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t2.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t2.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t2.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t2.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t2.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t2.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t2.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t2.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t2.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t2.contactX | `1.320900` | `1.320900` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t2.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorA.t2.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorA.t2.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t2.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t2.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t2.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t2.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t2.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t2.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorA.t2.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t2.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t2.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t2.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t2.allSensors | `1` | `1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t2.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t2.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t2.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t2.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t2.mySensorsOnly | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t2.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t2.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t2.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t2.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t2.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t2.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t2.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t2.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t2.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t2.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t2.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorB.t2.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorB.t2.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t2.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t2.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t2.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t2.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t2.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t2.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorB.t2.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t2.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t2.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t2.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t2.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t2.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t2.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t2.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t2.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t2.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t2.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t2.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t2.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t2.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t2.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t2.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t2.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t2.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t2.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t2.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t2.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidA.t3.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.solidA.t3.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t3.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t3.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t3.mask | `0` | `0` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t3.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t3.enabled | `0` | `0` | PASS |
| `intIter.size` | query.solidA.t3.listSize | `1` | `1` | PASS |
| `intIter.get` | query.solidA.t3.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t3.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t3.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t3.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t3.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t3.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t3.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t3.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t3.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t3.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t3.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t3.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t3.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t3.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t3.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t3.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t3.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t3.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t3.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t3.contactX | `1.206276` | `1.206276` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t3.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidB.t3.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.solidB.t3.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t3.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t3.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t3.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t3.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t3.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t3.listSize | `1` | `1` | PASS |
| `intIter.get` | query.solidB.t3.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t3.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t3.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t3.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t3.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t3.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t3.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t3.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t3.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t3.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t3.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t3.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t3.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t3.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t3.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t3.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t3.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t3.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t3.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t3.contactX | `1.206276` | `1.206276` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t3.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorA.t3.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorA.t3.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t3.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t3.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t3.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t3.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t3.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t3.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorA.t3.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t3.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t3.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t3.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t3.allSensors | `1` | `1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t3.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t3.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t3.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t3.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t3.mySensorsOnly | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t3.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t3.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t3.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t3.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t3.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t3.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t3.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t3.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t3.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t3.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t3.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorB.t3.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorB.t3.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t3.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t3.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t3.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t3.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t3.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t3.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorB.t3.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t3.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t3.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t3.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t3.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t3.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t3.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t3.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t3.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t3.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t3.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t3.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t3.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t3.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t3.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t3.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t3.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t3.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t3.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t3.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t3.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidA.t4.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t4.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t4.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t4.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t4.mask | `0` | `0` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t4.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t4.enabled | `0` | `0` | PASS |
| `intIter.size` | query.solidA.t4.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t4.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t4.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t4.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t4.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t4.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t4.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t4.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t4.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t4.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t4.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t4.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t4.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t4.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t4.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t4.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t4.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t4.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t4.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t4.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t4.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t4.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t4.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t4.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t4.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t4.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t4.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t4.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t4.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t4.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t4.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t4.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t4.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t4.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t4.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t4.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t4.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t4.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t4.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t4.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t4.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t4.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t4.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t4.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t4.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t4.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t4.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t4.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t4.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t4.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t4.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t4.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorA.t4.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t4.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t4.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t4.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t4.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t4.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t4.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorA.t4.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t4.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t4.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t4.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t4.allSensors | `1` | `1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t4.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t4.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t4.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t4.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t4.mySensorsOnly | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t4.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t4.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t4.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t4.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t4.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t4.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t4.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t4.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t4.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t4.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t4.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorB.t4.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorB.t4.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t4.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t4.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t4.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t4.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t4.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t4.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorB.t4.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t4.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t4.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t4.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t4.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t4.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t4.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t4.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t4.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t4.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t4.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t4.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t4.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t4.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t4.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t4.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t4.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t4.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t4.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t4.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t4.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidA.t5.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t5.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t5.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t5.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t5.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t5.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t5.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t5.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t5.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t5.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t5.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t5.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t5.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t5.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t5.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t5.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t5.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t5.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t5.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t5.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t5.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t5.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t5.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t5.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t5.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t5.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t5.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t5.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t5.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t5.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t5.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t5.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t5.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t5.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t5.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t5.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t5.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t5.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t5.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t5.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t5.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t5.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t5.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t5.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t5.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t5.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t5.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t5.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t5.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t5.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t5.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t5.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t5.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t5.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t5.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t5.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t5.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t5.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t5.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorA.t5.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t5.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t5.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t5.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t5.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t5.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t5.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorA.t5.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t5.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t5.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t5.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t5.allSensors | `1` | `1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t5.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t5.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t5.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t5.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t5.mySensorsOnly | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t5.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t5.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t5.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t5.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t5.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t5.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t5.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t5.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t5.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t5.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t5.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorB.t5.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorB.t5.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t5.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t5.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t5.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t5.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t5.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t5.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorB.t5.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t5.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t5.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t5.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t5.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t5.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t5.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t5.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t5.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t5.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t5.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t5.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t5.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t5.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t5.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t5.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t5.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t5.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t5.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t5.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t5.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidA.t6.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t6.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t6.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t6.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t6.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t6.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t6.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t6.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t6.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t6.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t6.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t6.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t6.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t6.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t6.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t6.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t6.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t6.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t6.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t6.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t6.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t6.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t6.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t6.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t6.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t6.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t6.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t6.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t6.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t6.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t6.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t6.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t6.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t6.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t6.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t6.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t6.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t6.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t6.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t6.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t6.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t6.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t6.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t6.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t6.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t6.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t6.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t6.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t6.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t6.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t6.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t6.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t6.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t6.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t6.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t6.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t6.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t6.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t6.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorA.t6.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t6.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t6.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t6.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t6.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t6.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t6.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorA.t6.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t6.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t6.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t6.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t6.allSensors | `1` | `1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t6.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t6.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t6.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t6.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t6.mySensorsOnly | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t6.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t6.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t6.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t6.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t6.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t6.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t6.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t6.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t6.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t6.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t6.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorB.t6.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorB.t6.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t6.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t6.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t6.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t6.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t6.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t6.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorB.t6.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t6.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t6.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t6.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t6.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t6.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t6.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t6.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t6.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t6.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t6.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t6.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t6.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t6.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t6.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t6.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t6.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t6.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t6.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t6.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t6.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidA.t7.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidA.t7.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidA.t7.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidA.t7.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t7.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t7.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t7.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t7.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidA.t7.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t7.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t7.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidA.t7.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidA.t7.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidA.t7.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t7.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t7.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidA.t7.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t7.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t7.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t7.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t7.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t7.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t7.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t7.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t7.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t7.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t7.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t7.contactX | `-1` | `1.132917` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t7.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidB.t7.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidB.t7.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidB.t7.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidB.t7.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t7.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t7.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t7.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t7.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidB.t7.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t7.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t7.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidB.t7.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidB.t7.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidB.t7.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t7.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t7.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidB.t7.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t7.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t7.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t7.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t7.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t7.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t7.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t7.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t7.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t7.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t7.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t7.contactX | `-1` | `1.132917` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t7.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorA.t7.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorA.t7.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t7.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t7.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t7.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t7.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t7.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t7.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorA.t7.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t7.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t7.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t7.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t7.allSensors | `1` | `1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t7.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t7.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t7.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t7.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t7.mySensorsOnly | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t7.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t7.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t7.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t7.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t7.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t7.pairNX | `-1.000000` | `-1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t7.pairNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t7.contactNX | `-1.000000` | `-1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t7.contactNY | `-0.000000` | `-0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t7.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t7.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.sensorB.t7.colliding | `1` | `1` | PASS |
| `physicsData.isColliding` | query.sensorB.t7.anyColliding | `1` | `1` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t7.exists | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t7.category | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t7.mask | `0` | `0` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t7.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t7.enabled | `0` | `0` | PASS |
| `intIter.size` | query.sensorB.t7.listSize | `1` | `1` | PASS |
| `intIter.get` | query.sensorB.t7.idMatches | `1` | `1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t7.firstExists | `1` | `1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t7.shapeExists | `1` | `1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t7.entityMatches | `1` | `1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t7.allSensors | `0` | `0` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t7.bodyColliding | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t7.bodyPair | `1` | `1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t7.bodyShape | `1` | `1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t7.shapeColliding | `1` | `1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t7.mySensorsOnly | `0` | `0` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t7.missingShape | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t7.myCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t7.myIDMatches | `1` | `1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t7.contactCount | `1` | `1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t7.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t7.pairNX | `1.000000` | `1.000000` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t7.pairNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t7.contactNX | `1.000000` | `1.000000` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t7.contactNY | `0.000000` | `0.000000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t7.contactX | `101.500000` | `101.500000` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t7.contactY | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | query.solidA.t8.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidA.t8.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidA.t8.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidA.t8.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t8.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t8.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t8.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t8.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidA.t8.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t8.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t8.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidA.t8.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidA.t8.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidA.t8.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t8.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t8.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidA.t8.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t8.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t8.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t8.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t8.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t8.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t8.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t8.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t8.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t8.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t8.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t8.contactX | `-1` | `1.085967` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t8.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidB.t8.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidB.t8.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidB.t8.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidB.t8.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t8.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t8.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t8.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t8.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidB.t8.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t8.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t8.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidB.t8.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidB.t8.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidB.t8.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t8.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t8.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidB.t8.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t8.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t8.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t8.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t8.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t8.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t8.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t8.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t8.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t8.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t8.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t8.contactX | `-1` | `1.085967` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t8.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorA.t8.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t8.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t8.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t8.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t8.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t8.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t8.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t8.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t8.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t8.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t8.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t8.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t8.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t8.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t8.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t8.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t8.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t8.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t8.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t8.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t8.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t8.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t8.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t8.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t8.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t8.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t8.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t8.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t8.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t8.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t8.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t8.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t8.category | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t8.mask | `0` | `0` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t8.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t8.enabled | `0` | `0` | PASS |
| `intIter.size` | query.sensorB.t8.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t8.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t8.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t8.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t8.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t8.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t8.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t8.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t8.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t8.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t8.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t8.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t8.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t8.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t8.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t8.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t8.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t8.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t8.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t8.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t8.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t8.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidA.t9.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidA.t9.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidA.t9.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidA.t9.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t9.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t9.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t9.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidA.t9.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t9.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t9.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidA.t9.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidA.t9.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidA.t9.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t9.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t9.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidA.t9.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t9.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t9.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t9.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t9.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t9.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t9.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t9.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t9.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t9.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t9.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t9.contactX | `-1` | `1.055919` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t9.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidB.t9.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidB.t9.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidB.t9.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidB.t9.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t9.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t9.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t9.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidB.t9.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t9.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t9.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidB.t9.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidB.t9.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidB.t9.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t9.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t9.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidB.t9.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t9.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t9.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t9.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t9.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t9.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t9.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t9.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t9.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t9.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t9.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t9.contactX | `-1` | `1.055919` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t9.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorA.t9.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t9.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t9.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t9.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t9.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t9.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t9.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t9.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t9.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t9.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t9.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t9.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t9.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t9.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t9.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t9.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t9.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t9.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t9.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t9.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t9.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t9.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t9.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t9.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t9.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t9.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t9.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t9.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t9.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t9.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t9.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t9.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t9.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t9.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t9.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t9.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t9.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t9.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t9.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t9.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t9.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t9.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t9.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t9.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t9.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t9.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t9.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t9.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t9.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t9.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t9.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t9.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t9.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t9.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t9.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t9.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidA.t10.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidA.t10.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidA.t10.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidA.t10.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t10.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t10.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t10.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidA.t10.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t10.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t10.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidA.t10.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidA.t10.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidA.t10.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t10.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t10.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidA.t10.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t10.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t10.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t10.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t10.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t10.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t10.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t10.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t10.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t10.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t10.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t10.contactX | `-1` | `1.036688` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t10.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidB.t10.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidB.t10.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidB.t10.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidB.t10.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t10.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t10.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t10.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidB.t10.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t10.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t10.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidB.t10.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidB.t10.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidB.t10.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t10.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t10.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidB.t10.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t10.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t10.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t10.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t10.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t10.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t10.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t10.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t10.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t10.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t10.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t10.contactX | `-1` | `1.036688` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t10.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorA.t10.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t10.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t10.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t10.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t10.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t10.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t10.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t10.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t10.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t10.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t10.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t10.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t10.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t10.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t10.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t10.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t10.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t10.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t10.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t10.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t10.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t10.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t10.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t10.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t10.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t10.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t10.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t10.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t10.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t10.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t10.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t10.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t10.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t10.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t10.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t10.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t10.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t10.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t10.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t10.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t10.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t10.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t10.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t10.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t10.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t10.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t10.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t10.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t10.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t10.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t10.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t10.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t10.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t10.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t10.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t10.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidA.t11.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidA.t11.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidA.t11.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidA.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t11.group | `-1` | `-1` | PASS |
| `physicsBody.isCollidable` | query.solidA.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t11.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidA.t11.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t11.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t11.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidA.t11.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidA.t11.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidA.t11.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t11.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t11.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidA.t11.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t11.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t11.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t11.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t11.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t11.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t11.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t11.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t11.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t11.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t11.contactX | `-1` | `1.024380` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidA.t11.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidB.t11.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.solidB.t11.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.solidB.t11.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.solidB.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t11.group | `-1` | `-1` | PASS |
| `physicsBody.isCollidable` | query.solidB.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t11.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.solidB.t11.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t11.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t11.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.solidB.t11.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.solidB.t11.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.solidB.t11.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t11.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t11.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.solidB.t11.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t11.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t11.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t11.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t11.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t11.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t11.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t11.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t11.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t11.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t11.contactX | `-1` | `1.024380` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.solidB.t11.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorA.t11.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.sensorA.t11.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.sensorA.t11.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.sensorA.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t11.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t11.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.sensorA.t11.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t11.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t11.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.sensorA.t11.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.sensorA.t11.allSensors | `-1` | `1` | FAIL |
| `collisionInfo.isBodyColliding` | query.sensorA.t11.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t11.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t11.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.sensorA.t11.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t11.mySensorsOnly | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t11.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t11.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t11.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t11.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t11.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t11.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t11.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t11.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t11.contactX | `-1` | `101.500000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t11.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorB.t11.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.sensorB.t11.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.sensorB.t11.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.sensorB.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t11.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t11.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.sensorB.t11.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t11.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t11.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.sensorB.t11.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.sensorB.t11.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.sensorB.t11.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t11.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t11.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.sensorB.t11.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t11.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t11.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t11.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t11.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t11.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t11.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t11.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t11.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t11.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t11.contactX | `-1` | `101.500000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t11.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidA.t12.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t12.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t12.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t12.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t12.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t12.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t12.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t12.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t12.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t12.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t12.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t12.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t12.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t12.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t12.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t12.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t12.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t12.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t12.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t12.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t12.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t12.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t12.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t12.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t12.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t12.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t12.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t12.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t12.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t12.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t12.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t12.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t12.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t12.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t12.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t12.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t12.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t12.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t12.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t12.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t12.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t12.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t12.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t12.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t12.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t12.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t12.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t12.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t12.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t12.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t12.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t12.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t12.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t12.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t12.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t12.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t12.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.sensorA.t12.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.sensorA.t12.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.sensorA.t12.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t12.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t12.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t12.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.sensorA.t12.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t12.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t12.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.sensorA.t12.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.sensorA.t12.allSensors | `-1` | `1` | FAIL |
| `collisionInfo.isBodyColliding` | query.sensorA.t12.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t12.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t12.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.sensorA.t12.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t12.mySensorsOnly | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t12.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t12.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t12.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t12.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t12.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t12.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t12.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t12.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t12.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t12.contactX | `-1` | `101.500000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t12.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorB.t12.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.sensorB.t12.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.sensorB.t12.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.sensorB.t12.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t12.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t12.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t12.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.sensorB.t12.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t12.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t12.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.sensorB.t12.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.sensorB.t12.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.sensorB.t12.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t12.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t12.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.sensorB.t12.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t12.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t12.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t12.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t12.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t12.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t12.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t12.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t12.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t12.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t12.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t12.contactX | `-1` | `101.500000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t12.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.solidA.t13.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t13.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t13.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t13.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t13.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t13.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t13.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t13.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t13.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t13.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t13.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t13.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t13.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t13.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t13.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t13.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t13.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t13.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t13.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t13.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t13.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t13.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t13.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t13.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t13.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t13.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t13.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t13.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t13.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t13.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t13.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t13.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t13.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t13.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t13.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t13.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t13.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t13.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t13.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t13.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t13.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t13.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t13.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t13.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t13.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t13.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t13.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t13.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t13.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t13.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t13.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t13.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t13.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t13.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t13.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t13.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t13.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.sensorA.t13.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.sensorA.t13.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.sensorA.t13.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t13.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t13.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t13.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.sensorA.t13.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t13.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t13.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.sensorA.t13.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.sensorA.t13.allSensors | `-1` | `1` | FAIL |
| `collisionInfo.isBodyColliding` | query.sensorA.t13.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t13.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t13.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.sensorA.t13.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t13.mySensorsOnly | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t13.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t13.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t13.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t13.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t13.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t13.pairNX | `-1` | `-1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t13.pairNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t13.contactNX | `-1` | `-1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t13.contactNY | `-1` | `-0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t13.contactX | `-1` | `101.500000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t13.contactY | `-1` | `0.000000` | FAIL |
| `physicsData.isColliding` | query.sensorB.t13.colliding | `0` | `1` | FAIL |
| `physicsData.isColliding` | query.sensorB.t13.anyColliding | `0` | `1` | FAIL |
| `physicsData.getCollisionInfo` | query.sensorB.t13.exists | `0` | `1` | FAIL |
| `physicsBody.getCollisionCategory` | query.sensorB.t13.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t13.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t13.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t13.listSize | `-1` | `1` | FAIL |
| `intIter.get` | query.sensorB.t13.idMatches | `-1` | `1` | FAIL |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t13.firstExists | `-1` | `1` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t13.shapeExists | `-1` | `1` | FAIL |
| `collisionInfo.getEntity` | query.sensorB.t13.entityMatches | `-1` | `1` | FAIL |
| `collisionInfo.isAllSensors` | query.sensorB.t13.allSensors | `-1` | `0` | FAIL |
| `collisionInfo.isBodyColliding` | query.sensorB.t13.bodyColliding | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t13.bodyPair | `-1` | `1` | FAIL |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t13.bodyShape | `-1` | `1` | FAIL |
| `collisionInfo.isShapeColliding` | query.sensorB.t13.shapeColliding | `-1` | `1` | FAIL |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t13.mySensorsOnly | `-1` | `0` | FAIL |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t13.missingShape | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t13.myCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t13.myIDMatches | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t13.contactCount | `-1` | `1` | FAIL |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t13.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t13.pairNX | `-1` | `1.000000` | FAIL |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t13.pairNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t13.contactNX | `-1` | `1.000000` | FAIL |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t13.contactNY | `-1` | `0.000000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t13.contactX | `-1` | `101.500000` | FAIL |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t13.contactY | `-1` | `0.000000` | FAIL |
| `Physics.raycast` | ray.solid.t0.count | `2` | `2` | PASS |
| `Physics.raycast` | ray.solid.t0.trace | `AC` | `AC` | PASS |
| `Physics.raycastSimple` | ray.solid.t0.simple | `0.200000` | `0.200000` | PASS |
| `Physics.raycastOne` | ray.solid.t0.oneHit | `1` | `1` | PASS |
| `Physics.raycastOne` | ray.solid.t0.oneEntity | `A` | `A` | PASS |
| `Physics.raycastOne` | ray.solid.t0.oneSidMatches | `1` | `1` | PASS |
| `Physics.raycastOne` | ray.solid.t0.oneLambda | `0.200000` | `0.200000` | PASS |
| `Physics.raycastOne` | ray.solid.t0.oneNX | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | ray.solid.t0.oneNY | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | ray.sensors.t0.count | `3` | `3` | PASS |
| `Physics.raycast` | ray.sensors.t0.trace | `ABC` | `ABC` | PASS |
| `Physics.raycastSimple` | ray.sensors.t0.simple | `0.200000` | `0.200000` | PASS |
| `Physics.raycastOne` | ray.sensors.t0.oneHit | `1` | `1` | PASS |
| `Physics.raycastOne` | ray.sensors.t0.oneEntity | `A` | `A` | PASS |
| `Physics.raycastOne` | ray.sensors.t0.oneSidMatches | `1` | `1` | PASS |
| `Physics.raycastOne` | ray.sensors.t0.oneLambda | `0.200000` | `0.200000` | PASS |
| `Physics.raycastOne` | ray.sensors.t0.oneNX | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | ray.sensors.t0.oneNY | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | ray.inside.t0.count | `3` | `3` | PASS |
| `Physics.raycast` | ray.inside.t0.trace | `ABC` | `ABC` | PASS |
| `Physics.raycastSimple` | ray.inside.t0.simple | `0.600000` | `0.600000` | PASS |
| `Physics.raycastOne` | ray.inside.t0.oneHit | `1` | `1` | PASS |
| `Physics.raycastOne` | ray.inside.t0.oneEntity | `A` | `A` | PASS |
| `Physics.raycastOne` | ray.inside.t0.oneSidMatches | `1` | `1` | PASS |
| `Physics.raycastOne` | ray.inside.t0.oneLambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | ray.inside.t0.oneNX | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | ray.inside.t0.oneNY | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | ray.miss.t0.count | `0` | `0` | PASS |
| `Physics.raycast` | ray.miss.t0.trace | `` | `` | PASS |
| `Physics.raycastSimple` | ray.miss.t0.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | ray.miss.t0.oneHit | `0` | `0` | PASS |
| `Physics.raycastOne` | ray.miss.t0.oneEntity | `none` | `none` | PASS |
| `Physics.raycastOne` | ray.miss.t0.oneSidMatches | `-1` | `-1` | PASS |
| `Physics.raycastOne` | ray.miss.t0.oneLambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | ray.miss.t0.oneNX | `-1` | `-1` | PASS |
| `Physics.raycastOne` | ray.miss.t0.oneNY | `-1` | `-1` | PASS |
| `physicsBody.raycast` | ray.bodyA.t0.hit | `1` | `1` | PASS |
| `physicsBody.raycast` | ray.bodyA.t0.lambda | `0.200000` | `0.200000` | PASS |
| `physicsBody.raycast` | ray.bodyA.t0.nx | `-1.000000` | `-1.000000` | PASS |
| `physicsBody.raycast` | ray.bodyA.t0.ny | `0.000000` | `0.000000` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyA.t0.shapeHit | `1` | `1` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyA.t0.shapeMatches | `1` | `1` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyA.t0.shapeLambda | `0.200000` | `0.200000` | PASS |
| `physicsData.raycastShapesOne` | ray.bodyA.t0.pdShapeHit | `1` | `1` | PASS |
| `physicsData.raycastShapesOne` | ray.bodyA.t0.pdShapeMatches | `1` | `1` | PASS |
| `physicsData.raycast` | ray.bodyA.t0.pdHit | `1` | `1` | PASS |
| `physicsData.raycast` | ray.bodyA.t0.pdLambda | `0.200000` | `0.200000` | PASS |
| `physicsBody.raycast` | ray.bodyB.t0.hit | `1` | `1` | PASS |
| `physicsBody.raycast` | ray.bodyB.t0.lambda | `0.450000` | `0.450000` | PASS |
| `physicsBody.raycast` | ray.bodyB.t0.nx | `-1.000000` | `-1.000000` | PASS |
| `physicsBody.raycast` | ray.bodyB.t0.ny | `0.000000` | `0.000000` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyB.t0.shapeHit | `1` | `1` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyB.t0.shapeMatches | `1` | `1` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyB.t0.shapeLambda | `0.450000` | `0.450000` | PASS |
| `physicsData.raycastShapesOne` | ray.bodyB.t0.pdShapeHit | `1` | `1` | PASS |
| `physicsData.raycastShapesOne` | ray.bodyB.t0.pdShapeMatches | `1` | `1` | PASS |
| `physicsData.raycast` | ray.bodyB.t0.pdHit | `1` | `1` | PASS |
| `physicsData.raycast` | ray.bodyB.t0.pdLambda | `0.450000` | `0.450000` | PASS |
| `physicsBody.raycast` | ray.bodyMiss.t0.hit | `0` | `0` | PASS |
| `physicsBody.raycast` | ray.bodyMiss.t0.lambda | `-1` | `-1` | PASS |
| `physicsBody.raycast` | ray.bodyMiss.t0.nx | `-1` | `-1` | PASS |
| `physicsBody.raycast` | ray.bodyMiss.t0.ny | `-1` | `-1` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyMiss.t0.shapeHit | `0` | `0` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyMiss.t0.shapeMatches | `-1` | `-1` | PASS |
| `physicsBody.raycastShapesOne` | ray.bodyMiss.t0.shapeLambda | `-1` | `-1` | PASS |
| `physicsData.raycastShapesOne` | ray.bodyMiss.t0.pdShapeHit | `0` | `0` | PASS |
| `physicsData.raycastShapesOne` | ray.bodyMiss.t0.pdShapeMatches | `-1` | `-1` | PASS |
| `physicsData.raycast` | ray.bodyMiss.t0.pdHit | `0` | `0` | PASS |
| `physicsData.raycast` | ray.bodyMiss.t0.pdLambda | `-1` | `-1` | PASS |
