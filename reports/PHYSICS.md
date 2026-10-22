# Physics test report

Generated: 2026-10-22 18:46 UTC

Matched: **6475/6475 comparable readings (100.00%)**. 29 inconclusive, 3 local-only, and 5 awaiting-retail reading(s) are listed separately. This measures captured cases, not total physics parity.

Functions with readings: **341/561**. The inventory comes from the local API reference and includes OpenEgg-only methods.

## Sample groups

Scenes sample motion; other groups cover callbacks, removal, queries, rays, joints, and filter reentry.

| Group | Matched | Cases |
| --- | ---: | ---: |
| scene.free | 240 | 240 |
| scene.fast | 240 | 240 |
| scene.solid | 360 | 360 |
| scene.sensor | 300 | 300 |
| contact.solid | 108 | 108 |
| contact.sensor | 108 | 108 |
| removal.body | 98 | 98 |
| query.solidA | 406 | 406 |
| query.solidB | 406 | 406 |
| query.sensorA | 406 | 406 |
| query.sensorB | 406 | 406 |
| ray.solid | 9 | 9 |
| ray.sensors | 9 | 9 |
| ray.inside | 9 | 9 |
| ray.miss | 9 | 9 |
| ray.bodyA | 11 | 11 |
| ray.bodyB | 11 | 11 |
| ray.bodyMiss | 11 | 11 |
| reentry.sensor | 45 | 45 |
| joint.distance | 156 | 156 |
| joint.revolute | 156 | 156 |
| joint.pin | 132 | 132 |
| joint.prismatic | 156 | 156 |
| joint.relative | 156 | 156 |
| joint.registry | 5 | 5 |
| joint.control | 9 | 9 |
| joint.config | 6 | 6 |
| raymove.solid | 96 | 96 |
| raymove.sensor | 96 | 96 |
| raymove.inside | 96 | 96 |
| raymove.miss | 96 | 96 |
| entityLife.pair | 98 | 98 |
| entityLife.delete | 4 | 4 |
| entityMgr.update | 40 | 40 |
| entityMgr.create | 1 | 1 |
| entityMgr.getNumEntities | 3 | 3 |
| entityMgr.isEntityIDValid | 2 | 2 |
| entityMgr.getEntity | 2 | 2 |
| entityMgr.getAllEntities | 7 | 7 |
| entityMgr.tagForDeletion | 3 | 3 |
| entityMgr.flush | 9 | 9 |
| entityMgr.iteratorAfterDelete | 3 | 3 |
| entityMgr.iteratorAfterAdd | 6 | 6 |
| entityMgr.iteratorEmptyStayedEmpty | 1 | 1 |
| entityMgr.deleteAll | 7 | 7 |
| entityQ.id | 5 | 5 |
| entityQ.count | 4 | 4 |
| entityQ.filter | 7 | 7 |
| entityQ.near | 7 | 7 |
| entityQ.box | 2 | 2 |
| entityQ.shape | 13 | 13 |
| entityQ.collisionIgnore | 2 | 2 |
| entityQ.iterator | 5 | 5 |
| entityCallbacks.initialEnabled | 1 | 1 |
| entityCallbacks.enabledAfterSet | 1 | 1 |
| entityCallbacks.orderAfterSetEnable | 1 | 1 |
| entityCallbacks.enabledAfterCallback | 1 | 1 |
| entityCallbacks.orderAfterEnable | 1 | 1 |
| entityCallbacks.enabledAfterRepeat | 1 | 1 |
| entityCallbacks.disabledAfterSet | 1 | 1 |
| entityCallbacks.orderAfterSetDisable | 1 | 1 |
| entityCallbacks.disabledAfterCallback | 1 | 1 |
| entityCallbacks.orderAfterDisable | 1 | 1 |
| entityCallbacks.noHookEnable | 1 | 1 |
| entityCallbacks.noHookDisable | 1 | 1 |
| entityCallbacks.finalOrder | 1 | 1 |
| entityGC.scheduled | 1 | 1 |
| entityGC.collectingBeforeTick | 1 | 1 |
| entityGC.collectingAfterTick | 1 | 1 |
| entityGC.activeAfterGC | 1 | 1 |
| entityGC.lookupAfterGC | 1 | 1 |
| entityGC.bodyAfterGC | 1 | 1 |
| entityGC.deletedBeforeGC | 1 | 1 |
| entityGC.collectingAfterDeleteGC | 1 | 1 |
| entityGC.deletedAfterGC | 1 | 1 |
| entityGCCycles.firstCount | 1 | 1 |
| entityGCCycles.firstAX | 1 | 1 |
| entityGCCycles.firstBX | 1 | 1 |
| entityGCCycles.firstCX | 1 | 1 |
| entityGCCycles.afterDeleteCount | 1 | 1 |
| entityGCCycles.afterDeleteAValid | 1 | 1 |
| entityGCCycles.afterDeleteBValid | 1 | 1 |
| entityGCCycles.afterDeleteCValid | 1 | 1 |
| entityGCCycles.recreateCount | 1 | 1 |
| entityGCCycles.recreateDX | 1 | 1 |
| entityGCCycles.recreateAX | 1 | 1 |
| entityGCCycles.clearCount | 1 | 1 |
| entityGCCycles.clearAValid | 1 | 1 |
| entityGCCycles.clearDValid | 1 | 1 |
| entityGCIter.retainedSize | 1 | 1 |
| entityGCIter.retainedEntity | 1 | 1 |
| entityGCIter.deletedSize | 1 | 1 |
| entityGCIter.deletedEntryNull | 1 | 1 |
| entityGCIter.nullAfterGC | 1 | 1 |
| entityGCIter.newIteratorSize | 1 | 1 |
| entityGCIter.reusedIteratorSize | 1 | 1 |
| entityGCIter.secondFound | 1 | 1 |
| entityGCIter.clearCount | 1 | 1 |
| process.created | 1 | 1 |
| process.constructed | 1 | 1 |
| process.running | 2 | 2 |
| process.same | 1 | 1 |
| process.count | 2 | 2 |
| process.initialized | 1 | 1 |
| process.updates | 2 | 2 |
| process.posts | 1 | 1 |
| process.terminate | 3 | 3 |
| process.destroyed | 1 | 1 |
| processPause.created | 1 | 1 |
| processPause.initA | 1 | 1 |
| processPause.initB | 1 | 1 |
| processPause.updatesBefore | 1 | 1 |
| processPause.postsBefore | 1 | 1 |
| processPause.updatesBeforePause | 1 | 1 |
| processPause.postsBeforePause | 1 | 1 |
| processPause.pauseObserved | 1 | 1 |
| processPause.bHeld | 1 | 1 |
| processPause.pauseRequestState | 1 | 1 |
| processPause.unpauseObserved | 1 | 1 |
| processPause.resumed | 1 | 1 |
| processPause.updatesAfterResume | 1 | 1 |
| processPause.postsAfterResume | 1 | 1 |
| processPause.callbackOrder | 1 | 1 |
| processPause.destroyed | 1 | 1 |
| processPause.running | 1 | 1 |
| processPause.count | 1 | 1 |
| callbackMutation.trace | 1 | 1 |
| callbackMutation.fired | 1 | 1 |
| callbackMutation.otherValidBefore | 1 | 1 |
| callbackMutation.countBefore | 1 | 1 |
| callbackMutation.addedValidPending | 1 | 1 |
| callbackMutation.addedValidAfterCallbackFlush | 1 | 1 |
| callbackMutation.countAfterCreate | 1 | 1 |
| callbackMutation.jointType | 1 | 1 |
| callbackMutation.otherTagged | 1 | 1 |
| callbackMutation.otherValidTagged | 1 | 1 |
| callbackMutation.selfTagged | 1 | 1 |
| callbackMutation.otherValidAfterFlush | 1 | 1 |
| callbackMutation.selfValidAfterFlush | 1 | 1 |
| callbackMutation.addedValidAfterFlush | 1 | 1 |
| callbackMutation.finalCount | 1 | 1 |
| gcPressure.countAfterCreate | 1 | 1 |
| gcPressure.rootCount | 1 | 1 |
| gcPressure.firstRootValid | 1 | 1 |
| gcPressure.countBeforeCollect | 1 | 1 |
| gcPressure.validAfterCollect | 1 | 1 |
| gcPressure.retainedRootValid | 1 | 1 |
| gcPressure.totalAfterCollect | 1 | 1 |
| gcPressure.emptyAfterClear | 1 | 1 |
| gcPressure.oldIdsInvalid | 1 | 1 |
| sceneTransition.oldCreated | 1 | 1 |
| sceneTransition.speedBefore | 1 | 1 |
| sceneTransition.oldRunning | 1 | 1 |
| sceneTransition.oldDestroyed | 1 | 1 |
| sceneTransition.oldUpdates | 1 | 1 |
| sceneTransition.oldProcessGone | 1 | 1 |
| sceneTransition.newProcessRunning | 1 | 1 |
| sceneTransition.entitySurvived | 1 | 1 |
| sceneTransition.newInitialized | 1 | 1 |
| sceneTransition.processCount | 1 | 1 |
| pauseScale.halfSpeed | 1 | 1 |
| pauseScale.doubleSpeed | 1 | 1 |
| pauseScale.halfDeltaApplied | 1 | 1 |
| pauseScale.doubleDeltaApplied | 1 | 1 |
| pauseScale.pauseSeen | 1 | 1 |
| pauseScale.bHeld | 1 | 1 |
| pauseScale.resumed | 1 | 1 |
| pauseScale.finalSpeed | 1 | 1 |
| bodyCtl.position | 1 | 1 |
| bodyCtl.velocity | 1 | 1 |
| bodyCtl.speed | 1 | 1 |
| bodyCtl.setSpeed | 1 | 1 |
| bodyCtl.direction | 1 | 1 |
| bodyCtl.mass | 1 | 1 |
| bodyCtl.maxSpeed | 1 | 1 |
| bodyCtl.maxRotateSpeed | 1 | 1 |
| bodyCtl.friction | 1 | 1 |
| bodyCtl.restitution | 1 | 1 |
| bodyCtl.sensor | 1 | 1 |
| bodyCtl.collidable | 1 | 1 |
| bodyCtl.bullet | 1 | 1 |
| bodyCtl.category | 1 | 1 |
| bodyCtl.mask | 1 | 1 |
| bodyCtl.group | 1 | 1 |
| bodyCtl.angularVelocity | 1 | 1 |
| bodyCtl.shapeCount | 1 | 1 |
| bodyCtl.pointInside | 1 | 1 |
| bodyCtl.pointOutside | 1 | 1 |
| bodyCtl.sleeping | 1 | 1 |
| bodyCtl.flushAfterOne | 1 | 1 |
| bodyCtl.flushAfterTen | 1 | 1 |
| bodyCtl.updateEntitiesOff | 1 | 1 |
| bodyCtl.updateEntitiesOn | 1 | 1 |
| bodyCtl.updateInterval | 1 | 1 |
| bodyCtl.replaceBodyExists | 1 | 1 |
| bodyCtl.replaceBodyCount | 1 | 1 |
| bodyCtl.replaceBodyPos | 1 | 1 |
| bodyCtl.replaceBodyOutOfRangeExists | 1 | 1 |
| bodyCtl.removeBodyCount | 1 | 1 |
| bodyCtl.removeAllBodyBefore | 1 | 1 |
| bodyCtl.removeAllBodyAfter | 1 | 1 |
| bodyCtl.dataAppendFirst | 1 | 1 |
| bodyCtl.dataAppendSecond | 1 | 1 |
| bodyCtl.dataAppendShapeCount | 1 | 1 |
| bodyCtl.directionSetX | 1 | 1 |
| bodyCtl.directionSetY | 1 | 1 |
| bodyCtl.rotateVelSet | 1 | 1 |
| bodyCtl.bodyBoundsMinX | 1 | 1 |
| bodyCtl.bodyBoundsMaxX | 1 | 1 |
| bodyCtl.bodyBoundsMinY | 1 | 1 |
| bodyCtl.bodyBoundsMaxY | 1 | 1 |
| bodyCtl.dataBoundsMinX | 1 | 1 |
| bodyCtl.dataBoundsMaxX | 1 | 1 |
| bodyCtl.dataBoundsMinY | 1 | 1 |
| bodyCtl.dataBoundsMaxY | 1 | 1 |
| bodyCtl.forceAfterOneX | 1 | 1 |
| bodyCtl.forceAfterOneAngular | 1 | 1 |
| bodyCtl.forceAfterTenX | 1 | 1 |
| bodyCtl.forceAfterTenAngular | 1 | 1 |
| bodyCtl.ownerMatches | 1 | 1 |
| bodyCtl.bodyIDPositive | 1 | 1 |
| bodyCtl.shapeIDPositive | 1 | 1 |
| bodyCtl.shapeDensityBefore | 1 | 1 |
| bodyCtl.shapeDensityAfter | 1 | 1 |
| bodyCtl.shapeDensityValidMass | 1 | 1 |
| bodyCtl.shapeDensityInvalidMass | 1 | 1 |
| bodyCtl.shapeMaskValid | 1 | 1 |
| bodyCtl.shapeSensorOff | 1 | 1 |
| bodyCtl.shapeSensorOn | 1 | 1 |
| bodyCtl.shapeSensorInvalidReached | 1 | 1 |
| bodyCtl.shapeSensorInvalidContinues | 1 | 1 |
| bodyCtl.shapeMaskGetterInvalidReached | 1 | 1 |
| bodyCmd.thrust | 288 | 288 |
| bodyCmd.brake | 288 | 288 |
| bodyCmd.turn | 288 | 288 |
| bodyCmd.sleep | 288 | 288 |
| bodyCmd.idle | 288 | 288 |

## Per function

