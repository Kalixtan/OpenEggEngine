# Physics test report

Generated: 2026-10-20 19:13 UTC

Matched: **597/597 declared readings (100%)**. This measures captured cases, not total physics parity.

Functions with readings: **29/282**. The inventory comes from the local API reference and includes OpenEgg-only methods.

## Sample groups

Scenes sample motion; contact cases sample callbacks and body removal each tick.

| Group | Matched | Cases |
| --- | ---: | ---: |
| scene.free | 48 | 48 |
| scene.fast | 48 | 48 |
| scene.solid | 72 | 72 |
| scene.sensor | 60 | 60 |
| contact.solid | 108 | 108 |
| contact.sensor | 108 | 108 |
| removal.body | 98 | 98 |

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
| `Physics.raycast` | 0 | 0 | untested |
| `Physics.raycastOne` | 0 | 0 | untested |
| `Physics.raycastShapes` | 0 | 0 | untested |
| `Physics.raycastSimple` | 0 | 0 | untested |
| `Physics.setNumFramesToIgnoreCollisions` | 0 | 0 | untested |
| `collisionInfo.getCollidingShapeIDList` | 0 | 0 | untested |
| `collisionInfo.getContactNormalOfMyCollidingShape` | 0 | 0 | untested |
| `collisionInfo.getEntity` | 0 | 0 | untested |
| `collisionInfo.getFirstShapeCollisionInfo` | 0 | 0 | untested |
| `collisionInfo.getNumCollidingShapes` | 0 | 0 | untested |
| `collisionInfo.getShapeCollisionInfo` | 0 | 0 | untested |
| `collisionInfo.isAllSensors` | 0 | 0 | untested |
| `collisionInfo.isBodyColliding` | 0 | 0 | untested |
| `collisionInfo.isBodyCollidingWithBody` | 0 | 0 | untested |
| `collisionInfo.isBodyCollidingWithShape` | 0 | 0 | untested |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | 0 | 0 | untested |
| `collisionInfo.isShapeColliding` | 0 | 0 | untested |
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
| `physicsBody.getCategoriesToCollideWith` | 2 | 2 | PASS |
| `physicsBody.getCenterOfMass` | 0 | 0 | untested |
| `physicsBody.getCollisionCategory` | 2 | 2 | PASS |
| `physicsBody.getCollisionGroup` | 0 | 0 | untested |
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
| `physicsBody.isCollidable` | 0 | 0 | untested |
| `physicsBody.isContainsShape` | 0 | 0 | untested |
| `physicsBody.isPointWithin` | 2 | 2 | PASS |
| `physicsBody.isPointWithinShape` | 0 | 0 | untested |
| `physicsBody.isRect` | 0 | 0 | untested |
| `physicsBody.isSensor` | 7 | 7 | PASS |
| `physicsBody.isShapeSensor` | 0 | 0 | untested |
| `physicsBody.isSleeping` | 0 | 0 | untested |
| `physicsBody.putToSleep` | 0 | 0 | untested |
| `physicsBody.raycast` | 0 | 0 | untested |
| `physicsBody.raycastShapesOne` | 0 | 0 | untested |
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
| `physicsData.getCollisionInfo` | 0 | 0 | untested |
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
| `physicsData.isColliding` | 49 | 49 | PASS |
| `physicsData.isPointWithin` | 0 | 0 | untested |
| `physicsData.isRect` | 0 | 0 | untested |
| `physicsData.isSensor` | 7 | 7 | PASS |
| `physicsData.isSleeping` | 0 | 0 | untested |
| `physicsData.putToSleep` | 0 | 0 | untested |
| `physicsData.raycast` | 0 | 0 | untested |
| `physicsData.raycastShapesOne` | 0 | 0 | untested |
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
