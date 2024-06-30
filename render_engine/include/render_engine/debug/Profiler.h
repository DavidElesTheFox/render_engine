#pragma once

// TODO Replace with CMake parameter
#define RENDER_ENGINE_ENABLE_OPTICK 1

#if RENDER_ENGINE_ENABLE_OPTICK
#include <optick.h>

#define PROFILE_SCOPE(...) OPTICK_EVENT(__VA_ARGS__)
#define PROFILE_FRAME(...) OPTICK_FRAME(__VA_ARGS__)
#define PROFILE_THREAD(name) OPTICK_THREAD(name)

#else
#define PROFILE_SCOPE(...) 
#define PROFILE_FRAME(...) 
#define PROFILE_THREAD(name) 
#endif