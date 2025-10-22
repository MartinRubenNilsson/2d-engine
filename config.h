#pragma once
#include "graphics_api_config.h"

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifdef _WIN32
#define PLATFORM_WINDOWS
#endif

#define APPLICATION_NAME "Action RPG"
#define ENGINE_NAME "No Engine"
#define WINDOW_TITLE "Action RPG"
#define GAME_FRAMEBUFFER_WIDTH 320
#define GAME_FRAMEBUFFER_HEIGHT 180

#ifdef _DEBUG
#define _DEBUG_RENDERDOC
#define _DEBUG_GRAPHICS
#endif