| Function | Matched | Cases | Status |
| --- | ---: | ---: | --- |
| `ALARM.getAlarmPercentage` | 3 | 3 | PASS |
| `ALARM.getInterval` | 1 | 1 | PASS |
| `ALARM.getMSElapsed` | 4 | 4 | PASS |
| `ALARM.getTimeTillAlarm` | 1 | 1 | PASS |
| `ALARM.hardReset` | 1 | 1 | PASS |
| `ALARM.isAlarming` | 2 | 2 | PASS |
| `ALARM.isEnabled` | 2 | 2 | PASS |
| `ALARM.phaseReset` | 3 | 3 | PASS |
| `ALARM.randomReset` | 2 | 2 | PASS |
| `ALARM.reset` | 2 | 2 | PASS |
| `ALARM.setInterval` | 4 | 4 | PASS |
| `ALARM.setIntervalAndHardReset` | 1 | 1 | PASS |
| `ALARM.setIsEnabled` | 1 | 1 | PASS |
| `ALARM.update` | 5 | 5 | PASS |
| `Entity.addAlarm` | 0 | 0 | untested |
| `Entity.addLFO` | 0 | 0 | untested |
| `Entity.addTelescope` | 1 | 1 | PASS |
| `Entity.callMethod` | 1 | 1 | PASS |
| `Entity.clearTelescopes` | 1 | 1 | PASS |
| `Entity.configure` | 0 | 0 | untested |
| `Entity.constructorSetIsManual` | 0 | 0 | untested |
| `Entity.constructorSetManual` | 0 | 0 | untested |
| `Entity.constructorSetNoPhysics` | 0 | 0 | untested |
| `Entity.constructorSetNoPhysicsCollide` | 0 | 0 | untested |
| `Entity.constructorSetNoPhysicsUpdate` | 0 | 0 | untested |
| `Entity.constructorSetNoUpdate` | 0 | 0 | untested |
| `Entity.createParticle` | 1 | 1 | PASS |
| `Entity.deleteAlarm` | 0 | 0 | untested |
| `Entity.deleteAllParticles` | 0 | 0 | untested |
| `Entity.deleteLFO` | 1 | 1 | PASS |
| `Entity.die` | 1 | 1 | PASS |
| `Entity.disownVoice` | 1 | 1 | PASS |
| `Entity.drawExplosionIfGot` | 0 | 0 | untested |
| `Entity.forceAudibilityRecalc` | 1 | 1 | PASS |
| `Entity.getAlarm` | 0 | 0 | untested |
| `Entity.getAllParticles` | 1 | 1 | PASS |
| `Entity.getAudibilityPerc` | 0 | 0 | untested |
| `Entity.getCurrentScreenX` | 0 | 0 | untested |
| `Entity.getCurrentScreenY` | 0 | 0 | untested |
| `Entity.getDescriptionBits` | 1 | 1 | PASS |
| `Entity.getDistanceTo` | 1 | 1 | PASS |
| `Entity.getEntityCreationPos` | 0 | 0 | untested |
| `Entity.getLimitVolPerc` | 1 | 1 | PASS |
| `Entity.getMainBody` | 0 | 0 | untested |
| `Entity.getMaxCX` | 1 | 1 | PASS |
| `Entity.getMaxCY` | 1 | 1 | PASS |
| `Entity.getMinCX` | 1 | 1 | PASS |
| `Entity.getMinCY` | 1 | 1 | PASS |
| `Entity.getMixerID` | 1 | 1 | PASS |
| `Entity.getMixerParams` | 0 | 0 | untested |
| `Entity.getNumOffScreensAudible` | 1 | 1 | PASS |
| `Entity.getNumParticles` | 1 | 1 | PASS |
| `Entity.getObjectID` | 2 | 2 | PASS |
| `Entity.getPhysicsData` | 1 | 1 | PASS |
| `Entity.getRadiansToFace` | 1 | 1 | PASS |
| `Entity.getStyle` | 0 | 0 | untested |
| `Entity.getType` | 1 | 1 | PASS |
| `Entity.getTypeName` | 1 | 1 | PASS |
| `Entity.getUID` | 0 | 0 | untested |
| `Entity.getUniqueAlarmID` | 0 | 0 | untested |
| `Entity.getUniqueLFOID` | 0 | 0 | untested |
| `Entity.getVectorTo` | 2 | 2 | PASS |
| `Entity.getVectorToNormalized` | 2 | 2 | PASS |
| `Entity.getVertexShaderParamIndex` | 0 | 0 | untested |
| `Entity.getWhichSide` | 0 | 0 | untested |
| `Entity.giveBackToBulletCache` | 0 | 0 | untested |
| `Entity.initializeScreenExtents` | 4 | 4 | PASS |
| `Entity.isAlwaysMusicTriggered` | 0 | 0 | untested |
| `Entity.isAlwaysProcessed` | 0 | 0 | untested |
| `Entity.isBeingEdited` | 1 | 1 | PASS |
| `Entity.isBoundedToGameBoard` | 0 | 0 | untested |
| `Entity.isEditable` | 1 | 1 | PASS |
| `Entity.isEnabled` | 10 | 10 | PASS |
| `Entity.isGetting` | 1 | 1 | PASS |
| `Entity.isGot` | 3 | 3 | PASS |
| `Entity.isGround` | 0 | 0 | untested |
| `Entity.isInGameBoard` | 0 | 0 | untested |
| `Entity.isInView` | 0 | 0 | untested |
| `Entity.isInvincible` | 0 | 0 | untested |
| `Entity.isInvisible` | 2 | 2 | PASS |
| `Entity.isOkToRun` | 2 | 2 | PASS |
| `Entity.isOnCurrentScreen` | 0 | 0 | untested |
| `Entity.isOnScreen` | 2 | 2 | PASS |
| `Entity.isPlacing` | 1 | 1 | PASS |
| `Entity.isPlayer` | 0 | 0 | untested |
| `Entity.isPlayerRelated` | 0 | 0 | untested |
| `Entity.isSameType` | 1 | 1 | PASS |
| `Entity.isSauceBase` | 1 | 1 | PASS |
| `Entity.isStaticDraw` | 1 | 1 | PASS |
| `Entity.isTaggedForDeletion` | 1 | 1 | PASS |
| `Entity.isType` | 2 | 2 | PASS |
| `Entity.isWizard` | 0 | 0 | untested |
| `Entity.is_getAudibilityPerc_neverOnScreen` | 1 | 1 | PASS |
| `Entity.loadAnimation` | 0 | 0 | untested |
| `Entity.onDisable` | 3 | 3 | PASS |
| `Entity.onEnable` | 5 | 5 | PASS |
| `Entity.onMusicTrigger` | 0 | 0 | untested |
| `Entity.playInstrumentWithMixer` | 0 | 0 | untested |
| `Entity.playSound` | 0 | 0 | untested |
| `Entity.playSoundWithMixer` | 0 | 0 | untested |
| `Entity.recordScreenExtents` | 0 | 0 | untested |
| `Entity.redrawSauceBase` | 1 | 1 | PASS |
| `Entity.refreshFunctionCache` | 0 | 0 | untested |
| `Entity.setDescriptionBits` | 1 | 1 | PASS |
| `Entity.setEditSensorBox` | 0 | 0 | untested |
| `Entity.setEntityCreationPos` | 0 | 0 | untested |
| `Entity.setGot` | 2 | 2 | PASS |
| `Entity.setIsAlwaysMusicTriggered` | 0 | 0 | untested |
| `Entity.setIsAlwaysProcessed` | 1 | 1 | PASS |
| `Entity.setIsAutoRemove` | 0 | 0 | untested |
| `Entity.setIsAutoRemoveStranded` | 0 | 0 | untested |
| `Entity.setIsBeingEdited` | 2 | 2 | PASS |
| `Entity.setIsBoundedToGameBoard` | 0 | 0 | untested |
| `Entity.setIsDeletable` | 0 | 0 | untested |
| `Entity.setIsEditable` | 2 | 2 | PASS |
| `Entity.setIsEnabled` | 2 | 2 | PASS |
| `Entity.setIsInvisible` | 2 | 2 | PASS |
| `Entity.setIsPlacing` | 2 | 2 | PASS |
| `Entity.setIsSauceBase` | 2 | 2 | PASS |
| `Entity.setIsStaticDraw` | 2 | 2 | PASS |
| `Entity.setLimitVolPerc` | 1 | 1 | PASS |
| `Entity.setMixerID` | 1 | 1 | PASS |
| `Entity.setNumOffScreensAudible` | 1 | 1 | PASS |
| `Entity.setUIDData` | 0 | 0 | untested |
| `Entity.set_getAudibilityPerc_neverOnScreen` | 2 | 2 | PASS |
| `Entity.setupDrawList` | 0 | 0 | untested |
| `Entity.setupStaticDrawList` | 0 | 0 | untested |
| `Entity.tagForDeletion` | 4 | 4 | PASS |
| `Entity.unloadAnimation` | 0 | 0 | untested |
| `Entity.unloadAnimationAll` | 0 | 0 | untested |
| `Entity.update` | 2 | 2 | PASS |
| `Entity.updateExplosionIfGot` | 1 | 1 | PASS |
| `EntityManager.createEntity` | 2 | 2 | PASS |
| `EntityManager.createEntityAsPlayer` | 0 | 0 | untested |
| `EntityManager.deleteAllEntities` | 6 | 6 | PASS |
| `EntityManager.flush` | 18 | 18 | PASS |
| `EntityManager.getAllEntities` | 11 | 11 | PASS |
| `EntityManager.getAllManualEntities` | 1 | 1 | PASS |
| `EntityManager.getAllNonManualEntities` | 1 | 1 | PASS |
| `EntityManager.getAllPlayerEntities` | 1 | 1 | PASS |
| `EntityManager.getAllPlayerEntitiesRandomly` | 1 | 1 | PASS |
| `EntityManager.getDummyEntity` | 1 | 1 | PASS |
| `EntityManager.getEntitiesNear` | 7 | 7 | PASS |
| `EntityManager.getEntitiesRoughlyWithin` | 2 | 2 | PASS |
| `EntityManager.getEntity` | 5 | 5 | PASS |
| `EntityManager.getEntityFromUID` | 2 | 2 | PASS |
| `EntityManager.getEntityPerfCost` | 3 | 3 | PASS |
| `EntityManager.getEntityPerfMaxCost` | 1 | 1 | PASS |
| `EntityManager.getNumEntities` | 24 | 24 | PASS |
| `EntityManager.getNumManualEntities` | 1 | 1 | PASS |
| `EntityManager.getNumNonManualEntities` | 1 | 1 | PASS |
| `EntityManager.getNumPlayers` | 1 | 1 | PASS |
| `EntityManager.getRandomEntity` | 1 | 1 | PASS |
| `EntityManager.getRandomPlayer` | 1 | 1 | PASS |
| `EntityManager.isEntityIDValid` | 62 | 63 | AWAITING RETAIL |
| `EntityManager.setEntityPerfCost` | 2 | 2 | PASS |
| `EntityManager.setEntityPerfMaxCost` | 1 | 1 | PASS |
| `GameMonkeyScript.clean` | 2 | 2 | PASS |
| `GameMonkeyScript.isCollectingGarbage` | 3 | 4 | AWAITING RETAIL |
| `GameSystem.TemporaryStorageCopy` | 0 | 0 | untested |
| `GameSystem.TemporaryStoragePaste` | 0 | 0 | untested |
| `GameSystem.debug_isStopped` | 0 | 0 | untested |
| `GameSystem.debug_setStop` | 0 | 0 | untested |
| `GameSystem.flushProcesses` | 0 | 0 | untested |
| `GameSystem.getAllProcesses` | 0 | 0 | untested |
| `GameSystem.getAutoLoadArea` | 0 | 0 | untested |
| `GameSystem.getAutoLoadLevel` | 0 | 0 | untested |
| `GameSystem.getAutoLoadMode` | 0 | 0 | untested |
| `GameSystem.getAutoLoadPath` | 0 | 0 | untested |
| `GameSystem.getCurrentTick` | 0 | 0 | untested |
| `GameSystem.getEntityUpdateProcess` | 0 | 0 | untested |
| `GameSystem.getMSAlive` | 0 | 0 | untested |
| `GameSystem.getMSSincePlayStart` | 0 | 0 | untested |
| `GameSystem.getProcess` | 1 | 1 | PASS |
| `GameSystem.getProcessByIndex` | 0 | 0 | untested |
| `GameSystem.getProcessCount` | 4 | 4 | PASS |
| `GameSystem.getRandomAngle` | 0 | 0 | untested |
| `GameSystem.getRandomBoolean` | 0 | 0 | untested |
| `GameSystem.getRandomDir` | 0 | 0 | untested |
| `GameSystem.getRandomInteger` | 0 | 0 | untested |
| `GameSystem.getRandomReal` | 0 | 0 | untested |
| `GameSystem.getRandomRealExp` | 0 | 0 | untested |
| `GameSystem.getRandomSign` | 0 | 0 | untested |
| `GameSystem.getRandomSignAndReal` | 0 | 0 | untested |
| `GameSystem.getSimulationSpeed` | 2 | 2 | PASS |
| `GameSystem.getUpdateInterval` | 1 | 1 | PASS |
| `GameSystem.isAutoLoadLevel` | 0 | 0 | untested |
| `GameSystem.isBeingNice` | 0 | 0 | untested |
| `GameSystem.isLoading` | 0 | 0 | untested |
| `GameSystem.isPaused` | 7 | 7 | PASS |
| `GameSystem.isProcessRunning` | 6 | 6 | PASS |
| `GameSystem.isShouldUpdateEntities` | 2 | 2 | PASS |
| `GameSystem.openConsole` | 0 | 0 | untested |
| `GameSystem.quit` | 0 | 0 | untested |
| `GameSystem.resetPlayTimer` | 0 | 0 | untested |
| `GameSystem.resetSimulationTimer` | 0 | 0 | untested |
| `GameSystem.runProcess` | 15 | 15 | PASS |
| `GameSystem.setALevelLoaded` | 0 | 0 | untested |
| `GameSystem.setAsPauseProcess` | 4 | 4 | PASS |
| `GameSystem.setIsBeingNice` | 0 | 0 | untested |
| `GameSystem.setIsInputEnabled` | 0 | 0 | untested |
| `GameSystem.setIsLoading` | 0 | 0 | untested |
| `GameSystem.setIsShouldUpdateEntities` | 2 | 2 | PASS |
| `GameSystem.setSimulationSpeed` | 4 | 4 | PASS |
| `GameSystem.startNewProcessList` | 2 | 2 | PASS |
| `GameSystem.trace` | 0 | 0 | untested |
| `Global.PHYSICSSHAPECOLLECTIONDEF` | 1 | 1 | PASS |
| `Global.TRIANGLELFO` | 1 | 1 | PASS |
| `LFO.getValue` | 2 | 2 | PASS |
| `LFO.setContour` | 2 | 2 | PASS |
| `LFO.setPhase` | 8 | 8 | PASS |
| `LFO.setRandomPhase` | 0 | 0 | untested |
| `LFO.setRange` | 1 | 1 | PASS |
| `LFO.setRate` | 7 | 7 | PASS |
| `LFO.update` | 7 | 7 | PASS |
| `Physics.createJoint` | 6 | 6 | PASS |
| `Physics.deleteAllJoints` | 1 | 1 | PASS |
| `Physics.deleteJoint` | 1 | 1 | PASS |
| `Physics.flush` | 8 | 8 | PASS |
| `Physics.getBodyFromShapeID` | 1 | 1 | PASS |
| `Physics.getEntityFromShapeID` | 1 | 1 | PASS |
| `Physics.getJoint` | 2 | 2 | PASS |
| `Physics.getNumFramesToIgnoreCollisions` | 3 | 3 | PASS |
| `Physics.getShapeByID` | 1 | 1 | PASS |
| `Physics.getShapesNear` | 2 | 2 | PASS |
| `Physics.getShapesRoughlyWithin` | 2 | 2 | PASS |
| `Physics.getShapesWithin` | 1 | 1 | PASS |
| `Physics.isShapeSensor` | 2 | 2 | PASS |
| `Physics.raycast` | 104 | 104 | PASS |
| `Physics.raycastOne` | 264 | 264 | PASS |
| `Physics.raycastShapes` | 2 | 2 | PASS |
| `Physics.raycastSimple` | 52 | 52 | PASS |
| `Physics.setNumFramesToIgnoreCollisions` | 2 | 2 | PASS |
| `Process.addAlarm` | 0 | 0 | untested |
| `Process.addLFO` | 0 | 0 | untested |
| `Process.callMethod` | 0 | 0 | untested |
| `Process.configure` | 0 | 0 | untested |
| `Process.deleteAlarm` | 0 | 0 | untested |
| `Process.deleteLFO` | 0 | 0 | untested |
| `Process.drawGrounds` | 0 | 0 | untested |
| `Process.getAlarm` | 0 | 0 | untested |
| `Process.getCurrentScaleDescription` | 0 | 0 | untested |
| `Process.getCurrentScreenX` | 0 | 0 | untested |
| `Process.getCurrentScreenY` | 0 | 0 | untested |
| `Process.getDrawList` | 0 | 0 | untested |
| `Process.getFireThickness` | 0 | 0 | untested |
| `Process.getGroundCountOnScreen` | 0 | 0 | untested |
| `Process.getName` | 0 | 0 | untested |
| `Process.getNoteWithPositionAndOffset` | 0 | 0 | untested |
| `Process.getObjectID` | 0 | 0 | untested |
| `Process.getPostDrawList` | 0 | 0 | untested |
| `Process.getPreDrawList` | 0 | 0 | untested |
| `Process.getScaleIndex` | 0 | 0 | untested |
| `Process.getScreenHeight` | 0 | 0 | untested |
| `Process.getScreenWidth` | 0 | 0 | untested |
| `Process.getScreenX` | 0 | 0 | untested |
| `Process.getScreenY` | 0 | 0 | untested |
| `Process.getShader` | 0 | 0 | untested |
| `Process.getTable` | 0 | 0 | untested |
| `Process.getTranspose` | 0 | 0 | untested |
| `Process.getUniqueAlarmID` | 0 | 0 | untested |
| `Process.getUniqueLFOID` | 0 | 0 | untested |
| `Process.isBeingEdited` | 0 | 0 | untested |
| `Process.isHasErrors` | 0 | 0 | untested |
| `Process.isInited` | 0 | 0 | untested |
| `Process.isLoading` | 0 | 0 | untested |
| `Process.lastScale` | 0 | 0 | untested |
| `Process.nextScale` | 0 | 0 | untested |
| `Process.notifyEvent` | 0 | 0 | untested |
| `Process.onMusicTrigger` | 0 | 0 | untested |
| `Process.refreshFunctionCache` | 0 | 0 | untested |
| `Process.registerEntity` | 0 | 0 | untested |
| `Process.rename` | 0 | 0 | untested |
| `Process.setAsPauseProcess` | 0 | 0 | untested |
| `Process.setCurrentScreenX` | 0 | 0 | untested |
| `Process.setCurrentScreenY` | 0 | 0 | untested |
| `Process.setFireThickness` | 0 | 0 | untested |
| `Process.setGlobalVolume` | 0 | 0 | untested |
| `Process.setIsRefreshDrawListOnChange` | 0 | 0 | untested |
| `Process.setIsStealsInputFocus` | 0 | 0 | untested |
| `Process.setIsUpdatesEntities` | 0 | 0 | untested |
| `Process.setScaleIndex` | 0 | 0 | untested |
| `Process.setTranspose` | 0 | 0 | untested |
| `Process.setupDrawList` | 0 | 0 | untested |
| `Process.setupDrawLists` | 0 | 0 | untested |
| `Process.setupPostDrawList` | 0 | 0 | untested |
| `Process.setupPreDrawList` | 0 | 0 | untested |
| `Process.terminate` | 5 | 5 | PASS |
| `Process.unregisterEntity` | 0 | 0 | untested |
| `collisionInfo.getCollidingShapeIDList` | 0 | 0 | untested |
| `collisionInfo.getContactNormalOfMyCollidingShape` | 112 | 112 | PASS |
| `collisionInfo.getEntity` | 56 | 56 | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | 56 | 56 | PASS |
| `collisionInfo.getNumCollidingShapes` | 0 | 0 | untested |
| `collisionInfo.getShapeCollisionInfo` | 112 | 112 | PASS |
| `collisionInfo.isAllSensors` | 56 | 56 | PASS |
| `collisionInfo.isBodyColliding` | 56 | 56 | PASS |
| `collisionInfo.isBodyCollidingWithBody` | 56 | 56 | PASS |
| `collisionInfo.isBodyCollidingWithShape` | 56 | 56 | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | 56 | 56 | PASS |
| `collisionInfo.isShapeColliding` | 56 | 56 | PASS |
| `intIter.get` | 56 | 56 | PASS |
| `intIter.goToNext` | 0 | 0 | untested |
| `intIter.isValid` | 0 | 0 | untested |
| `intIter.reset` | 0 | 0 | untested |
| `intIter.size` | 56 | 56 | PASS |
| `jointIter.get` | 5 | 5 | PASS |
| `jointIter.goToNext` | 5 | 5 | PASS |
| `jointIter.isValid` | 1 | 1 | PASS |
| `jointIter.size` | 1 | 1 | PASS |
| `jointType.getAnchorPos` | 120 | 120 | PASS |
| `jointType.getBody1` | 60 | 60 | PASS |
| `jointType.getBody2` | 48 | 48 | PASS |
| `jointType.getEntity1` | 60 | 60 | PASS |
| `jointType.getEntity2` | 48 | 48 | PASS |
| `jointType.getObjectID` | 1 | 1 | PASS |
| `jointType.getRTMotorSpeed` | 13 | 13 | PASS |
| `jointType.getReactionForce` | 120 | 120 | PASS |
| `jointType.getReactionTorque` | 60 | 60 | PASS |
| `jointType.getType` | 60 | 60 | PASS |
| `jointType.isBodiesCollidable` | 62 | 62 | PASS |
| `jointType.setAngularSpring` | 0 | 0 | untested |
| `jointType.setIsBodiesCollidable` | 2 | 2 | PASS |
| `jointType.setIsNeedsHighAccuracy` | 0 | 0 | untested |
| `jointType.setLinearSpring` | 0 | 0 | untested |
| `jointType.setMotor` | 4 | 4 | PASS |
| `jointType.setPinAngularConstraint` | 0 | 0 | untested |
| `jointType.setPinMotor` | 1 | 1 | PASS |
| `jointType.setPrismaticTranslation` | 1 | 1 | PASS |
| `jointType.setPrismaticWorldAxis` | 1 | 1 | PASS |
| `jointType.setRTMaxMotorTorque` | 1 | 1 | PASS |
| `jointType.setRTMotorSpeed` | 3 | 3 | PASS |
| `jointType.setRelativeVel` | 1 | 1 | PASS |
| `jointType.setRevoluteAngularConstraint` | 0 | 0 | untested |
| `jointType.setRevoluteMotor` | 4 | 4 | PASS |
| `jointType.setupDistanceJoint` | 1 | 1 | PASS |
| `jointType.setupJoint` | 2 | 2 | PASS |
| `jointType.setupPinJoint` | 1 | 1 | PASS |
| `jointType.setupRelativeJoint` | 1 | 1 | PASS |
| `physicsBody.appendShapes` | 2 | 2 | PASS |
| `physicsBody.applyForce` | 8 | 8 | PASS |
| `physicsBody.brake` | 288 | 288 | PASS |
| `physicsBody.computeAngularVel` | 0 | 0 | untested |
| `physicsBody.disableRestitutionThisFrame` | 0 | 0 | untested |
| `physicsBody.doesRotationResponse` | 1 | 1 | PASS |
| `physicsBody.dummy` | 0 | 0 | untested |
| `physicsBody.getAABB` | 8 | 8 | PASS |
| `physicsBody.getAngularVel` | 1 | 1 | PASS |
| `physicsBody.getBBox` | 0 | 0 | untested |
| `physicsBody.getBrakeAcc` | 1 | 1 | PASS |
| `physicsBody.getCategoriesToCollideWith` | 75 | 75 | PASS |
| `physicsBody.getCenterOfMass` | 1 | 1 | PASS |
| `physicsBody.getCollisionCategory` | 60 | 60 | PASS |
| `physicsBody.getCollisionGroup` | 58 | 58 | PASS |
| `physicsBody.getDir` | 323 | 323 | PASS |
| `physicsBody.getDirRef` | 0 | 0 | untested |
| `physicsBody.getDirty` | 0 | 0 | untested |
| `physicsBody.getEntity` | 2 | 2 | PASS |
| `physicsBody.getFriction` | 1 | 1 | PASS |
| `physicsBody.getHeight` | 1 | 1 | PASS |
| `physicsBody.getMass` | 1 | 1 | PASS |
| `physicsBody.getMaxPos` | 1 | 1 | PASS |
| `physicsBody.getMaxRotateSpeed` | 1 | 1 | PASS |
| `physicsBody.getMaxSpeed` | 1 | 1 | PASS |
| `physicsBody.getMinPos` | 1 | 1 | PASS |
| `physicsBody.getNumShapeVerts` | 1 | 1 | PASS |
| `physicsBody.getNumShapes` | 4 | 4 | PASS |
| `physicsBody.getObjectID` | 2 | 2 | PASS |
| `physicsBody.getPos` | 946 | 946 | PASS |
| `physicsBody.getPosRef` | 1 | 1 | PASS |
| `physicsBody.getPosVec` | 1 | 1 | PASS |
| `physicsBody.getRadius` | 2 | 2 | PASS |
| `physicsBody.getRestitution` | 1 | 1 | PASS |
| `physicsBody.getRotateAcc` | 1 | 1 | PASS |
| `physicsBody.getRotateVel` | 162 | 162 | PASS |
| `physicsBody.getShape` | 0 | 0 | untested |
| `physicsBody.getShapeCategoriesToCollideWith` | 3 | 5 | AWAITING RETAIL |
| `physicsBody.getShapeHeight` | 2 | 2 | PASS |
| `physicsBody.getShapeIndex` | 1 | 1 | PASS |
| `physicsBody.getShapeObjectID` | 2 | 2 | PASS |
| `physicsBody.getShapeOffset` | 1 | 1 | PASS |
| `physicsBody.getShapeRadius` | 2 | 2 | PASS |
| `physicsBody.getShapeType` | 2 | 2 | PASS |
| `physicsBody.getShapeVert` | 0 | 0 | untested |
| `physicsBody.getShapeVerts` | 0 | 0 | untested |
| `physicsBody.getShapeWidth` | 2 | 2 | PASS |
| `physicsBody.getShapeWorldCenterPos` | 1 | 1 | PASS |
| `physicsBody.getShapeWorldVert` | 1 | 1 | PASS |
| `physicsBody.getSpeed` | 163 | 163 | PASS |
| `physicsBody.getThrustAcc` | 1 | 1 | PASS |
| `physicsBody.getVel` | 802 | 802 | PASS |
| `physicsBody.getVelRef` | 1 | 1 | PASS |
| `physicsBody.getWidth` | 1 | 1 | PASS |
| `physicsBody.getWorldDir` | 1 | 1 | PASS |
| `physicsBody.getWorldPos` | 1 | 1 | PASS |
| `physicsBody.isBullet` | 61 | 61 | PASS |
| `physicsBody.isCircle` | 0 | 1 | LOCAL-ONLY |
| `physicsBody.isCollidable` | 56 | 56 | PASS |
| `physicsBody.isContainsShape` | 1 | 1 | PASS |
| `physicsBody.isPointWithin` | 4 | 4 | PASS |
| `physicsBody.isPointWithinShape` | 1 | 1 | PASS |
| `physicsBody.isRect` | 0 | 1 | LOCAL-ONLY |
| `physicsBody.isSensor` | 8 | 8 | PASS |
| `physicsBody.isShapeSensor` | 1 | 1 | PASS |
| `physicsBody.isSleeping` | 161 | 162 | AWAITING RETAIL |
| `physicsBody.putToSleep` | 0 | 0 | untested |
| `physicsBody.raycast` | 12 | 12 | PASS |
| `physicsBody.raycastShapesOne` | 9 | 9 | PASS |
| `physicsBody.rebuildBodyMass` | 0 | 0 | untested |
| `physicsBody.removeShape` | 0 | 0 | untested |
| `physicsBody.reverseThrust` | 288 | 288 | PASS |
| `physicsBody.rotate` | 0 | 0 | untested |
| `physicsBody.rotateLeft` | 0 | 0 | untested |
| `physicsBody.rotateRight` | 0 | 0 | untested |
| `physicsBody.rotateTo` | 288 | 288 | PASS |
| `physicsBody.setAllowSleeping` | 288 | 288 | PASS |
| `physicsBody.setAngularVel` | 2 | 2 | PASS |
| `physicsBody.setAsCircle` | 0 | 0 | untested |
| `physicsBody.setAsRect` | 0 | 0 | untested |
| `physicsBody.setBrakeAcc` | 288 | 288 | PASS |
| `physicsBody.setCategoriesToCollideWith` | 1 | 1 | PASS |
| `physicsBody.setCollisionCategory` | 1 | 1 | PASS |
| `physicsBody.setCollisionGroup` | 1 | 1 | PASS |
| `physicsBody.setDir` | 2 | 2 | PASS |
| `physicsBody.setDoesRotationResponse` | 0 | 0 | untested |
| `physicsBody.setEdgeShapeVertex` | 0 | 0 | untested |
| `physicsBody.setFriction` | 1 | 1 | PASS |
| `physicsBody.setIsBullet` | 1 | 1 | PASS |
| `physicsBody.setIsCollidable` | 1 | 1 | PASS |
| `physicsBody.setIsSensor` | 1 | 1 | PASS |
| `physicsBody.setIsShapeSensor` | 8 | 8 | PASS |
| `physicsBody.setIsUsingContinuousPhysics` | 0 | 0 | untested |
| `physicsBody.setMass` | 1 | 1 | PASS |
| `physicsBody.setMaxRotateSpeed` | 289 | 289 | PASS |
| `physicsBody.setMaxRotationDecayRate` | 0 | 0 | untested |
| `physicsBody.setMaxSpeed` | 289 | 289 | PASS |
| `physicsBody.setMaxVelocityDecayRate` | 0 | 0 | untested |
| `physicsBody.setPolygonShapeVertices` | 0 | 0 | untested |
| `physicsBody.setPos` | 1 | 1 | PASS |
| `physicsBody.setRestitution` | 1 | 1 | PASS |
| `physicsBody.setRotateAcc` | 288 | 288 | PASS |
| `physicsBody.setRotateVel` | 1 | 1 | PASS |
| `physicsBody.setRotationDecayRate` | 0 | 0 | untested |
| `physicsBody.setShapeCategoriesToCollideWith` | 2 | 4 | AWAITING RETAIL |
| `physicsBody.setShapeDensity` | 8 | 8 | PASS |
| `physicsBody.setSpeed` | 1 | 1 | PASS |
| `physicsBody.setThrustAcc` | 288 | 288 | PASS |
| `physicsBody.setVel` | 292 | 292 | PASS |
| `physicsBody.setVelocityDecayRate` | 0 | 0 | untested |
| `physicsBody.stopRotatingTo` | 288 | 288 | PASS |
| `physicsBody.thrust` | 288 | 288 | PASS |
| `physicsData.appendShapes` | 5 | 5 | PASS |
| `physicsData.brake` | 0 | 0 | untested |
| `physicsData.createBody` | 4 | 4 | PASS |
| `physicsData.doesRotationResponse` | 1 | 1 | PASS |
| `physicsData.getAABB` | 8 | 8 | PASS |
| `physicsData.getBody` | 1 | 1 | PASS |
| `physicsData.getBrakeAcc` | 1 | 1 | PASS |
| `physicsData.getCategoriesToCollideWith` | 1 | 1 | PASS |
| `physicsData.getCollidingEntities` | 0 | 0 | untested |
| `physicsData.getCollisionCategory` | 1 | 1 | PASS |
| `physicsData.getCollisionGroup` | 1 | 1 | PASS |
| `physicsData.getCollisionInfo` | 56 | 56 | PASS |
| `physicsData.getDir` | 0 | 0 | untested |
| `physicsData.getHeight` | 1 | 1 | PASS |
| `physicsData.getJointsOnMe` | 5 | 5 | PASS |
| `physicsData.getMass` | 1 | 1 | PASS |
| `physicsData.getMaxPos` | 1 | 1 | PASS |
| `physicsData.getMaxRotateSpeed` | 1 | 1 | PASS |
| `physicsData.getMaxSpeed` | 1 | 1 | PASS |
| `physicsData.getMinPos` | 1 | 1 | PASS |
| `physicsData.getNumBodies` | 26 | 26 | PASS |
| `physicsData.getPos` | 1 | 1 | PASS |
| `physicsData.getPosRef` | 1 | 1 | PASS |
| `physicsData.getRadius` | 1 | 1 | PASS |
| `physicsData.getRotateAcc` | 1 | 1 | PASS |
| `physicsData.getShapeByID` | 1 | 1 | PASS |
| `physicsData.getShapeRadius` | 1 | 1 | PASS |
| `physicsData.getSpeed` | 1 | 1 | PASS |
| `physicsData.getThrustAcc` | 1 | 1 | PASS |
| `physicsData.getType` | 1 | 1 | PASS |
| `physicsData.getVel` | 1 | 1 | PASS |
| `physicsData.getWidth` | 1 | 1 | PASS |
| `physicsData.isCircle` | 1 | 1 | PASS |
| `physicsData.isCollidable` | 1 | 1 | PASS |
| `physicsData.isColliding` | 290 | 290 | PASS |
| `physicsData.isPointWithin` | 1 | 1 | PASS |
| `physicsData.isRect` | 1 | 1 | PASS |
| `physicsData.isSensor` | 8 | 8 | PASS |
| `physicsData.isSleeping` | 0 | 0 | untested |
| `physicsData.putToSleep` | 288 | 288 | PASS |
| `physicsData.raycast` | 6 | 6 | PASS |
| `physicsData.raycastShapesOne` | 6 | 6 | PASS |
| `physicsData.removeBody` | 4 | 4 | PASS |
| `physicsData.replaceBody` | 6 | 6 | PASS |
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
| `physicsData.wakeUp` | 288 | 288 | PASS |
| `physicsShape.dummy` | 0 | 0 | untested |
| `physicsShape.getIsSensor` | 0 | 1 | LOCAL-ONLY |
| `physicsShape.getType` | 1 | 1 | PASS |
| `physicsShape.getVertexCount` | 1 | 1 | PASS |
| `physicsShape.getVertexWorldPos` | 1 | 1 | PASS |
| `physicsShape.isSensor` | 1 | 1 | PASS |
| `physicsShape.setCategoriesToCollideWith` | 1 | 1 | PASS |
| `physicsShape.setCollisionCategory` | 1 | 1 | PASS |
| `physicsShape.setFrictionForPlayer` | 1 | 1 | PASS |
| `physicsShape.setIsSensor` | 1 | 1 | PASS |
| `shapeCollisionInfo.disableContact` | 0 | 0 | untested |
| `shapeCollisionInfo.getContactNormal` | 112 | 112 | PASS |
| `shapeCollisionInfo.getContactPos` | 112 | 112 | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | 112 | 112 | PASS |
| `shapeCollisionInfo.getNumContacts` | 56 | 56 | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | 56 | 56 | PASS |
| `shapeDef.addBox` | 1 | 1 | PASS |
| `shapeDef.addCircle` | 1 | 1 | PASS |
| `shapeDef.addEdges` | 1 | 1 | PASS |
| `shapeDef.addPolygon` | 1 | 1 | PASS |
| `shapeDef.getCategoriesToCollideWith` | 1 | 1 | PASS |
| `shapeDef.getCollisionCategory` | 2 | 2 | PASS |
| `shapeDef.getFriction` | 1 | 1 | PASS |
| `shapeDef.getMass` | 2 | 2 | PASS |
| `shapeDef.getRestitution` | 1 | 1 | PASS |
| `shapeDef.isSensor` | 3 | 3 | PASS |
| `shapeDef.reset` | 1 | 1 | PASS |
| `shapeDef.setCategoriesToCollideWith` | 1 | 1 | PASS |
| `shapeDef.setCollisionCategory` | 1 | 1 | PASS |
| `shapeDef.setFriction` | 1 | 1 | PASS |
| `shapeDef.setFrictionForPlayer` | 1 | 1 | PASS |
| `shapeDef.setIsSensor` | 1 | 1 | PASS |
| `shapeDef.setMass` | 1 | 1 | PASS |
| `shapeDef.setRestitution` | 1 | 1 | PASS |

