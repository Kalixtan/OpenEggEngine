#!/usr/bin/env python3
"""Compare the physics script probe with its retail PS3 capture."""

import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from compare_probe import check_baseline_source, read_readings
from probe_expectations import expected_readings
from run import ROOT, run_script

HERE = Path(__file__).resolve().parent


def percent(passed, total):
    return f"{passed * 100 / total:.1f}%" if total else "—"


PROBE = HERE / "probe.gm"
BASELINE = HERE / "retail-baseline.tsv"
SCENES = HERE / "scenes.gm"
SCENE_BASELINE = HERE / "scenes-retail.tsv"
SCENE_PLAN = HERE / "scenes-plan.json"
CONTACTS = HERE / "contacts.gm"
CONTACT_BASELINE = HERE / "contacts-retail.tsv"
CONTACT_PLAN = HERE / "contacts-plan.json"
REMOVAL = HERE / "removal.gm"
REMOVAL_BASELINE = HERE / "removal-retail.tsv"
REMOVAL_PLAN = HERE / "removal-plan.json"
QUERIES = HERE / "queries.gm"
QUERY_BASELINE = HERE / "queries-retail.tsv"
QUERY_PLAN = HERE / "queries-plan.json"
RAYS = HERE / "rays.gm"
RAY_BASELINE = HERE / "rays-retail.tsv"
RAY_PLAN = HERE / "rays-plan.json"
REENTRY = HERE / "filter_reentry.gm"
REENTRY_BASELINE = HERE / "filter_reentry-retail.tsv"
REENTRY_PLAN = HERE / "filter_reentry-plan.json"
JOINTS = HERE / "joints.gm"
JOINT_BASELINE = HERE / "joints-retail.tsv"
JOINT_PLAN = HERE / "joints-plan.json"
RAY_MOTION = HERE / "ray_motion.gm"
RAY_MOTION_BASELINE = HERE / "ray_motion-retail.tsv"
RAY_MOTION_PLAN = HERE / "ray_motion-plan.json"
ENTITY_LIFECYCLE = HERE / "entity_lifecycle.gm"
ENTITY_LIFECYCLE_BASELINE = HERE / "entity_lifecycle-retail.tsv"
ENTITY_LIFECYCLE_PLAN = HERE / "entity_lifecycle-plan.json"
ENTITY_MANAGER = HERE / "entity_manager.gm"
ENTITY_MANAGER_BASELINE = HERE / "entity_manager-retail.tsv"
ENTITY_MANAGER_PLAN = HERE / "entity_manager-plan.json"
ENTITY_QUERIES = HERE / "entity_queries.gm"
ENTITY_QUERIES_BASELINE = HERE / "entity_queries-retail.tsv"
ENTITY_QUERIES_PLAN = HERE / "entity_queries-plan.json"
ENTITY_QUERIES_SUPPORT = HERE / "EntityQueryProbeEntity.gm"
ENTITY_STATE = HERE / "entity_state.gm"
ENTITY_STATE_PLAN = HERE / "entity_state-plan.json"
ENTITY_STATE_BASELINE = HERE / "entity_state-retail.tsv"
ENTITY_STATE_SUPPORT = HERE / "EntityStateProbeEntity.gm"
PROCESS_ORDER = HERE / "process_order.gm"
PROCESS_ORDER_PLAN = HERE / "process_order-plan.json"
PROCESS_ORDER_BASELINE = HERE / "process_order-retail.tsv"
PROCESS_PAUSE = HERE / "process_pause.gm"
PROCESS_PAUSE_PLAN = HERE / "process_pause-plan.json"
PROCESS_PAUSE_BASELINE = HERE / "process_pause-retail.tsv"
PROCESS_ORDER_SUPPORT = HERE / "ProcessParityProbe.gm"
PROCESS_PAUSE_SUPPORT = (HERE / "ProcessPauseA.gm", HERE / "ProcessPauseB.gm")
BODY_CONTROLS = HERE / "body_controls.gm"
BODY_CONTROLS_BASELINE = HERE / "body_controls-retail.tsv"
BODY_CONTROLS_PLAN = HERE / "body_controls-plan.json"
CONTACT_SUPPORT = HERE / "ProbeContactEntity.gm"
ENTITY_LIFECYCLE_SUPPORT = HERE / "LifecycleProbeEntity.gm"
REPORT = ROOT / "reports" / "PHYSICS.md"
PREFIXES = ("physicsBody.", "physicsData.", "physicsShape.", "shapeDef.",
            "collisionInfo.", "shapeCollisionInfo.", "intIter.",
            "jointType.", "jointIter.", "Physics.", "Entity.",
            "EntityManager.", "EntityListRef.", "GameSystem.", "Process.",
            "ALARM.", "LFO.")
SPECIAL = {"EntityManager.createEntity", "Entity.getPhysicsData",
           "Global.PHYSICSSHAPECOLLECTIONDEF", "Entity.isEnabled",
           "EntityManager.isEntityIDValid", "EntityManager.getNumEntities",
           "EntityManager.getAllEntities", "EntityManager.flush",
           "EntityManager.deleteAllEntities", "Entity.tagForDeletion",
           "Entity.isTaggedForDeletion", "Entity.getObjectID", "Entity.getUID",
           "EntityManager.getEntity", "EntityManager.getEntityFromUID",
           "EntityManager.getNumManualEntities", "EntityManager.getNumNonManualEntities",
           "EntityManager.getNumPlayers", "EntityManager.getAllManualEntities",
           "EntityManager.getAllNonManualEntities", "EntityManager.getAllPlayerEntities",
           "EntityManager.getAllPlayerEntitiesRandomly", "EntityManager.getRandomEntity",
           "EntityManager.getRandomPlayer", "EntityManager.getEntitiesNear",
           "EntityManager.getEntitiesRoughlyWithin", "ALARM.setIntervalAndHardReset",
           "Global.TRIANGLELFO"}
