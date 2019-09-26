#[[


]]


cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)
project(imgui_example_sdl_sdl VERSION 1.0.0 LANGUAGES CXX)


# Find packages.
#
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(SDL2W 2.0.0 REQUIRED)


# Target.
#
add_executable(${PROJECT_NAME} "")


# Target properties.
#
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 98
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

if (WIN32)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			WIN32_EXECUTABLE TRUE
	)
endif ()

# Dependencies.
#
if (MINGW)
	target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
			-mconsole
	)
endif ()

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		SDL2W::SDL2W
		opengl32
)

# Linking options.
#
if (MSVC)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			LINK_FLAGS "/SUBSYSTEM:CONSOLE"
	)
endif ()

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		../..
		..
		.
)

# Header files.
#
target_sources(
	${PROJECT_NAME}
	PRIVATE
		../../imgui.h
		../imgui_impl_sdl.h
		../imgui_impl_sdl_ws.h
		imgui_sw.hpp
)

# Source files.
#
target_sources(
	${PROJECT_NAME}
	PRIVATE
		../../imgui.cpp
		../../imgui_demo.cpp
		../../imgui_draw.cpp
		../../imgui_widgets.cpp
		../imgui_impl_sdl.cpp
		../imgui_impl_sdl_ws.cpp
		imgui_sw.cpp
		main.cpp
)

# Install.
#
install(
	TARGETS
	${PROJECT_NAME}
		RUNTIME DESTINATION "."
)