## Contact callbacks

| Hook | Matched | Cases |
| --- | ---: | ---: |
| `Contact callback argument` | 50 | 50 |
| `Contact callback order` | 25 | 25 |
| `Entity callbacks` | 32 | 32 |
| `Entity.onEntityCollide` | 64 | 64 |
| `Entity.onEntityCollidePreSolve` | 64 | 64 |
| `Entity.onEntitySeparate` | 64 | 64 |
| `Entity.onEntityTouch` | 64 | 64 |
| `EntityListRef.get` | 7 | 7 |
| `EntityListRef.goToNext` | 5 | 5 |
| `EntityListRef.isValid` | 1 | 1 |
| `EntityListRef.size` | 8 | 8 |

## Inconclusive probes

| Function | Case | Retail | OpenEgg | Reason |
| --- | --- | --- | --- | --- |
| `Entity.getUID` | entityQ.id.managerUID | `null` | `91001` | The lightweight retail test entity lacks EntityBase's UID access setup. UID registration and lookup are tested separately. |
| `Entity.addLFO` | entityState.addLFO | `0` | `1` | The synthetic entity fixture does not expose the retail native LFO owner; game scripts attach LFOs to fully initialized retail entities. |
| `Entity.isAlwaysProcessed` | entityState.alwaysProcessed | `null` | `1` | Synthetic entity fixture exposes the setter but no retail getter state for this property. |
| `Entity.addAlarm` | entityState.attachedAlarmCreated | `0` | `1` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.deleteAlarm` | entityState.attachedAlarmDeleted | `1` | `1` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `ALARM.getMSElapsed` | entityState.attachedAlarmElapsed | `fixture-limited` | `25.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.getAlarm` | entityState.attachedAlarmFound | `0` | `1` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `ALARM.getInterval` | entityState.attachedAlarmInterval | `fixture-limited` | `100.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.callMethod` | entityState.callMethod | `null` | `12` | Retail returns null for a declared fixture method, even after refreshFunctionCache; real EntityBase dispatch remains unverified. |
| `Entity.callMethod` | entityState.callMethodBehavior | `null` | `12` | Retail returns null for a declared fixture method; behavior-table dispatch needs a retail EntityBase-backed probe. |
| `Entity.callMethod` | entityState.callMethodMultiple | `null` | `34` | Retail returns null for a declared fixture method; argument forwarding needs a retail EntityBase-backed probe. |
| `Entity.getEntityCreationPos` | entityState.creationPosX | `null` | `12.500000` | Synthetic entity fixture has no retail backing entity state for the setter/getter round trip. |
| `Entity.getEntityCreationPos` | entityState.creationPosY | `null` | `-4.250000` | Synthetic entity fixture has no retail backing entity state for the setter/getter round trip. |
| `Entity.addAlarm` | entityState.hasAddAlarm | `0` | `1` | GameMonkey's table-member existence operator does not report engine type-library methods on the retail entity object. |
| `Entity.addLFO` | entityState.hasAddLFO | `0` | `1` | GameMonkey's table-member existence operator does not report engine type-library methods on the retail entity object. |
| `Entity.isAlwaysProcessed` | entityState.hasAlwaysProcessedGetter | `0` | `1` | GameMonkey's table-member existence operator does not report engine type-library methods on the retail entity object. |
| `Entity.callMethod` | entityState.hasCallMethod | `0` | `1` | GameMonkey's table-member existence operator cannot report engine type-library methods. |
| `Entity.getEntityCreationPos` | entityState.hasCreationPosGetter | `0` | `1` | GameMonkey's table-member existence operator does not report engine type-library methods on the retail entity object. |
| `Entity.getWhichSide` | entityState.hasWhichSide | `0` | `1` | GameMonkey's table-member existence operator cannot report engine type-library methods. |
| `Entity.getMainBody` | entityState.mainBody | `fixture-limited` | `fixture-limited` | Retail probe entity does not expose initialized PhysicsData to the script helper; direct calls fault before reporting a comparable result. |
| `LFO.update` | entityState.ownedLfoAfterHalf | `fixture-limited` | `0.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `LFO.update` | entityState.ownedLfoAfterQuarter | `fixture-limited` | `-1.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.deleteLFO` | entityState.ownedLfoHandleAfterDelete | `fixture-limited` | `0.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.getUniqueLFOID` | entityState.ownedLfoIDsDistinct | `0` | `1` | The synthetic entity fixture does not provide the retail-native LFO ID allocator. |
| `LFO.getValue` | entityState.ownedLfoInitial | `fixture-limited` | `-2.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.addLFO` | entityState.ownedLfoReplacementInitial | `fixture-limited` | `0.000000` | The synthetic entity fixture has no retail-backed native owner for this API. |
| `Entity.getWhichSide` | entityState.whichSide | `null` | `1` | Retail returns null for this synthetic entity pair; native call setup remains unresolved. |
| `Entity.getWhichSide` | entityState.whichSideBehind | `null` | `1` | Retail returns null for this synthetic entity pair; native call setup remains unresolved. |
| `Entity.getWhichSide` | entityState.whichSideNegative | `null` | `0` | Retail returns null for this synthetic entity pair; native call setup remains unresolved. |

## OpenEgg-only functions

These extensions have no corresponding retail method and are excluded from the parity score.

| Function | Case | Retail | OpenEgg | Reason |
| --- | --- | --- | --- | --- |
| `physicsBody.isCircle` | bodyApi.physicsBody.isCircle | `null` | `1` | OpenEgg convenience method; no retail method exists |
| `physicsBody.isRect` | bodyApi.physicsBody.isRect | `null` | `0` | OpenEgg convenience method; no retail method exists |
| `physicsShape.getIsSensor` | bodyApi.physicsShape.getIsSensor | `null` | `1` | OpenEgg alias; retail exposes isSensor |

## Awaiting retail capture

These cases ran on OpenEgg but lack comparable retail readings; excluded from parity scores.