ALIASES = {"PhysicsBody.": "physicsBody.", "PhysicsData.": "physicsData.",
           "ShapeDef.": "shapeDef."}
ASSOCIATED = {
    "entityState.invisible": ("Entity.setIsInvisible",),
    "entityState.staticDraw": ("Entity.setIsStaticDraw",),
    "entityState.editable": ("Entity.setIsEditable",),
    "entityState.beingEdited": ("Entity.setIsBeingEdited",),
    "entityState.placing": ("Entity.setIsPlacing",),
    "entityState.alwaysProcessed": ("Entity.setIsAlwaysProcessed",),
    "entityState.sauceBase": ("Entity.setIsSauceBase",),
    "entityState.descriptionBits": ("Entity.setDescriptionBits",),
    "entityState.creationPosX": ("Entity.setEntityCreationPos",),
    "entityState.creationPosY": ("Entity.setEntityCreationPos",),
    "entityState.limitVolPerc": ("Entity.setLimitVolPerc",),
    "entityState.offScreensAudible": ("Entity.setNumOffScreensAudible",),
    "entityState.neverOnScreen": ("Entity.set_getAudibilityPerc_neverOnScreen",),
    "entityState.mixerID": ("Entity.setMixerID",),
    "entityState.deleted": ("Entity.tagForDeletion", "EntityManager.flush"),
    "entityState.gotSet": ("Entity.setGot",),
    "entityState.gotCleared": ("Entity.setGot",),
    "entityState.screenMinX": ("Entity.initializeScreenExtents",),
    "entityState.screenMaxX": ("Entity.initializeScreenExtents",),
    "entityState.screenMinY": ("Entity.initializeScreenExtents",),
    "entityState.screenMaxY": ("Entity.initializeScreenExtents",),
    "entityState.perfFirst": ("EntityManager.setEntityPerfCost",),
    "entityState.perfAdditional": ("EntityManager.setEntityPerfCost",),
    "entityState.perfMax": ("EntityManager.setEntityPerfMaxCost",),
    "entityState.alarmIntervalSet": ("ALARM.setInterval",),
    "entityState.alarmElapsedSetInterval": ("ALARM.setInterval",),
    "entityState.alarmElapsedResetAlias": ("ALARM.reset",),
    "entityState.alarmRandomResetRange": ("ALARM.randomReset",),
    "entityState.alarmEnabledDisabled": ("ALARM.setIsEnabled",),
    "entityState.lfoPhase0": ("LFO.setPhase",),
    "entityState.addLFO": ("Entity.addLFO",),
    "entityState.compatibilityCallsComplete": (
        "Entity.deleteLFO", "Entity.addTelescope", "Entity.clearTelescopes",
        "Entity.disownVoice", "Entity.forceAudibilityRecalc", "Entity.redrawSauceBase",
        "Entity.update", "Entity.updateExplosionIfGot", "Entity.die"),
    "entityState.clearFlags": ("Entity.setIsInvisible", "Entity.setIsStaticDraw",
                               "Entity.setIsEditable", "Entity.setIsBeingEdited",
                               "Entity.setIsPlacing", "Entity.setIsAlwaysProcessed",
                               "Entity.setIsSauceBase",
                               "Entity.set_getAudibilityPerc_neverOnScreen"),
    "entityMgr.tagForDeletion.tagged": ("Entity.tagForDeletion",),
    "entityMgr.deleteAll.activeStillValid": ("EntityManager.deleteAllEntities",),
    "entityMgr.deleteAll.pendingStillInvalid": ("EntityManager.deleteAllEntities",),
    "entityMgr.deleteAll.countBeforeFlush": ("EntityManager.deleteAllEntities",),
    "entityMgr.deleteAll.bRemoved": ("EntityManager.deleteAllEntities", "EntityManager.flush"),
    "entityMgr.deleteAll.countAfterFlush": ("EntityManager.flush",),
    "entityMgr.flush.aRemoved": ("EntityManager.flush",),
    "entityMgr.flush.cRemovedByObserver": ("EntityManager.flush",),
    "entityMgr.flush.bRetained": ("EntityManager.flush",),
    "entityMgr.flush.destroyCount": ("EntityManager.flush",),
    "entityMgr.flush.destroyOrder": ("EntityManager.flush",),
    "entityMgr.flush.destructorSpawnPending": ("EntityManager.flush",),
    "entityMgr.flush.pendingCount": ("EntityManager.flush",),
    "entityMgr.flush.committedCount": ("EntityManager.flush",),
    "entityMgr.flush.destructorSpawnCommitted": ("EntityManager.flush",),
    "entityMgr.deleteAll.destroyCount": ("EntityManager.deleteAllEntities", "EntityManager.flush"),
    "PhysicsBody.appendShapes.index": ("shapeDef.addCircle", "Global.PHYSICSSHAPECOLLECTIONDEF"),
    "joint.distance.t0.type": ("Physics.createJoint", "jointType.setupDistanceJoint"),
    "joint.revolute.t0.type": ("Physics.createJoint", "jointType.setupJoint"),
    "joint.pin.t0.type": ("Physics.createJoint", "jointType.setupPinJoint"),
    "joint.prismatic.t0.type": ("Physics.createJoint", "jointType.setupJoint",
                                "jointType.setPrismaticWorldAxis"),
    "joint.relative.t0.type": ("Physics.createJoint", "jointType.setupRelativeJoint",
                               "jointType.setRelativeVel"),
}
SCENE_FIELDS = {
    "x": "physicsBody.getPos", "y": "physicsBody.getPos",
    "vx": "physicsBody.getVel", "vy": "physicsBody.getVel",
    "colliding": "physicsData.isColliding", "bullet": "physicsBody.isBullet",
}
CONTACT_FIELDS = {
    "trace": "Contact callback order",
    "aP": "Entity.onEntityCollidePreSolve", "bP": "Entity.onEntityCollidePreSolve",
    "aC": "Entity.onEntityCollide", "bC": "Entity.onEntityCollide",
    "aT": "Entity.onEntityTouch", "bT": "Entity.onEntityTouch",
    "aS": "Entity.onEntitySeparate", "bS": "Entity.onEntitySeparate",
    "aArg": "Contact callback argument", "bArg": "Contact callback argument",
    "aBodies": "physicsData.getNumBodies", "bBodies": "physicsData.getNumBodies",
    "colliding": "physicsData.isColliding",
}
QUERY_FIELDS = {
    "colliding": "physicsData.isColliding", "anyColliding": "physicsData.isColliding",
    "exists": "physicsData.getCollisionInfo",
    "category": "physicsBody.getCollisionCategory",
    "mask": "physicsBody.getCategoriesToCollideWith",
    "group": "physicsBody.getCollisionGroup",
    "enabled": "physicsBody.isCollidable",
    "listSize": "intIter.size", "idMatches": "intIter.get",
    "firstExists": "collisionInfo.getFirstShapeCollisionInfo",
    "shapeExists": "collisionInfo.getShapeCollisionInfo",
    "entityMatches": "collisionInfo.getEntity",
    "allSensors": "collisionInfo.isAllSensors",
    "bodyColliding": "collisionInfo.isBodyColliding",
    "bodyPair": "collisionInfo.isBodyCollidingWithBody",
    "bodyShape": "collisionInfo.isBodyCollidingWithShape",
    "shapeColliding": "collisionInfo.isShapeColliding",
    "mySensorsOnly": "collisionInfo.isMyShapeCollidingWithSensorsOnly",
    "missingShape": "collisionInfo.getShapeCollisionInfo",
    "myCount": "shapeCollisionInfo.getNumOfMyShapesColliding",
    "myIDMatches": "shapeCollisionInfo.getMyContactShapeID",
    "contactCount": "shapeCollisionInfo.getNumContacts",
    "badMyID": "shapeCollisionInfo.getMyContactShapeID",
    "pairNX": "collisionInfo.getContactNormalOfMyCollidingShape",
    "pairNY": "collisionInfo.getContactNormalOfMyCollidingShape",
    "contactNX": "shapeCollisionInfo.getContactNormal",
    "contactNY": "shapeCollisionInfo.getContactNormal",
    "contactX": "shapeCollisionInfo.getContactPos",
    "contactY": "shapeCollisionInfo.getContactPos",
}
RAY_GLOBAL_FIELDS = {"count": "Physics.raycast", "trace": "Physics.raycast",
                     "simple": "Physics.raycastSimple"}
