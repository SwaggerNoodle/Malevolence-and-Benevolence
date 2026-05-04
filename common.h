#ifndef MALEVOLENCE_AND_BENEVOLENCE_COMMON_H
#define MALEVOLENCE_AND_BENEVOLENCE_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#define MAX_VILLAGERS 32
#define MAX_WORK_SITES 16
#define MAX_RENDER_OBJECTS 128
#define MAX_SWAPCHAIN_IMAGES 8
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_CUBE_VERTICES 8
#define MAX_CUBE_INDICES 36


#define VK_CHECK(x) do { \
        VkResult _vk_result = (x); \
        if (_vk_result != VK_SUCCESS) { \
                fprintf(stderr, "%s failed with VkResult %d at %s:%d\n", #x, (int)_vk_result, __FILE__, __LINE__); \
                return false; \
        } \
} while (0)

#endif
