# Retail API gaps

OpenEgg is an early reimplementation. This inventory covers every recovered retail wrapper/API-like name in the local review data that has no matching OpenEgg binding name. It includes methods whether or not retail scripts call them.

## Summary

- **8 registration-confirmed methods remain unimplemented**. The indexed body-position methods `getBodyPos` and `setBodyPos` have been added; see [`docs/api/retail_missing.tsv`](docs/api/retail_missing.tsv).
- **178 additional recovered wrappers** have no matching OpenEgg binding name, but their script-facing registration/owner is not recorded in the available inventory. They are listed as unresolved candidates, not asserted as confirmed public API.
- Parameters and return types are marked unknown unless recovered; names and addresses alone do not establish a full calling contract.
- A same-name local binding can still have incomplete behavior. This file tracks missing names, not parity.

## Registration-confirmed missing methods

### Physics and particle APIs

| Retail API | Retail code address | Description | Parameters / return |
|---|---:|---|---|
| `Entity.getLifePercentage` | `0x9cb90` | Returns the particle lifetime as a fraction of its maximum lifetime. | Parameters and return type not yet recovered. |
| `Entity.getMaxTimeToLive` | `0x9ce08` | Returns the particle maximum lifetime. | Parameters and return type not yet recovered. |
| `Entity.getTimeToLive` | `0x9cccc` | Returns the particle current remaining lifetime. | Parameters and return type not yet recovered. |
| `Physics.getEntitiesJoinedToMe` | `0x958ec` | Returns entities joined to the receiving physics body. | Parameters and return type not yet recovered. |
| `Physics.getNearestCollidingEntity` | `0x8bf04` | Returns the nearest entity reported as colliding with the receiving body. | Parameters and return type not yet recovered. |
| `PhysicsBody.getBodyBBox` | `0x92fe4` | Returns the bounding box associated with a body in the physics data. | Parameters and return type not yet recovered. |
| `PhysicsBody.getBodyIndex` | `0x93408` | Returns the body index stored in the physics data. | Parameters and return type not yet recovered. |
| `PhysicsShape.getExteriorDir` | `0x8d198` | Returns the exterior direction associated with the physics shape. | Parameters and return type not yet recovered. |

## Unmatched recovered wrappers

These names occur in the recovered retail wrapper inventory but have no matching method name in OpenEgg’s documented binding catalog or registered GameMonkey string library. Registration ownership is not established for the rows below, so confirm each as script-facing before treating it as a definite public API gap.

