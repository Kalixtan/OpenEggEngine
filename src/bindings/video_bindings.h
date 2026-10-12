#pragma once
#include "gmMachine.h"
#include <optional>
#include <string>

void RegisterVideoBindings(gmMachine& a_machine);

// Per-render-frame consumer for Video graphics-state maintenance fields.
// Currently includes the bytecode-recovered delayed texture-purge counter.
void UpdateVideoTextureMaintenance();

// Resolves a resource path using retail's active PS3_ override probe. Shared
// by Video/DrawList textures and AnimationModule FSA loading.
std::optional<std::string> ResolvePlatformResourcePath(const char* a_path, int a_area);
std::optional<std::string> ResolveTextureAssetPath(const char* a_path, int a_area);