RAY_ONE_FIELDS = {"oneHit", "oneEntity", "oneSidMatches", "oneLambda",
                  "oneNX", "oneNY"}
RAY_BODY_FIELDS = {"hit", "lambda", "nx", "ny"}
RAY_SHAPE_FIELDS = {"shapeHit", "shapeMatches", "shapeLambda"}
RAY_PD_SHAPE_FIELDS = {"pdShapeHit", "pdShapeMatches"}
RAY_PD_FIELDS = {"pdHit", "pdLambda"}
REENTRY_FIELDS = {"colliding": "physicsData.isColliding",
                  "mask": "physicsBody.getCategoriesToCollideWith",
                  "bX": "physicsBody.getPos"}
JOINT_FIELDS = {
    "type": "jointType.getType", "body1X": "jointType.getBody1",
    "body2X": "jointType.getBody2", "entity1Same": "jointType.getEntity1",
    "entity2Same": "jointType.getEntity2",
    "collidable": "jointType.isBodiesCollidable",
    "anchorX": "jointType.getAnchorPos", "anchorY": "jointType.getAnchorPos",
    "forceX": "jointType.getReactionForce", "forceY": "jointType.getReactionForce",
    "torque": "jointType.getReactionTorque",
    "aX": "physicsBody.getPos", "bX": "physicsBody.getPos",
}
RAY_MOTION_FIELDS = {"count": "Physics.raycast", "trace": "Physics.raycast",
                     "simple": "Physics.raycastSimple", "hit": "Physics.raycastOne",
                     "entity": "Physics.raycastOne", "lambda": "Physics.raycastOne",
                     "nx": "Physics.raycastOne", "ny": "Physics.raycastOne"}
