// dear imgui: Renderer for OpenGL2 (legacy OpenGL, fixed pipeline)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in imgui_impl_opengl3.cpp**
// This code is mostly provided as a reference to learn how ImGui integration works, because it is shorter to read.
// If your code is using GL3+ context or any semi modern OpenGL calls, using this is likely to make everything more
// complicated, will require your code to reset every single OpenGL attributes to their initial state, and might
// confuse your GPU driver.
// The GL2 code is unable to reset attributes or even call e.g. "glUseProgram(0)" because they don't exist in that API.


#ifndef IMGUI_IMPL_SDL_SOFT_INCLUDED
#define IMGUI_IMPL_SDL_SOFT_INCLUDED


#include "imgui.h"


extern ImU32* color_buffer_;


IMGUI_IMPL_API bool ImGui_ImplSdlSoft_Init();
IMGUI_IMPL_API void ImGui_ImplSdlSoft_Shutdown();
IMGUI_IMPL_API void ImGui_ImplSdlSoft_NewFrame();
IMGUI_IMPL_API void ImGui_ImplSdlSoft_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown

IMGUI_IMPL_API bool ImGui_ImplSdlSoft_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplSdlSoft_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplSdlSoft_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplSdlSoft_DestroyDeviceObjects();


#endif // IMGUI_IMPL_SDL_SOFT_INCLUDED
