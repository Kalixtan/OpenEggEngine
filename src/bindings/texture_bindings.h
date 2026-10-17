#pragma once
#include "gmMachine.h"
#include "../render/texture.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class Renderer;

void RegisterTextureBindings(gmMachine& a_machine);

// Wraps a_handle (an existing GL texture handle from Renderer::LoadTexture)
// as a real TEXTURE object.
gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle);
gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle,
                              std::shared_ptr<void> a_resourceLease);
gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle,
                              std::shared_ptr<void> a_resourceLease,
                              float a_resizeFactor);
std::shared_ptr<void> MakeTextureResourceLease(unsigned int a_handle, int a_area,
                                                const std::string& a_path);
// Retain a path-loaded texture as manager-owned until the matching retail
// unload call releases it. Unlike Video.getTexture, Video.loadTexture does
// not return a script TEXTURE object.
void RetainManagedTextureResource(unsigned int a_handle, int a_area,
                                  const std::string& a_path);
// Releases manager-only leases during Video's delayed purge pass. Leases
// still referenced by a script TEXTURE or recorded draw command stay live.
std::size_t PurgeUnusedManagedTextureResources();
// Disconnects all outstanding leases from a renderer before its destruction.
void ReleaseTextureResourceLeases(Renderer& a_renderer);
std::shared_ptr<void> ResolveTextureResourceLease(gmMachine& a_machine,
                                                   const gmVariable& a_value);
bool UnloadTextureResourceByPath(int a_area, const std::string& a_path);
bool UnloadTextureResourceByHandle(gmMachine& a_machine, const gmVariable& a_value,
                                   int a_area);
std::size_t GetLiveTextureResourceCount();

// Extracts the underlying GL handle from a_var. Accepts either a real
// TEXTURE object (the normal case) or a bare gmint (defensive fallback --
// nothing in this engine should still be passing a raw int for a texture
// once every call site is converted, but a stray 0/null default is
// harmless to tolerate). Returns 0 (an always-invalid handle) for
// anything else.
unsigned int ResolveTextureHandle(gmMachine& a_machine, const gmVariable& a_var);
// Returns decoded/native texture dimensions, including palette-only textures
// created by Video.createPalette (which have no desktop GL image handle).
bool GetTextureNativeDimensions(gmMachine& a_machine, const gmVariable& a_value,
                                int& a_width, int& a_height);
float ResolveTextureDimensionScale(gmMachine& a_machine, const gmVariable& a_var);
// Applies the native TEXTURE dimension multiplication/truncation sequence.
float RetailScaledTextureDimension(uint32_t a_nativeDimension, float a_factor);
// Retail TEXTURE.setTextureShaderType writes this native selector at +0x48.
int GetTextureShaderType(gmMachine& a_machine, const gmVariable& a_var);

extern gmType g_textureType;

// Wraps a fresh palette-backed TEXTURE object (Video.createPalette).
gmVariable MakePaletteHandle(gmMachine& a_machine);

// Returns embedded palette storage for a palette-backed TEXTURE, else
// nullptr (defensive -- callers should treat that as "not a palette").
PaletteData* ResolvePaletteData(gmMachine& a_machine, const gmVariable& a_var);
std::shared_ptr<PaletteData> ResolvePaletteDataLease(gmMachine& a_machine,
                                                      const gmVariable& a_var);
void SetGeneratedPaletteColours(PaletteData& a_palette,
                                float a_fillR, float a_fillG, float a_fillB,
                                float a_outlineR, float a_outlineG, float a_outlineB);
// Applies the byte channels produced by Video.setTexturePaletteColour's
// PPC wrapper through the same native texture-palette storage used by
// TEXTURE.setPaletteColour.
void SetTexturePaletteColourByHandle(unsigned int a_handle, int a_index,
                                     uint8_t a_r, uint8_t a_g,
                                     uint8_t a_b, uint8_t a_a);
// Mirrors draw-list opcode 0x0f writing the image object's +0x4c child.
void BindTextureSubTexture(unsigned int a_handle, std::shared_ptr<PaletteData> a_palette);