ENTITY_LIFECYCLE_FIELDS = {
    "aValid": "EntityManager.isEntityIDValid", "bValid": "EntityManager.isEntityIDValid",
    "aEnabled": "Entity.isEnabled", "aBodies": "physicsData.getNumBodies",
    "aColliding": "physicsData.isColliding", "trace": "Entity callbacks",
    "aP": "Entity.onEntityCollidePreSolve", "bP": "Entity.onEntityCollidePreSolve",
    "aC": "Entity.onEntityCollide", "bC": "Entity.onEntityCollide",
    "aT": "Entity.onEntityTouch", "bT": "Entity.onEntityTouch",
    "aS": "Entity.onEntitySeparate", "bS": "Entity.onEntitySeparate",
}
ENTITY_MANAGER_FIELDS = {
    "create.count": "EntityManager.createEntity",
    "getNumEntities.pending": "EntityManager.getNumEntities",
    "getNumEntities.totalPending": "EntityManager.getNumEntities",
    "getNumEntities.committed": "EntityManager.getNumEntities",
    "isEntityIDValid.pending": "EntityManager.isEntityIDValid",
    "isEntityIDValid.committed": "EntityManager.isEntityIDValid",
    "getAllEntities.pendingSize": "EntityManager.getAllEntities",
    "getAllEntities.snapshotSize": "EntityManager.getAllEntities",
    "getAllEntities.refilledSize": "EntityManager.getAllEntities",
    "getAllEntities.firstValid": "EntityManager.getAllEntities",
    "getAllEntities.sentinelValid": "EntityListRef.isValid",
    "getAllEntities.wrapValid": "EntityListRef.goToNext",
    "getAllEntities.wrapFirst": "EntityListRef.get",
    "tagForDeletion.tagged": "Entity.isTaggedForDeletion",
    "tagForDeletion.stillValid": "EntityManager.isEntityIDValid",
    "tagForDeletion.countBeforeFlush": "EntityManager.getNumEntities",
    "flush.aRemoved": "EntityManager.isEntityIDValid",
    "flush.cRemovedByObserver": "EntityManager.isEntityIDValid",
    "flush.bRetained": "EntityManager.isEntityIDValid",
    "flush.destroyCount": "Entity callbacks",
    "flush.destroyOrder": "Entity callbacks",
    "flush.destructorSpawnPending": "EntityManager.isEntityIDValid",
    "flush.pendingCount": "EntityManager.getNumEntities",
    "flush.committedCount": "EntityManager.getAllEntities",
    "flush.destructorSpawnCommitted": "EntityManager.isEntityIDValid",
    "deleteAll.activeStillValid": "EntityManager.isEntityIDValid",
    "deleteAll.pendingStillInvalid": "EntityManager.isEntityIDValid",
    "deleteAll.countBeforeFlush": "EntityManager.getNumEntities",
    "deleteAll.bRemoved": "EntityManager.isEntityIDValid",
    "deleteAll.dRemoved": "EntityManager.isEntityIDValid",
    "deleteAll.countAfterFlush": "EntityManager.getNumEntities",
    "deleteAll.destroyCount": "Entity callbacks",
}
ENTITY_QUERY_FIELDS = {
    "id.sameObject": "EntityManager.getEntity",
    "id.uidLookup": "EntityManager.getEntityFromUID",
    "id.managerUID": "Entity.getUID",
    "id.objectStable": "Entity.getObjectID",
    "id.uidMissing": "EntityManager.getEntityFromUID",
    "id.missingObjectID": "EntityManager.getEntity",
    "count.all": "EntityManager.getNumEntities",
    "count.manual": "EntityManager.getNumManualEntities",
    "count.nonManual": "EntityManager.getNumNonManualEntities",
    "count.players": "EntityManager.getNumPlayers",
    "filter.all": "EntityManager.getAllEntities",
    "filter.manual": "EntityManager.getAllManualEntities",
    "filter.nonManual": "EntityManager.getAllNonManualEntities",
    "filter.players": "EntityManager.getAllPlayerEntities",
    "filter.playersRandom": "EntityManager.getAllPlayerEntitiesRandomly",
    "filter.randomType": "EntityManager.getRandomEntity",
    "filter.randomPlayer": "EntityManager.getRandomPlayer",
    "near.size": "EntityManager.getEntitiesNear",
    "near.a": "EntityManager.getEntitiesNear",
    "near.b": "EntityManager.getEntitiesNear",
    "near.farExcluded": "EntityManager.getEntitiesNear",
    "near.negativeEmpty": "EntityManager.getEntitiesNear",
    "near.reuseSize": "EntityManager.getEntitiesNear",
    "near.reuseFar": "EntityManager.getEntitiesNear",
    "box.inclusive": "EntityManager.getEntitiesRoughlyWithin",
    "box.bOutside": "EntityManager.getEntitiesRoughlyWithin",
    "shape.bodyLookup": "Physics.getBodyFromShapeID",
    "shape.entityLookup": "Physics.getEntityFromShapeID",
    "shape.sensorSolid": "Physics.isShapeSensor",
    "shape.sensorSensor": "Physics.isShapeSensor",
    "shape.lookup": "Physics.getShapeByID",
    "shape.bodyMembership": "physicsData.getShapeByID",
    "shape.nearCount": "Physics.getShapesNear",
    "shape.exactCount": "Physics.getShapesWithin",
    "shape.boxHasA": "Physics.getShapesRoughlyWithin",
    "shape.boxExcludesB": "Physics.getShapesRoughlyWithin",
    "shape.negativeEmpty": "Physics.getShapesNear",
    "shape.rayCount": "Physics.raycastShapes",
    "shape.rayHasA": "Physics.raycastShapes",
    "collisionIgnore.setGet": "Physics.setNumFramesToIgnoreCollisions",
    "collisionIgnore.reset": "Physics.setNumFramesToIgnoreCollisions",
    "iterator.snapshotStable": "EntityManager.getAllEntities",
    "iterator.newEntityExcluded": "EntityManager.getAllEntities",
    "iterator.resetFirst": "EntityManager.getAllEntities",
    "iterator.deletedSnapshotSizeStable": "EntityManager.getAllEntities",
    "iterator.deletedEntryNull": "EntityManager.getAllEntities",
}
ENTITY_STATE_FIELDS = {
    "invisible": "Entity.isInvisible", "staticDraw": "Entity.isStaticDraw",
    "editable": "Entity.isEditable", "beingEdited": "Entity.isBeingEdited",
    "placing": "Entity.isPlacing", "alwaysProcessed": "Entity.isAlwaysProcessed",
    "sauceBase": "Entity.isSauceBase", "descriptionBits": "Entity.getDescriptionBits",
    "clearFlags": "Entity.isInvisible",
    "creationPosX": "Entity.getEntityCreationPos", "creationPosY": "Entity.getEntityCreationPos",
    "limitVolPerc": "Entity.getLimitVolPerc",
    "offScreensAudible": "Entity.getNumOffScreensAudible",
    "neverOnScreen": "Entity.is_getAudibilityPerc_neverOnScreen",
    "mixerID": "Entity.getMixerID", "typeName": "Entity.getTypeName",
    "numericType": "Entity.getType",
    "objectIDsDistinct": "Entity.getObjectID", "isType": "Entity.isType",
    "wrongType": "Entity.isType", "sameType": "Entity.isSameType",
    "distance": "Entity.getDistanceTo", "vectorToX": "Entity.getVectorTo",
    "vectorToY": "Entity.getVectorTo", "vectorNormX": "Entity.getVectorToNormalized",
    "vectorNormY": "Entity.getVectorToNormalized", "radiansToFace": "Entity.getRadiansToFace",
    "whichSide": "Entity.getWhichSide", "mainBody": "Entity.getMainBody",
    "hasAlwaysProcessedGetter": "Entity.isAlwaysProcessed",
    "hasCreationPosGetter": "Entity.getEntityCreationPos",
    "hasAddAlarm": "Entity.addAlarm", "hasAddLFO": "Entity.addLFO",
    "hasCallMethod": "Entity.callMethod", "hasWhichSide": "Entity.getWhichSide",
    "screenMinX": "Entity.getMinCX", "screenMaxX": "Entity.getMaxCX",
    "screenMinY": "Entity.getMinCY", "screenMaxY": "Entity.getMaxCY",
    "onScreenInside": "Entity.isOnScreen", "onScreenOutside": "Entity.isOnScreen",
    "gotDefault": "Entity.isGot", "gotSet": "Entity.isGot", "gotCleared": "Entity.isGot",
    "getting": "Entity.isGetting", "callMethod": "Entity.callMethod",
    "createParticle": "Entity.createParticle", "particleCount": "Entity.getNumParticles",
    "particleIterator": "Entity.getAllParticles", "notOkToRun": "Entity.isOkToRun",
    "alarmInterval": "ALARM.getInterval", "alarmElapsedInitial": "ALARM.getMSElapsed",
    "alarmEnabledReady": "ALARM.isEnabled", "alarmEnabledDisabled": "ALARM.isEnabled",
    "alarmInitiallyReady": "ALARM.isAlarming", "alarmTimeInitial": "ALARM.getTimeTillAlarm",
    "alarmUpdateEarly": "ALARM.update", "alarmElapsedEarly": "ALARM.getMSElapsed",
    "alarmIntervalSet": "ALARM.setInterval", "alarmElapsedSetInterval": "ALARM.setInterval",
    "alarmUpdateReady": "ALARM.update", "alarmReady": "ALARM.isAlarming",
    "alarmPercentageReady": "ALARM.getAlarmPercentage", "alarmElapsedReset": "ALARM.hardReset",
    "alarmElapsedResetAlias": "ALARM.reset", "alarmRandomResetRange": "ALARM.randomReset",
    "alarmIntervalReset": "ALARM.setIntervalAndHardReset", "alarmElapsedPhase": "ALARM.phaseReset",
    "alarmUpdateDisabled": "ALARM.update", "alarmElapsedDisabled": "ALARM.getMSElapsed",
    "addLFO": "Entity.addLFO", "lfoObjectNonNull": "Global.TRIANGLELFO",
    "lfoRate": "LFO.setRate", "lfoRate50": "LFO.setRate",
    "lfoRate100": "LFO.setRate", "lfoRate400": "LFO.setRate",
    "lfoPhase0": "LFO.setRange", "lfoPhaseQuarter": "LFO.setPhase",
    "lfoPhaseHalf": "LFO.setPhase", "lfoPhaseThreeQuarter": "LFO.setPhase",
    "lfoContourBefore": "LFO.setContour", "lfoContourAfter": "LFO.setContour",
    "lfoValue": "LFO.getValue", "lfoUpdatedValue": "LFO.update",
    "compatibilityCallsComplete": "Entity.update",
    "okToRun": "Entity.isOkToRun", "deleted": "EntityManager.isEntityIDValid",
    "otherDeleted": "EntityManager.isEntityIDValid", "dummyStable": "EntityManager.getDummyEntity",
    "perfFirst": "EntityManager.getEntityPerfCost",
    "perfAdditional": "EntityManager.getEntityPerfCost",
    "perfMax": "EntityManager.getEntityPerfMaxCost",
    "perfMissing": "EntityManager.getEntityPerfCost",
}
BODY_CONTROL_FIELDS = {
    "position": "physicsBody.setPos", "velocity": "physicsBody.setVel",
    "speed": "physicsBody.getSpeed", "setSpeed": "physicsBody.setSpeed",
    "direction": "physicsBody.getDir",
    "mass": "physicsBody.setMass", "maxSpeed": "physicsBody.setMaxSpeed",
    "maxRotateSpeed": "physicsBody.setMaxRotateSpeed",
    "friction": "physicsBody.setFriction", "restitution": "physicsBody.setRestitution",
    "sensor": "physicsBody.setIsSensor", "collidable": "physicsBody.setIsCollidable",
    "bullet": "physicsBody.setIsBullet", "category": "physicsBody.setCollisionCategory",
    "mask": "physicsBody.setCategoriesToCollideWith",
    "group": "physicsBody.setCollisionGroup", "angularVelocity": "physicsBody.setAngularVel",
    "pointInside": "physicsBody.isPointWithin", "pointOutside": "physicsBody.isPointWithin",
    "shapeCount": "physicsBody.getNumShapes", "sleeping": "physicsBody.isSleeping",
}
LOCAL_ONLY = {
    "physicsBody.isCircle": "OpenEgg convenience method; no retail method exists",
    "physicsBody.isRect": "OpenEgg convenience method; no retail method exists",
    "physicsShape.getIsSensor": "OpenEgg alias; retail exposes isSensor",
}


