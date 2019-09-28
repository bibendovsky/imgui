// dear imgui: Renderer for SDL
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
// ...


#include "imgui_impl_sdl_soft.h"

#include <vector>

#include "imgui_sdl_soft.h"


namespace
{


int g_FontTexture = 0;


} // namespace


bool ImGui_ImplSdlSoft_Init()
{
	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_sdl_soft";
	io.IniFilename = NULL;

	return true;
}

void ImGui_ImplSdlSoft_Shutdown()
{
	ImGui_ImplSdlSoft_DestroyDeviceObjects();
}

void ImGui_ImplSdlSoft_NewFrame()
{
	if (g_FontTexture == 0)
	{
		ImGui_ImplSdlSoft_CreateDeviceObjects();
	}
}

void ImGui_ImplSdlSoft_RenderDrawData(ImDrawData* draw_data)
{
	const int width = static_cast<int>(draw_data->DisplaySize.x);
	const int height = static_cast<int>(draw_data->DisplaySize.y);
	const int area = width * height;

	if (imgui_sw::color_buffer_.size() < area)
	{
		imgui_sw::color_buffer_.resize(area);
	}

	imgui_sw::paint_imgui(&imgui_sw::color_buffer_[0], width, height);
}

void ImGui_ImplSdlSoft_DestroyFontsTexture()
{
	imgui_sw::unbind_imgui_painting();
	g_FontTexture = 0;
}

bool ImGui_ImplSdlSoft_CreateDeviceObjects()
{
	g_FontTexture = 1;
	imgui_sw::bind_imgui_painting();

	return true;
}

void ImGui_ImplSdlSoft_DestroyDeviceObjects()
{
	ImGui_ImplSdlSoft_DestroyFontsTexture();
}
