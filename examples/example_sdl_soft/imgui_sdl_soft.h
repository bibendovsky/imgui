// By Emil Ernerfeldt 2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// WHAT:
//   This is a software renderer for Dear ImGui.
//   It is decently fast, but has a lot of room for optimization.
//   The goal was to get something fast and decently accurate in not too many lines of code.
// LIMITATIONS:
//   * It is not pixel-perfect, but it is good enough for must use cases.
//   * It does not support painting with any other texture than the default font texture.


#include <vector>

#include "imgui.h"


namespace imgui_sw
{


typedef ImVector<ImU32> ColorBuffer;
extern ColorBuffer color_buffer_;


/// Call once a the start of your program.
void bind_imgui_painting();

/// The buffer is assumed to follow how ImGui packs pixels, i.e. ABGR by default.
/// Change with IMGUI_USE_BGRA_PACKED_COLOR.
/// If width/height differs from ImGui::GetIO().DisplaySize then
/// the function scales the UI to fit the given pixel buffer.
void paint_imgui(
	ImU32* const pixels,
	const int width_pixels,
	const int height_pixels);

/// Free the resources allocated by bind_imgui_painting.
void unbind_imgui_painting();


} // imgui_sw
