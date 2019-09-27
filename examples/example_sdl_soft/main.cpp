// dear imgui: standalone example application for SDL2 + OpenGL
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl_opengl3/ folder**
// See imgui_impl_sdl.cpp for details.


#include <iostream>

#include "SDL.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdl_soft.h"
#include "imgui_sdl_soft.h"


// Main code
int main(int, char**)
{
	const int sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER;

	const int sdl_init_result = SDL_Init(sdl_init_flags);

	if (sdl_init_result != 0)
	{
		std::cout << "[ERROR] " << SDL_GetError() << std::endl;

		return 1;
	}

	// Setup window
	SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(
		//SDL_WINDOW_OPENGL |
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_ALLOW_HIGHDPI |
		0
	);

	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;

	const int fb_width = 1280;
	const int fb_height = 720;

	const int sdl_create_window_and_renderer_result =
		SDL_CreateWindowAndRenderer(fb_width, fb_height, window_flags, &window, &renderer);

	if (sdl_create_window_and_renderer_result != 0)
	{
		std::cout << "[ERROR] " << SDL_GetError() << std::endl;

		return 1;
	}

	SDL_SetWindowTitle(window, "Dear ImGui SDL2 soft example");

	SDL_Texture* texture = NULL;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, fb_width, fb_height);

	if (!texture)
	{
		std::cout << "[ERROR] " << SDL_GetError() << std::endl;

		return 1;
	}


	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	static_cast<void>(io);

	// Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForD3D(window);
	ImGui_ImplSdlSoft_Init();

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);

	const SDL_Color sdl_clear_color =
	{
		static_cast<Uint8>(clear_color.x * 255.0F),
		static_cast<Uint8>(clear_color.y * 255.0F),
		static_cast<Uint8>(clear_color.z * 255.0F),
		static_cast<Uint8>(clear_color.w * 255.0F)
	};


	// Main loop
	bool done = false;

	while (!done)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT)
			{
				done = true;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplSdlSoft_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()!
		//    You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
		{
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0F;
			static int counter = 0;

			// Create a window called "Hello, world!" and append into it.
			ImGui::Begin("Hello, world!");

			// Display some text (you can use a format strings too)
			ImGui::Text("This is some useful text.");

			// Edit bools storing our window open/close state
			ImGui::Checkbox("Demo Window", &show_demo_window);

			ImGui::Checkbox("Another Window", &show_another_window);

			// Edit 1 float using a slider from 0.0 to 1.0
			ImGui::SliderFloat("float", &f, 0.0F, 1.0F);

			// Edit 3 floats representing a color
			ImGui::ColorEdit3("clear color", reinterpret_cast<float*>(&clear_color));

			// Buttons return true when clicked (most widgets return true when edited/activated)
			if (ImGui::Button("Button"))
			{
				++counter;
			}

			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0F / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			// Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Begin("Another Window", &show_another_window);

			ImGui::Text("Hello from another window!");

			if (ImGui::Button("Close Me"))
			{
				show_another_window = false;
			}

			ImGui::End();
		}

		// Rendering
		ImGui::Render();

		SDL_Rect sdl_viewport;
		sdl_viewport.x = 0;
		sdl_viewport.y = 0;
		sdl_viewport.w = static_cast<int>(io.DisplaySize.x);
		sdl_viewport.h = static_cast<int>(io.DisplaySize.y);

		SDL_RenderSetViewport(renderer, &sdl_viewport);

		SDL_SetRenderDrawColor(
			renderer,
			sdl_clear_color.r,
			sdl_clear_color.g,
			sdl_clear_color.b,
			sdl_clear_color.a
		);

		SDL_RenderClear(renderer);

		ImGui_ImplSdlSoft_RenderDrawData(ImGui::GetDrawData());

		if (!imgui_sw::color_buffer_.empty())
		{
			void* raw_pixels = NULL;
			int pitch = 0;
			const int ideal_pitch = fb_width * 4;

			const int sdl_lock_texture_result = SDL_LockTexture(texture, NULL, &raw_pixels, &pitch);

			if (sdl_lock_texture_result == 0)
			{
				std::uint32_t* pixels = static_cast<std::uint32_t*>(raw_pixels);

				if (pitch == ideal_pitch)
				{
					std::uninitialized_copy_n(
						&imgui_sw::color_buffer_[0],
						fb_width * fb_height,
						pixels
					);
				}
				else
				{
					for (int i = 0; i < fb_height; ++i)
					{
						std::uninitialized_copy_n(
							&imgui_sw::color_buffer_[i * fb_width],
							fb_width,
							pixels
						);

						pixels += pitch;
					}
				}

				SDL_UnlockTexture(texture);
			}

			std::fill(
				imgui_sw::color_buffer_.begin(),
				imgui_sw::color_buffer_.end(),
				std::uint32_t()
			);
		}

		const int sdl_render_copy_result = SDL_RenderCopy(renderer, texture, NULL, NULL);

		SDL_RenderPresent(renderer);
	}

	// Cleanup
	ImGui_ImplSdlSoft_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