def inventory():
    with (ROOT / "docs/api/function_notes.tsv").open() as source:
        names = {row["name"] for row in csv.DictReader(source, delimiter="\t")
                 if row["name"].startswith(PREFIXES) or row["name"] in SPECIAL}
    names.add("ALARM.setIntervalAndHardReset")
    return sorted(names)


def owner(name, names):
    if name.startswith("processPause."):
        field = name[len("processPause."):]
        function = {
            "created": "GameSystem.runProcess",
            "initA": "GameSystem.runProcess",
            "initB": "GameSystem.runProcess",
            "updatesBefore": "GameSystem.runProcess",
            "postsBefore": "GameSystem.runProcess",
            "updatesBeforePause": "GameSystem.runProcess",
            "postsBeforePause": "GameSystem.runProcess",
            "pauseObserved": "GameSystem.isPaused",
            "bHeld": "GameSystem.isPaused",
            "pauseRequestState": "GameSystem.isPaused",
            "unpauseObserved": "GameSystem.isPaused",
            "resumed": "GameSystem.isPaused",
            "updatesAfterResume": "GameSystem.setAsPauseProcess",
            "postsAfterResume": "GameSystem.setAsPauseProcess",
            "callbackOrder": "GameSystem.setAsPauseProcess",
            "destroyed": "Process.terminate",
            "running": "GameSystem.isProcessRunning",
            "count": "GameSystem.getProcessCount",
        }.get(field)
        if function not in names:
            raise ValueError(f"unknown process pause field: {field}")
        return function
    if name.startswith("process."):
        field = name[len("process."):]
        function = {
            "created": "GameSystem.runProcess",
            "constructed": "GameSystem.runProcess",
            "running": "GameSystem.isProcessRunning",
            "same": "GameSystem.getProcess",
            "count.created": "GameSystem.getProcessCount",
            "initialized": "GameSystem.runProcess",
            "updates.before": "GameSystem.runProcess",
            "posts.before": "GameSystem.runProcess",
            "updates.after": "GameSystem.runProcess",
            "terminate.queued.running": "Process.terminate",
            "terminate.queued.destroyed": "Process.terminate",
            "terminate.queued.count": "Process.terminate",
            "destroyed": "Process.terminate",
            "running.after": "GameSystem.isProcessRunning",
            "count.after": "GameSystem.getProcessCount",
        }.get(field)
        if function not in names:
            raise ValueError(f"unknown process field: {field}")
        return function
    if name.startswith("entityMgr."):
        field = name[len("entityMgr."):]
        if field.startswith("update.t"):
            function = {
                "parentUpdates": "Entity callbacks",
                "parentValid": "EntityManager.isEntityIDValid",
                "childCreated": "Entity callbacks",
                "childValid": "EntityManager.isEntityIDValid",
                "childValidDuringCreate": "EntityManager.isEntityIDValid",
                "childCountDuringCreate": "EntityManager.getNumEntities",
                "childUpdates": "Entity callbacks",
                "childTagged": "Entity callbacks",
            }[field.rsplit(".", 1)[1]]
        else:
            function = ENTITY_MANAGER_FIELDS[field]
        if function not in names and function != "Entity callbacks" and \
                not function.startswith("EntityListRef."):
            raise ValueError(f"unknown entity manager function: {function}")
        return function
    if name.startswith("entityQ."):
        function = ENTITY_QUERY_FIELDS[name[len("entityQ."):]]
        if function not in names:
            raise ValueError(f"unknown entity query function: {function}")
        return function
    if name.startswith("entityState."):
        function = ENTITY_STATE_FIELDS[name[len("entityState."):]]
        if function not in names:
            raise ValueError(f"unknown entity state function: {function}")
        return function
    if name.startswith("bodyCtl."):
        function = BODY_CONTROL_FIELDS[name[len("bodyCtl."):]]
        if function not in names:
            raise ValueError(f"unknown body-control function: {function}")
        return function
    if name.startswith("bodyApi."):
        function = name[len("bodyApi."):]
        if function not in names:
            raise ValueError(f"unknown body API function: {function}")
        return function
    if name.startswith("entityLife."):
        function = ENTITY_LIFECYCLE_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names and function != "Entity callbacks" and \
                not function.startswith("Entity.onEntity"):
            raise ValueError(f"unknown entity lifecycle function: {function}")
        return function
    if name.startswith("joint."):
        function = JOINT_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names:
            raise ValueError(f"unknown joint function: {function}")
        return function
    if name.startswith("raymove."):
        function = RAY_MOTION_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names:
            raise ValueError(f"unknown moving ray function: {function}")
        return function
    if name.startswith("reentry."):
        function = REENTRY_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names:
            raise ValueError(f"unknown reentry function: {function}")
        return function
    if name.startswith("query."):
        function = QUERY_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names:
            raise ValueError(f"unknown query function: {function}")
        return function
    if name.startswith("ray."):
        field = name.rsplit(".", 1)[1]
        if field in RAY_GLOBAL_FIELDS:
            function = RAY_GLOBAL_FIELDS[field]
        elif field in RAY_ONE_FIELDS:
            function = "Physics.raycastOne"
        elif field in RAY_BODY_FIELDS:
            function = "physicsBody.raycast"
        elif field in RAY_SHAPE_FIELDS:
            function = "physicsBody.raycastShapesOne"
        elif field in RAY_PD_SHAPE_FIELDS:
            function = "physicsData.raycastShapesOne"
        elif field in RAY_PD_FIELDS:
            function = "physicsData.raycast"
        else:
            raise ValueError(f"unknown ray field: {field}")
        if function not in names:
            raise ValueError(f"unknown ray function: {function}")
        return function
    if name.startswith(("contact.", "removal.")):
        field = name.rsplit(".", 1)[1]
        function = CONTACT_FIELDS[field]
        if function not in names and not function.startswith(("Entity.", "Contact ")):
            raise ValueError(f"unknown contact function: {function}")
        return function
    if name.startswith("scene."):
        function = SCENE_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names:
            raise ValueError(f"unknown scene function: {function}")
        return function
    converted = name
    for old, new in ALIASES.items():
        if name.startswith(old):
            converted = new + name[len(old):]
            break
    matches = [item for item in names if converted == item or converted.startswith(item + ".")]
    if not matches:
        raise ValueError(f"unmapped physics reading: {name}")
    return max(matches, key=len)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--report", type=Path, default=REPORT)
    args = parser.parse_args()
    names = inventory()
    log_dir = ROOT / "tests/_results/physics"
    log_dir.mkdir(parents=True, exist_ok=True)
    problems = []
    rows = []
    inconclusive_rows = []
    local_only_rows = []
    awaiting_retail_rows = []
    local_only_counts = defaultdict(int)
    awaiting_retail_counts = defaultdict(int)
    grouped = defaultdict(list)
    scenes = defaultdict(list)
    for probe, baseline, plan, support in ((PROBE, BASELINE, None, ()),
                                           (SCENES, SCENE_BASELINE, SCENE_PLAN, ()),
                                           (CONTACTS, CONTACT_BASELINE, CONTACT_PLAN, (CONTACT_SUPPORT,)),
                                           (REMOVAL, REMOVAL_BASELINE, REMOVAL_PLAN, (CONTACT_SUPPORT,)),
                                           (QUERIES, QUERY_BASELINE, QUERY_PLAN, (CONTACT_SUPPORT,)),
                                           (RAYS, RAY_BASELINE, RAY_PLAN, (CONTACT_SUPPORT,)),
                                           (REENTRY, REENTRY_BASELINE, REENTRY_PLAN, (CONTACT_SUPPORT,)),
                                           (JOINTS, JOINT_BASELINE, JOINT_PLAN, ()),
                                           (RAY_MOTION, RAY_MOTION_BASELINE, RAY_MOTION_PLAN, ()),
                                           (ENTITY_LIFECYCLE, ENTITY_LIFECYCLE_BASELINE,
                                            ENTITY_LIFECYCLE_PLAN,
                                            (ENTITY_LIFECYCLE_SUPPORT,)),
                                           (ENTITY_MANAGER, ENTITY_MANAGER_BASELINE,
                                            ENTITY_MANAGER_PLAN,
                                            (HERE / "EntityManagerProbeEntity.gm",)),
                                           (ENTITY_QUERIES, ENTITY_QUERIES_BASELINE,
                                            ENTITY_QUERIES_PLAN,
                                            (ENTITY_QUERIES_SUPPORT,)),
                                           (ENTITY_STATE, ENTITY_STATE_BASELINE,
                                            ENTITY_STATE_PLAN,
                                            (ENTITY_STATE_SUPPORT,)),
                                           (PROCESS_ORDER, PROCESS_ORDER_BASELINE,
                                            PROCESS_ORDER_PLAN, (PROCESS_ORDER_SUPPORT,)),
                                           (PROCESS_PAUSE, PROCESS_PAUSE_BASELINE,
                                            PROCESS_PAUSE_PLAN, PROCESS_PAUSE_SUPPORT),
                                           (BODY_CONTROLS, BODY_CONTROLS_BASELINE,
                                            BODY_CONTROLS_PLAN,
                                            (ENTITY_QUERIES_SUPPORT,))):
        declared = expected_readings(probe, plan)
        plan_data = json.loads(plan.read_text()) if plan else {}
        awaiting_retail = set(plan_data.get("awaitingRetail", []))
        if not awaiting_retail:
            check_baseline_source(baseline, probe, support)
        inconclusive = plan_data.get("inconclusive", {})
        if len(declared) != len(set(declared)):
            raise ValueError(f"duplicate probe names: {probe}")
        _, error, log_path, _ = run_script(args.engine.resolve(), HERE, probe.name,
                                           HERE / "smoke.harness", 20, log_dir,
                                           strict="survey")
        if set(declared) <= awaiting_retail:
            retail, retail_dupes = {}, set()
        else:
            retail, retail_dupes = read_readings(baseline)
        local, local_dupes = read_readings(log_path)
        if error:
            problems.append(f"{probe.name}: {error}")
        for label, values, dupes in (("retail", retail, retail_dupes),
                                     ("OpenEgg", local, local_dupes)):
            required = set(declared) - awaiting_retail if label == "retail" else declared
            problems += [f"{probe.name} {label}: missing {key}"
                         for key in required if key not in values]
            problems += [f"{probe.name} {label}: unexpected {key}"
                         for key in values if key not in declared]
            problems += [f"{probe.name} {label}: duplicate {key}"
                         for key in sorted(dupes)]
        for case in declared:
            function = owner(case, names)
            if case in awaiting_retail:
                awaiting_retail_rows.append((case, function, local.get(case)))
                awaiting_retail_counts[function] += 1
                for extra in ASSOCIATED.get(case, ()):
                    if extra not in names:
                        raise ValueError(f"unknown associated function: {extra}")
                    awaiting_retail_counts[extra] += 1
                continue
            if case in inconclusive:
                inconclusive_rows.append((case, function, retail.get(case),
                                          local.get(case), inconclusive[case]))
                continue
            if function in LOCAL_ONLY:
                local_only_rows.append((case, function, retail.get(case),
                                        local.get(case), LOCAL_ONLY[function]))
                local_only_counts[function] += 1
                continue
            passed = retail.get(case) == local.get(case) and case in retail and case in local
            rows.append((case, function, retail.get(case), local.get(case), passed))
            grouped[function].append(passed)
            if case.startswith(("scene.", "contact.", "removal.", "entityLife.",
                                "entityMgr.", "entityQ.", "query.",
                                "ray.", "reentry.", "joint.", "raymove.",
                                "bodyCtl.", "process.", "processPause.")):
                scenes[".".join(case.split(".")[:2])].append(passed)
            for extra in ASSOCIATED.get(case, ()):
                if extra not in names:
                    raise ValueError(f"unknown associated function: {extra}")
                grouped[extra].append(passed)
    count = sum(row[4] for row in rows)
    tested = sum(bool(grouped[name]) or bool(local_only_counts[name]) or
                 bool(awaiting_retail_counts[name]) for name in names)
    lines = ["# Physics test report", "",
             f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}", "",
             f"Matched: **{count}/{len(rows)} comparable readings ({percent(count, len(rows))})**. "
             f"{len(inconclusive_rows)} inconclusive, {len(local_only_rows)} local-only, and "
             f"{len(awaiting_retail_rows)} awaiting-retail reading(s) are listed separately. "
             "This measures captured cases, not total physics parity.", "",
             f"Functions with readings: **{tested}/{len(names)}**. The inventory comes from "
             "the local API reference and includes OpenEgg-only methods.", "",
             "## Sample groups", "",
             "Scenes sample motion; other groups cover callbacks, removal, queries, rays, joints, and filter reentry.", "",
             "| Group | Matched | Cases |", "| --- | ---: | ---: |"]
    for scene, checks in scenes.items():
        lines.append(f"| {scene} | {sum(checks)} | {len(checks)} |")
    lines += ["",
             "## Per function", "",
             "| Function | Matched | Cases | Status |", "| --- | ---: | ---: | --- |"]
    for function in names:
        checks = grouped[function]
        matched = sum(checks)
        local_count = local_only_counts[function]
        pending_count = awaiting_retail_counts[function]
        status = ("LOCAL-ONLY" if local_count else "AWAITING RETAIL" if pending_count else
                  "untested" if not checks else
                  "PASS" if matched == len(checks) else "FAIL")
        lines.append(f"| `{function}` | {matched} | {len(checks) + local_count + pending_count} | {status} |")
    lines += ["", "## Contact callbacks", "",
              "| Hook | Matched | Cases |", "| --- | ---: | ---: |"]
    for function in sorted(set(grouped) - set(names)):
        checks = grouped[function]
        lines.append(f"| `{function}` | {sum(checks)} | {len(checks)} |")
    if inconclusive_rows:
        lines += ["", "## Inconclusive probes", "",
                  "| Function | Case | Retail | OpenEgg | Reason |",
                  "| --- | --- | --- | --- | --- |"]
        for case, function, retail_value, local_value, reason in inconclusive_rows:
            lines.append(f"| `{function}` | {case} | `{retail_value}` | `{local_value}` | {reason} |")
    if local_only_rows:
        lines += ["", "## OpenEgg-only functions", "",
                  "These extensions have no corresponding retail method and are excluded from the parity score.", "",
                  "| Function | Case | Retail | OpenEgg | Reason |",
                  "| --- | --- | --- | --- | --- |"]
        for case, function, retail_value, local_value, reason in local_only_rows:
            lines.append(f"| `{function}` | {case} | `{retail_value}` | `{local_value}` | {reason} |")
    if awaiting_retail_rows:
        lines += ["", "## Awaiting retail capture", "",
                  "These cases ran on OpenEgg but have no retail readings yet; they are excluded from parity scores.", "",
                  "| Function | Case | OpenEgg |", "| --- | --- | --- |"]
        for case, function, local_value in awaiting_retail_rows:
            lines.append(f"| `{function}` | {case} | `{local_value}` |")
    lines += ["", "## Readings", "", "| Function | Case | Retail | OpenEgg | Result |",
              "| --- | --- | --- | --- | --- |"]
    for case, function, a, b, passed in rows:
        lines.append(f"| `{function}` | {case} | `{a}` | `{b}` | {'PASS' if passed else 'FAIL'} |")
    if problems:
        lines += ["", "## Capture problems", ""] + [f"- {item}" for item in problems]
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text("\n".join(lines) + "\n")
    print(f"physics: {count}/{len(rows)} matched, {len(problems)} capture problems")
    print(f"Report: {args.report}")
    return 1 if count != len(rows) or problems else 0


if __name__ == "__main__":
    sys.exit(main())