| Function | Case | OpenEgg | Note |
| --- | --- | --- | --- |
| `physicsBody.isSleeping` | entityGC.detachedBodySleeping | `1.000000` | RPCS3 faults after owner deletion when retained PhysicsBody.isSleeping() is called; later readings are not reached. |
| `GameMonkeyScript.isCollectingGarbage` | entityGC.collectingAfterReleaseGC | `0` | No retail reading captured. |
| `EntityManager.isEntityIDValid` | entityGC.entityStillDeleted | `1` | No retail reading captured. |
| `physicsBody.setShapeCategoriesToCollideWith` | bodyCtl.shapeMaskInvalid | `5` | A separate retail capture faults at 0x00091c40 while reading 0x28; this combined probe stops earlier at the getter fault. |
| `physicsBody.getShapeCategoriesToCollideWith` | bodyCtl.shapeMaskGetterInvalid | `-1` | Invalid shape index faults at 0x00091320 while reading 0x3e. |

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
| `physicsBody.getPos` | scene.free.t12.x | `-80.000031` | `-80.000031` | PASS |
| `physicsBody.getPos` | scene.free.t12.y | `110.000031` | `110.000031` | PASS |
| `physicsBody.getVel` | scene.free.t12.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t12.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t12.x | `-99.999954` | `-99.999954` | PASS |
| `physicsBody.getPos` | scene.fast.t12.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t12.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t12.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t12.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t12.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t12.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t12.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t12.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t12.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t12.x | `69.999992` | `69.999992` | PASS |
| `physicsBody.getPos` | scene.sensor.t12.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t12.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t12.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t12.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t13.x | `-78.333366` | `-78.333366` | PASS |
| `physicsBody.getPos` | scene.free.t13.y | `110.833366` | `110.833366` | PASS |
| `physicsBody.getVel` | scene.free.t13.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t13.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t13.x | `-66.666626` | `-66.666626` | PASS |
| `physicsBody.getPos` | scene.fast.t13.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t13.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t13.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t13.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t13.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t13.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t13.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t13.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t13.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t13.x | `73.333328` | `73.333328` | PASS |
| `physicsBody.getPos` | scene.sensor.t13.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t13.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t13.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t13.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t14.x | `-76.666702` | `-76.666702` | PASS |
| `physicsBody.getPos` | scene.free.t14.y | `111.666702` | `111.666702` | PASS |
| `physicsBody.getVel` | scene.free.t14.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t14.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t14.x | `-33.333294` | `-33.333294` | PASS |
| `physicsBody.getPos` | scene.fast.t14.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t14.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t14.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t14.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t14.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t14.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t14.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t14.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t14.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t14.x | `76.666664` | `76.666664` | PASS |
| `physicsBody.getPos` | scene.sensor.t14.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t14.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t14.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t14.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t15.x | `-75.000038` | `-75.000038` | PASS |
| `physicsBody.getPos` | scene.free.t15.y | `112.500038` | `112.500038` | PASS |
| `physicsBody.getVel` | scene.free.t15.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t15.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t15.x | `0.000038` | `0.000038` | PASS |
| `physicsBody.getPos` | scene.fast.t15.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t15.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t15.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t15.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t15.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t15.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t15.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t15.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t15.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t15.x | `80.000000` | `80.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t15.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t15.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t15.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t15.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t16.x | `-73.333374` | `-73.333374` | PASS |
| `physicsBody.getPos` | scene.free.t16.y | `113.333374` | `113.333374` | PASS |
| `physicsBody.getVel` | scene.free.t16.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t16.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t16.x | `33.333370` | `33.333370` | PASS |
| `physicsBody.getPos` | scene.fast.t16.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t16.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t16.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t16.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t16.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t16.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t16.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t16.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t16.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t16.x | `83.333336` | `83.333336` | PASS |
| `physicsBody.getPos` | scene.sensor.t16.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t16.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t16.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t16.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t17.x | `-71.666710` | `-71.666710` | PASS |
| `physicsBody.getPos` | scene.free.t17.y | `114.166710` | `114.166710` | PASS |
| `physicsBody.getVel` | scene.free.t17.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t17.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t17.x | `66.666702` | `66.666702` | PASS |
| `physicsBody.getPos` | scene.fast.t17.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t17.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t17.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t17.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t17.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t17.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t17.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t17.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t17.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t17.x | `86.666672` | `86.666672` | PASS |
| `physicsBody.getPos` | scene.sensor.t17.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t17.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t17.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t17.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t18.x | `-70.000046` | `-70.000046` | PASS |
| `physicsBody.getPos` | scene.free.t18.y | `115.000046` | `115.000046` | PASS |
| `physicsBody.getVel` | scene.free.t18.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t18.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t18.x | `100.000031` | `100.000031` | PASS |
| `physicsBody.getPos` | scene.fast.t18.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t18.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t18.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t18.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t18.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t18.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t18.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t18.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t18.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t18.x | `90.000008` | `90.000008` | PASS |
| `physicsBody.getPos` | scene.sensor.t18.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t18.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t18.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t18.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t19.x | `-68.333382` | `-68.333382` | PASS |
| `physicsBody.getPos` | scene.free.t19.y | `115.833382` | `115.833382` | PASS |
| `physicsBody.getVel` | scene.free.t19.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t19.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t19.x | `133.333359` | `133.333359` | PASS |
| `physicsBody.getPos` | scene.fast.t19.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t19.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t19.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t19.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t19.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t19.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t19.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t19.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t19.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t19.x | `93.333344` | `93.333344` | PASS |
| `physicsBody.getPos` | scene.sensor.t19.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t19.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t19.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t19.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t20.x | `-66.666718` | `-66.666718` | PASS |
| `physicsBody.getPos` | scene.free.t20.y | `116.666718` | `116.666718` | PASS |
| `physicsBody.getVel` | scene.free.t20.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t20.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t20.x | `166.666687` | `166.666687` | PASS |
| `physicsBody.getPos` | scene.fast.t20.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t20.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t20.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t20.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t20.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t20.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t20.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t20.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t20.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t20.x | `96.666679` | `96.666679` | PASS |
| `physicsBody.getPos` | scene.sensor.t20.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t20.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t20.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t20.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t21.x | `-65.000053` | `-65.000053` | PASS |
| `physicsBody.getPos` | scene.free.t21.y | `117.500053` | `117.500053` | PASS |
| `physicsBody.getVel` | scene.free.t21.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t21.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t21.x | `200.000015` | `200.000015` | PASS |
| `physicsBody.getPos` | scene.fast.t21.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t21.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t21.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t21.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t21.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t21.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t21.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t21.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t21.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t21.x | `100.000015` | `100.000015` | PASS |
| `physicsBody.getPos` | scene.sensor.t21.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t21.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t21.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t21.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t22.x | `-63.333385` | `-63.333385` | PASS |
| `physicsBody.getPos` | scene.free.t22.y | `118.333389` | `118.333389` | PASS |
| `physicsBody.getVel` | scene.free.t22.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t22.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t22.x | `233.333344` | `233.333344` | PASS |
| `physicsBody.getPos` | scene.fast.t22.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t22.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t22.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t22.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t22.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t22.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t22.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t22.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t22.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t22.x | `103.333351` | `103.333351` | PASS |
| `physicsBody.getPos` | scene.sensor.t22.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t22.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t22.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t22.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t23.x | `-61.666718` | `-61.666718` | PASS |
| `physicsBody.getPos` | scene.free.t23.y | `119.166725` | `119.166725` | PASS |
| `physicsBody.getVel` | scene.free.t23.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t23.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t23.x | `266.666687` | `266.666687` | PASS |
| `physicsBody.getPos` | scene.fast.t23.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t23.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t23.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t23.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t23.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t23.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t23.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t23.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t23.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t23.x | `106.666687` | `106.666687` | PASS |
| `physicsBody.getPos` | scene.sensor.t23.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t23.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t23.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t23.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t24.x | `-60.000050` | `-60.000050` | PASS |
| `physicsBody.getPos` | scene.free.t24.y | `120.000061` | `120.000061` | PASS |
| `physicsBody.getVel` | scene.free.t24.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t24.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t24.x | `300.000031` | `300.000031` | PASS |
| `physicsBody.getPos` | scene.fast.t24.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t24.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t24.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t24.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t24.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t24.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t24.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t24.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t24.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t24.x | `110.000023` | `110.000023` | PASS |
| `physicsBody.getPos` | scene.sensor.t24.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t24.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t24.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t24.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t25.x | `-58.333382` | `-58.333382` | PASS |
| `physicsBody.getPos` | scene.free.t25.y | `120.833397` | `120.833397` | PASS |
| `physicsBody.getVel` | scene.free.t25.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t25.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t25.x | `333.333374` | `333.333374` | PASS |
| `physicsBody.getPos` | scene.fast.t25.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t25.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t25.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t25.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t25.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t25.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t25.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t25.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t25.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t25.x | `113.333359` | `113.333359` | PASS |
| `physicsBody.getPos` | scene.sensor.t25.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t25.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t25.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t25.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t26.x | `-56.666714` | `-56.666714` | PASS |
| `physicsBody.getPos` | scene.free.t26.y | `121.666733` | `121.666733` | PASS |
| `physicsBody.getVel` | scene.free.t26.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t26.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t26.x | `366.666718` | `366.666718` | PASS |
| `physicsBody.getPos` | scene.fast.t26.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t26.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t26.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t26.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t26.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t26.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t26.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t26.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t26.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t26.x | `116.666695` | `116.666695` | PASS |
| `physicsBody.getPos` | scene.sensor.t26.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t26.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t26.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t26.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t27.x | `-55.000046` | `-55.000046` | PASS |
| `physicsBody.getPos` | scene.free.t27.y | `122.500069` | `122.500069` | PASS |
| `physicsBody.getVel` | scene.free.t27.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t27.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t27.x | `400.000061` | `400.000061` | PASS |
| `physicsBody.getPos` | scene.fast.t27.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t27.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t27.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t27.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t27.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t27.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t27.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t27.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t27.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t27.x | `120.000031` | `120.000031` | PASS |
| `physicsBody.getPos` | scene.sensor.t27.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t27.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t27.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t27.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t28.x | `-53.333378` | `-53.333378` | PASS |
| `physicsBody.getPos` | scene.free.t28.y | `123.333405` | `123.333405` | PASS |
| `physicsBody.getVel` | scene.free.t28.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t28.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t28.x | `433.333405` | `433.333405` | PASS |
| `physicsBody.getPos` | scene.fast.t28.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t28.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t28.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t28.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t28.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t28.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t28.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t28.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t28.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t28.x | `123.333366` | `123.333366` | PASS |
| `physicsBody.getPos` | scene.sensor.t28.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t28.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t28.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t28.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t29.x | `-51.666710` | `-51.666710` | PASS |
| `physicsBody.getPos` | scene.free.t29.y | `124.166740` | `124.166740` | PASS |
| `physicsBody.getVel` | scene.free.t29.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t29.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t29.x | `466.666748` | `466.666748` | PASS |
| `physicsBody.getPos` | scene.fast.t29.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t29.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t29.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t29.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t29.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t29.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t29.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t29.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t29.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t29.x | `126.666702` | `126.666702` | PASS |
| `physicsBody.getPos` | scene.sensor.t29.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t29.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t29.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t29.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t30.x | `-50.000042` | `-50.000042` | PASS |
| `physicsBody.getPos` | scene.free.t30.y | `125.000076` | `125.000076` | PASS |
| `physicsBody.getVel` | scene.free.t30.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t30.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t30.x | `500.000092` | `500.000092` | PASS |
| `physicsBody.getPos` | scene.fast.t30.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t30.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t30.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t30.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t30.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t30.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t30.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t30.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t30.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t30.x | `130.000031` | `130.000031` | PASS |
| `physicsBody.getPos` | scene.sensor.t30.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t30.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t30.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t30.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t31.x | `-48.333374` | `-48.333374` | PASS |
| `physicsBody.getPos` | scene.free.t31.y | `125.833412` | `125.833412` | PASS |
| `physicsBody.getVel` | scene.free.t31.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t31.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t31.x | `533.333435` | `533.333435` | PASS |
| `physicsBody.getPos` | scene.fast.t31.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t31.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t31.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t31.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t31.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t31.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t31.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t31.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t31.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t31.x | `133.333359` | `133.333359` | PASS |
| `physicsBody.getPos` | scene.sensor.t31.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t31.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t31.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t31.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t32.x | `-46.666706` | `-46.666706` | PASS |
| `physicsBody.getPos` | scene.free.t32.y | `126.666748` | `126.666748` | PASS |
| `physicsBody.getVel` | scene.free.t32.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t32.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t32.x | `566.666748` | `566.666748` | PASS |
| `physicsBody.getPos` | scene.fast.t32.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t32.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t32.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t32.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t32.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t32.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t32.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t32.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t32.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t32.x | `136.666687` | `136.666687` | PASS |
| `physicsBody.getPos` | scene.sensor.t32.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t32.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t32.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t32.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t33.x | `-45.000038` | `-45.000038` | PASS |
| `physicsBody.getPos` | scene.free.t33.y | `127.500084` | `127.500084` | PASS |
| `physicsBody.getVel` | scene.free.t33.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t33.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t33.x | `600.000061` | `600.000061` | PASS |
| `physicsBody.getPos` | scene.fast.t33.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t33.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t33.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t33.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t33.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t33.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t33.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t33.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t33.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t33.x | `140.000015` | `140.000015` | PASS |
| `physicsBody.getPos` | scene.sensor.t33.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t33.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t33.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t33.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t34.x | `-43.333370` | `-43.333370` | PASS |
| `physicsBody.getPos` | scene.free.t34.y | `128.333420` | `128.333420` | PASS |
| `physicsBody.getVel` | scene.free.t34.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t34.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t34.x | `633.333374` | `633.333374` | PASS |
| `physicsBody.getPos` | scene.fast.t34.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t34.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t34.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t34.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t34.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t34.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t34.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t34.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t34.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t34.x | `143.333344` | `143.333344` | PASS |
| `physicsBody.getPos` | scene.sensor.t34.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t34.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t34.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t34.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t35.x | `-41.666702` | `-41.666702` | PASS |
| `physicsBody.getPos` | scene.free.t35.y | `129.166748` | `129.166748` | PASS |
| `physicsBody.getVel` | scene.free.t35.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t35.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t35.x | `666.666687` | `666.666687` | PASS |
| `physicsBody.getPos` | scene.fast.t35.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t35.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t35.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t35.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t35.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t35.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t35.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t35.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t35.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t35.x | `146.666672` | `146.666672` | PASS |
| `physicsBody.getPos` | scene.sensor.t35.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t35.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t35.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t35.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t36.x | `-40.000034` | `-40.000034` | PASS |
| `physicsBody.getPos` | scene.free.t36.y | `130.000076` | `130.000076` | PASS |
| `physicsBody.getVel` | scene.free.t36.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t36.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t36.x | `700.000000` | `700.000000` | PASS |
| `physicsBody.getPos` | scene.fast.t36.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t36.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t36.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t36.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t36.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t36.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t36.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t36.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t36.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t36.x | `150.000000` | `150.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t36.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t36.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t36.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t36.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t37.x | `-38.333366` | `-38.333366` | PASS |
| `physicsBody.getPos` | scene.free.t37.y | `130.833405` | `130.833405` | PASS |
| `physicsBody.getVel` | scene.free.t37.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t37.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t37.x | `733.333313` | `733.333313` | PASS |
| `physicsBody.getPos` | scene.fast.t37.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t37.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t37.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t37.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t37.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t37.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t37.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t37.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t37.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t37.x | `153.333328` | `153.333328` | PASS |
| `physicsBody.getPos` | scene.sensor.t37.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t37.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t37.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t37.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t38.x | `-36.666698` | `-36.666698` | PASS |
| `physicsBody.getPos` | scene.free.t38.y | `131.666733` | `131.666733` | PASS |
| `physicsBody.getVel` | scene.free.t38.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t38.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t38.x | `766.666626` | `766.666626` | PASS |
| `physicsBody.getPos` | scene.fast.t38.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t38.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t38.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t38.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t38.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t38.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t38.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t38.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t38.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t38.x | `156.666656` | `156.666656` | PASS |
| `physicsBody.getPos` | scene.sensor.t38.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t38.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t38.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t38.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t39.x | `-35.000031` | `-35.000031` | PASS |
| `physicsBody.getPos` | scene.free.t39.y | `132.500061` | `132.500061` | PASS |
| `physicsBody.getVel` | scene.free.t39.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t39.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t39.x | `799.999939` | `799.999939` | PASS |
| `physicsBody.getPos` | scene.fast.t39.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t39.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t39.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t39.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t39.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t39.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t39.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t39.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t39.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t39.x | `159.999985` | `159.999985` | PASS |
| `physicsBody.getPos` | scene.sensor.t39.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t39.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t39.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t39.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t40.x | `-33.333363` | `-33.333363` | PASS |
| `physicsBody.getPos` | scene.free.t40.y | `133.333389` | `133.333389` | PASS |
| `physicsBody.getVel` | scene.free.t40.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t40.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t40.x | `833.333252` | `833.333252` | PASS |
| `physicsBody.getPos` | scene.fast.t40.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t40.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t40.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t40.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t40.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t40.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t40.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t40.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t40.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t40.x | `163.333313` | `163.333313` | PASS |
| `physicsBody.getPos` | scene.sensor.t40.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t40.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t40.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t40.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t41.x | `-31.666697` | `-31.666697` | PASS |
| `physicsBody.getPos` | scene.free.t41.y | `134.166718` | `134.166718` | PASS |
| `physicsBody.getVel` | scene.free.t41.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t41.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t41.x | `866.666565` | `866.666565` | PASS |
| `physicsBody.getPos` | scene.fast.t41.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t41.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t41.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t41.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t41.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t41.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t41.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t41.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t41.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t41.x | `166.666641` | `166.666641` | PASS |
| `physicsBody.getPos` | scene.sensor.t41.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t41.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t41.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t41.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t42.x | `-30.000031` | `-30.000031` | PASS |
| `physicsBody.getPos` | scene.free.t42.y | `135.000046` | `135.000046` | PASS |
| `physicsBody.getVel` | scene.free.t42.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t42.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t42.x | `899.999878` | `899.999878` | PASS |
| `physicsBody.getPos` | scene.fast.t42.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t42.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t42.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t42.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t42.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t42.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t42.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t42.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t42.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t42.x | `169.999969` | `169.999969` | PASS |
| `physicsBody.getPos` | scene.sensor.t42.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t42.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t42.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t42.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t43.x | `-28.333364` | `-28.333364` | PASS |
| `physicsBody.getPos` | scene.free.t43.y | `135.833374` | `135.833374` | PASS |
| `physicsBody.getVel` | scene.free.t43.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t43.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t43.x | `933.333191` | `933.333191` | PASS |
| `physicsBody.getPos` | scene.fast.t43.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t43.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t43.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t43.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t43.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t43.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t43.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t43.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t43.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t43.x | `173.333298` | `173.333298` | PASS |
| `physicsBody.getPos` | scene.sensor.t43.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t43.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t43.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t43.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t44.x | `-26.666698` | `-26.666698` | PASS |
| `physicsBody.getPos` | scene.free.t44.y | `136.666702` | `136.666702` | PASS |
| `physicsBody.getVel` | scene.free.t44.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t44.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t44.x | `966.666504` | `966.666504` | PASS |
| `physicsBody.getPos` | scene.fast.t44.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t44.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t44.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t44.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t44.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t44.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t44.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t44.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t44.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t44.x | `176.666626` | `176.666626` | PASS |
| `physicsBody.getPos` | scene.sensor.t44.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t44.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t44.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t44.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t45.x | `-25.000032` | `-25.000032` | PASS |
| `physicsBody.getPos` | scene.free.t45.y | `137.500031` | `137.500031` | PASS |
| `physicsBody.getVel` | scene.free.t45.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t45.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t45.x | `999.999817` | `999.999817` | PASS |
| `physicsBody.getPos` | scene.fast.t45.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t45.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t45.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t45.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t45.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t45.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t45.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t45.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t45.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t45.x | `179.999954` | `179.999954` | PASS |
| `physicsBody.getPos` | scene.sensor.t45.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t45.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t45.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t45.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t46.x | `-23.333366` | `-23.333366` | PASS |
| `physicsBody.getPos` | scene.free.t46.y | `138.333359` | `138.333359` | PASS |
| `physicsBody.getVel` | scene.free.t46.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t46.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t46.x | `1033.333130` | `1033.333130` | PASS |
| `physicsBody.getPos` | scene.fast.t46.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t46.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t46.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t46.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t46.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t46.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t46.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t46.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t46.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t46.x | `183.333282` | `183.333282` | PASS |
| `physicsBody.getPos` | scene.sensor.t46.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t46.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t46.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t46.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t47.x | `-21.666700` | `-21.666700` | PASS |
| `physicsBody.getPos` | scene.free.t47.y | `139.166687` | `139.166687` | PASS |
| `physicsBody.getVel` | scene.free.t47.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t47.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t47.x | `1066.666504` | `1066.666504` | PASS |
| `physicsBody.getPos` | scene.fast.t47.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t47.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t47.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t47.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t47.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t47.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t47.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t47.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t47.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t47.x | `186.666611` | `186.666611` | PASS |
| `physicsBody.getPos` | scene.sensor.t47.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t47.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t47.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t47.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t48.x | `-20.000034` | `-20.000034` | PASS |
| `physicsBody.getPos` | scene.free.t48.y | `140.000015` | `140.000015` | PASS |
| `physicsBody.getVel` | scene.free.t48.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t48.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t48.x | `1099.999878` | `1099.999878` | PASS |
| `physicsBody.getPos` | scene.fast.t48.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t48.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t48.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t48.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t48.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t48.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t48.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t48.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t48.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t48.x | `189.999939` | `189.999939` | PASS |
| `physicsBody.getPos` | scene.sensor.t48.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t48.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t48.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t48.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t49.x | `-18.333368` | `-18.333368` | PASS |
| `physicsBody.getPos` | scene.free.t49.y | `140.833344` | `140.833344` | PASS |
| `physicsBody.getVel` | scene.free.t49.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t49.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t49.x | `1133.333252` | `1133.333252` | PASS |
| `physicsBody.getPos` | scene.fast.t49.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t49.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t49.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t49.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t49.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t49.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t49.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t49.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t49.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t49.x | `193.333267` | `193.333267` | PASS |
| `physicsBody.getPos` | scene.sensor.t49.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t49.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t49.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t49.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t50.x | `-16.666702` | `-16.666702` | PASS |
| `physicsBody.getPos` | scene.free.t50.y | `141.666672` | `141.666672` | PASS |
| `physicsBody.getVel` | scene.free.t50.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t50.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t50.x | `1166.666626` | `1166.666626` | PASS |
| `physicsBody.getPos` | scene.fast.t50.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t50.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t50.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t50.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t50.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t50.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t50.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t50.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t50.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t50.x | `196.666595` | `196.666595` | PASS |
| `physicsBody.getPos` | scene.sensor.t50.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t50.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t50.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t50.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t51.x | `-15.000035` | `-15.000035` | PASS |
| `physicsBody.getPos` | scene.free.t51.y | `142.500000` | `142.500000` | PASS |
| `physicsBody.getVel` | scene.free.t51.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t51.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t51.x | `1200.000000` | `1200.000000` | PASS |
| `physicsBody.getPos` | scene.fast.t51.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t51.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t51.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t51.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t51.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t51.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t51.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t51.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t51.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t51.x | `199.999924` | `199.999924` | PASS |
| `physicsBody.getPos` | scene.sensor.t51.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t51.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t51.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t51.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t52.x | `-13.333368` | `-13.333368` | PASS |
| `physicsBody.getPos` | scene.free.t52.y | `143.333328` | `143.333328` | PASS |
| `physicsBody.getVel` | scene.free.t52.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t52.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t52.x | `1233.333374` | `1233.333374` | PASS |
| `physicsBody.getPos` | scene.fast.t52.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t52.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t52.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t52.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t52.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t52.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t52.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t52.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t52.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t52.x | `203.333252` | `203.333252` | PASS |
| `physicsBody.getPos` | scene.sensor.t52.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t52.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t52.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t52.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t53.x | `-11.666701` | `-11.666701` | PASS |
| `physicsBody.getPos` | scene.free.t53.y | `144.166656` | `144.166656` | PASS |
| `physicsBody.getVel` | scene.free.t53.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t53.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t53.x | `1266.666748` | `1266.666748` | PASS |
| `physicsBody.getPos` | scene.fast.t53.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t53.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t53.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t53.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t53.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t53.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t53.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t53.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t53.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t53.x | `206.666580` | `206.666580` | PASS |
| `physicsBody.getPos` | scene.sensor.t53.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t53.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t53.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t53.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t54.x | `-10.000034` | `-10.000034` | PASS |
| `physicsBody.getPos` | scene.free.t54.y | `144.999985` | `144.999985` | PASS |
| `physicsBody.getVel` | scene.free.t54.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t54.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t54.x | `1300.000122` | `1300.000122` | PASS |
| `physicsBody.getPos` | scene.fast.t54.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t54.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t54.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t54.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t54.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t54.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t54.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t54.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t54.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t54.x | `209.999908` | `209.999908` | PASS |
| `physicsBody.getPos` | scene.sensor.t54.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t54.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t54.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t54.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t55.x | `-8.333367` | `-8.333367` | PASS |
| `physicsBody.getPos` | scene.free.t55.y | `145.833313` | `145.833313` | PASS |
| `physicsBody.getVel` | scene.free.t55.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t55.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t55.x | `1333.333496` | `1333.333496` | PASS |
| `physicsBody.getPos` | scene.fast.t55.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t55.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t55.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t55.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t55.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t55.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t55.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t55.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t55.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t55.x | `213.333237` | `213.333237` | PASS |
| `physicsBody.getPos` | scene.sensor.t55.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t55.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t55.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t55.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t56.x | `-6.666701` | `-6.666701` | PASS |
| `physicsBody.getPos` | scene.free.t56.y | `146.666641` | `146.666641` | PASS |
| `physicsBody.getVel` | scene.free.t56.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t56.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t56.x | `1366.666870` | `1366.666870` | PASS |
| `physicsBody.getPos` | scene.fast.t56.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t56.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t56.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t56.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t56.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t56.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t56.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t56.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t56.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t56.x | `216.666565` | `216.666565` | PASS |
| `physicsBody.getPos` | scene.sensor.t56.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t56.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t56.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t56.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t57.x | `-5.000034` | `-5.000034` | PASS |
| `physicsBody.getPos` | scene.free.t57.y | `147.499969` | `147.499969` | PASS |
| `physicsBody.getVel` | scene.free.t57.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t57.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t57.x | `1400.000244` | `1400.000244` | PASS |
| `physicsBody.getPos` | scene.fast.t57.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t57.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t57.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t57.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t57.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t57.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t57.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t57.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t57.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t57.x | `219.999893` | `219.999893` | PASS |
| `physicsBody.getPos` | scene.sensor.t57.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t57.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t57.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t57.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t58.x | `-3.333368` | `-3.333368` | PASS |
| `physicsBody.getPos` | scene.free.t58.y | `148.333298` | `148.333298` | PASS |
| `physicsBody.getVel` | scene.free.t58.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t58.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t58.x | `1433.333618` | `1433.333618` | PASS |
| `physicsBody.getPos` | scene.fast.t58.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t58.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t58.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t58.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t58.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t58.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t58.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t58.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t58.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t58.x | `223.333221` | `223.333221` | PASS |
| `physicsBody.getPos` | scene.sensor.t58.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t58.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t58.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t58.colliding | `0` | `0` | PASS |
| `physicsBody.getPos` | scene.free.t59.x | `-1.666701` | `-1.666701` | PASS |
| `physicsBody.getPos` | scene.free.t59.y | `149.166626` | `149.166626` | PASS |
| `physicsBody.getVel` | scene.free.t59.vx | `0.100000` | `0.100000` | PASS |
| `physicsBody.getVel` | scene.free.t59.vy | `0.050000` | `0.050000` | PASS |
| `physicsBody.getPos` | scene.fast.t59.x | `1466.666992` | `1466.666992` | PASS |
| `physicsBody.getPos` | scene.fast.t59.y | `500.000000` | `500.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t59.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | scene.fast.t59.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t59.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.solid.t59.y | `3.005000` | `3.005000` | PASS |
| `physicsBody.getVel` | scene.solid.t59.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.solid.t59.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.solid.t59.colliding | `1` | `1` | PASS |
| `physicsBody.isBullet` | scene.solid.t59.bullet | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | scene.sensor.t59.x | `226.666550` | `226.666550` | PASS |
| `physicsBody.getPos` | scene.sensor.t59.y | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | scene.sensor.t59.vx | `0.200000` | `0.200000` | PASS |
| `physicsBody.getVel` | scene.sensor.t59.vy | `0.000000` | `0.000000` | PASS |
| `physicsData.isColliding` | scene.sensor.t59.colliding | `0` | `0` | PASS |
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
| `physicsData.isColliding` | filter.mutualMaskBlocked | `0` | `0` | PASS |
| `physicsData.isColliding` | filter.mutualMaskAllowed | `1` | `1` | PASS |
| `physicsData.isColliding` | filter.positiveGroupOverridesMasks | `1` | `1` | PASS |
| `physicsData.isColliding` | filter.negativeGroupBlocks | `0` | `0` | PASS |
| `physicsData.isColliding` | filter.differentGroupsUseMasks | `0` | `0` | PASS |
| `physicsData.isColliding` | filter.wideGroupInteraction | `0` | `0` | PASS |
| `physicsBody.getCollisionGroup` | filter.wideGroupReadback | `65537` | `65537` | PASS |
| `physicsData.isColliding` | filter.wideCategoryInteraction | `1` | `1` | PASS |
| `physicsBody.getCollisionCategory` | filter.wideCategoryReadback | `1` | `1` | PASS |
| `physicsData.isColliding` | filter.wideMaskInteraction | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | filter.wideMaskReadback | `1` | `1` | PASS |
| `physicsData.isColliding` | filter.negativeGroupAfterReenable | `0` | `0` | PASS |
| `physicsData.isColliding` | filter.deferredGroupReadback | `0` | `0` | PASS |
| `physicsData.isColliding` | filter.deferredGroupInteraction | `1` | `1` | PASS |
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
| `physicsData.isColliding` | query.solidA.t7.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t7.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t7.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t7.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t7.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t7.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t7.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t7.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t7.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t7.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t7.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t7.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t7.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t7.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t7.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t7.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t7.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t7.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t7.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t7.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t7.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t7.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t7.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t7.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t7.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t7.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t7.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t7.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t7.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t7.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t7.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t7.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t7.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t7.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t7.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t7.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t7.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t7.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t7.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t7.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t7.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t7.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t7.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t7.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t7.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t7.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t7.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t7.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t7.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t7.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t7.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t7.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t7.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t7.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t7.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t7.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t7.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t7.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | query.solidA.t8.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t8.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t8.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t8.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t8.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t8.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t8.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t8.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t8.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t8.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t8.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t8.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t8.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t8.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t8.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t8.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t8.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t8.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t8.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t8.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t8.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t8.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t8.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t8.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t8.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t8.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t8.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t8.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t8.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t8.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t8.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t8.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t8.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t8.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t8.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t8.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t8.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t8.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t8.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t8.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t8.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t8.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t8.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t8.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t8.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t8.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t8.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t8.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t8.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t8.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t8.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t8.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t8.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t8.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t8.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t8.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t8.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t8.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | query.solidA.t9.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t9.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t9.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t9.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t9.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t9.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t9.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t9.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t9.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t9.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t9.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t9.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t9.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t9.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t9.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t9.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t9.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t9.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t9.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t9.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t9.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t9.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t9.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t9.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t9.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t9.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t9.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t9.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t9.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t9.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t9.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t9.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t9.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t9.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t9.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t9.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t9.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t9.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t9.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t9.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t9.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t9.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t9.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t9.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t9.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t9.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t9.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t9.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t9.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t9.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t9.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t9.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t9.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t9.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t9.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t9.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | query.solidA.t10.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t10.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t10.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t10.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t10.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidA.t10.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t10.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t10.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t10.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t10.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t10.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t10.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t10.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t10.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t10.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t10.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t10.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t10.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t10.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t10.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t10.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t10.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t10.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t10.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t10.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t10.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t10.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t10.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t10.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t10.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t10.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t10.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t10.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.solidB.t10.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t10.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t10.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t10.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t10.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t10.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t10.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t10.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t10.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t10.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t10.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t10.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t10.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t10.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t10.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t10.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t10.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t10.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t10.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t10.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t10.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t10.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t10.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | query.solidA.t11.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidA.t11.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidA.t11.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidA.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidA.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidA.t11.group | `-1` | `-1` | PASS |
| `physicsBody.isCollidable` | query.solidA.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidA.t11.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidA.t11.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidA.t11.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t11.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidA.t11.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidA.t11.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidA.t11.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidA.t11.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidA.t11.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidA.t11.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidA.t11.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidA.t11.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidA.t11.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t11.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidA.t11.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidA.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t11.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidA.t11.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t11.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidA.t11.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t11.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidA.t11.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.solidB.t11.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.solidB.t11.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.solidB.t11.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.solidB.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.solidB.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.solidB.t11.group | `-1` | `-1` | PASS |
| `physicsBody.isCollidable` | query.solidB.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.solidB.t11.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.solidB.t11.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.solidB.t11.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t11.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.solidB.t11.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.solidB.t11.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.solidB.t11.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.solidB.t11.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.solidB.t11.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.solidB.t11.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.solidB.t11.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.solidB.t11.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.solidB.t11.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t11.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.solidB.t11.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.solidB.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t11.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.solidB.t11.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t11.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.solidB.t11.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t11.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.solidB.t11.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorA.t11.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t11.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t11.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t11.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t11.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t11.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t11.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t11.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t11.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t11.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t11.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t11.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t11.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t11.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t11.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t11.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t11.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t11.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t11.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t11.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t11.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t11.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t11.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t11.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t11.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t11.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t11.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t11.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t11.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t11.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t11.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t11.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t11.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t11.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t11.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t11.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t11.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t11.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t11.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t11.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t11.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t11.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t11.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t11.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t11.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t11.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t11.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t11.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t11.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t11.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t11.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t11.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t11.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | query.sensorA.t12.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t12.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t12.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t12.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t12.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t12.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t12.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t12.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t12.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t12.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t12.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t12.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t12.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t12.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t12.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t12.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t12.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t12.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t12.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t12.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t12.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t12.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t12.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t12.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t12.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t12.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t12.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t12.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t12.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t12.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t12.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t12.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t12.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t12.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t12.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t12.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t12.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t12.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t12.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t12.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t12.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t12.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t12.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t12.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t12.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t12.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t12.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t12.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t12.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t12.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t12.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t12.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t12.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t12.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t12.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t12.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | query.sensorA.t13.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorA.t13.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorA.t13.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorA.t13.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorA.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorA.t13.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorA.t13.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorA.t13.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorA.t13.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorA.t13.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t13.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorA.t13.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorA.t13.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorA.t13.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorA.t13.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorA.t13.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorA.t13.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorA.t13.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorA.t13.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorA.t13.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t13.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorA.t13.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorA.t13.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t13.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorA.t13.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t13.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorA.t13.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t13.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorA.t13.contactY | `-1` | `-1` | PASS |
| `physicsData.isColliding` | query.sensorB.t13.colliding | `0` | `0` | PASS |
| `physicsData.isColliding` | query.sensorB.t13.anyColliding | `0` | `0` | PASS |
| `physicsData.getCollisionInfo` | query.sensorB.t13.exists | `0` | `0` | PASS |
| `physicsBody.getCollisionCategory` | query.sensorB.t13.category | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | query.sensorB.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getCollisionGroup` | query.sensorB.t13.group | `0` | `0` | PASS |
| `physicsBody.isCollidable` | query.sensorB.t13.enabled | `1` | `1` | PASS |
| `intIter.size` | query.sensorB.t13.listSize | `-1` | `-1` | PASS |
| `intIter.get` | query.sensorB.t13.idMatches | `-1` | `-1` | PASS |
| `collisionInfo.getFirstShapeCollisionInfo` | query.sensorB.t13.firstExists | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t13.shapeExists | `-1` | `-1` | PASS |
| `collisionInfo.getEntity` | query.sensorB.t13.entityMatches | `-1` | `-1` | PASS |
| `collisionInfo.isAllSensors` | query.sensorB.t13.allSensors | `-1` | `-1` | PASS |
| `collisionInfo.isBodyColliding` | query.sensorB.t13.bodyColliding | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithBody` | query.sensorB.t13.bodyPair | `-1` | `-1` | PASS |
| `collisionInfo.isBodyCollidingWithShape` | query.sensorB.t13.bodyShape | `-1` | `-1` | PASS |
| `collisionInfo.isShapeColliding` | query.sensorB.t13.shapeColliding | `-1` | `-1` | PASS |
| `collisionInfo.isMyShapeCollidingWithSensorsOnly` | query.sensorB.t13.mySensorsOnly | `-1` | `-1` | PASS |
| `collisionInfo.getShapeCollisionInfo` | query.sensorB.t13.missingShape | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumOfMyShapesColliding` | query.sensorB.t13.myCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t13.myIDMatches | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getNumContacts` | query.sensorB.t13.contactCount | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getMyContactShapeID` | query.sensorB.t13.badMyID | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t13.pairNX | `-1` | `-1` | PASS |
| `collisionInfo.getContactNormalOfMyCollidingShape` | query.sensorB.t13.pairNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t13.contactNX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactNormal` | query.sensorB.t13.contactNY | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t13.contactX | `-1` | `-1` | PASS |
| `shapeCollisionInfo.getContactPos` | query.sensorB.t13.contactY | `-1` | `-1` | PASS |
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
| `physicsData.isColliding` | reentry.sensor.t0.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t0.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t0.bX | `20.000000` | `20.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t1.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t1.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t1.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t2.colliding | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t2.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t2.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t3.colliding | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t3.mask | `0` | `0` | PASS |
| `physicsBody.getPos` | reentry.sensor.t3.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t4.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t4.mask | `0` | `0` | PASS |
| `physicsBody.getPos` | reentry.sensor.t4.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t5.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t5.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t5.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t6.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t6.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t6.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t7.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t7.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t7.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t8.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t8.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t8.bX | `3.100000` | `3.100000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t9.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t9.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t9.bX | `3.100000` | `3.100000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t10.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t10.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t10.bX | `20.000000` | `20.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t11.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t11.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t11.bX | `20.000000` | `20.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t12.colliding | `0` | `0` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t12.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t12.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t13.colliding | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t13.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t13.bX | `3.000000` | `3.000000` | PASS |
| `physicsData.isColliding` | reentry.sensor.t14.colliding | `1` | `1` | PASS |
| `physicsBody.getCategoriesToCollideWith` | reentry.sensor.t14.mask | `65535` | `65535` | PASS |
| `physicsBody.getPos` | reentry.sensor.t14.bX | `3.000000` | `3.000000` | PASS |
| `jointType.getType` | joint.distance.t0.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t0.body1X | `0.000000` | `0.000000` | PASS |
| `jointType.getBody2` | joint.distance.t0.body2X | `5.000000` | `5.000000` | PASS |
| `jointType.getEntity1` | joint.distance.t0.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t0.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t0.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t0.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.distance.t0.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t0.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t0.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t0.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t0.aX | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t0.bX | `5.000000` | `5.000000` | PASS |
| `jointType.getType` | joint.revolute.t0.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t0.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t0.body2X | `35.000000` | `35.000000` | PASS |
| `jointType.getEntity1` | joint.revolute.t0.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t0.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t0.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t0.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t0.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t0.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t0.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t0.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t0.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t0.bX | `35.000000` | `35.000000` | PASS |
| `jointType.getType` | joint.pin.t0.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t0.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t0.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t0.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t0.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t0.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t0.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t0.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t0.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t0.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t0.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t0.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t0.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t0.body2X | `95.000000` | `95.000000` | PASS |
| `jointType.getEntity1` | joint.prismatic.t0.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t0.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t0.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t0.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t0.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t0.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t0.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t0.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t0.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t0.bX | `95.000000` | `95.000000` | PASS |
| `jointType.getType` | joint.relative.t0.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t0.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t0.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t0.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t0.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t0.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t0.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t0.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t0.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t0.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t0.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t0.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t0.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t1.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t1.body1X | `4.166666` | `4.166666` | PASS |
| `jointType.getBody2` | joint.distance.t1.body2X | `9.166666` | `9.166666` | PASS |
| `jointType.getEntity1` | joint.distance.t1.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t1.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t1.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t1.anchorX | `4.166666` | `4.166666` | PASS |
| `jointType.getAnchorPos` | joint.distance.t1.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t1.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t1.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t1.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t1.aX | `4.166666` | `4.166666` | PASS |
| `physicsBody.getPos` | joint.distance.t1.bX | `9.166666` | `9.166666` | PASS |
| `jointType.getType` | joint.revolute.t1.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t1.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t1.body2X | `33.331329` | `33.331329` | PASS |
| `jointType.getEntity1` | joint.revolute.t1.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t1.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t1.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t1.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t1.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t1.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t1.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t1.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t1.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t1.bX | `33.331329` | `33.331329` | PASS |
| `jointType.getType` | joint.pin.t1.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t1.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t1.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t1.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t1.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t1.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t1.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t1.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t1.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t1.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t1.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t1.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t1.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t1.body2X | `96.666664` | `96.666664` | PASS |
| `jointType.getEntity1` | joint.prismatic.t1.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t1.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t1.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t1.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t1.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t1.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t1.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t1.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t1.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t1.bX | `96.666664` | `96.666664` | PASS |
| `jointType.getType` | joint.relative.t1.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t1.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t1.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t1.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t1.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t1.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t1.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t1.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t1.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t1.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t1.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t1.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t1.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t2.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t2.body1X | `8.333332` | `8.333332` | PASS |
| `jointType.getBody2` | joint.distance.t2.body2X | `13.333332` | `13.333332` | PASS |
| `jointType.getEntity1` | joint.distance.t2.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t2.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t2.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t2.anchorX | `8.333332` | `8.333332` | PASS |
| `jointType.getAnchorPos` | joint.distance.t2.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t2.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t2.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t2.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t2.aX | `8.333332` | `8.333332` | PASS |
| `physicsBody.getPos` | joint.distance.t2.bX | `13.333332` | `13.333332` | PASS |
| `jointType.getType` | joint.revolute.t2.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t2.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t2.body2X | `30.570898` | `30.570898` | PASS |
| `jointType.getEntity1` | joint.revolute.t2.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t2.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t2.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t2.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t2.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t2.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t2.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t2.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t2.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t2.bX | `30.570898` | `30.570898` | PASS |
| `jointType.getType` | joint.pin.t2.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t2.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t2.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t2.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t2.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t2.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t2.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t2.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t2.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t2.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t2.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t2.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t2.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t2.body2X | `98.333328` | `98.333328` | PASS |
| `jointType.getEntity1` | joint.prismatic.t2.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t2.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t2.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t2.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t2.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t2.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t2.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t2.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t2.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t2.bX | `98.333328` | `98.333328` | PASS |
| `jointType.getType` | joint.relative.t2.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t2.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t2.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t2.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t2.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t2.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t2.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t2.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t2.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t2.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t2.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t2.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t2.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t3.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t3.body1X | `12.499998` | `12.499998` | PASS |
| `jointType.getBody2` | joint.distance.t3.body2X | `17.499998` | `17.499998` | PASS |
| `jointType.getEntity1` | joint.distance.t3.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t3.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t3.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t3.anchorX | `12.499998` | `12.499998` | PASS |
| `jointType.getAnchorPos` | joint.distance.t3.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t3.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t3.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t3.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t3.aX | `12.499998` | `12.499998` | PASS |
| `physicsBody.getPos` | joint.distance.t3.bX | `17.499998` | `17.499998` | PASS |
| `jointType.getType` | joint.revolute.t3.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t3.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t3.body2X | `28.045357` | `28.045357` | PASS |
| `jointType.getEntity1` | joint.revolute.t3.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t3.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t3.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t3.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t3.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t3.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t3.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t3.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t3.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t3.bX | `28.045357` | `28.045357` | PASS |
| `jointType.getType` | joint.pin.t3.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t3.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t3.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t3.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t3.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t3.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t3.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t3.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t3.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t3.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t3.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t3.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t3.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t3.body2X | `99.999992` | `99.999992` | PASS |
| `jointType.getEntity1` | joint.prismatic.t3.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t3.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t3.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t3.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t3.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t3.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t3.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t3.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t3.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t3.bX | `99.999992` | `99.999992` | PASS |
| `jointType.getType` | joint.relative.t3.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t3.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t3.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t3.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t3.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t3.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t3.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t3.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t3.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t3.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t3.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t3.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t3.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t4.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t4.body1X | `16.666664` | `16.666664` | PASS |
| `jointType.getBody2` | joint.distance.t4.body2X | `21.666664` | `21.666664` | PASS |
| `jointType.getEntity1` | joint.distance.t4.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t4.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t4.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t4.anchorX | `16.666664` | `16.666664` | PASS |
| `jointType.getAnchorPos` | joint.distance.t4.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t4.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t4.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t4.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t4.aX | `16.666664` | `16.666664` | PASS |
| `physicsBody.getPos` | joint.distance.t4.bX | `21.666664` | `21.666664` | PASS |
| `jointType.getType` | joint.revolute.t4.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t4.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t4.body2X | `26.220781` | `26.220781` | PASS |
| `jointType.getEntity1` | joint.revolute.t4.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t4.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t4.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t4.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t4.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t4.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t4.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t4.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t4.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t4.bX | `26.220781` | `26.220781` | PASS |
| `jointType.getType` | joint.pin.t4.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t4.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t4.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t4.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t4.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t4.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t4.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t4.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t4.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t4.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t4.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t4.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t4.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t4.body2X | `101.666656` | `101.666656` | PASS |
| `jointType.getEntity1` | joint.prismatic.t4.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t4.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t4.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t4.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t4.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t4.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t4.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t4.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t4.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t4.bX | `101.666656` | `101.666656` | PASS |
| `jointType.getType` | joint.relative.t4.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t4.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t4.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t4.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t4.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t4.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t4.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t4.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t4.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t4.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t4.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t4.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t4.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t5.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t5.body1X | `20.833330` | `20.833330` | PASS |
| `jointType.getBody2` | joint.distance.t5.body2X | `25.833330` | `25.833330` | PASS |
| `jointType.getEntity1` | joint.distance.t5.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t5.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t5.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t5.anchorX | `20.833330` | `20.833330` | PASS |
| `jointType.getAnchorPos` | joint.distance.t5.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t5.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t5.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t5.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t5.aX | `20.833330` | `20.833330` | PASS |
| `physicsBody.getPos` | joint.distance.t5.bX | `25.833330` | `25.833330` | PASS |
| `jointType.getType` | joint.revolute.t5.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t5.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t5.body2X | `25.225103` | `25.225103` | PASS |
| `jointType.getEntity1` | joint.revolute.t5.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t5.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t5.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t5.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t5.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t5.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t5.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t5.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t5.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t5.bX | `25.225103` | `25.225103` | PASS |
| `jointType.getType` | joint.pin.t5.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t5.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t5.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t5.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t5.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t5.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t5.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t5.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t5.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t5.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t5.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t5.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t5.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t5.body2X | `103.333321` | `103.333321` | PASS |
| `jointType.getEntity1` | joint.prismatic.t5.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t5.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t5.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t5.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t5.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t5.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t5.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t5.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t5.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t5.bX | `103.333321` | `103.333321` | PASS |
| `jointType.getType` | joint.relative.t5.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t5.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t5.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t5.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t5.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t5.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t5.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t5.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t5.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t5.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t5.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t5.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t5.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t6.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t6.body1X | `24.999996` | `24.999996` | PASS |
| `jointType.getBody2` | joint.distance.t6.body2X | `29.999996` | `29.999996` | PASS |
| `jointType.getEntity1` | joint.distance.t6.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t6.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t6.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t6.anchorX | `24.999996` | `24.999996` | PASS |
| `jointType.getAnchorPos` | joint.distance.t6.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t6.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t6.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t6.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t6.aX | `24.999996` | `24.999996` | PASS |
| `physicsBody.getPos` | joint.distance.t6.bX | `29.999996` | `29.999996` | PASS |
| `jointType.getType` | joint.revolute.t6.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t6.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t6.body2X | `25.015690` | `25.015690` | PASS |
| `jointType.getEntity1` | joint.revolute.t6.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t6.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t6.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t6.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t6.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t6.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t6.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t6.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t6.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t6.bX | `25.015690` | `25.015690` | PASS |
| `jointType.getType` | joint.pin.t6.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t6.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t6.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t6.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t6.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t6.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t6.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t6.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t6.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t6.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t6.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t6.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t6.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t6.body2X | `104.999985` | `104.999985` | PASS |
| `jointType.getEntity1` | joint.prismatic.t6.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t6.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t6.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t6.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t6.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t6.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t6.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t6.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t6.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t6.bX | `104.999985` | `104.999985` | PASS |
| `jointType.getType` | joint.relative.t6.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t6.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t6.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t6.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t6.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t6.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t6.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t6.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t6.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t6.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t6.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t6.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t6.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t7.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t7.body1X | `29.166664` | `29.166664` | PASS |
| `jointType.getBody2` | joint.distance.t7.body2X | `34.166664` | `34.166664` | PASS |
| `jointType.getEntity1` | joint.distance.t7.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t7.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t7.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t7.anchorX | `29.166664` | `29.166664` | PASS |
| `jointType.getAnchorPos` | joint.distance.t7.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t7.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t7.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t7.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t7.aX | `29.166664` | `29.166664` | PASS |
| `physicsBody.getPos` | joint.distance.t7.bX | `34.166664` | `34.166664` | PASS |
| `jointType.getType` | joint.revolute.t7.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t7.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t7.body2X | `25.463972` | `25.463972` | PASS |
| `jointType.getEntity1` | joint.revolute.t7.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t7.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t7.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t7.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t7.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t7.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t7.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t7.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t7.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t7.bX | `25.463972` | `25.463972` | PASS |
| `jointType.getType` | joint.pin.t7.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t7.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t7.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t7.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t7.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t7.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t7.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t7.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t7.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t7.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t7.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t7.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t7.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t7.body2X | `106.666649` | `106.666649` | PASS |
| `jointType.getEntity1` | joint.prismatic.t7.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t7.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t7.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t7.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t7.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t7.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t7.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t7.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t7.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t7.bX | `106.666649` | `106.666649` | PASS |
| `jointType.getType` | joint.relative.t7.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t7.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t7.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t7.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t7.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t7.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t7.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t7.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t7.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t7.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t7.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t7.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t7.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t8.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t8.body1X | `33.333332` | `33.333332` | PASS |
| `jointType.getBody2` | joint.distance.t8.body2X | `38.333332` | `38.333332` | PASS |
| `jointType.getEntity1` | joint.distance.t8.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t8.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t8.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t8.anchorX | `33.333332` | `33.333332` | PASS |
| `jointType.getAnchorPos` | joint.distance.t8.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t8.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t8.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t8.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t8.aX | `33.333332` | `33.333332` | PASS |
| `physicsBody.getPos` | joint.distance.t8.bX | `38.333332` | `38.333332` | PASS |
| `jointType.getType` | joint.revolute.t8.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t8.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t8.body2X | `26.404593` | `26.404593` | PASS |
| `jointType.getEntity1` | joint.revolute.t8.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t8.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t8.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t8.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t8.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t8.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t8.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t8.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t8.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t8.bX | `26.404593` | `26.404593` | PASS |
| `jointType.getType` | joint.pin.t8.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t8.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t8.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t8.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t8.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t8.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t8.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t8.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t8.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t8.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t8.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t8.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t8.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t8.body2X | `108.333313` | `108.333313` | PASS |
| `jointType.getEntity1` | joint.prismatic.t8.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t8.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t8.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t8.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t8.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t8.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t8.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t8.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t8.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t8.bX | `108.333313` | `108.333313` | PASS |
| `jointType.getType` | joint.relative.t8.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t8.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t8.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t8.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t8.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t8.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t8.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t8.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t8.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t8.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t8.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t8.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t8.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t9.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t9.body1X | `37.500000` | `37.500000` | PASS |
| `jointType.getBody2` | joint.distance.t9.body2X | `42.500000` | `42.500000` | PASS |
| `jointType.getEntity1` | joint.distance.t9.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t9.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t9.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t9.anchorX | `37.500000` | `37.500000` | PASS |
| `jointType.getAnchorPos` | joint.distance.t9.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t9.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t9.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t9.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t9.aX | `37.500000` | `37.500000` | PASS |
| `physicsBody.getPos` | joint.distance.t9.bX | `42.500000` | `42.500000` | PASS |
| `jointType.getType` | joint.revolute.t9.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t9.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t9.body2X | `27.665192` | `27.665192` | PASS |
| `jointType.getEntity1` | joint.revolute.t9.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t9.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t9.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t9.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t9.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t9.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t9.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t9.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t9.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t9.bX | `27.665192` | `27.665192` | PASS |
| `jointType.getType` | joint.pin.t9.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t9.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t9.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t9.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t9.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t9.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t9.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t9.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t9.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t9.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t9.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t9.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t9.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t9.body2X | `109.999977` | `109.999977` | PASS |
| `jointType.getEntity1` | joint.prismatic.t9.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t9.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t9.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t9.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t9.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t9.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t9.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t9.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t9.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t9.bX | `109.999977` | `109.999977` | PASS |
| `jointType.getType` | joint.relative.t9.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t9.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t9.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t9.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t9.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t9.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t9.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t9.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t9.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t9.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t9.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t9.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t9.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t10.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t10.body1X | `41.666668` | `41.666668` | PASS |
| `jointType.getBody2` | joint.distance.t10.body2X | `46.666668` | `46.666668` | PASS |
| `jointType.getEntity1` | joint.distance.t10.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t10.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t10.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t10.anchorX | `41.666668` | `41.666668` | PASS |
| `jointType.getAnchorPos` | joint.distance.t10.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t10.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t10.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t10.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t10.aX | `41.666668` | `41.666668` | PASS |
| `physicsBody.getPos` | joint.distance.t10.bX | `46.666668` | `46.666668` | PASS |
| `jointType.getType` | joint.revolute.t10.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t10.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t10.body2X | `29.084349` | `29.084349` | PASS |
| `jointType.getEntity1` | joint.revolute.t10.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t10.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t10.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t10.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t10.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t10.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t10.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t10.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t10.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t10.bX | `29.084349` | `29.084349` | PASS |
| `jointType.getType` | joint.pin.t10.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t10.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t10.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t10.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t10.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t10.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t10.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t10.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t10.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t10.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t10.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t10.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t10.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t10.body2X | `111.666641` | `111.666641` | PASS |
| `jointType.getEntity1` | joint.prismatic.t10.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t10.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t10.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t10.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t10.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t10.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t10.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t10.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t10.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t10.bX | `111.666641` | `111.666641` | PASS |
| `jointType.getType` | joint.relative.t10.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t10.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t10.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t10.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t10.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t10.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t10.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t10.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t10.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t10.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t10.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t10.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t10.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getType` | joint.distance.t11.type | `3` | `3` | PASS |
| `jointType.getBody1` | joint.distance.t11.body1X | `45.833336` | `45.833336` | PASS |
| `jointType.getBody2` | joint.distance.t11.body2X | `50.833336` | `50.833336` | PASS |
| `jointType.getEntity1` | joint.distance.t11.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.distance.t11.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.distance.t11.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.distance.t11.anchorX | `45.833336` | `45.833336` | PASS |
| `jointType.getAnchorPos` | joint.distance.t11.anchorY | `100.000000` | `100.000000` | PASS |
| `jointType.getReactionForce` | joint.distance.t11.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.distance.t11.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.distance.t11.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.distance.t11.aX | `45.833336` | `45.833336` | PASS |
| `physicsBody.getPos` | joint.distance.t11.bX | `50.833336` | `50.833336` | PASS |
| `jointType.getType` | joint.revolute.t11.type | `1` | `1` | PASS |
| `jointType.getBody1` | joint.revolute.t11.body1X | `30.000000` | `30.000000` | PASS |
| `jointType.getBody2` | joint.revolute.t11.body2X | `30.521730` | `30.521730` | PASS |
| `jointType.getEntity1` | joint.revolute.t11.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.revolute.t11.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.revolute.t11.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t11.anchorX | `30.000000` | `30.000000` | PASS |
| `jointType.getAnchorPos` | joint.revolute.t11.anchorY | `200.000000` | `200.000000` | PASS |
| `jointType.getReactionForce` | joint.revolute.t11.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.revolute.t11.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.revolute.t11.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t11.aX | `30.000000` | `30.000000` | PASS |
| `physicsBody.getPos` | joint.revolute.t11.bX | `30.521730` | `30.521730` | PASS |
| `jointType.getType` | joint.pin.t11.type | `9` | `9` | PASS |
| `jointType.getBody1` | joint.pin.t11.body1X | `60.000000` | `60.000000` | PASS |
| `jointType.getEntity1` | joint.pin.t11.entity1Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.pin.t11.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.pin.t11.anchorX | `60.000000` | `60.000000` | PASS |
| `jointType.getAnchorPos` | joint.pin.t11.anchorY | `300.000000` | `300.000000` | PASS |
| `jointType.getReactionForce` | joint.pin.t11.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.pin.t11.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.pin.t11.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t11.aX | `60.000000` | `60.000000` | PASS |
| `physicsBody.getPos` | joint.pin.t11.bX | `60.000000` | `60.000000` | PASS |
| `jointType.getType` | joint.prismatic.t11.type | `2` | `2` | PASS |
| `jointType.getBody1` | joint.prismatic.t11.body1X | `90.000000` | `90.000000` | PASS |
| `jointType.getBody2` | joint.prismatic.t11.body2X | `113.333305` | `113.333305` | PASS |
| `jointType.getEntity1` | joint.prismatic.t11.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.prismatic.t11.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.prismatic.t11.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t11.anchorX | `90.000000` | `90.000000` | PASS |
| `jointType.getAnchorPos` | joint.prismatic.t11.anchorY | `400.000000` | `400.000000` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t11.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.prismatic.t11.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.prismatic.t11.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t11.aX | `90.000000` | `90.000000` | PASS |
| `physicsBody.getPos` | joint.prismatic.t11.bX | `113.333305` | `113.333305` | PASS |
| `jointType.getType` | joint.relative.t11.type | `10` | `10` | PASS |
| `jointType.getBody1` | joint.relative.t11.body1X | `120.000000` | `120.000000` | PASS |
| `jointType.getBody2` | joint.relative.t11.body2X | `125.000000` | `125.000000` | PASS |
| `jointType.getEntity1` | joint.relative.t11.entity1Same | `1` | `1` | PASS |
| `jointType.getEntity2` | joint.relative.t11.entity2Same | `1` | `1` | PASS |
| `jointType.isBodiesCollidable` | joint.relative.t11.collidable | `0` | `0` | PASS |
| `jointType.getAnchorPos` | joint.relative.t11.anchorX | `0.000000` | `0.000000` | PASS |
| `jointType.getAnchorPos` | joint.relative.t11.anchorY | `0.000000` | `0.000000` | PASS |
| `jointType.getReactionForce` | joint.relative.t11.forceX | `null` | `null` | PASS |
| `jointType.getReactionForce` | joint.relative.t11.forceY | `null` | `null` | PASS |
| `jointType.getReactionTorque` | joint.relative.t11.torque | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t11.aX | `120.000000` | `120.000000` | PASS |
| `physicsBody.getPos` | joint.relative.t11.bX | `125.000000` | `125.000000` | PASS |
| `jointType.getObjectID` | joint.registry.idDistinct | `1` | `1` | PASS |
| `Physics.getJoint` | joint.registry.lookup | `1` | `1` | PASS |
| `Physics.getJoint` | joint.registry.unknown | `1` | `1` | PASS |
| `jointIter.size` | jointIter.attachedSize | `1` | `1` | PASS |
| `jointIter.isValid` | jointIter.attachedValid | `1` | `1` | PASS |
| `jointIter.get` | jointIter.attachedType | `1` | `1` | PASS |
| `jointIter.goToNext` | jointIter.afterEndValid | `0` | `0` | PASS |
| `physicsData.getJointsOnMe` | jointIter.multipleSize | `2` | `2` | PASS |
| `jointIter.get` | jointIter.firstIsOriginal | `1` | `1` | PASS |
| `jointIter.get` | jointIter.secondIsExtra | `1` | `1` | PASS |
| `jointIter.goToNext` | jointIter.afterTwoValid | `0` | `0` | PASS |
| `Physics.deleteJoint` | joint.registry.deleted | `1` | `1` | PASS |
| `Physics.deleteAllJoints` | joint.registry.cleared | `1` | `1` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.rtSpeed | `0.625000` | `0.625000` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.motorReset | `0.625000` | `0.625000` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.motorSet | `0.625000` | `0.625000` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.genericMotor | `0.625000` | `0.625000` | PASS |
| `jointType.isBodiesCollidable` | joint.control.collidableOn | `1` | `1` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.motorAfterRebuild | `0.625000` | `0.625000` | PASS |
| `jointType.isBodiesCollidable` | joint.control.collidableOff | `0` | `0` | PASS |
| `jointType.getRTMotorSpeed` | joint.config.revoluteMotor | `0.375000` | `0.375000` | PASS |
| `jointType.getRTMotorSpeed` | joint.config.genericMotor | `0.250000` | `0.250000` | PASS |
| `jointType.getRTMotorSpeed` | joint.config.resetMotor | `0.000000` | `0.000000` | PASS |
| `jointType.getRTMotorSpeed` | joint.config.pinMotor | `0.000000` | `0.000000` | PASS |
| `jointType.getRTMotorSpeed` | joint.config.prismaticTranslation | `0.000000` | `0.000000` | PASS |
| `jointType.getRTMotorSpeed` | joint.config.prismaticMotor | `0.000000` | `0.000000` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.pinRtSpeed | `0.000000` | `0.000000` | PASS |
| `jointType.getRTMotorSpeed` | joint.control.prismaticRtSpeed | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.solid.t0.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t0.trace | `AC` | `AC` | PASS |
| `Physics.raycastSimple` | raymove.solid.t0.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t0.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t0.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.solid.t0.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t0.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t0.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t0.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t0.trace | `ABC` | `ABC` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t0.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t0.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t0.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.sensor.t0.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t0.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t0.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t0.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t0.trace | `ABC` | `ABC` | PASS |
| `Physics.raycastSimple` | raymove.inside.t0.simple | `0.400000` | `0.400000` | PASS |
| `Physics.raycastOne` | raymove.inside.t0.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t0.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.inside.t0.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t0.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t0.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t0.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t0.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t0.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t0.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t0.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t0.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t0.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t0.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t1.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t1.trace | `AC` | `AC` | PASS |
| `Physics.raycastSimple` | raymove.solid.t1.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t1.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t1.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.solid.t1.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t1.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t1.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t1.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t1.trace | `ABC` | `ABC` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t1.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t1.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t1.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.sensor.t1.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t1.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t1.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t1.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t1.trace | `ABC` | `ABC` | PASS |
| `Physics.raycastSimple` | raymove.inside.t1.simple | `0.400000` | `0.400000` | PASS |
| `Physics.raycastOne` | raymove.inside.t1.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t1.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.inside.t1.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t1.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t1.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t1.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t1.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t1.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t1.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t1.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t1.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t1.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t1.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t2.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t2.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t2.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t2.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t2.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t2.lambda | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t2.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t2.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t2.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t2.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t2.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.sensor.t2.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t2.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.sensor.t2.lambda | `0.327273` | `0.327273` | PASS |
| `Physics.raycastOne` | raymove.sensor.t2.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t2.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t2.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t2.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t2.simple | `0.400000` | `0.400000` | PASS |
| `Physics.raycastOne` | raymove.inside.t2.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t2.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.inside.t2.lambda | `0.177778` | `0.177778` | PASS |
| `Physics.raycastOne` | raymove.inside.t2.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t2.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t2.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t2.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t2.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t2.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t2.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t2.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t2.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t2.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t3.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t3.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t3.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t3.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t3.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t3.lambda | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t3.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t3.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t3.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t3.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t3.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.sensor.t3.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t3.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.sensor.t3.lambda | `0.327273` | `0.327273` | PASS |
| `Physics.raycastOne` | raymove.sensor.t3.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t3.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t3.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t3.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t3.simple | `0.400000` | `0.400000` | PASS |
| `Physics.raycastOne` | raymove.inside.t3.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t3.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.inside.t3.lambda | `0.177778` | `0.177778` | PASS |
| `Physics.raycastOne` | raymove.inside.t3.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t3.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t3.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t3.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t3.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t3.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t3.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t3.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t3.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t3.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t4.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t4.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t4.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t4.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t4.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t4.lambda | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t4.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t4.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t4.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t4.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t4.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.sensor.t4.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t4.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.sensor.t4.lambda | `0.236364` | `0.236364` | PASS |
| `Physics.raycastOne` | raymove.sensor.t4.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t4.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t4.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t4.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t4.simple | `0.400000` | `0.400000` | PASS |
| `Physics.raycastOne` | raymove.inside.t4.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t4.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.inside.t4.lambda | `0.066667` | `0.066667` | PASS |
| `Physics.raycastOne` | raymove.inside.t4.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t4.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t4.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t4.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t4.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t4.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t4.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t4.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t4.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t4.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t5.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t5.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t5.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t5.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t5.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t5.lambda | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.solid.t5.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t5.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t5.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t5.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t5.simple | `0.509091` | `0.509091` | PASS |
| `Physics.raycastOne` | raymove.sensor.t5.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t5.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.sensor.t5.lambda | `0.236364` | `0.236364` | PASS |
| `Physics.raycastOne` | raymove.sensor.t5.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t5.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t5.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t5.trace | `BCA` | `BCA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t5.simple | `0.400000` | `0.400000` | PASS |
| `Physics.raycastOne` | raymove.inside.t5.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t5.entity | `B` | `B` | PASS |
| `Physics.raycastOne` | raymove.inside.t5.lambda | `0.066667` | `0.066667` | PASS |
| `Physics.raycastOne` | raymove.inside.t5.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t5.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t5.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t5.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t5.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t5.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t5.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t5.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t5.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t5.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t6.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t6.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t6.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t6.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t6.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t6.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t6.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t6.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t6.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t6.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t6.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t6.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t6.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.sensor.t6.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t6.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t6.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t6.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t6.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t6.simple | `0.844444` | `0.844444` | PASS |
| `Physics.raycastOne` | raymove.inside.t6.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t6.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.inside.t6.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t6.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t6.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t6.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t6.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t6.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t6.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t6.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t6.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t6.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t6.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t7.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t7.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t7.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t7.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t7.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t7.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t7.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t7.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t7.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t7.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t7.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t7.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t7.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.sensor.t7.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t7.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t7.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t7.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t7.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t7.simple | `0.844444` | `0.844444` | PASS |
| `Physics.raycastOne` | raymove.inside.t7.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t7.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.inside.t7.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t7.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t7.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t7.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t7.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t7.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t7.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t7.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t7.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t7.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t7.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t8.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t8.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t8.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t8.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t8.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t8.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t8.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t8.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t8.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t8.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t8.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t8.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t8.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.sensor.t8.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t8.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t8.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t8.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t8.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t8.simple | `0.844444` | `0.844444` | PASS |
| `Physics.raycastOne` | raymove.inside.t8.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t8.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.inside.t8.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t8.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t8.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t8.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t8.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t8.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t8.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t8.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t8.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t8.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t8.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t9.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t9.trace | `CA` | `CA` | PASS |
| `Physics.raycastSimple` | raymove.solid.t9.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t9.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t9.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t9.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t9.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t9.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t9.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t9.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t9.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t9.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t9.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.sensor.t9.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t9.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t9.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t9.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t9.trace | `CBA` | `CBA` | PASS |
| `Physics.raycastSimple` | raymove.inside.t9.simple | `0.844444` | `0.844444` | PASS |
| `Physics.raycastOne` | raymove.inside.t9.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t9.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.inside.t9.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t9.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t9.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t9.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t9.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t9.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t9.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t9.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t9.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t9.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t9.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t10.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t10.trace | `AC` | `AC` | PASS |
| `Physics.raycastSimple` | raymove.solid.t10.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t10.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t10.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t10.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t10.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t10.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t10.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t10.trace | `ACB` | `ACB` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t10.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t10.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t10.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.sensor.t10.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t10.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t10.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t10.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t10.trace | `ACB` | `ACB` | PASS |
| `Physics.raycastSimple` | raymove.inside.t10.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t10.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t10.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.inside.t10.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t10.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t10.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t10.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t10.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t10.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t10.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t10.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t10.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t10.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t10.ny | `-1` | `-1` | PASS |
| `Physics.raycast` | raymove.solid.t11.count | `2` | `2` | PASS |
| `Physics.raycast` | raymove.solid.t11.trace | `AC` | `AC` | PASS |
| `Physics.raycastSimple` | raymove.solid.t11.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t11.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.solid.t11.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.solid.t11.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.solid.t11.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.solid.t11.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.sensor.t11.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.sensor.t11.trace | `ACB` | `ACB` | PASS |
| `Physics.raycastSimple` | raymove.sensor.t11.simple | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t11.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.sensor.t11.entity | `C` | `C` | PASS |
| `Physics.raycastOne` | raymove.sensor.t11.lambda | `0.145455` | `0.145455` | PASS |
| `Physics.raycastOne` | raymove.sensor.t11.nx | `-1.000000` | `-1.000000` | PASS |
| `Physics.raycastOne` | raymove.sensor.t11.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.inside.t11.count | `3` | `3` | PASS |
| `Physics.raycast` | raymove.inside.t11.trace | `ACB` | `ACB` | PASS |
| `Physics.raycastSimple` | raymove.inside.t11.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t11.hit | `1` | `1` | PASS |
| `Physics.raycastOne` | raymove.inside.t11.entity | `A` | `A` | PASS |
| `Physics.raycastOne` | raymove.inside.t11.lambda | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t11.nx | `0.000000` | `0.000000` | PASS |
| `Physics.raycastOne` | raymove.inside.t11.ny | `0.000000` | `0.000000` | PASS |
| `Physics.raycast` | raymove.miss.t11.count | `0` | `0` | PASS |
| `Physics.raycast` | raymove.miss.t11.trace | `` | `` | PASS |
| `Physics.raycastSimple` | raymove.miss.t11.simple | `1.000000` | `1.000000` | PASS |
| `Physics.raycastOne` | raymove.miss.t11.hit | `0` | `0` | PASS |
| `Physics.raycastOne` | raymove.miss.t11.entity | `none` | `none` | PASS |
| `Physics.raycastOne` | raymove.miss.t11.lambda | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t11.nx | `-1` | `-1` | PASS |
| `Physics.raycastOne` | raymove.miss.t11.ny | `-1` | `-1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t0.aValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t0.bValid | `0` | `0` | PASS |
| `Entity.isEnabled` | entityLife.pair.t0.aEnabled | `1` | `1` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t0.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t0.aColliding | `0` | `0` | PASS |
| `Entity callbacks` | entityLife.pair.t0.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t0.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t0.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t0.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t0.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t0.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t0.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t0.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t0.bS | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t1.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t1.bValid | `1` | `1` | PASS |
| `Entity.isEnabled` | entityLife.pair.t1.aEnabled | `1` | `1` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t1.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t1.aColliding | `0` | `0` | PASS |
| `Entity callbacks` | entityLife.pair.t1.trace | `` | `` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t1.aP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t1.aC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t1.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t1.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t1.bP | `0` | `0` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t1.bC | `0` | `0` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t1.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t1.bS | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t2.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t2.bValid | `1` | `1` | PASS |
| `Entity.isEnabled` | entityLife.pair.t2.aEnabled | `0` | `0` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t2.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t2.aColliding | `1` | `1` | PASS |
| `Entity callbacks` | entityLife.pair.t2.trace | `aPbPaCbC` | `aPbPaCbC` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t2.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t2.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t2.aT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t2.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t2.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t2.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t2.bT | `0` | `0` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t2.bS | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t3.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t3.bValid | `1` | `1` | PASS |
| `Entity.isEnabled` | entityLife.pair.t3.aEnabled | `0` | `0` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t3.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t3.aColliding | `1` | `1` | PASS |
| `Entity callbacks` | entityLife.pair.t3.trace | `aPbPaCbCbTaT` | `aPbPaCbCbTaT` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t3.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t3.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t3.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t3.aS | `0` | `0` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t3.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t3.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t3.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t3.bS | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t4.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t4.bValid | `1` | `1` | PASS |
| `Entity.isEnabled` | entityLife.pair.t4.aEnabled | `0` | `0` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t4.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t4.aColliding | `0` | `0` | PASS |
| `Entity callbacks` | entityLife.pair.t4.trace | `aPbPaCbCbTaTaSbS` | `aPbPaCbCbTaTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t4.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t4.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t4.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t4.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t4.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t4.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t4.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t4.bS | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t5.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t5.bValid | `1` | `1` | PASS |
| `Entity.isEnabled` | entityLife.pair.t5.aEnabled | `1` | `1` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t5.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t5.aColliding | `0` | `0` | PASS |
| `Entity callbacks` | entityLife.pair.t5.trace | `aPbPaCbCbTaTaSbS` | `aPbPaCbCbTaTaSbS` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t5.aP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t5.aC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t5.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t5.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t5.bP | `1` | `1` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t5.bC | `1` | `1` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t5.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t5.bS | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t6.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.pair.t6.bValid | `1` | `1` | PASS |
| `Entity.isEnabled` | entityLife.pair.t6.aEnabled | `1` | `1` | PASS |
| `physicsData.getNumBodies` | entityLife.pair.t6.aBodies | `1` | `1` | PASS |
| `physicsData.isColliding` | entityLife.pair.t6.aColliding | `1` | `1` | PASS |
| `Entity callbacks` | entityLife.pair.t6.trace | `aPbPaCbCbTaTaSbSaPbPaCbC` | `aPbPaCbCbTaTaSbSaPbPaCbC` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t6.aP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t6.aC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t6.aT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t6.aS | `1` | `1` | PASS |
| `Entity.onEntityCollidePreSolve` | entityLife.pair.t6.bP | `2` | `2` | PASS |
| `Entity.onEntityCollide` | entityLife.pair.t6.bC | `2` | `2` | PASS |
| `Entity.onEntityTouch` | entityLife.pair.t6.bT | `1` | `1` | PASS |
| `Entity.onEntitySeparate` | entityLife.pair.t6.bS | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.delete.request.aValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.delete.request.bValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.delete.afterFlush.aValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityLife.delete.afterFlush.bValid | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t0.parentUpdates | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t0.parentValid | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t0.childCreated | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t0.childValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t0.childValidDuringCreate | `0` | `0` | PASS |
| `EntityManager.getNumEntities` | entityMgr.update.t0.childCountDuringCreate | `2` | `2` | PASS |
| `Entity callbacks` | entityMgr.update.t0.childUpdates | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t0.childTagged | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t1.parentUpdates | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t1.parentValid | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t1.childCreated | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t1.childValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t1.childValidDuringCreate | `0` | `0` | PASS |
| `EntityManager.getNumEntities` | entityMgr.update.t1.childCountDuringCreate | `2` | `2` | PASS |
| `Entity callbacks` | entityMgr.update.t1.childUpdates | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t1.childTagged | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t2.parentUpdates | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t2.parentValid | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t2.childCreated | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t2.childValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t2.childValidDuringCreate | `0` | `0` | PASS |
| `EntityManager.getNumEntities` | entityMgr.update.t2.childCountDuringCreate | `2` | `2` | PASS |
| `Entity callbacks` | entityMgr.update.t2.childUpdates | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t2.childTagged | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t3.parentUpdates | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t3.parentValid | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t3.childCreated | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t3.childValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t3.childValidDuringCreate | `0` | `0` | PASS |
| `EntityManager.getNumEntities` | entityMgr.update.t3.childCountDuringCreate | `2` | `2` | PASS |
| `Entity callbacks` | entityMgr.update.t3.childUpdates | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t3.childTagged | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t4.parentUpdates | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t4.parentValid | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.update.t4.childCreated | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t4.childValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.update.t4.childValidDuringCreate | `0` | `0` | PASS |
| `EntityManager.getNumEntities` | entityMgr.update.t4.childCountDuringCreate | `2` | `2` | PASS |
| `Entity callbacks` | entityMgr.update.t4.childUpdates | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.update.t4.childTagged | `1` | `1` | PASS |
| `EntityManager.createEntity` | entityMgr.create.count | `3` | `3` | PASS |
| `EntityManager.getNumEntities` | entityMgr.getNumEntities.pending | `3` | `3` | PASS |
| `EntityManager.getNumEntities` | entityMgr.getNumEntities.totalPending | `3` | `3` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.isEntityIDValid.pending | `0` | `0` | PASS |
| `EntityManager.getEntity` | entityMgr.getEntity.pendingSame | `1` | `1` | PASS |
| `EntityManager.getAllEntities` | entityMgr.getAllEntities.pendingSize | `0` | `0` | PASS |
| `EntityManager.getNumEntities` | entityMgr.getNumEntities.committed | `3` | `3` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.isEntityIDValid.committed | `1` | `1` | PASS |
| `EntityManager.getEntity` | entityMgr.getEntity.committedSame | `1` | `1` | PASS |
| `EntityManager.getAllEntities` | entityMgr.getAllEntities.snapshotSize | `0` | `0` | PASS |
| `EntityManager.getAllEntities` | entityMgr.getAllEntities.refilledSize | `3` | `3` | PASS |
| `EntityManager.getAllEntities` | entityMgr.getAllEntities.firstValid | `1` | `1` | PASS |
| `EntityListRef.isValid` | entityMgr.getAllEntities.sentinelValid | `0` | `0` | PASS |
| `EntityListRef.goToNext` | entityMgr.getAllEntities.wrapValid | `1` | `1` | PASS |
| `EntityListRef.get` | entityMgr.getAllEntities.wrapFirst | `1` | `1` | PASS |
| `Entity.isTaggedForDeletion` | entityMgr.tagForDeletion.tagged | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.tagForDeletion.stillValid | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityMgr.tagForDeletion.countBeforeFlush | `3` | `3` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.flush.aRemoved | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.flush.cRemovedByObserver | `1` | `1` | PASS |
| `EntityListRef.size` | entityMgr.iteratorAfterDelete.size | `1` | `1` | PASS |
| `EntityListRef.get` | entityMgr.iteratorAfterDelete.currentNull | `1` | `1` | PASS |
| `EntityListRef.goToNext` | entityMgr.iteratorAfterDelete.nextIsB | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.flush.bRetained | `1` | `1` | PASS |
| `Entity callbacks` | entityMgr.flush.destroyCount | `2` | `2` | PASS |
| `Entity callbacks` | entityMgr.flush.destroyOrder | `a;c;` | `a;c;` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.flush.destructorSpawnPending | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityMgr.flush.pendingCount | `2` | `2` | PASS |
| `EntityManager.getAllEntities` | entityMgr.flush.committedCount | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.flush.destructorSpawnCommitted | `1` | `1` | PASS |
| `EntityListRef.size` | entityMgr.iteratorAfterAdd.size | `2` | `2` | PASS |
| `EntityListRef.get` | entityMgr.iteratorAfterAdd.currentTag | `b` | `b` | PASS |
| `EntityListRef.goToNext` | entityMgr.iteratorAfterAdd.step1Tag | `destructorSpawn` | `destructorSpawn` | PASS |
| `EntityListRef.goToNext` | entityMgr.iteratorAfterAdd.step2Tag | `null` | `null` | PASS |
| `EntityListRef.goToNext` | entityMgr.iteratorAfterAdd.step3Tag | `b` | `b` | PASS |
| `EntityListRef.size` | entityMgr.iteratorAfterAdd.foreignFiltered | `1` | `1` | PASS |
| `EntityListRef.size` | entityMgr.iteratorEmptyStayedEmpty | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.deleteAll.activeStillValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.deleteAll.pendingStillInvalid | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityMgr.deleteAll.countBeforeFlush | `3` | `3` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.deleteAll.bRemoved | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityMgr.deleteAll.dRemoved | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityMgr.deleteAll.countAfterFlush | `0` | `0` | PASS |
| `Entity callbacks` | entityMgr.deleteAll.destroyCount | `5` | `5` | PASS |
| `EntityManager.getEntity` | entityQ.id.sameObject | `1` | `1` | PASS |
| `EntityManager.getEntityFromUID` | entityQ.id.uidLookup | `1` | `1` | PASS |
| `Entity.getObjectID` | entityQ.id.objectStable | `1` | `1` | PASS |
| `EntityManager.getEntityFromUID` | entityQ.id.uidMissing | `1` | `1` | PASS |
| `EntityManager.getEntity` | entityQ.id.missingObjectID | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityQ.count.all | `5` | `5` | PASS |
| `EntityManager.getNumManualEntities` | entityQ.count.manual | `1` | `1` | PASS |
| `EntityManager.getNumNonManualEntities` | entityQ.count.nonManual | `4` | `4` | PASS |
| `EntityManager.getNumPlayers` | entityQ.count.players | `1` | `1` | PASS |
| `EntityManager.getAllEntities` | entityQ.filter.all | `5` | `5` | PASS |
| `EntityManager.getAllManualEntities` | entityQ.filter.manual | `1` | `1` | PASS |
| `EntityManager.getAllNonManualEntities` | entityQ.filter.nonManual | `4` | `4` | PASS |
| `EntityManager.getAllPlayerEntities` | entityQ.filter.players | `1` | `1` | PASS |
| `EntityManager.getAllPlayerEntitiesRandomly` | entityQ.filter.playersRandom | `1` | `1` | PASS |
| `EntityManager.getRandomEntity` | entityQ.filter.randomType | `1` | `1` | PASS |
| `EntityManager.getRandomPlayer` | entityQ.filter.randomPlayer | `1` | `1` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.size | `2` | `2` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.a | `1` | `1` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.b | `1` | `1` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.farExcluded | `1` | `1` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.negativeEmpty | `0` | `0` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.reuseSize | `1` | `1` | PASS |
| `EntityManager.getEntitiesNear` | entityQ.near.reuseFar | `1` | `1` | PASS |
| `EntityManager.getEntitiesRoughlyWithin` | entityQ.box.inclusive | `1` | `1` | PASS |
| `EntityManager.getEntitiesRoughlyWithin` | entityQ.box.bOutside | `1` | `1` | PASS |
| `Physics.getBodyFromShapeID` | entityQ.shape.bodyLookup | `1` | `1` | PASS |
| `Physics.getEntityFromShapeID` | entityQ.shape.entityLookup | `1` | `1` | PASS |
| `Physics.isShapeSensor` | entityQ.shape.sensorSolid | `0` | `0` | PASS |
| `Physics.isShapeSensor` | entityQ.shape.sensorSensor | `1` | `1` | PASS |
| `Physics.getShapeByID` | entityQ.shape.lookup | `1` | `1` | PASS |
| `physicsData.getShapeByID` | entityQ.shape.bodyMembership | `1` | `1` | PASS |
| `Physics.getShapesNear` | entityQ.shape.nearCount | `2` | `2` | PASS |
| `Physics.getShapesWithin` | entityQ.shape.exactCount | `1` | `1` | PASS |
| `Physics.getShapesRoughlyWithin` | entityQ.shape.boxHasA | `1` | `1` | PASS |
| `Physics.getShapesRoughlyWithin` | entityQ.shape.boxExcludesB | `1` | `1` | PASS |
| `Physics.getShapesNear` | entityQ.shape.negativeEmpty | `0` | `0` | PASS |
| `Physics.raycastShapes` | entityQ.shape.rayCount | `2` | `2` | PASS |
| `Physics.raycastShapes` | entityQ.shape.rayHasA | `0` | `0` | PASS |
| `Physics.setNumFramesToIgnoreCollisions` | entityQ.collisionIgnore.setGet | `3` | `3` | PASS |
| `Physics.setNumFramesToIgnoreCollisions` | entityQ.collisionIgnore.reset | `0` | `0` | PASS |
| `EntityManager.getAllEntities` | entityQ.iterator.snapshotStable | `0` | `0` | PASS |
| `EntityManager.getAllEntities` | entityQ.iterator.newEntityExcluded | `0` | `0` | PASS |
| `EntityManager.getAllEntities` | entityQ.iterator.resetFirst | `1` | `1` | PASS |
| `EntityManager.getAllEntities` | entityQ.iterator.deletedSnapshotSizeStable | `1` | `1` | PASS |
| `EntityManager.getAllEntities` | entityQ.iterator.deletedEntryNull | `1` | `1` | PASS |
| `ALARM.getMSElapsed` | entityState.alarmElapsedDisabled | `50.000000` | `50.000000` | PASS |
| `ALARM.getMSElapsed` | entityState.alarmElapsedEarly | `40.000000` | `40.000000` | PASS |
| `ALARM.getMSElapsed` | entityState.alarmElapsedInitial | `0.000000` | `0.000000` | PASS |
| `ALARM.phaseReset` | entityState.alarmElapsedPhase | `50.000000` | `50.000000` | PASS |
| `ALARM.hardReset` | entityState.alarmElapsedReset | `0.000000` | `0.000000` | PASS |
| `ALARM.reset` | entityState.alarmElapsedResetAlias | `0.000000` | `0.000000` | PASS |
| `ALARM.setInterval` | entityState.alarmElapsedSetInterval | `0.000000` | `0.000000` | PASS |
| `ALARM.isEnabled` | entityState.alarmEnabledDisabled | `0` | `0` | PASS |
| `ALARM.isEnabled` | entityState.alarmEnabledReady | `1` | `1` | PASS |
| `ALARM.isAlarming` | entityState.alarmInitiallyReady | `0` | `0` | PASS |
| `ALARM.getInterval` | entityState.alarmInterval | `100.000000` | `100.000000` | PASS |
| `ALARM.setIntervalAndHardReset` | entityState.alarmIntervalReset | `200.000000` | `200.000000` | PASS |
| `ALARM.setInterval` | entityState.alarmIntervalSet | `300.000000` | `300.000000` | PASS |
| `ALARM.getMSElapsed` | entityState.alarmLargeStepElapsed | `250.000000` | `250.000000` | PASS |
| `ALARM.getAlarmPercentage` | entityState.alarmLargeStepPercentage | `1.000000` | `1.000000` | PASS |
| `ALARM.update` | entityState.alarmLargeStepReady | `1` | `1` | PASS |
| `ALARM.phaseReset` | entityState.alarmNegativePhase | `0.000000` | `0.000000` | PASS |
| `ALARM.getAlarmPercentage` | entityState.alarmPercentageReady | `1.000000` | `1.000000` | PASS |
| `ALARM.phaseReset` | entityState.alarmPhaseAboveOne | `100.000000` | `100.000000` | PASS |
| `ALARM.randomReset` | entityState.alarmRandomResetRange | `1` | `1` | PASS |
| `ALARM.isAlarming` | entityState.alarmReady | `1` | `1` | PASS |
| `ALARM.getTimeTillAlarm` | entityState.alarmTimeInitial | `100.000000` | `100.000000` | PASS |
| `ALARM.update` | entityState.alarmUpdateDisabled | `0` | `0` | PASS |
| `ALARM.update` | entityState.alarmUpdateEarly | `0` | `0` | PASS |
| `ALARM.update` | entityState.alarmUpdateReady | `1` | `1` | PASS |
| `ALARM.getAlarmPercentage` | entityState.alarmZeroIntervalPercentage | `1.000000` | `1.000000` | PASS |
| `ALARM.update` | entityState.alarmZeroIntervalReady | `1` | `1` | PASS |
| `Entity.isBeingEdited` | entityState.beingEdited | `1` | `1` | PASS |
| `Entity.callMethod` | entityState.callMethodMissing | `1` | `1` | PASS |
| `Entity.isInvisible` | entityState.clearFlags | `1` | `1` | PASS |
| `Entity.update` | entityState.compatibilityCallsComplete | `1` | `1` | PASS |
| `Entity.createParticle` | entityState.createParticle | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityState.deleted | `1` | `1` | PASS |
| `Entity.getDescriptionBits` | entityState.descriptionBits | `37` | `37` | PASS |
| `Entity.getDistanceTo` | entityState.distance | `5.000000` | `5.000000` | PASS |
| `EntityManager.getDummyEntity` | entityState.dummyStable | `1` | `1` | PASS |
| `Entity.isEditable` | entityState.editable | `1` | `1` | PASS |
| `Entity.isGetting` | entityState.getting | `0` | `0` | PASS |
| `Entity.isGot` | entityState.gotCleared | `1` | `1` | PASS |
| `Entity.isGot` | entityState.gotDefault | `1` | `1` | PASS |
| `Entity.isGot` | entityState.gotSet | `1` | `1` | PASS |
| `Entity.isInvisible` | entityState.invisible | `1` | `1` | PASS |
| `Entity.isType` | entityState.isType | `1` | `1` | PASS |
| `LFO.setContour` | entityState.lfoContourAfter | `0.750000` | `0.750000` | PASS |
| `LFO.setContour` | entityState.lfoContourBefore | `-0.250000` | `-0.250000` | PASS |
| `LFO.update` | entityState.lfoNegativeDelta | `2.500000` | `2.500000` | PASS |
| `LFO.setPhase` | entityState.lfoNegativePhase | `-2.000000` | `-2.000000` | PASS |
| `Global.TRIANGLELFO` | entityState.lfoObjectNonNull | `1` | `1` | PASS |
| `LFO.setRange` | entityState.lfoPhase0 | `-1.000000` | `-1.000000` | PASS |
| `LFO.setPhase` | entityState.lfoPhaseAboveOne | `4.000000` | `4.000000` | PASS |
| `LFO.setPhase` | entityState.lfoPhaseHalf | `0.000000` | `0.000000` | PASS |
| `LFO.setPhase` | entityState.lfoPhaseQuarter | `-0.500000` | `-0.500000` | PASS |
| `LFO.setPhase` | entityState.lfoPhaseThreeQuarter | `0.500000` | `0.500000` | PASS |
| `LFO.setRate` | entityState.lfoRate | `50.000000` | `50.000000` | PASS |
| `LFO.setRate` | entityState.lfoRate100 | `25.000000` | `25.000000` | PASS |
| `LFO.setRate` | entityState.lfoRate400 | `100.000000` | `100.000000` | PASS |
| `LFO.setRate` | entityState.lfoRate50 | `12.500000` | `12.500000` | PASS |
| `LFO.update` | entityState.lfoUpdatedValue | `0.500000` | `0.500000` | PASS |
| `LFO.getValue` | entityState.lfoValue | `-0.500000` | `-0.500000` | PASS |
| `LFO.update` | entityState.lfoZeroDelta | `2.500000` | `2.500000` | PASS |
| `LFO.setRate` | entityState.lfoZeroRate | `3.500000` | `3.500000` | PASS |
| `LFO.update` | entityState.lfoZeroRateAfterUpdate | `3.500000` | `3.500000` | PASS |
| `LFO.setPhase` | entityState.lfoZeroRateAtQuarter | `2.500000` | `2.500000` | PASS |
| `LFO.getValue` | entityState.lfoZeroRateBeforeStep | `3.500000` | `3.500000` | PASS |
| `LFO.setPhase` | entityState.lfoZeroRateFreshInitial | `2.500000` | `2.500000` | PASS |
| `LFO.update` | entityState.lfoZeroRateFreshLargeStep | `3.500000` | `3.500000` | PASS |
| `LFO.setRate` | entityState.lfoZeroRateFreshReadback | `0.250000` | `0.250000` | PASS |
| `LFO.update` | entityState.lfoZeroRateFreshSmallStep | `3.000000` | `3.000000` | PASS |
| `LFO.setRate` | entityState.lfoZeroRateReadback | `0.250000` | `0.250000` | PASS |
| `LFO.update` | entityState.lfoZeroRateSmallStep | `-2.000000` | `-2.000000` | PASS |
| `Entity.getLimitVolPerc` | entityState.limitVolPerc | `0.375000` | `0.375000` | PASS |
| `Entity.getMixerID` | entityState.mixerID | `7` | `7` | PASS |
| `Entity.is_getAudibilityPerc_neverOnScreen` | entityState.neverOnScreen | `1` | `1` | PASS |
| `Entity.isOkToRun` | entityState.notOkToRun | `1` | `1` | PASS |
| `Entity.getType` | entityState.numericType | `1` | `1` | PASS |
| `Entity.getObjectID` | entityState.objectIDsDistinct | `1` | `1` | PASS |
| `Entity.getNumOffScreensAudible` | entityState.offScreensAudible | `3` | `3` | PASS |
| `Entity.isOkToRun` | entityState.okToRun | `1` | `1` | PASS |
| `Entity.isOnScreen` | entityState.onScreenInside | `1` | `1` | PASS |
| `Entity.isOnScreen` | entityState.onScreenOutside | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityState.otherDeleted | `1` | `1` | PASS |
| `Entity.getNumParticles` | entityState.particleCount | `null` | `null` | PASS |
| `Entity.getAllParticles` | entityState.particleIterator | `null` | `null` | PASS |
| `EntityManager.getEntityPerfCost` | entityState.perfAdditional | `4` | `4` | PASS |
| `EntityManager.getEntityPerfCost` | entityState.perfFirst | `10` | `10` | PASS |
| `EntityManager.getEntityPerfMaxCost` | entityState.perfMax | `42` | `42` | PASS |
| `EntityManager.getEntityPerfCost` | entityState.perfMissing | `0` | `0` | PASS |
| `Entity.isPlacing` | entityState.placing | `1` | `1` | PASS |
| `Entity.getRadiansToFace` | entityState.radiansToFace | `0.927295` | `0.927295` | PASS |
| `Entity.isSameType` | entityState.sameType | `1` | `1` | PASS |
| `Entity.isSauceBase` | entityState.sauceBase | `1` | `1` | PASS |
| `Entity.getMaxCX` | entityState.screenMaxX | `4` | `4` | PASS |
| `Entity.getMaxCY` | entityState.screenMaxY | `4` | `4` | PASS |
| `Entity.getMinCX` | entityState.screenMinX | `2` | `2` | PASS |
| `Entity.getMinCY` | entityState.screenMinY | `2` | `2` | PASS |
| `Entity.isStaticDraw` | entityState.staticDraw | `1` | `1` | PASS |
| `Entity.getTypeName` | entityState.typeName | `EntityStateProbeEntity` | `EntityStateProbeEntity` | PASS |
| `Entity.getVectorToNormalized` | entityState.vectorNormX | `0.600000` | `0.600000` | PASS |
| `Entity.getVectorToNormalized` | entityState.vectorNormY | `0.800000` | `0.800000` | PASS |
| `Entity.getVectorTo` | entityState.vectorToX | `3.000000` | `3.000000` | PASS |
| `Entity.getVectorTo` | entityState.vectorToY | `4.000000` | `4.000000` | PASS |
| `Entity.isType` | entityState.wrongType | `1` | `1` | PASS |
| `Entity.isEnabled` | entityCallbacks.initialEnabled | `0` | `0` | PASS |
| `Entity.isEnabled` | entityCallbacks.enabledAfterSet | `1` | `1` | PASS |
| `Entity.setIsEnabled` | entityCallbacks.orderAfterSetEnable | `` | `` | PASS |
| `Entity.onEnable` | entityCallbacks.enabledAfterCallback | `1` | `1` | PASS |
| `Entity.onEnable` | entityCallbacks.orderAfterEnable | `E` | `E` | PASS |
| `Entity.onEnable` | entityCallbacks.enabledAfterRepeat | `2` | `2` | PASS |
| `Entity.isEnabled` | entityCallbacks.disabledAfterSet | `1` | `1` | PASS |
| `Entity.setIsEnabled` | entityCallbacks.orderAfterSetDisable | `EE` | `EE` | PASS |
| `Entity.onDisable` | entityCallbacks.disabledAfterCallback | `1` | `1` | PASS |
| `Entity.onDisable` | entityCallbacks.orderAfterDisable | `EED` | `EED` | PASS |
| `Entity.onEnable` | entityCallbacks.noHookEnable | `1` | `1` | PASS |
| `Entity.onDisable` | entityCallbacks.noHookDisable | `1` | `1` | PASS |
| `Entity.onEnable` | entityCallbacks.finalOrder | `EED` | `EED` | PASS |
| `GameMonkeyScript.clean` | entityGC.scheduled | `1` | `1` | PASS |
| `GameMonkeyScript.isCollectingGarbage` | entityGC.collectingBeforeTick | `1` | `1` | PASS |
| `GameMonkeyScript.isCollectingGarbage` | entityGC.collectingAfterTick | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityGC.activeAfterGC | `1` | `1` | PASS |
| `EntityManager.getEntity` | entityGC.lookupAfterGC | `1` | `1` | PASS |
| `physicsBody.getPos` | entityGC.bodyAfterGC | `7.000000` | `7.000000` | PASS |
| `EntityManager.flush` | entityGC.deletedBeforeGC | `1` | `1` | PASS |
| `GameMonkeyScript.isCollectingGarbage` | entityGC.collectingAfterDeleteGC | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityGC.deletedAfterGC | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityGCCycles.firstCount | `3` | `3` | PASS |
| `physicsBody.getPos` | entityGCCycles.firstAX | `2.000000` | `2.000000` | PASS |
| `physicsBody.getPos` | entityGCCycles.firstBX | `4.000000` | `4.000000` | PASS |
| `physicsBody.getPos` | entityGCCycles.firstCX | `6.000000` | `6.000000` | PASS |
| `EntityManager.getNumEntities` | entityGCCycles.afterDeleteCount | `2` | `2` | PASS |
| `EntityManager.isEntityIDValid` | entityGCCycles.afterDeleteAValid | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | entityGCCycles.afterDeleteBValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityGCCycles.afterDeleteCValid | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityGCCycles.recreateCount | `3` | `3` | PASS |
| `physicsBody.getPos` | entityGCCycles.recreateDX | `8.000000` | `8.000000` | PASS |
| `physicsBody.getPos` | entityGCCycles.recreateAX | `2.000000` | `2.000000` | PASS |
| `EntityManager.getNumEntities` | entityGCCycles.clearCount | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityGCCycles.clearAValid | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | entityGCCycles.clearDValid | `0` | `0` | PASS |
| `EntityListRef.size` | entityGCIter.retainedSize | `1` | `1` | PASS |
| `EntityListRef.get` | entityGCIter.retainedEntity | `1` | `1` | PASS |
| `EntityListRef.size` | entityGCIter.deletedSize | `0` | `0` | PASS |
| `EntityListRef.get` | entityGCIter.deletedEntryNull | `1` | `1` | PASS |
| `EntityListRef.get` | entityGCIter.nullAfterGC | `1` | `1` | PASS |
| `EntityListRef.size` | entityGCIter.newIteratorSize | `1` | `1` | PASS |
| `EntityListRef.size` | entityGCIter.reusedIteratorSize | `2` | `2` | PASS |
| `EntityListRef.get` | entityGCIter.secondFound | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | entityGCIter.clearCount | `0` | `0` | PASS |
| `GameSystem.runProcess` | process.created | `1` | `1` | PASS |
| `GameSystem.runProcess` | process.constructed | `1` | `1` | PASS |
| `GameSystem.isProcessRunning` | process.running | `1` | `1` | PASS |
| `GameSystem.getProcess` | process.same | `1` | `1` | PASS |
| `GameSystem.getProcessCount` | process.count.created | `1` | `1` | PASS |
| `GameSystem.runProcess` | process.initialized | `1` | `1` | PASS |
| `GameSystem.runProcess` | process.updates.before | `0` | `0` | PASS |
| `GameSystem.runProcess` | process.posts.before | `1` | `1` | PASS |
| `GameSystem.runProcess` | process.updates.after | `1` | `1` | PASS |
| `Process.terminate` | process.terminate.queued.running | `1` | `1` | PASS |
| `Process.terminate` | process.terminate.queued.destroyed | `0` | `0` | PASS |
| `Process.terminate` | process.terminate.queued.count | `2` | `2` | PASS |
| `Process.terminate` | process.destroyed | `1` | `1` | PASS |
| `GameSystem.isProcessRunning` | process.running.after | `0` | `0` | PASS |
| `GameSystem.getProcessCount` | process.count.after | `1` | `1` | PASS |
| `GameSystem.runProcess` | processPause.created | `1` | `1` | PASS |
| `GameSystem.runProcess` | processPause.initA | `1` | `1` | PASS |
| `GameSystem.runProcess` | processPause.initB | `1` | `1` | PASS |
| `GameSystem.runProcess` | processPause.updatesBefore | `0` | `0` | PASS |
| `GameSystem.runProcess` | processPause.postsBefore | `2` | `2` | PASS |
| `GameSystem.runProcess` | processPause.updatesBeforePause | `2` | `2` | PASS |
| `GameSystem.runProcess` | processPause.postsBeforePause | `4` | `4` | PASS |
| `GameSystem.isPaused` | processPause.pauseObserved | `1` | `1` | PASS |
| `GameSystem.isPaused` | processPause.bHeld | `2` | `2` | PASS |
| `GameSystem.isPaused` | processPause.pauseRequestState | `0` | `0` | PASS |
| `GameSystem.isPaused` | processPause.unpauseObserved | `1` | `1` | PASS |
| `GameSystem.isPaused` | processPause.resumed | `1` | `1` | PASS |
| `GameSystem.setAsPauseProcess` | processPause.updatesAfterResume | `8` | `8` | PASS |
| `GameSystem.setAsPauseProcess` | processPause.postsAfterResume | `10` | `10` | PASS |
| `GameSystem.setAsPauseProcess` | processPause.callbackOrder | `M0M1M2A1B1M3A2B2A3B3M4A4B4M5` | `M0M1M2A1B1M3A2B2A3B3M4A4B4M5` | PASS |
| `Process.terminate` | processPause.destroyed | `2` | `2` | PASS |
| `GameSystem.isProcessRunning` | processPause.running | `0` | `0` | PASS |
| `GameSystem.getProcessCount` | processPause.count | `1` | `1` | PASS |
| `Entity callbacks` | callbackMutation.trace | `A` | `A` | PASS |
| `Entity callbacks` | callbackMutation.fired | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | callbackMutation.otherValidBefore | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | callbackMutation.countBefore | `2` | `2` | PASS |
| `EntityManager.isEntityIDValid` | callbackMutation.addedValidPending | `0` | `0` | PASS |
| `EntityManager.flush` | callbackMutation.addedValidAfterCallbackFlush | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | callbackMutation.countAfterCreate | `3` | `3` | PASS |
| `Physics.createJoint` | callbackMutation.jointType | `3` | `3` | PASS |
| `Entity.tagForDeletion` | callbackMutation.otherTagged | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | callbackMutation.otherValidTagged | `1` | `1` | PASS |
| `Entity.tagForDeletion` | callbackMutation.selfTagged | `1` | `1` | PASS |
| `EntityManager.flush` | callbackMutation.otherValidAfterFlush | `0` | `0` | PASS |
| `EntityManager.flush` | callbackMutation.selfValidAfterFlush | `0` | `0` | PASS |
| `EntityManager.flush` | callbackMutation.addedValidAfterFlush | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | callbackMutation.finalCount | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | gcPressure.countAfterCreate | `96` | `96` | PASS |
| `GameMonkeyScript.clean` | gcPressure.rootCount | `6` | `6` | PASS |
| `EntityManager.isEntityIDValid` | gcPressure.firstRootValid | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | gcPressure.countBeforeCollect | `64` | `64` | PASS |
| `EntityManager.isEntityIDValid` | gcPressure.validAfterCollect | `64` | `64` | PASS |
| `EntityManager.isEntityIDValid` | gcPressure.retainedRootValid | `1` | `1` | PASS |
| `EntityManager.getNumEntities` | gcPressure.totalAfterCollect | `64` | `64` | PASS |
| `EntityManager.deleteAllEntities` | gcPressure.emptyAfterClear | `0` | `0` | PASS |
| `EntityManager.isEntityIDValid` | gcPressure.oldIdsInvalid | `1` | `1` | PASS |
| `GameSystem.runProcess` | sceneTransition.oldCreated | `1` | `1` | PASS |
| `GameSystem.getSimulationSpeed` | sceneTransition.speedBefore | `0.500000` | `0.500000` | PASS |
| `GameSystem.isProcessRunning` | sceneTransition.oldRunning | `1` | `1` | PASS |
| `GameSystem.startNewProcessList` | sceneTransition.oldDestroyed | `1` | `1` | PASS |
| `GameSystem.runProcess` | sceneTransition.oldUpdates | `1` | `1` | PASS |
| `GameSystem.isProcessRunning` | sceneTransition.oldProcessGone | `1` | `1` | PASS |
| `GameSystem.isProcessRunning` | sceneTransition.newProcessRunning | `1` | `1` | PASS |
| `EntityManager.isEntityIDValid` | sceneTransition.entitySurvived | `0` | `0` | PASS |
| `GameSystem.startNewProcessList` | sceneTransition.newInitialized | `1` | `1` | PASS |
| `GameSystem.getProcessCount` | sceneTransition.processCount | `1` | `1` | PASS |
| `GameSystem.setSimulationSpeed` | pauseScale.halfSpeed | `0.500000` | `0.500000` | PASS |
| `GameSystem.setSimulationSpeed` | pauseScale.doubleSpeed | `2.000000` | `2.000000` | PASS |
| `GameSystem.setSimulationSpeed` | pauseScale.halfDeltaApplied | `0` | `0` | PASS |
| `GameSystem.setSimulationSpeed` | pauseScale.doubleDeltaApplied | `0` | `0` | PASS |
| `GameSystem.isPaused` | pauseScale.pauseSeen | `1` | `1` | PASS |
| `GameSystem.setAsPauseProcess` | pauseScale.bHeld | `3` | `3` | PASS |
| `GameSystem.isPaused` | pauseScale.resumed | `1` | `1` | PASS |
| `GameSystem.getSimulationSpeed` | pauseScale.finalSpeed | `1.000000` | `1.000000` | PASS |
| `physicsBody.setPos` | bodyCtl.position | `1` | `1` | PASS |
| `physicsBody.setVel` | bodyCtl.velocity | `1` | `1` | PASS |
| `physicsBody.getSpeed` | bodyCtl.speed | `5.000000` | `5.000000` | PASS |
| `physicsBody.setSpeed` | bodyCtl.setSpeed | `10.000000` | `10.000000` | PASS |
| `physicsBody.getDir` | bodyCtl.direction | `1` | `1` | PASS |
| `physicsBody.setMass` | bodyCtl.mass | `3.000000` | `3.000000` | PASS |
| `physicsBody.setMaxSpeed` | bodyCtl.maxSpeed | `12.000000` | `12.000000` | PASS |
| `physicsBody.setMaxRotateSpeed` | bodyCtl.maxRotateSpeed | `2.000000` | `2.000000` | PASS |
| `physicsBody.setFriction` | bodyCtl.friction | `0.250000` | `0.250000` | PASS |
| `physicsBody.setRestitution` | bodyCtl.restitution | `0.750000` | `0.750000` | PASS |
| `physicsBody.setIsSensor` | bodyCtl.sensor | `1` | `1` | PASS |
| `physicsBody.setIsCollidable` | bodyCtl.collidable | `0` | `0` | PASS |
| `physicsBody.setIsBullet` | bodyCtl.bullet | `1.000000` | `1.000000` | PASS |
| `physicsBody.setCollisionCategory` | bodyCtl.category | `4` | `4` | PASS |
| `physicsBody.setCategoriesToCollideWith` | bodyCtl.mask | `7` | `7` | PASS |
| `physicsBody.setCollisionGroup` | bodyCtl.group | `-2` | `-2` | PASS |
| `physicsBody.setAngularVel` | bodyCtl.angularVelocity | `0.500000` | `0.500000` | PASS |
| `physicsBody.getNumShapes` | bodyCtl.shapeCount | `1` | `1` | PASS |
| `physicsBody.isPointWithin` | bodyCtl.pointInside | `1` | `1` | PASS |
| `physicsBody.isPointWithin` | bodyCtl.pointOutside | `0` | `0` | PASS |
| `physicsBody.isSleeping` | bodyCtl.sleeping | `0.000000` | `0.000000` | PASS |
| `Physics.flush` | bodyCtl.flushAfterOne | `200.009995` | `200.009995` | PASS |
| `Physics.flush` | bodyCtl.flushAfterTen | `200.099945` | `200.099945` | PASS |
| `GameSystem.isShouldUpdateEntities` | bodyCtl.updateEntitiesOff | `0` | `0` | PASS |
| `GameSystem.isShouldUpdateEntities` | bodyCtl.updateEntitiesOn | `1` | `1` | PASS |
| `GameSystem.getUpdateInterval` | bodyCtl.updateInterval | `16.666666` | `16.666666` | PASS |
| `physicsData.replaceBody` | bodyCtl.replaceBodyExists | `1` | `1` | PASS |
| `physicsData.getNumBodies` | bodyCtl.replaceBodyCount | `1` | `1` | PASS |
| `physicsBody.getPos` | bodyCtl.replaceBodyPos | `0.000000` | `0.000000` | PASS |
| `physicsData.replaceBody` | bodyCtl.replaceBodyOutOfRangeExists | `1` | `1` | PASS |
| `physicsData.removeBody` | bodyCtl.removeBodyCount | `1` | `1` | PASS |
| `physicsData.createBody` | bodyCtl.removeAllBodyBefore | `2` | `2` | PASS |
| `physicsData.removeBody` | bodyCtl.removeAllBodyAfter | `0` | `0` | PASS |
| `physicsData.appendShapes` | bodyCtl.dataAppendFirst | `0` | `0` | PASS |
| `physicsData.appendShapes` | bodyCtl.dataAppendSecond | `1` | `1` | PASS |
| `physicsBody.getNumShapes` | bodyCtl.dataAppendShapeCount | `2` | `2` | PASS |
| `physicsBody.getDir` | bodyCtl.directionSetX | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getDir` | bodyCtl.directionSetY | `1.000000` | `1.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCtl.rotateVelSet | `0.250000` | `0.250000` | PASS |
| `physicsBody.getAABB` | bodyCtl.bodyBoundsMinX | `3.599999` | `3.599999` | PASS |
| `physicsBody.getAABB` | bodyCtl.bodyBoundsMaxX | `5.599999` | `5.599999` | PASS |
| `physicsBody.getAABB` | bodyCtl.bodyBoundsMinY | `4.799999` | `4.799999` | PASS |
| `physicsBody.getAABB` | bodyCtl.bodyBoundsMaxY | `6.799999` | `6.799999` | PASS |
| `physicsData.getAABB` | bodyCtl.dataBoundsMinX | `3.599999` | `3.599999` | PASS |
| `physicsData.getAABB` | bodyCtl.dataBoundsMaxX | `5.599999` | `5.599999` | PASS |
| `physicsData.getAABB` | bodyCtl.dataBoundsMinY | `4.799999` | `4.799999` | PASS |
| `physicsData.getAABB` | bodyCtl.dataBoundsMaxY | `6.799999` | `6.799999` | PASS |
| `physicsBody.applyForce` | bodyCtl.forceAfterOneX | `0.333333` | `0.333333` | PASS |
| `physicsBody.applyForce` | bodyCtl.forceAfterOneAngular | `-0.333333` | `-0.333333` | PASS |
| `physicsBody.applyForce` | bodyCtl.forceAfterTenX | `0.333333` | `0.333333` | PASS |
| `physicsBody.applyForce` | bodyCtl.forceAfterTenAngular | `-0.333333` | `-0.333333` | PASS |
| `physicsBody.getEntity` | bodyCtl.ownerMatches | `1` | `1` | PASS |
| `physicsBody.getObjectID` | bodyCtl.bodyIDPositive | `1` | `1` | PASS |
| `physicsBody.getShapeObjectID` | bodyCtl.shapeIDPositive | `1` | `1` | PASS |
| `physicsBody.setShapeDensity` | bodyCtl.shapeDensityBefore | `1` | `1` | PASS |
| `physicsBody.setShapeDensity` | bodyCtl.shapeDensityAfter | `1` | `1` | PASS |
| `physicsBody.setShapeDensity` | bodyCtl.shapeDensityValidMass | `6.283185` | `6.283185` | PASS |
| `physicsBody.setShapeDensity` | bodyCtl.shapeDensityInvalidMass | `6.283185` | `6.283185` | PASS |
| `physicsBody.setShapeCategoriesToCollideWith` | bodyCtl.shapeMaskValid | `5` | `5` | PASS |
| `physicsBody.setIsShapeSensor` | bodyCtl.shapeSensorOff | `0` | `0` | PASS |
| `physicsBody.setIsShapeSensor` | bodyCtl.shapeSensorOn | `1` | `1` | PASS |
| `physicsBody.setIsShapeSensor` | bodyCtl.shapeSensorInvalidReached | `1` | `1` | PASS |
| `physicsBody.setIsShapeSensor` | bodyCtl.shapeSensorInvalidContinues | `1` | `1` | PASS |
| `physicsBody.getShapeCategoriesToCollideWith` | bodyCtl.shapeMaskGetterInvalidReached | `1` | `1` | PASS |
| `physicsBody.doesRotationResponse` | bodyApi.physicsBody.doesRotationResponse | `1` | `1` | PASS |
| `physicsBody.getAngularVel` | bodyApi.physicsBody.getAngularVel | `0.500000` | `0.500000` | PASS |
| `physicsBody.getBrakeAcc` | bodyApi.physicsBody.getBrakeAcc | `999.000000` | `999.000000` | PASS |
| `physicsBody.getCategoriesToCollideWith` | bodyApi.physicsBody.getCategoriesToCollideWith | `7` | `7` | PASS |
| `physicsBody.getCollisionCategory` | bodyApi.physicsBody.getCollisionCategory | `4` | `4` | PASS |
| `physicsBody.getCollisionGroup` | bodyApi.physicsBody.getCollisionGroup | `-2` | `-2` | PASS |
| `physicsBody.getFriction` | bodyApi.physicsBody.getFriction | `0.250000` | `0.250000` | PASS |
| `physicsBody.getMass` | bodyApi.physicsBody.getMass | `3.000000` | `3.000000` | PASS |
| `physicsBody.getMaxRotateSpeed` | bodyApi.physicsBody.getMaxRotateSpeed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getMaxSpeed` | bodyApi.physicsBody.getMaxSpeed | `12.000000` | `12.000000` | PASS |
| `physicsBody.getNumShapeVerts` | bodyApi.physicsBody.getNumShapeVerts | `0` | `0` | PASS |
| `physicsBody.getRadius` | bodyApi.physicsBody.getRadius | `1.000000` | `1.000000` | PASS |
| `physicsBody.getRestitution` | bodyApi.physicsBody.getRestitution | `0.750000` | `0.750000` | PASS |
| `physicsBody.getRotateAcc` | bodyApi.physicsBody.getRotateAcc | `999.000000` | `999.000000` | PASS |
| `physicsBody.getRotateVel` | bodyApi.physicsBody.getRotateVel | `0.500000` | `0.500000` | PASS |
| `physicsBody.getShapeCategoriesToCollideWith` | bodyApi.physicsBody.getShapeCategoriesToCollideWith | `7` | `7` | PASS |
| `physicsBody.getShapeHeight` | bodyApi.physicsBody.getShapeHeight | `2.000000` | `2.000000` | PASS |
| `physicsBody.getShapeRadius` | bodyApi.physicsBody.getShapeRadius | `1.000000` | `1.000000` | PASS |
| `physicsBody.getShapeType` | bodyApi.physicsBody.getShapeType | `0` | `0` | PASS |
| `physicsBody.getShapeWidth` | bodyApi.physicsBody.getShapeWidth | `2.000000` | `2.000000` | PASS |
| `physicsBody.getCenterOfMass` | bodyApi.physicsBody.getCenterOfMass | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPosRef` | bodyApi.physicsBody.getPosRef | `4.000000` | `4.000000` | PASS |
| `physicsBody.getPosVec` | bodyApi.physicsBody.getPosVec | `5.000000` | `5.000000` | PASS |
| `physicsBody.getVelRef` | bodyApi.physicsBody.getVelRef | `6.000000` | `6.000000` | PASS |
| `physicsBody.getWidth` | bodyApi.physicsBody.getWidth | `2.000000` | `2.000000` | PASS |
| `physicsBody.getHeight` | bodyApi.physicsBody.getHeight | `2.000000` | `2.000000` | PASS |
| `physicsBody.getMinPos` | bodyApi.physicsBody.getMinPos | `3.000000` | `3.000000` | PASS |
| `physicsBody.getMaxPos` | bodyApi.physicsBody.getMaxPos | `5.000000` | `5.000000` | PASS |
| `physicsBody.getShapeIndex` | bodyApi.physicsBody.getShapeIndex | `0` | `0` | PASS |
| `physicsBody.getShapeOffset` | bodyApi.physicsBody.getShapeOffset | `0.000000` | `0.000000` | PASS |
| `physicsBody.getShapeWorldCenterPos` | bodyApi.physicsBody.getShapeWorldCenterPos | `4.000000` | `4.000000` | PASS |
| `physicsBody.getShapeWorldVert` | bodyApi.physicsBody.getShapeWorldVert | `4.000000` | `4.000000` | PASS |
| `physicsBody.getWorldDir` | bodyApi.physicsBody.getWorldDir | `1.000000` | `1.000000` | PASS |
| `physicsBody.getWorldPos` | bodyApi.physicsBody.getWorldPos | `5.000000` | `5.000000` | PASS |
| `physicsBody.isBullet` | bodyApi.physicsBody.isBullet | `1.000000` | `1.000000` | PASS |
| `physicsBody.isSensor` | bodyApi.physicsBody.isSensor | `1` | `1` | PASS |
| `physicsBody.getSpeed` | bodyApi.physicsBody.getSpeed | `10.000000` | `10.000000` | PASS |
| `physicsBody.getThrustAcc` | bodyApi.physicsBody.getThrustAcc | `999.000000` | `999.000000` | PASS |
| `physicsBody.isContainsShape` | bodyApi.physicsBody.isContainsShape | `1` | `1` | PASS |
| `physicsBody.isPointWithinShape` | bodyApi.physicsBody.isPointWithinShape | `1` | `1` | PASS |
| `physicsBody.isShapeSensor` | bodyApi.physicsBody.isShapeSensor | `1` | `1` | PASS |
| `physicsData.doesRotationResponse` | bodyApi.physicsData.doesRotationResponse | `1` | `1` | PASS |
| `physicsData.getBrakeAcc` | bodyApi.physicsData.getBrakeAcc | `null` | `null` | PASS |
| `physicsData.getCategoriesToCollideWith` | bodyApi.physicsData.getCategoriesToCollideWith | `null` | `null` | PASS |
| `physicsData.getCollisionCategory` | bodyApi.physicsData.getCollisionCategory | `null` | `null` | PASS |
| `physicsData.getCollisionGroup` | bodyApi.physicsData.getCollisionGroup | `null` | `null` | PASS |
| `physicsData.getHeight` | bodyApi.physicsData.getHeight | `2.000000` | `2.000000` | PASS |
| `physicsData.getMass` | bodyApi.physicsData.getMass | `3.000000` | `3.000000` | PASS |
| `physicsData.getMaxRotateSpeed` | bodyApi.physicsData.getMaxRotateSpeed | `2.000000` | `2.000000` | PASS |
| `physicsData.getMaxSpeed` | bodyApi.physicsData.getMaxSpeed | `null` | `null` | PASS |
| `physicsData.getNumBodies` | bodyApi.physicsData.getNumBodies | `1` | `1` | PASS |
| `physicsData.getRadius` | bodyApi.physicsData.getRadius | `1.000000` | `1.000000` | PASS |
| `physicsData.getRotateAcc` | bodyApi.physicsData.getRotateAcc | `999.000000` | `999.000000` | PASS |
| `physicsData.getShapeRadius` | bodyApi.physicsData.getShapeRadius | `1.000000` | `1.000000` | PASS |
| `physicsData.getPos` | bodyApi.physicsData.getPos | `4.000000` | `4.000000` | PASS |
| `physicsData.getPosRef` | bodyApi.physicsData.getPosRef | `5.000000` | `5.000000` | PASS |
| `physicsData.getVel` | bodyApi.physicsData.getVel | `6.000000` | `6.000000` | PASS |
| `physicsData.getMinPos` | bodyApi.physicsData.getMinPos | `3.000000` | `3.000000` | PASS |
| `physicsData.getMaxPos` | bodyApi.physicsData.getMaxPos | `5.000000` | `5.000000` | PASS |
| `physicsData.isPointWithin` | bodyApi.physicsData.isPointWithin | `1` | `1` | PASS |
| `physicsData.getSpeed` | bodyApi.physicsData.getSpeed | `null` | `null` | PASS |
| `physicsData.getThrustAcc` | bodyApi.physicsData.getThrustAcc | `null` | `null` | PASS |
| `physicsData.getType` | bodyApi.physicsData.getType | `0` | `0` | PASS |
| `physicsData.getWidth` | bodyApi.physicsData.getWidth | `2.000000` | `2.000000` | PASS |
| `physicsData.isCircle` | bodyApi.physicsData.isCircle | `null` | `null` | PASS |
| `physicsData.isCollidable` | bodyApi.physicsData.isCollidable | `1` | `1` | PASS |
| `physicsData.isRect` | bodyApi.physicsData.isRect | `null` | `null` | PASS |
| `physicsData.isSensor` | bodyApi.physicsData.isSensor | `1` | `1` | PASS |
| `physicsShape.getType` | bodyApi.physicsShape.getType | `circle` | `circle` | PASS |
| `physicsShape.getVertexCount` | bodyApi.physicsShape.getVertexCount | `1` | `1` | PASS |
| `physicsShape.getVertexWorldPos` | bodyApi.physicsShape.getVertexWorldPos | `4.000000` | `4.000000` | PASS |
| `physicsShape.isSensor` | bodyApi.physicsShape.isSensor | `1` | `1` | PASS |
| `physicsShape.setCategoriesToCollideWith` | bodyApi.physicsShape.setCategoriesToCollideWith | `1` | `1` | PASS |
| `physicsShape.setCollisionCategory` | bodyApi.physicsShape.setCollisionCategory | `1` | `1` | PASS |
| `physicsShape.setFrictionForPlayer` | bodyApi.physicsShape.setFrictionForPlayer | `1` | `1` | PASS |
| `physicsShape.setIsSensor` | bodyApi.physicsShape.setIsSensor | `1` | `1` | PASS |
| `shapeDef.setFriction` | bodyApi.shapeDef.setFriction | `1` | `1` | PASS |
| `shapeDef.getFriction` | bodyApi.shapeDef.getFriction | `0.400000` | `0.400000` | PASS |
| `shapeDef.setRestitution` | bodyApi.shapeDef.setRestitution | `0.600000` | `0.600000` | PASS |
| `shapeDef.setFrictionForPlayer` | bodyApi.shapeDef.setFrictionForPlayer | `1` | `1` | PASS |
| `shapeDef.setCollisionCategory` | bodyApi.shapeDef.setCollisionCategory | `8` | `8` | PASS |
| `shapeDef.setCategoriesToCollideWith` | bodyApi.shapeDef.setCategoriesToCollideWith | `11` | `11` | PASS |
| `shapeDef.reset` | bodyApi.shapeDef.reset | `1` | `1` | PASS |
| `shapeDef.addBox` | bodyApi.shapeDef.addBox | `1` | `1` | PASS |
| `shapeDef.setMass` | bodyApi.shapeDef.setMass | `0.000000` | `0.000000` | PASS |
| `shapeDef.setIsSensor` | bodyApi.shapeDef.setIsSensor | `1` | `1` | PASS |
| `shapeDef.getRestitution` | bodyApi.shapeDef.getRestitution | `0.000000` | `0.000000` | PASS |
| `shapeDef.addPolygon` | bodyApi.shapeDef.addPolygon | `1` | `1` | PASS |
| `shapeDef.addEdges` | bodyApi.shapeDef.addEdges | `1` | `1` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t0.x | `-100.000000` | `-100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t0.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t0.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t0.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t0.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t0.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t0.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t0.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t0.x | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t0.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t0.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t0.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t0.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t0.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t0.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t0.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t0.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t0.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t0.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t0.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t0.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t0.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t0.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t0.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t0.x | `200.000000` | `200.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t0.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t0.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t0.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t0.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t0.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t0.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t0.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t0.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t0.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t0.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t0.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t0.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t0.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t0.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t0.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t0.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t1.x | `-50.000000` | `-50.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t1.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t1.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t1.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t1.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t1.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t1.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t1.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t1.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t1.x | `33.333332` | `33.333332` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t1.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t1.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t1.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t1.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t1.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t1.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t1.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t1.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t1.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t1.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t1.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t1.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t1.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t1.dirX | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t1.dirY | `1.000000` | `1.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t1.rotateVel | `0.094248` | `0.094248` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t1.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t1.x | `200.000000` | `200.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t1.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t1.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t1.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t1.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t1.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t1.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t1.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t1.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t1.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t1.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t1.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t1.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t1.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t1.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t1.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t1.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t1.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t2.x | `-0.000002` | `-0.000002` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t2.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t2.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t2.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t2.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t2.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t2.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t2.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t2.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t2.x | `66.666664` | `66.666664` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t2.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t2.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t2.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t2.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t2.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t2.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t2.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t2.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t2.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t2.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t2.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t2.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t2.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t2.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t2.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t2.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t2.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t2.x | `200.000000` | `200.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t2.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t2.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t2.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t2.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t2.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t2.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t2.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t2.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t2.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t2.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t2.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t2.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t2.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t2.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t2.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t2.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t2.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t3.x | `49.999996` | `49.999996` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t3.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t3.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t3.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t3.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t3.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t3.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t3.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t3.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t3.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t3.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t3.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t3.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t3.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t3.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t3.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t3.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t3.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t3.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t3.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t3.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t3.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t3.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t3.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t3.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t3.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t3.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t3.x | `200.000000` | `200.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t3.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t3.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t3.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t3.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t3.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t3.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t3.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t3.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t3.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t3.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t3.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t3.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t3.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t3.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t3.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t3.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t3.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t4.x | `99.999992` | `99.999992` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t4.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t4.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t4.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t4.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t4.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t4.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t4.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t4.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t4.x | `133.333328` | `133.333328` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t4.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t4.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t4.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t4.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t4.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t4.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t4.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t4.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t4.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t4.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t4.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t4.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t4.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t4.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t4.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t4.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t4.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t4.x | `200.000000` | `200.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t4.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t4.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t4.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t4.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t4.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t4.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t4.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t4.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t4.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t4.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t4.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t4.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t4.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t4.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t4.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t4.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t4.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t5.x | `149.999985` | `149.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t5.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t5.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t5.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t5.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t5.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t5.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t5.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t5.x | `166.666656` | `166.666656` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t5.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t5.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t5.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t5.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t5.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t5.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t5.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t5.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t5.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t5.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t5.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t5.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t5.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t5.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t5.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t5.x | `200.000000` | `200.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t5.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t5.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t5.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t5.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t5.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t5.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t5.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t5.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t5.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t5.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t5.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t5.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t5.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t5.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t5.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t5.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t6.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t6.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t6.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t6.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t6.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t6.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t6.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t6.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t6.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t6.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t6.vx | `2.000000` | `2.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t6.speed | `2.000000` | `2.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t6.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t6.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t6.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t6.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t6.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t6.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t6.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t6.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t6.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t6.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t6.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t6.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t6.x | `216.666672` | `216.666672` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t6.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t6.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t6.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t6.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t6.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t6.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t6.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t6.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t6.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t6.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t6.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t6.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t6.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t6.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t6.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t6.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t7.x | `249.999985` | `249.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t7.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t7.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t7.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t7.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t7.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t7.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t7.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t7.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t7.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t7.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t7.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t7.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t7.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t7.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t7.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t7.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t7.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t7.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t7.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t7.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t7.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t7.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t7.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t7.x | `233.333344` | `233.333344` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t7.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t7.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t7.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t7.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t7.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t7.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t7.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t7.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t7.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t7.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t7.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t7.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t7.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t7.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t7.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t7.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t8.x | `299.999969` | `299.999969` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t8.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t8.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t8.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t8.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t8.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t8.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t8.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t8.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t8.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t8.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t8.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t8.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t8.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t8.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t8.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t8.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t8.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t8.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t8.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t8.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t8.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t8.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t8.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t8.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t8.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t8.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t8.x | `250.000015` | `250.000015` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t8.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t8.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t8.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t8.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t8.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t8.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t8.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t8.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t8.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t8.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t8.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t8.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t8.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t8.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t8.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t8.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t8.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t9.x | `349.999969` | `349.999969` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t9.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t9.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t9.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t9.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t9.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t9.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t9.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t9.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t9.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t9.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t9.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t9.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t9.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t9.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t9.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t9.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t9.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t9.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t9.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t9.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t9.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t9.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t9.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t9.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t9.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t9.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t9.x | `266.666687` | `266.666687` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t9.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t9.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t9.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t9.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t9.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t9.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t9.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t9.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t9.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t9.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t9.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t9.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t9.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t9.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t9.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t9.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t9.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t10.x | `399.999969` | `399.999969` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t10.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t10.vx | `3.000000` | `3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t10.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t10.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t10.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t10.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t10.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t10.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t10.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t10.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t10.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t10.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t10.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t10.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t10.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t10.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t10.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t10.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t10.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t10.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t10.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t10.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t10.dirX | `0.047565` | `0.047565` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t10.dirY | `0.998868` | `0.998868` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t10.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t10.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t10.x | `283.333344` | `283.333344` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t10.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t10.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t10.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t10.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t10.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t10.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t10.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t10.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t10.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t10.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t10.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t10.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t10.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t10.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t10.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t10.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t10.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t11.x | `394.444427` | `394.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t11.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t11.vx | `-0.333333` | `-0.333333` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t11.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t11.speed | `0.333333` | `0.333333` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t11.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t11.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t11.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t11.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t11.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t11.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t11.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t11.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t11.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t11.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t11.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t11.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t11.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t11.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t11.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t11.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t11.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t11.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t11.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t11.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t11.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t11.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t11.x | `300.000000` | `300.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t11.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t11.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t11.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t11.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t11.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t11.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t11.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t11.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t11.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t11.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t11.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t11.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t11.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t11.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t11.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t11.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t11.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t12.x | `344.444427` | `344.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t12.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t12.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t12.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t12.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t12.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t12.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t12.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t12.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t12.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t12.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t12.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t12.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t12.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t12.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t12.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t12.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t12.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t12.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t12.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t12.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t12.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t12.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t12.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t12.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t12.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t12.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t12.x | `316.666656` | `316.666656` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t12.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t12.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t12.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t12.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t12.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t12.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t12.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t12.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t12.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t12.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t12.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t12.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t12.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t12.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t12.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t12.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t12.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t13.x | `294.444427` | `294.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t13.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t13.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t13.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t13.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t13.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t13.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t13.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t13.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t13.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t13.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t13.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t13.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t13.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t13.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t13.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t13.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t13.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t13.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t13.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t13.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t13.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t13.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t13.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t13.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t13.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t13.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t13.x | `333.333313` | `333.333313` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t13.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t13.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t13.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t13.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t13.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t13.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t13.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t13.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t13.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t13.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t13.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t13.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t13.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t13.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t13.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t13.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t13.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t14.x | `244.444427` | `244.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t14.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t14.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t14.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t14.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t14.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t14.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t14.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t14.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t14.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t14.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t14.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t14.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t14.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t14.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t14.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t14.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t14.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t14.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t14.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t14.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t14.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t14.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t14.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t14.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t14.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t14.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t14.x | `349.999969` | `349.999969` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t14.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t14.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t14.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t14.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t14.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t14.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t14.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t14.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t14.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t14.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t14.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t14.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t14.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t14.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t14.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t14.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t14.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t15.x | `194.444427` | `194.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t15.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t15.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t15.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t15.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t15.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t15.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t15.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t15.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t15.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t15.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t15.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t15.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t15.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t15.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t15.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t15.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t15.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t15.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t15.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t15.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t15.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t15.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t15.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t15.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t15.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t15.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t15.x | `366.666626` | `366.666626` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t15.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t15.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t15.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t15.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t15.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t15.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t15.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t15.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t15.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t15.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t15.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t15.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t15.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t15.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t15.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t15.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t15.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t16.x | `144.444427` | `144.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t16.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t16.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t16.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t16.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t16.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t16.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t16.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t16.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t16.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t16.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t16.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t16.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t16.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t16.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t16.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t16.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t16.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t16.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t16.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t16.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t16.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t16.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t16.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t16.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t16.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t16.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t16.x | `383.333282` | `383.333282` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t16.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t16.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t16.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t16.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t16.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t16.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t16.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t16.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t16.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t16.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t16.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t16.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t16.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t16.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t16.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t16.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t16.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t17.x | `94.444427` | `94.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t17.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t17.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t17.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t17.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t17.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t17.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t17.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t17.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t17.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t17.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t17.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t17.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t17.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t17.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t17.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t17.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t17.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t17.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t17.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t17.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t17.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t17.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t17.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t17.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t17.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t17.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t17.x | `399.999939` | `399.999939` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t17.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t17.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t17.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t17.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t17.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t17.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t17.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t17.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t17.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t17.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t17.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t17.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t17.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t17.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t17.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t17.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t17.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t18.x | `44.444427` | `44.444427` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t18.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t18.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t18.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t18.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t18.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t18.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t18.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t18.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t18.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t18.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t18.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t18.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t18.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t18.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t18.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t18.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t18.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t18.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t18.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t18.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t18.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t18.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t18.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t18.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t18.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t18.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t18.x | `416.666595` | `416.666595` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t18.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t18.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t18.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t18.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t18.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t18.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t18.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t18.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t18.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t18.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t18.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t18.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t18.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t18.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t18.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t18.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t18.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t19.x | `-5.555571` | `-5.555571` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t19.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t19.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t19.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t19.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t19.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t19.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t19.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t19.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t19.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t19.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t19.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t19.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t19.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t19.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t19.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t19.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t19.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t19.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t19.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t19.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t19.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t19.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t19.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t19.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t19.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t19.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t19.x | `433.333252` | `433.333252` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t19.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t19.vx | `1.000000` | `1.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t19.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t19.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t19.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t19.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t19.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t19.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t19.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t19.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t19.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t19.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t19.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t19.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t19.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t19.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t19.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t20.x | `-55.555569` | `-55.555569` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t20.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t20.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t20.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t20.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t20.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t20.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t20.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t20.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t20.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t20.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t20.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t20.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t20.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t20.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t20.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t20.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t20.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t20.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t20.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t20.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t20.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t20.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t20.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t20.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t20.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t20.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t20.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t20.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t20.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t20.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t20.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t20.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t20.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t20.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t20.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t20.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t20.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t20.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t20.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t20.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t20.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t20.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t20.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t20.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t21.x | `-105.555565` | `-105.555565` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t21.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t21.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t21.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t21.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t21.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t21.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t21.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t21.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t21.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t21.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t21.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t21.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t21.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t21.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t21.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t21.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t21.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t21.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t21.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t21.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t21.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t21.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t21.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t21.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t21.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t21.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t21.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t21.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t21.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t21.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t21.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t21.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t21.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t21.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t21.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t21.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t21.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t21.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t21.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t21.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t21.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t21.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t21.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t21.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t22.x | `-155.555557` | `-155.555557` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t22.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t22.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t22.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t22.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t22.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t22.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t22.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t22.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t22.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t22.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t22.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t22.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t22.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t22.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t22.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t22.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t22.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t22.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t22.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t22.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t22.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t22.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t22.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t22.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t22.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t22.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t22.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t22.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t22.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t22.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t22.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t22.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t22.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t22.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t22.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t22.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t22.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t22.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t22.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t22.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t22.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t22.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t22.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t22.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t23.x | `-205.555557` | `-205.555557` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t23.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t23.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t23.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t23.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t23.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t23.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t23.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t23.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t23.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t23.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t23.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t23.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t23.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t23.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t23.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t23.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t23.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t23.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t23.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t23.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t23.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t23.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t23.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t23.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t23.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t23.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t23.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t23.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t23.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t23.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t23.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t23.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t23.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t23.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t23.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t23.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t23.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t23.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t23.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t23.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t23.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t23.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t23.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t23.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t24.x | `-255.555557` | `-255.555557` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t24.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t24.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t24.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t24.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t24.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t24.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t24.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t24.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t24.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t24.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t24.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t24.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t24.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t24.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t24.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t24.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t24.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t24.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t24.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t24.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t24.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t24.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t24.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t24.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t24.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t24.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t24.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t24.y | `3000.000000` | `3000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t24.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t24.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t24.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t24.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t24.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t24.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t24.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t24.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t24.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t24.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t24.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t24.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t24.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t24.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t24.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t24.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t25.x | `-305.555542` | `-305.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t25.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t25.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t25.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t25.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t25.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t25.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t25.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t25.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t25.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t25.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t25.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t25.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t25.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t25.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t25.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t25.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t25.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t25.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t25.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t25.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t25.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t25.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t25.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t25.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t25.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t25.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t25.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t25.y | `3016.666748` | `3016.666748` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t25.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t25.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t25.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t25.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t25.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t25.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t25.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t25.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t25.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t25.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t25.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t25.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t25.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t25.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t25.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t25.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t26.x | `-355.555542` | `-355.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t26.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t26.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t26.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t26.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t26.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t26.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t26.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t26.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t26.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t26.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t26.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t26.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t26.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t26.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t26.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t26.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t26.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t26.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t26.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t26.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t26.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t26.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t26.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t26.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t26.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t26.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t26.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t26.y | `3033.333496` | `3033.333496` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t26.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t26.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t26.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t26.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t26.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t26.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t26.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t26.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t26.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t26.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t26.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t26.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t26.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t26.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t26.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t26.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t27.x | `-405.555542` | `-405.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t27.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t27.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t27.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t27.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t27.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t27.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t27.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t27.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t27.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t27.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t27.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t27.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t27.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t27.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t27.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t27.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t27.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t27.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t27.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t27.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t27.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t27.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t27.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t27.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t27.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t27.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t27.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t27.y | `3050.000244` | `3050.000244` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t27.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t27.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t27.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t27.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t27.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t27.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t27.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t27.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t27.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t27.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t27.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t27.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t27.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t27.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t27.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t27.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t28.x | `-455.555542` | `-455.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t28.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t28.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t28.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t28.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t28.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t28.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t28.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t28.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t28.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t28.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t28.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t28.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t28.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t28.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t28.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t28.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t28.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t28.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t28.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t28.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t28.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t28.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t28.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t28.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t28.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t28.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t28.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t28.y | `3066.666992` | `3066.666992` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t28.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t28.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t28.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t28.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t28.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t28.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t28.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t28.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t28.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t28.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t28.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t28.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t28.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t28.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t28.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t28.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t29.x | `-505.555542` | `-505.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t29.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t29.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t29.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t29.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t29.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t29.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t29.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t29.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t29.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t29.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t29.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t29.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t29.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t29.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t29.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t29.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t29.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t29.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t29.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t29.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t29.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t29.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t29.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t29.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t29.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t29.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t29.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t29.y | `3083.333740` | `3083.333740` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t29.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t29.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t29.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t29.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t29.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t29.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t29.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t29.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t29.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t29.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t29.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t29.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t29.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t29.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t29.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t29.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t30.x | `-555.555542` | `-555.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t30.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t30.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t30.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t30.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t30.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t30.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t30.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t30.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t30.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t30.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t30.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t30.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t30.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t30.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t30.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t30.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t30.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t30.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t30.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t30.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t30.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t30.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t30.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t30.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t30.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t30.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t30.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t30.y | `3100.000488` | `3100.000488` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t30.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t30.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t30.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t30.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t30.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t30.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t30.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t30.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t30.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t30.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t30.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t30.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t30.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t30.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t30.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t30.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t31.x | `-605.555542` | `-605.555542` | PASS |
| `physicsBody.getPos` | bodyCmd.thrust.t31.y | `100.000000` | `100.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t31.vx | `-3.000000` | `-3.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.thrust.t31.vy | `0.000000` | `0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.thrust.t31.speed | `3.000000` | `3.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t31.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.thrust.t31.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.thrust.t31.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.thrust.t31.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t31.x | `199.999985` | `199.999985` | PASS |
| `physicsBody.getPos` | bodyCmd.brake.t31.y | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t31.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.brake.t31.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.brake.t31.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t31.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.brake.t31.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.brake.t31.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.brake.t31.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t31.x | `100.000000` | `100.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.turn.t31.y | `2000.000000` | `2000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t31.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.turn.t31.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.turn.t31.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t31.dirX | `0.052559` | `0.052559` | PASS |
| `physicsBody.getDir` | bodyCmd.turn.t31.dirY | `0.998618` | `0.998618` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.turn.t31.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.turn.t31.sleeping | `1.000000` | `1.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t31.x | `449.999908` | `449.999908` | PASS |
| `physicsBody.getPos` | bodyCmd.sleep.t31.y | `3116.667236` | `3116.667236` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t31.vx | `0.000000` | `0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.sleep.t31.vy | `1.000000` | `1.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.sleep.t31.speed | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t31.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.sleep.t31.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.sleep.t31.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.sleep.t31.sleeping | `0.000000` | `0.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t31.x | `1000.000000` | `1000.000000` | PASS |
| `physicsBody.getPos` | bodyCmd.idle.t31.y | `4000.000000` | `4000.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t31.vx | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getVel` | bodyCmd.idle.t31.vy | `-0.000000` | `-0.000000` | PASS |
| `physicsBody.getSpeed` | bodyCmd.idle.t31.speed | `0.000000` | `0.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t31.dirX | `1.000000` | `1.000000` | PASS |
| `physicsBody.getDir` | bodyCmd.idle.t31.dirY | `0.000000` | `0.000000` | PASS |
| `physicsBody.getRotateVel` | bodyCmd.idle.t31.rotateVel | `0.000000` | `0.000000` | PASS |
| `physicsBody.isSleeping` | bodyCmd.idle.t31.sleeping | `1.000000` | `1.000000` | PASS |