| Retail wrapper/API name | Code address | Retail registration | Ghidra label |
|---|---:|---|---|
| `Alarm.ALARM` | `0x65e30` | not recorded | `GE3_API_Alarm_ALARM` |
| `Alarm.setIntervalAndHardReset` | `0x660e0` | not recorded | `GE3_API_Alarm_setIntervalAndHardReset` |
| `Colour.COLOUR_MakeInterpolatedColour` | `0xa757c` | not recorded | `GE3_API_Colour_COLOUR_MakeInterpolatedColour` |
| `ColourScheme.COLOURSCHEME_CopyColoursToActiveColours` | `0x1d3404` | not recorded | `GE3_API_ColourScheme_COLOURSCHEME_CopyColoursToActiveColours` |
| `ColourScheme.COLOURSCHEME_GetColour` | `0x1d20c4` | not recorded | `GE3_API_ColourScheme_COLOURSCHEME_GetColour` |
| `ColourScheme.COLOURSCHEME_SetTexturePaletteAlpha` | `0x1d2ea8` | not recorded | `GE3_API_ColourScheme_COLOURSCHEME_SetTexturePaletteAlpha` |
| `ColourScheme.COLOURSCHEME_SetTexturePaletteColour` | `0x1d2e34` | not recorded | `GE3_API_ColourScheme_COLOURSCHEME_SetTexturePaletteColour` |
| `ColourScheme.COLOURSCHEME_SetTransitionTime` | `0x1d1f80` | not recorded | `GE3_API_ColourScheme_COLOURSCHEME_SetTransitionTime` |
| `Debug.forAllProcesses` | `0x1af5bc` | not recorded | `GE3_API_Debug_forAllProcesses` |
| `Entity.face` | `0x9e000` | not recorded | `GE3_API_Entity_face` |
| `Entity.facePos` | `0x9e124` | not recorded | `GE3_API_Entity_facePos` |
| `Entity.faceTarget` | `0x9e0b0` | not recorded | `GE3_API_Entity_faceTarget` |
| `Entity.findClosestEntity` | `0x9e474` | not recorded | `GE3_API_Entity_findClosestEntity` |
| `Entity.findClosestPlayer` | `0x9e2d0` | not recorded | `GE3_API_Entity_findClosestPlayer` |
| `Entity.forceDraw` | `0x9b828` | not recorded | `GE3_API_Entity_forceDraw` |
| `Entity.freeze` | `0x9a6d8` | not recorded | `GE3_API_Entity_freeze` |
| `Entity.isCanCollideWith` | `0x9bba8` | not recorded | `GE3_API_Entity_isCanCollideWith` |
| `Entity.isDoneTransitioning` | `0x9f094` | not recorded | `GE3_API_Entity_isDoneTransitioning` |
| `Entity.isInFront` | `0x9ec5c` | not recorded | `GE3_API_Entity_isInFront` |
| `Entity.leadTarget` | `0x9e198` | not recorded | `GE3_API_Entity_leadTarget` |
| `Entity.setExponentialTransition` | `0x9f004` | not recorded | `GE3_API_Entity_setExponentialTransition` |
| `Entity.setLinearTransition` | `0x9ef74` | not recorded | `GE3_API_Entity_setLinearTransition` |
| `Entity.setMaxTimeToLive` | `0x9cd78` | not recorded | `GE3_API_Entity_setMaxTimeToLive` |
| `Entity.setTimeToLive` | `0x9cc3c` | not recorded | `GE3_API_Entity_setTimeToLive` |
| `EntityBase.ENTITYBASE_GetMixerChannelIDList` | `0x1c5144` | not recorded | `GE3_API_EntityBase_ENTITYBASE_GetMixerChannelIDList` |
| `EntityBase.ENTITYBASE_GetMixerIDList` | `0x1c52c4` | not recorded | `GE3_API_EntityBase_ENTITYBASE_GetMixerIDList` |
| `EntityBase.getGlobalVolume` | `0x1c609c` | not recorded | `GE3_API_EntityBase_getGlobalVolume` |
| `EntityBase.getStepHeight` | `0x1c5b3c` | not recorded | `GE3_API_EntityBase_getStepHeight` |
| `EntityBase.getStepWidth` | `0x1c5adc` | not recorded | `GE3_API_EntityBase_getStepWidth` |
| `EntityBase.setEditSensorCircle` | `0x1c6470` | not recorded | `GE3_API_EntityBase_setEditSensorCircle` |
| `EntityRegistration.isAnimating` | `0x1d0848` | not recorded | `GE3_API_EntityRegistration_isAnimating` |
| `EntityRegistration.registerGrounds` | `0x1d04b4` | not recorded | `GE3_API_EntityRegistration_registerGrounds` |
| `EntityRegistration.setIsAnimating` | `0x1d08d4` | not recorded | `GE3_API_EntityRegistration_setIsAnimating` |
| `Game.GAME_GetNoteGotCircleGraphicShader` | `0x1ae2a8` | not recorded | `GE3_API_Game_GAME_GetNoteGotCircleGraphicShader` |
| `Game.GAME_IsEffectOn` | `0x1adfbc` | not recorded | `GE3_API_Game_GAME_IsEffectOn` |
| `Game.GAME_SetIsEffectOn` | `0x1ae070` | not recorded | `GE3_API_Game_GAME_SetIsEffectOn` |
| `Game.GAME_hack_isPlayerFingers` | `0x1ae68c` | not recorded | `GE3_API_Game_GAME_hack_isPlayerFingers` |
| `Game.GAME_hack_setIsPlayerFingers` | `0x1ae71c` | not recorded | `GE3_API_Game_GAME_hack_setIsPlayerFingers` |
| `GameMonkeyScript.PS3ReloadAnims` | `0x6ffd4` | not recorded | `GE3_API_GameMonkeyScript_PS3ReloadAnims` |
| `GameMonkeyScript.check` | `0x6ffec` | not recorded | `GE3_API_GameMonkeyScript_check` |
| `GameMonkeyScript.clearErrorFileStuff` | `0x6f510` | not recorded | `GE3_API_GameMonkeyScript_clearErrorFileStuff` |
| `GameMonkeyScript.closeConsole` | `0x70040` | not recorded | `GE3_API_GameMonkeyScript_closeConsole` |
| `GameMonkeyScript.enableGC` | `0x6f730` | not recorded | `GE3_API_GameMonkeyScript_enableGC` |
| `GameMonkeyScript.format` | `0xe4110` | not recorded | `GE3_API_GameMonkeyScript_format` |
| `GameMonkeyScript.getChangeID` | `0x6ff2c` | not recorded | `GE3_API_GameMonkeyScript_getChangeID` |
| `GameMonkeyScript.getErrorFileCol` | `0x6f490` | not recorded | `GE3_API_GameMonkeyScript_getErrorFileCol` |
| `GameMonkeyScript.getErrorFileLine` | `0x6f410` | not recorded | `GE3_API_GameMonkeyScript_getErrorFileLine` |
| `GameMonkeyScript.getErrorFilename` | `0x6f354` | not recorded | `GE3_API_GameMonkeyScript_getErrorFilename` |
| `GameMonkeyScript.getOutput` | `0x6fd50` | not recorded | `GE3_API_GameMonkeyScript_getOutput` |
| `GameMonkeyScript.globals` | `0xe3d80` | not recorded | `GE3_API_GameMonkeyScript_globals` |
| `GameMonkeyScript.gmVersion` | `0xe3624` | not recorded | `GE3_API_GameMonkeyScript_gmVersion` |
| `GameMonkeyScript.isConsoleOpen` | `0x7008c` | not recorded | `GE3_API_GameMonkeyScript_isConsoleOpen` |
| `GameMonkeyScript.isShouldPauseOnTaskSwitch` | `0x70290` | not recorded | `GE3_API_GameMonkeyScript_isShouldPauseOnTaskSwitch` |
| `GameMonkeyScript.logText` | `0x6fcb4` | not recorded | `GE3_API_GameMonkeyScript_logText` |
| `GameMonkeyScript.outputToFile` | `0x6fed0` | not recorded | `GE3_API_GameMonkeyScript_outputToFile` |
| `GameMonkeyScript.saveTableToCompressedJsonFile` | `0x6eeb0` | not recorded | `GE3_API_GameMonkeyScript_saveTableToCompressedJsonFile` |
| `GameMonkeyScript.saveTableToCompressedJsonSaveRequest` | `0x6f028` | not recorded | `GE3_API_GameMonkeyScript_saveTableToCompressedJsonSaveRequest` |
| `GameMonkeyScript.saveTableToJsonFile` | `0x6dda8` | not recorded | `GE3_API_GameMonkeyScript_saveTableToJsonFile` |
| `GameMonkeyScript.setAlwaysReloadScript` | `0x6cc08` | not recorded | `GE3_API_GameMonkeyScript_setAlwaysReloadScript` |
| `GameMonkeyScript.setShouldPauseOnTaskSwitch` | `0x70344` | not recorded | `GE3_API_GameMonkeyScript_setShouldPauseOnTaskSwitch` |
| `GameMonkeyScript.sysCollectGarbage` | `0xe39e0` | not recorded | `GE3_API_GameMonkeyScript_sysCollectGarbage` |
| `GameMonkeyScript.sysGetDesiredMemoryUsageHard` | `0xe3adc` | not recorded | `GE3_API_GameMonkeyScript_sysGetDesiredMemoryUsageHard` |
| `GameMonkeyScript.sysGetDesiredMemoryUsageSoft` | `0xe3b3c` | not recorded | `GE3_API_GameMonkeyScript_sysGetDesiredMemoryUsageSoft` |
| `GameMonkeyScript.sysGetMemoryUsage` | `0xe3a14` | not recorded | `GE3_API_GameMonkeyScript_sysGetMemoryUsage` |
| `GameMonkeyScript.sysGetStatsGCNumFullCollects` | `0xe3bd8` | not recorded | `GE3_API_GameMonkeyScript_sysGetStatsGCNumFullCollects` |
| `GameMonkeyScript.sysGetStatsGCNumIncCollects` | `0xe3c38` | not recorded | `GE3_API_GameMonkeyScript_sysGetStatsGCNumIncCollects` |
| `GameMonkeyScript.sysGetStatsGCNumWarnings` | `0xe3c98` | not recorded | `GE3_API_GameMonkeyScript_sysGetStatsGCNumWarnings` |
| `GameMonkeyScript.sysIsGCRunning` | `0xe3cf8` | not recorded | `GE3_API_GameMonkeyScript_sysIsGCRunning` |
| `GameMonkeyScript.sysSetDesiredMemoryUsageAuto` | `0xe3b9c` | not recorded | `GE3_API_GameMonkeyScript_sysSetDesiredMemoryUsageAuto` |
| `GameMonkeyScript.sysSetDesiredMemoryUsageHard` | `0xe3a7c` | not recorded | `GE3_API_GameMonkeyScript_sysSetDesiredMemoryUsageHard` |
| `GameMonkeyScript.sysSetDesiredMemoryUsageSoft` | `0xe3aac` | not recorded | `GE3_API_GameMonkeyScript_sysSetDesiredMemoryUsageSoft` |
| `GameMonkeyScript.sysTime` | `0xe3de0` | not recorded | `GE3_API_GameMonkeyScript_sysTime` |
| `GameMonkeyScript.tableClear` | `0xe3f68` | not recorded | `GE3_API_GameMonkeyScript_tableClear` |
| `GameMonkeyScript.tableCount` | `0xe3e40` | not recorded | `GE3_API_GameMonkeyScript_tableCount` |
| `GameMonkeyScript.tableDuplicate` | `0xe3ebc` | not recorded | `GE3_API_GameMonkeyScript_tableDuplicate` |
| `GameMonkeyScript.traceInfo` | `0x6f774` | not recorded | `GE3_API_GameMonkeyScript_traceInfo` |
| `GameMonkeyScript.traceTick` | `0x6ffe4` | not recorded | `GE3_API_GameMonkeyScript_traceTick` |
| `GameMonkeyScript.typeId` | `0xe36bc` | not recorded | `GE3_API_GameMonkeyScript_typeId` |
| `GameMonkeyScript.typeName` | `0xe3740` | not recorded | `GE3_API_GameMonkeyScript_typeName` |
| `GameMonkeyScript.typeRegisterOperator` | `0xe3818` | not recorded | `GE3_API_GameMonkeyScript_typeRegisterOperator` |
| `GameMonkeyScript.typeRegisterVariable` | `0xe3978` | not recorded | `GE3_API_GameMonkeyScript_typeRegisterVariable` |
| `Input.setLightBar` | `0x74580` | not recorded | `GE3_API_Input_setLightBar` |
| `Input.setMousePos` | `0x732e4` | not recorded | `GE3_API_Input_setMousePos` |
| `Input.translateString` | `0x72560` | not recorded | `GE3_API_Input_translateString` |
| `MathUtil.MATHUTIL_ConvertAngleToTWOPIDomain` | `0xa52b8` | not recorded | `GE3_API_MathUtil_MATHUTIL_ConvertAngleToTWOPIDomain` |
| `MathUtil.MATHUTIL_GetAngleDeg` | `0xa3d50` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetAngleDeg` |
| `MathUtil.MATHUTIL_GetAngleDeg360` | `0xa3e24` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetAngleDeg360` |
| `MathUtil.MATHUTIL_GetContouredPercentage` | `0xa4ae8` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetContouredPercentage` |
| `MathUtil.MATHUTIL_GetDegreesBetween` | `0xa42dc` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetDegreesBetween` |
| `MathUtil.MATHUTIL_GetDigitValue` | `0xa4e3c` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetDigitValue` |
| `MathUtil.MATHUTIL_GetNumDigits` | `0xa4d68` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetNumDigits` |
| `MathUtil.MATHUTIL_GetValueInRange` | `0xa3c0c` | not recorded | `GE3_API_MathUtil_MATHUTIL_GetValueInRange` |
| `MathUtil.MATHUTIL_IsAngleBetween` | `0xa5168` | not recorded | `GE3_API_MathUtil_MATHUTIL_IsAngleBetween` |
| `MathUtil.MATHUTIL_RADTODEG` | `0xa3930` | not recorded | `GE3_API_MathUtil_MATHUTIL_RADTODEG` |
| `Misc.drawAlpha` | `0x1a305c` | not recorded | `GE3_API_Misc_drawAlpha` |
| `Misc.hangTimeUpdate` | `0x1a3160` | not recorded | `GE3_API_Misc_hangTimeUpdate` |
| `MusicTheory.getCurrentTransposeIndex` | `0x1a44fc` | not recorded | `GE3_API_MusicTheory_getCurrentTransposeIndex` |
| `MusicTheory.getModeIndex` | `0x1a4f04` | not recorded | `GE3_API_MusicTheory_getModeIndex` |
| `MusicTheory.getNoteIndexWithPositionAndOffset` | `0x1a5048` | not recorded | `GE3_API_MusicTheory_getNoteIndexWithPositionAndOffset` |
| `MusicTheory.getNumNotesPerOctave` | `0x1a4ab4` | not recorded | `GE3_API_MusicTheory_getNumNotesPerOctave` |
| `MusicTheory.getRootNote` | `0x1a4f84` | not recorded | `GE3_API_MusicTheory_getRootNote` |
| `MusicTheory.getTransposeBottom` | `0x1a47c0` | not recorded | `GE3_API_MusicTheory_getTransposeBottom` |
| `MusicTheory.getTransposeIndex` | `0x1a45a8` | not recorded | `GE3_API_MusicTheory_getTransposeIndex` |
| `MusicTheory.getTransposeTop` | `0x1a4910` | not recorded | `GE3_API_MusicTheory_getTransposeTop` |
| `MusicTheory.setModeIndex` | `0x1a4e90` | not recorded | `GE3_API_MusicTheory_setModeIndex` |
| `Random.RANDOM` | `0x64754` | not recorded | `GE3_API_Random_RANDOM` |
| `Random.RANDOM_GetRandomSign` | `0xa3618` | not recorded | `GE3_API_Random_RANDOM_GetRandomSign` |
| `Random.getNext` | `0x64918` | not recorded | `GE3_API_Random_getNext` |
| `Random.getNextGaussian` | `0x64a6c` | not recorded | `GE3_API_Random_getNextGaussian` |
| `Random.getNextInt` | `0x64b8c` | not recorded | `GE3_API_Random_getNextInt` |
| `Random.getOriginalSeed` | `0x6486c` | not recorded | `GE3_API_Random_getOriginalSeed` |
| `RealPoint.REALPOINT_MakeRandomDir` | `0x64ce8` | not recorded | `GE3_API_RealPoint_REALPOINT_MakeRandomDir` |
| `RealPoint.divideEquals` | `0x6510c` | not recorded | `GE3_API_RealPoint_divideEquals` |
| `RealPoint.getRef` | `0x65e00` | not recorded | `GE3_API_RealPoint_getRef` |
| `RealPoint.minusEquals` | `0x650a0` | not recorded | `GE3_API_RealPoint_minusEquals` |
| `RealPoint.plusEquals` | `0x65068` | not recorded | `GE3_API_RealPoint_plusEquals` |
| `RealPoint.timesEquals` | `0x650d8` | not recorded | `GE3_API_RealPoint_timesEquals` |
| `SHADER.getFragmentShaderParamIndex` | `0x6b588` | not recorded | `GE3_API_SHADER_getFragmentShaderParamIndex` |
| `SaveData.SAVEDATA_addTicketReq` | `0xae028` | not recorded | `GE3_API_SaveData_SAVEDATA_addTicketReq` |
| `Video.getNumCellsX` | `0x19f368` | not recorded | `GE3_API_Video_getNumCellsX` |
| `Video.getNumCellsY` | `0x19f3e8` | not recorded | `GE3_API_Video_getNumCellsY` |
| `VideoHelper.VIDEOHELPER_DrawLine` | `0x1b3f88` | not recorded | `GE3_API_VideoHelper_VIDEOHELPER_DrawLine` |
| `VideoHelper.VIDEOHELPER_DrawPolygon` | `0x1b62c0` | not recorded | `GE3_API_VideoHelper_VIDEOHELPER_DrawPolygon` |
| `VideoHelper.VIDEOHELPER_DrawThickRect` | `0x1b5450` | not recorded | `GE3_API_VideoHelper_VIDEOHELPER_DrawThickRect` |
| `VideoHelper.VIDEOHELPER_DrawThickRectCentered` | `0x1b5860` | not recorded | `GE3_API_VideoHelper_VIDEOHELPER_DrawThickRectCentered` |
| `VideoHelper.VIDEOHELPER_ShowDisplaySafeAreaSettings` | `0x1b85ec` | not recorded | `GE3_API_VideoHelper_VIDEOHELPER_ShowDisplaySafeAreaSettings` |
| `Watermelon.Watermelon_ActivityStream_Page` | `0x1be4c8` | not recorded | `GE3_API_Watermelon_Watermelon_ActivityStream_Page` |
| `Watermelon.Watermelon_ActivityStream_Post` | `0x1be2c8` | not recorded | `GE3_API_Watermelon_Watermelon_ActivityStream_Post` |
| `Watermelon.Watermelon_Album_Page` | `0x1bf704` | not recorded | `GE3_API_Watermelon_Watermelon_Album_Page` |
| `Watermelon.Watermelon_Albums_Page` | `0x1bf4c8` | not recorded | `GE3_API_Watermelon_Watermelon_Albums_Page` |
| `Watermelon.Watermelon_Favourite_All` | `0x1ba9ac` | not recorded | `GE3_API_Watermelon_Watermelon_Favourite_All` |
| `Watermelon.Watermelon_Favourite_Delete` | `0x1bae6c` | not recorded | `GE3_API_Watermelon_Watermelon_Favourite_Delete` |
| `Watermelon.Watermelon_Favourite_Put` | `0x1babe4` | not recorded | `GE3_API_Watermelon_Watermelon_Favourite_Put` |
| `Watermelon.Watermelon_FeaturedLevel_Get` | `0x1bf2d4` | not recorded | `GE3_API_Watermelon_Watermelon_FeaturedLevel_Get` |
| `Watermelon.Watermelon_FeaturedLevel_Post` | `0x1be72c` | not recorded | `GE3_API_Watermelon_Watermelon_FeaturedLevel_Post` |
| `Watermelon.Watermelon_Followers_Page` | `0x1be050` | not recorded | `GE3_API_Watermelon_Watermelon_Followers_Page` |
| `Watermelon.Watermelon_Following_All` | `0x1bd3f4` | not recorded | `GE3_API_Watermelon_Watermelon_Following_All` |
| `Watermelon.Watermelon_Following_Delete` | `0x1bdb48` | not recorded | `GE3_API_Watermelon_Watermelon_Following_Delete` |
| `Watermelon.Watermelon_Following_Get` | `0x1bddcc` | not recorded | `GE3_API_Watermelon_Watermelon_Following_Get` |
| `Watermelon.Watermelon_Following_Page` | `0x1bd5e0` | not recorded | `GE3_API_Watermelon_Watermelon_Following_Page` |
| `Watermelon.Watermelon_Following_Put` | `0x1bd854` | not recorded | `GE3_API_Watermelon_Watermelon_Following_Put` |
| `Watermelon.Watermelon_GetBetaParticipationAgreement` | `0x1bfa24` | not recorded | `GE3_API_Watermelon_Watermelon_GetBetaParticipationAgreement` |
| `Watermelon.Watermelon_GetCloudSave` | `0x1bfb38` | not recorded | `GE3_API_Watermelon_Watermelon_GetCloudSave` |
| `Watermelon.Watermelon_GetContentImage` | `0x1bf948` | not recorded | `GE3_API_Watermelon_Watermelon_GetContentImage` |
| `Watermelon.Watermelon_GetEula` | `0x1b9538` | not recorded | `GE3_API_Watermelon_Watermelon_GetEula` |
| `Watermelon.Watermelon_GetEulaVersion` | `0x1b9640` | not recorded | `GE3_API_Watermelon_Watermelon_GetEulaVersion` |
| `Watermelon.Watermelon_GetLevelData` | `0x1b9f50` | not recorded | `GE3_API_Watermelon_Watermelon_GetLevelData` |
| `Watermelon.Watermelon_GetLevelDescription` | `0x1b9d70` | not recorded | `GE3_API_Watermelon_Watermelon_GetLevelDescription` |
| `Watermelon.Watermelon_GetLevelThumbnail` | `0x1b9e60` | not recorded | `GE3_API_Watermelon_Watermelon_GetLevelThumbnail` |
| `Watermelon.Watermelon_GetLevelVersions` | `0x1ba1c8` | not recorded | `GE3_API_Watermelon_Watermelon_GetLevelVersions` |
| `Watermelon.Watermelon_GetSnapshot` | `0x1ba408` | not recorded | `GE3_API_Watermelon_Watermelon_GetSnapshot` |
| `Watermelon.Watermelon_GetSoundData` | `0x1ba648` | not recorded | `GE3_API_Watermelon_Watermelon_GetSoundData` |
| `Watermelon.Watermelon_Get_Friends` | `0x1bd0e8` | not recorded | `GE3_API_Watermelon_Watermelon_Get_Friends` |
| `Watermelon.Watermelon_Grief_Post` | `0x1bca84` | not recorded | `GE3_API_Watermelon_Watermelon_Grief_Post` |
| `Watermelon.Watermelon_Index_Page` | `0x1ba738` | not recorded | `GE3_API_Watermelon_Watermelon_Index_Page` |
| `Watermelon.Watermelon_Leaderboard_All` | `0x1bc070` | not recorded | `GE3_API_Watermelon_Watermelon_Leaderboard_All` |
| `Watermelon.Watermelon_Leaderboard_Get` | `0x1bbc54` | not recorded | `GE3_API_Watermelon_Watermelon_Leaderboard_Get` |
| `Watermelon.Watermelon_Leaderboard_Near` | `0x1bc498` | not recorded | `GE3_API_Watermelon_Watermelon_Leaderboard_Near` |
| `Watermelon.Watermelon_Leaderboard_Page` | `0x1bc25c` | not recorded | `GE3_API_Watermelon_Watermelon_Leaderboard_Page` |
| `Watermelon.Watermelon_Leaderboard_Post` | `0x1bbe40` | not recorded | `GE3_API_Watermelon_Watermelon_Leaderboard_Post` |
| `Watermelon.Watermelon_LevelHearts_Delete` | `0x1bce4c` | not recorded | `GE3_API_Watermelon_Watermelon_LevelHearts_Delete` |
| `Watermelon.Watermelon_LevelHearts_Get` | `0x1bcd6c` | not recorded | `GE3_API_Watermelon_Watermelon_LevelHearts_Get` |
| `Watermelon.Watermelon_LevelHearts_Page` | `0x1bcf2c` | not recorded | `GE3_API_Watermelon_Watermelon_LevelHearts_Page` |
| `Watermelon.Watermelon_LevelHearts_Post` | `0x1bcc8c` | not recorded | `GE3_API_Watermelon_Watermelon_LevelHearts_Post` |
| `Watermelon.Watermelon_Metadata_All` | `0x1bb0f4` | not recorded | `GE3_API_Watermelon_Watermelon_Metadata_All` |
| `Watermelon.Watermelon_Metadata_Get` | `0x1bb2e0` | not recorded | `GE3_API_Watermelon_Watermelon_Metadata_Get` |
| `Watermelon.Watermelon_Metadata_Put` | `0x1bb4e0` | not recorded | `GE3_API_Watermelon_Watermelon_Metadata_Put` |
| `Watermelon.Watermelon_PostCloudSave` | `0x1bfca4` | not recorded | `GE3_API_Watermelon_Watermelon_PostCloudSave` |
| `Watermelon.Watermelon_PublishLevel` | `0x1b9804` | not recorded | `GE3_API_Watermelon_Watermelon_PublishLevel` |
| `Watermelon.Watermelon_Queued_Delete` | `0x1bf084` | not recorded | `GE3_API_Watermelon_Watermelon_Queued_Delete` |
| `Watermelon.Watermelon_Queued_Get` | `0x1beddc` | not recorded | `GE3_API_Watermelon_Watermelon_Queued_Get` |
| `Watermelon.Watermelon_Queued_Page` | `0x1be950` | not recorded | `GE3_API_Watermelon_Watermelon_Queued_Page` |
| `Watermelon.Watermelon_Queued_Put` | `0x1beb8c` | not recorded | `GE3_API_Watermelon_Watermelon_Queued_Put` |
| `Watermelon.Watermelon_Rating_Get` | `0x1bb6fc` | not recorded | `GE3_API_Watermelon_Watermelon_Rating_Get` |
| `Watermelon.Watermelon_Rating_Put` | `0x1bb99c` | not recorded | `GE3_API_Watermelon_Watermelon_Rating_Put` |
| `Watermelon.Watermelon_Record_Post` | `0x1bc774` | not recorded | `GE3_API_Watermelon_Watermelon_Record_Post` |
| `Watermelon.Watermelon_SaveLevelAndAssetsToDisk` | `0x1ba040` | not recorded | `GE3_API_Watermelon_Watermelon_SaveLevelAndAssetsToDisk` |
| `Watermelon.Watermelon_UnpublishLevel` | `0x1b9c94` | not recorded | `GE3_API_Watermelon_Watermelon_UnpublishLevel` |

## Inventory limits

- The recovered wrapper inventory contains names for analysis; some rows may be internal wrappers or aliases rather than callable script APIs. The unmatched section preserves those rows instead of silently dropping potentially useful APIs.
- The registered method inventory is not proof that all matched OpenEgg methods behave like retail. Check per-function API notes in [`docs/api`](docs/api/) and parity status in [`COMPATIBILITY_STATUS.md`](COMPATIBILITY_STATUS.md).
- Add recovered parameters, return values, ownership, and edge cases only when supported by wrapper/decomp evidence. Do not infer them from names alone.
