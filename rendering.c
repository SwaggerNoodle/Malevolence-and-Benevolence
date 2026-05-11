#include "game.h"
#include "common.h"
#include "helpers.h"
#include "rendering.h"


const Vertex g_cube_vertices[MAX_CUBE_VERTICES] = {
    {-0.5f, -0.5f, -0.5f},
    { 0.5f, -0.5f, -0.5f},
    { 0.5f,  0.5f, -0.5f},
    {-0.5f,  0.5f, -0.5f},
    {-0.5f, -0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    { 0.5f,  0.5f,  0.5f},
    {-0.5f,  0.5f,  0.5f},
};
const uint16_t g_cube_indices[MAX_CUBE_INDICES] = {
    0,1,2, 2,3,0,
    4,5,6, 6,7,4,
    0,4,7, 7,3,0,
    1,5,6, 6,2,1,
    3,2,6, 6,7,3,
    0,1,5, 5,4,0
};



void pushRenderCube(GameState *game,
               float x,  float y,  float z,
               float sx, float sy, float sz,
               float r,  float g,  float b){
    RenderObject *obj;
    if(game->render_object_count >= MAX_RENDER_OBJECTS){
        return;
    }
    obj = &game->render_objects[game->render_object_count++];

    obj->x = x;
    obj->y = y;
    obj->z = z;
    obj->size_x = sx;
    obj->size_y = sy;
    obj->size_z = sz;
    obj->r = r;
    obj->g = g;
    obj->b = b;
}



bool projectWorldToScreen(GameState *game,
	                  float wx, float wy, float wz,
         	          int *screen_x, int *screen_y){
	int pixel_w = 0;
	int pixel_h = 0;

	CameraUBO ubo;
	const float *m;

	float clip_x;
	float clip_y;
	float clip_z;
	float clip_w;

	float ndc_x;
	float ndc_y;
	float ndc_z;

	if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)){
		pixel_w = game->window_width;
		pixel_h = game->window_height;
	}
	if (pixel_h <= 0){
	        pixel_h = 1;
	}
	
	buildCameraMatrix(game, &ubo);
	m = ubo.view_proj;

	clip_x = m[0] * wx + m[4] * wy + m[8] * wz + m[12];
	clip_y = m[1] * wx + m[5] * wy + m[9] * wz + m[13];
	clip_z = m[2] * wx + m[6] * wy + m[10] * wz + m[14];
	clip_w = m[3] * wx + m[7] * wy + m[11] * wz + m[15];

	if (clip_w <= 0.0001f){
		return false;
	}

	ndc_x = clip_x / clip_w;
	ndc_y = clip_y / clip_w;
	ndc_z = clip_z / clip_w;

	if (ndc_z < 0.0f || ndc_z > 1.0f){
		return false;
	}


	*screen_x = (int)((ndc_x * 0.5f + 0.5f) * (float)pixel_w);
	*screen_y = (int)((ndc_y * 0.5f + 0.5f) * (float)pixel_h);
	
	return true;
}



void buildCameraMatrix(GameState *game, CameraUBO *ubo){
	float proj[16];
	float rot[16];
	float trans[16];
	float view[16];
	float aspect = (float)game->vk.swapchain_extent.width / (float)game->vk.swapchain_extent.height;
	float pitch_rad = game->camera.pitch * (3.14159265f / 180.0f);

	Helper_mat4Perspective(proj, 60.0f * (3.14159265f / 180.0f), aspect, 0.1f, 100.0f);
	proj[5] *= -1.0f; //OpenGL to Vulkan translation bandaid

	Helper_mat4RotateX(rot, pitch_rad);
	Helper_mat4Translate(trans, -game->camera.x, -game->camera.y, -game->camera.z);

	Helper_mat4Multiply(view, rot, trans);
	Helper_mat4Multiply(ubo->view_proj, proj, view);
}



void buildRenderList(GameState *game){
    int i;
    game->render_object_count = 0;
    pushRenderCube(game,
               game->temple.x,      game->temple.y,      game->temple.z,
               game->temple.size_x, game->temple.size_y, game->temple.size_z,
               0.70f, 0.70f, 0.70f);

    for (i = 0; i < game->work_site_count; i++){
        WorkSite *site = &game->work_sites[i];
        float r = 0.7f;
        float g = 0.7f;
        float b = 0.7f;
        if (!site->active){
            continue;
        }
        switch (site->type){
            case SITE_FOOD:
                r = 0.90f; g = 0.65f; b = 0.20f;
                break;
            case SITE_WORSHIP:
                r = 0.55f; g = 0.35f; b = 0.9f;
                break;
        }
        pushRenderCube(game,
                   site->x,      site->y,      site->z,
                   site->size_x, site->size_y, site->size_z,
                   r, g, b);
    }
    for (i = 0; i < game->villager_count; i++){
        Villager *villager = &game->villagers[i];
        float r = 1.0f;
        float g = 0.9f;
        float b = 0.25f;
        if (!villager->active){
            continue;
        }
        switch (villager->state){
            case VILLAGER_IDLE:
                r = 1.00f; g = 0.90f; b = 0.25f;
                break;
            case VILLAGER_HELD:
                r = 1.00f; g = 0.40f; b = 0.25f;
                break;
            case VILLAGER_WALKING_TO_TARGET:
                r = 0.35f; g = 0.85f; b = 0.35f;
                break;
            case VILLAGER_WORKING:
                r = 0.35f; g = 1.00f; b = 0.35f;
        }
        pushRenderCube(game,
                   villager->x,      villager->y,      villager->z,
                   villager->size_x, villager->size_y, villager->size_z,
                   r, g, b);
    }
}



QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface){
    QueueFamilyIndices indices = {0};
    uint32_t count = 0;
    uint32_t i;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);

    if (count == 0){
        return indices;
    }

    VkQueueFamilyProperties *props = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * count);
    if (!props){
        return indices;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);
    for (i = 0; i < count; i++){
        VkBool32 present_support = VK_FALSE;
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.has_graphics = true;
            indices.graphics_family = i;
        }
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support){
            indices.has_present = true;
            indices.present_family = i;
        }
        if (indices.has_graphics && indices.has_present){
            break;
        }
    }
    free(props);
    return indices;
}



bool createMeshBuffers(GameState *game){
    VkDeviceSize vertex_size = sizeof(g_cube_vertices);
    VkDeviceSize index_size = sizeof(g_cube_indices);
    void *mapped = NULL;

    if (!Helper_createBuffer(game,
              vertex_size,
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              &game->vk.vertex_buffer,
              &game->vk.vertex_buffer_memory)){
        return false;
              }
    if (!Helper_createBuffer(game,
                          index_size,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &game->vk.index_buffer,
                          &game->vk.index_buffer_memory)){
        return false;
                          }

    VK_CHECK(vkMapMemory(game->vk.device, game->vk.vertex_buffer_memory, 0, vertex_size, 0, &mapped));
    memcpy(mapped, g_cube_vertices, (size_t)vertex_size);
    vkUnmapMemory(game->vk.device, game->vk.vertex_buffer_memory);

    VK_CHECK(vkMapMemory(game->vk.device, game->vk.index_buffer_memory, 0, index_size, 0, &mapped));
    memcpy(mapped, g_cube_indices, (size_t)index_size);
    vkUnmapMemory(game->vk.device, game->vk.index_buffer_memory);

    game->vk.index_count = MAX_CUBE_INDICES;
    return true;
}



bool createInstanceBuffer(GameState *game){
    VkDeviceSize size = sizeof(InstanceData) * MAX_RENDER_OBJECTS;

    return Helper_createBuffer(game,
                size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &game->vk.instance_buffer,
                &game->vk.instance_buffer_memory);
}



bool updateInstanceBuffer(GameState *game){

    InstanceData instances[MAX_RENDER_OBJECTS];
    void *mapped = NULL;
    uint32_t i;
    uint32_t count = (uint32_t)game->render_object_count;

    if (count > MAX_RENDER_OBJECTS){
        count = MAX_RENDER_OBJECTS;
    }

    for (i = 0; i < count; i++){
        instances[i].px = game->render_objects[i].x;
        instances[i].py = game->render_objects[i].y;
        instances[i].pz = game->render_objects[i].z;

        instances[i].sx = game->render_objects[i].size_x;
        instances[i].sy = game->render_objects[i].size_y;
        instances[i].sz = game->render_objects[i].size_z;

        instances[i].r = game->render_objects[i].r;
        instances[i].g = game->render_objects[i].g;
        instances[i].b = game->render_objects[i].b;
    }

    VK_CHECK(vkMapMemory(game->vk.device,
                 game->vk.instance_buffer_memory,
                 0,
                 sizeof(InstanceData) * count,
                 0,
                 &mapped));

    memcpy(mapped, instances, sizeof(InstanceData) * count);
    vkUnmapMemory(game->vk.device, game->vk.instance_buffer_memory);

    return true;
}



bool createUniformBuffers(GameState *game){
    uint32_t i;
    VkDeviceSize size = sizeof(CameraUBO);

    for (i = 0; i < game->vk.swapchain_image_count; i++){
        if (!Helper_createBuffer(game,
                  size,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &game->vk.uniform_buffers[i],
                  &game->vk.uniform_buffer_memories[i])){
            return false;
                  }
    }
    return true;
}



bool updateUniformBuffer(GameState *game, uint32_t image_index){
    CameraUBO ubo;
    void *mapped = NULL;

    buildCameraMatrix(game, &ubo);

    VK_CHECK(vkMapMemory(game->vk.device,
                 game->vk.uniform_buffer_memories[image_index],
                 0,
                 sizeof(ubo),
                 0,
                 &mapped));

    memcpy(mapped, &ubo, sizeof(ubo));
    vkUnmapMemory(game->vk.device, game->vk.uniform_buffer_memories[image_index]);

    return true;
}



bool createDescriptorSetLayout(GameState *game){
    VkDescriptorSetLayoutBinding ubo_binding = {0};
    VkDescriptorSetLayoutCreateInfo layout_info = {0};

    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(game->vk.device,
                         &layout_info,
                         NULL,
                         &game->vk.descriptor_set_layout));
    return true;
}



bool createDescriptorPool(GameState *game){
    VkDescriptorPoolSize pool_size = {0};
    VkDescriptorPoolCreateInfo pool_info = {0};

    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = game->vk.swapchain_image_count;

    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = game->vk.swapchain_image_count;

    VK_CHECK(vkCreateDescriptorPool(game->vk.device, &pool_info, NULL, &game->vk.descriptor_pool));
    return true;
}



bool createDescriptorSets(GameState *game){
    VkDescriptorSetLayout layouts[MAX_SWAPCHAIN_IMAGES];
    VkDescriptorSetAllocateInfo alloc_info = {0};
    VkWriteDescriptorSet writes[MAX_SWAPCHAIN_IMAGES] = {0};
    VkDescriptorBufferInfo buffer_infos[MAX_SWAPCHAIN_IMAGES] = {0};
    uint32_t i;

    for (i = 0; i < game->vk.swapchain_image_count; i++){
        layouts[i] = game->vk.descriptor_set_layout;
    }

    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = game->vk.descriptor_pool;
    alloc_info.descriptorSetCount = game->vk.swapchain_image_count;
    alloc_info.pSetLayouts = layouts;

    VK_CHECK(vkAllocateDescriptorSets(game->vk.device, &alloc_info, game->vk.descriptor_sets));

    for (i = 0; i < game->vk.swapchain_image_count; i++){
        buffer_infos[i].buffer = game->vk.uniform_buffers[i];
        buffer_infos[i].offset = 0;
        buffer_infos[i].range = sizeof(CameraUBO);

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = game->vk.descriptor_sets[i];
        writes[i].dstBinding = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[i].descriptorCount = 1;
        writes[i].pBufferInfo = &buffer_infos[i];
    }

    vkUpdateDescriptorSets(game->vk.device,
                   game->vk.swapchain_image_count,
                   writes,
                   0,
                   NULL);
    return true;
}



bool querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface, SwapchainSupportInfo *info){
    uint32_t count = 0;
    memset(info, 0, sizeof(*info));

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &info->capabilities));
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, NULL));
    if (count > 0){
        if (count > 32){
            count = 32;
        }
        info->format_count = count;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, info->formats));
    }
    count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, NULL));
    if (count > 0){
        if (count > 32){
            count = 32;
        }
        info->present_mode_count = count;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, info->present_modes));
    }
    return true;
}



bool deviceSupportsSwapchain(VkPhysicalDevice device){
    uint32_t extension_count = 0;
    uint32_t i;
    bool found_swapchain = false;

    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL));
    if (extension_count == 0){
        return false;
    }
    VkExtensionProperties *extensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * extension_count);
    if (!extensions){
        return false;
    }
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extensions));
    for (i = 0; i < extension_count; i++){
        if (strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0){
            found_swapchain = true;
            break;
        }
    }
    free(extensions);
    return found_swapchain;
}



bool pickPhysicalDevice(GameState *game){
    VulkanState *vk = &game->vk;
    uint32_t device_count = 0;
    uint32_t i;

    VK_CHECK(vkEnumeratePhysicalDevices(vk->instance, &device_count, NULL));
    if (device_count == 0){
        fprintf(stderr, "No Vulkan physical devices found\n");
        return false;
    }
    VkPhysicalDevice *devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * device_count);
    if (!devices){
        return false;
    }
    VK_CHECK(vkEnumeratePhysicalDevices(vk->instance, &device_count, devices));
    for (i = 0; i <device_count; i++){
        QueueFamilyIndices indices;
        SwapchainSupportInfo support;
        if (!deviceSupportsSwapchain(devices[i])){
            continue;
        }
        indices = findQueueFamilies(devices[i], vk->surface);
        if (!indices.has_graphics || !indices.has_present){
            continue;
        }
        if (!querySwapchainSupport(devices[i], vk->surface, &support)){
            continue;
        }
        if (support.format_count == 0 || support.present_mode_count == 0){
            continue;
        }
        vk->physical_device = devices[i];
        vk->graphics_family = indices.graphics_family;
        vk->present_family = indices.present_family;
        break;
    }
    free(devices);
    if (vk->physical_device == VK_NULL_HANDLE){
        fprintf(stderr, "No suitable Vulkan device found\n");
        return false;
    }
    return true;
}



bool createLogicalDevice(GameState *game){
    VulkanState *vk = &game->vk;
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_infos[2];
    uint32_t queue_info_count = 0;
    VkPhysicalDeviceFeatures features = {0};
    VkDeviceCreateInfo create_info = {0};
    const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    memset(queue_infos, 0, sizeof(queue_infos));
    queue_infos[queue_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos[queue_info_count].queueFamilyIndex = vk->graphics_family;
    queue_infos[queue_info_count].queueCount = 1;
    queue_infos[queue_info_count].pQueuePriorities = &priority;
    queue_info_count++;

    if (vk->present_family != vk->graphics_family){
        queue_infos[queue_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_infos[queue_info_count].queueFamilyIndex = vk->present_family;
        queue_infos[queue_info_count].queueCount = 1;
        queue_infos[queue_info_count].pQueuePriorities = &priority;
        queue_info_count++;
    }
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_info_count;
    create_info.pQueueCreateInfos = queue_infos;
    create_info.pEnabledFeatures = &features;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = extensions;

    VK_CHECK(vkCreateDevice(vk->physical_device, &create_info, NULL, &vk->device));
    vkGetDeviceQueue(vk->device, vk->graphics_family, 0, &vk->graphics_queue);
    vkGetDeviceQueue(vk->device, vk->present_family, 0, &vk->present_queue);
    return true;
}



VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupportInfo *support){
    uint32_t i;

    for (i = 0; i < support->format_count; i++){
        if (support->formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            support->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return support->formats[i];
            }
    }
    return support->formats[0];
}



VkPresentModeKHR choosePresentMode(const SwapchainSupportInfo *support){
    uint32_t i;

    for (i = 0; i < support->present_mode_count; i++){
        if (support->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            return support->present_modes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}



VkExtent2D chooseExtent(GameState *game, const VkSurfaceCapabilitiesKHR *caps){
    VkExtent2D extent;
    int pixel_w = 0;
    int pixel_h = 0;
    if (caps->currentExtent.width != UINT32_MAX){
        return caps->currentExtent;
    }
    if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)){
        pixel_w = game->window_width;
        pixel_h = game->window_height;
    }
    extent.width = (uint32_t)pixel_w;
    extent.height = (uint32_t)pixel_h;

    if (extent.width < caps->minImageExtent.width) extent.width = caps->minImageExtent.width;
    if (extent.width > caps->maxImageExtent.width) extent.width = caps->maxImageExtent.width;
    if (extent.height < caps->minImageExtent.height) extent.height = caps->minImageExtent.height;
    if (extent.height > caps->maxImageExtent.height) extent.height = caps->maxImageExtent.height;

    return extent;
}



void destroySwapchainObjects(GameState *game){
    VulkanState *vk = &game->vk;
    uint32_t i;

    for (i = 0; i < vk->swapchain_image_count; i++){
        if (vk->uniform_buffers[i] != VK_NULL_HANDLE){
            vkDestroyBuffer(vk->device, vk->uniform_buffers[i], NULL);
            vk->uniform_buffers[i] = VK_NULL_HANDLE;
        }
        if (vk->uniform_buffer_memories[i] != VK_NULL_HANDLE){
            vkFreeMemory(vk->device, vk->uniform_buffer_memories[i], NULL);
            vk->uniform_buffer_memories[i] = VK_NULL_HANDLE;
        }
    }

    if (vk->descriptor_pool != VK_NULL_HANDLE){
        vkDestroyDescriptorPool(vk->device, vk->descriptor_pool, NULL);
        vk->descriptor_pool = VK_NULL_HANDLE;
    }

    for (i = 0; i < vk->swapchain_image_count; i++){
        if (vk->framebuffers[i] != VK_NULL_HANDLE){
            vkDestroyFramebuffer(vk->device, vk->framebuffers[i], NULL);
            vk->framebuffers[i] = VK_NULL_HANDLE;
        }
        if (vk->swapchain_image_views[i] != VK_NULL_HANDLE){
            vkDestroyImageView(vk->device, vk->swapchain_image_views[i], NULL);
            vk->swapchain_image_views[i] = VK_NULL_HANDLE;
        }

    }

    if (vk->graphics_pipeline != VK_NULL_HANDLE){
        vkDestroyPipeline(vk->device, vk->graphics_pipeline, NULL);
        vk->graphics_pipeline = VK_NULL_HANDLE;
    }

    if (vk->pipeline_layout != VK_NULL_HANDLE){
        vkDestroyPipelineLayout(vk->device, vk->pipeline_layout, NULL);
        vk->pipeline_layout = VK_NULL_HANDLE;
    }

    if (vk->render_pass != VK_NULL_HANDLE){
        vkDestroyRenderPass(vk->device, vk->render_pass, NULL);
        vk->render_pass = VK_NULL_HANDLE;
    }
    if (vk->swapchain != VK_NULL_HANDLE){
        vkDestroySwapchainKHR(vk->device, vk->swapchain, NULL);
        vk->swapchain = VK_NULL_HANDLE;
    }
    vk->swapchain_image_count = 0;
}



bool createSwapchain(GameState *game){

	VulkanState *vk = &game->vk;
	SwapchainSupportInfo support;
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;
	VkSwapchainCreateInfoKHR create_info = {0};
	uint32_t image_count;
	uint32_t queue_family_indices[2];

	if (!querySwapchainSupport(vk->physical_device, vk->surface, &support)){
		return false;
	}

	surface_format = chooseSurfaceFormat(&support);
	present_mode = choosePresentMode(&support);
	vk->swapchain_extent = chooseExtent(game, &support.capabilities);

	image_count = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount){
		image_count = support.capabilities.maxImageCount;
	}
	if (image_count > MAX_SWAPCHAIN_IMAGES){
		image_count = MAX_SWAPCHAIN_IMAGES;
	}

	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = vk->surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = vk->swapchain_extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.preTransform = support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vk->graphics_family != vk->present_family){

		queue_family_indices[0] = vk->graphics_family;
		queue_family_indices[1] = vk->present_family;
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.pQueueFamilyIndices = queue_family_indices;
		create_info.queueFamilyIndexCount = 2;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VK_CHECK(vkCreateSwapchainKHR(vk->device, &create_info, NULL, &vk->swapchain));
	VK_CHECK(vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &image_count, NULL));
	if (image_count > MAX_SWAPCHAIN_IMAGES){
		fprintf(stderr, "Swapchain has more images than MAX_SWAPCHAIN_IMAGES\n");
		return false;
	}
	vk->swapchain_image_count = image_count;
	VK_CHECK(vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &image_count, vk->swapchain_images));
	vk->swapchain_format = surface_format.format;
	return true;
}



bool createImageViews(GameState *game){
    VulkanState *vk = &game->vk;
    uint32_t i;

    for (i = 0; i < vk->swapchain_image_count; i++){

        VkImageViewCreateInfo create_info ={0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = vk->swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = vk->swapchain_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(vk->device, &create_info, NULL, &vk->swapchain_image_views[i]));
    }
    return true;
}



bool createRenderPass(GameState *game){
    VulkanState *vk = &game->vk;
    VkAttachmentDescription color_attachment = {0};
    VkAttachmentReference color_attachment_ref = {0};
    VkSubpassDescription subpass = {0};
    VkSubpassDependency dependency = {0};
    VkRenderPassCreateInfo render_pass_info = {0};

    color_attachment.format = vk->swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(vk->device, &render_pass_info, NULL, &vk->render_pass));
    return true;
}



bool createFramebuffers(GameState *game){
    VulkanState *vk = &game->vk;
    uint32_t i;

    for (i = 0; i < vk->swapchain_image_count; i++) {
        VkImageView attachments[1] = { vk->swapchain_image_views[i] };
        VkFramebufferCreateInfo framebuffer_info = {0};

        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = vk->render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = vk->swapchain_extent.width;
        framebuffer_info.height = vk->swapchain_extent.height;
        framebuffer_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(vk->device, &framebuffer_info, NULL, &vk->framebuffers[i]));
    }

    return true;
}



bool createCommandPool(GameState *game){
    VulkanState *vk = &game->vk;
    VkCommandPoolCreateInfo pool_info = {0};

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = vk->graphics_family;

    VK_CHECK(vkCreateCommandPool(vk->device, &pool_info, NULL, &vk->command_pool));
    return true;
}



bool createCommandBuffers(GameState *game){
    VulkanState *vk = &game->vk;
    VkCommandBufferAllocateInfo alloc_info = {0};

    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = vk->command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = vk->swapchain_image_count;

    VK_CHECK(vkAllocateCommandBuffers(vk->device, &alloc_info, vk->command_buffers));
    return true;
}



bool createSyncObjects(GameState *game){
    VulkanState *vk = &game->vk;
    VkSemaphoreCreateInfo semaphore_info = {0};
    VkFenceCreateInfo fence_info = {0};
    uint32_t i;

    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(vk->device, &semaphore_info, NULL, &vk->image_available[i]));
        VK_CHECK(vkCreateSemaphore(vk->device, &semaphore_info, NULL, &vk->render_finished[i]));
        VK_CHECK(vkCreateFence(vk->device, &fence_info, NULL, &vk->in_flight[i]));
    }

    return true;
}



bool recreateSwapchain(GameState *game){
    VulkanState *vk = &game->vk;
    int pixel_w = 0;
    int pixel_h = 0;

    if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)) {
        pixel_w = game->window_width;
        pixel_h = game->window_height;
    }

    while (pixel_w == 0 || pixel_h == 0) {
        SDL_WaitEvent(NULL);
        if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)) {
            pixel_w = game->window_width;
            pixel_h = game->window_height;
        }
    }

    vkDeviceWaitIdle(vk->device);
    destroySwapchainObjects(game);

    if (!createSwapchain(game)) return false;
    if (!createImageViews(game)) return false;
    if (!createRenderPass(game)) return false;
    if (!createGraphicsPipeline(game)) return false;
    if (!createFramebuffers(game)) return false;
    if (!createUniformBuffers(game)) return false;
    if (!createDescriptorPool(game)) return false;
    if (!createDescriptorSets(game)) return false;

    vk->framebuffer_resized = false;
    return true;
}



bool createInstance(GameState *game){
    VulkanState *vk = &game->vk;
    Uint32 ext_count = 0;
    const char * const *ext_names = NULL;
    VkApplicationInfo app_info = {0};
    VkInstanceCreateInfo create_info = {0};

    ext_names = SDL_Vulkan_GetInstanceExtensions(&ext_count);
    if (!ext_names) {
        fprintf(stderr, "SDL_Vulkan_GetInstanceExtensions failed: %s\n", SDL_GetError());
        return false;
    }

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "BW Vulkan Prototype";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "CustomBWEngine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = ext_count;
    create_info.ppEnabledExtensionNames = ext_names;

    VK_CHECK(vkCreateInstance(&create_info, NULL, &vk->instance));
    return true;
}
bool createSurface(GameState *game) {
    VulkanState *vk = &game->vk;
    if (!SDL_Vulkan_CreateSurface(game->window, vk->instance, NULL, &vk->surface)) {
        fprintf(stderr, "SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}



bool createGraphicsPipeline(GameState *game){
	VkShaderModule vert_module = VK_NULL_HANDLE;
	VkShaderModule frag_module = VK_NULL_HANDLE;

	VkPipelineShaderStageCreateInfo shader_stages[2] = {0};

	VkVertexInputBindingDescription bindings[2] = {0};
	VkVertexInputAttributeDescription attributes[4] = {0};
	VkPipelineVertexInputStateCreateInfo vertex_input = {0};
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
	VkViewport viewport = {0};
	VkRect2D scissor = {0};
	VkPipelineViewportStateCreateInfo viewport_state = {0};
	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
	VkPipelineColorBlendStateCreateInfo color_blending = {0};
	VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
	VkGraphicsPipelineCreateInfo pipeline_info = {0};

	if (!Helper_createShaderModule(game, "shader.vert.spv", &vert_module)){
		return false;
	}
	if (!Helper_createShaderModule(game, "shader.frag.spv", &frag_module)){
		vkDestroyShaderModule(game->vk.device, vert_module, NULL);
		return false;
	}


	shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module = vert_module;
	shader_stages[0].pName = "main";

	shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module = frag_module;
	shader_stages[1].pName = "main";

	bindings[0].binding = 0;
	bindings[0].stride = sizeof(Vertex);
	bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	bindings[1].binding = 1;
	bindings[1].stride = sizeof(InstanceData);
	bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	attributes[0].location = 0;
	attributes[0].binding = 0;
	attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[0].offset = offsetof(Vertex, x);

	attributes[1].location = 1;
	attributes[1].binding = 1;
	attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[1].offset = offsetof(InstanceData, px);

	attributes[2].location = 2;
	attributes[2].binding = 1;
	attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[2].offset = offsetof(InstanceData, sx);

	attributes[3].location = 3;
	attributes[3].binding = 1;
	attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[3].offset = offsetof(InstanceData, r);

	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.vertexBindingDescriptionCount = 2;
	vertex_input.pVertexBindingDescriptions = bindings;
	vertex_input.vertexAttributeDescriptionCount = 4;
	vertex_input.pVertexAttributeDescriptions = attributes;

	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)game->vk.swapchain_extent.width;
	viewport.height = (float)game->vk.swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = game->vk.swapchain_extent;

	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;

	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &game->vk.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(game->vk.device,
					&pipeline_layout_info,
					NULL,
					&game->vk.pipeline_layout));

	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.layout = game->vk.pipeline_layout;
	pipeline_info.renderPass = game->vk.render_pass;
	pipeline_info.subpass = 0;

	VK_CHECK(vkCreateGraphicsPipelines(game->vk.device,
					   VK_NULL_HANDLE,
					   1,
					   &pipeline_info,
					   NULL,
					   &game->vk.graphics_pipeline));

	vkDestroyShaderModule(game->vk.device, frag_module, NULL);
	vkDestroyShaderModule(game->vk.device, vert_module, NULL);
	return true;
}



bool recordCommandBuffer(GameState *game, uint32_t image_index){
    VulkanState *vk = &game->vk;
    VkCommandBuffer cmd = vk->command_buffers[image_index];
    VkCommandBufferBeginInfo begin_info = {0};
    VkClearValue clear_value;
    VkRenderPassBeginInfo render_pass_info = {0};
    VkBuffer vertex_buffers[2];
    VkDeviceSize offsets[2] = {0, 0};

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    clear_value.color.float32[0] = 0.10f;
    clear_value.color.float32[1] = 0.12f;
    clear_value.color.float32[2] = 0.18f;
    clear_value.color.float32[3] = 1.0f;

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = vk->render_pass;
    render_pass_info.framebuffer = vk->framebuffers[image_index];
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent = vk->swapchain_extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->graphics_pipeline);

    vertex_buffers[0] = vk->vertex_buffer;
    vertex_buffers[1] = vk->instance_buffer;

    vkCmdBindVertexBuffers(cmd, 0, 2, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(cmd, vk->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                vk->pipeline_layout,
                0,
                1,
                &vk->descriptor_sets[image_index],
                0,
                NULL);

    vkCmdDrawIndexed(cmd,
             vk->index_count,
             (uint32_t)game->render_object_count,
             0,
             0,
             0);

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));
    return true;
}



bool drawFrame(GameState *game){
	VulkanState *vk = &game->vk;
	uint32_t image_index = 0;
	VkSubmitInfo submit_info = {0};
	VkPresentInfoKHR present_info = {0};
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkResult result;

	VK_CHECK(vkWaitForFences(vk->device, 1, &vk->in_flight[vk->current_frame], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(vk->device, 1, &vk->in_flight[vk->current_frame]));

	result = vkAcquireNextImageKHR(vk->device,
         	                       vk->swapchain,
                	               UINT64_MAX,
                        	       vk->image_available[vk->current_frame],
				       VK_NULL_HANDLE,
	                               &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
	    return recreateSwapchain(game);
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		fprintf(stderr, "vkAcquireNextImageKHR failed with %d\n", (int)result);
		return false;
	}

	VK_CHECK(vkResetCommandBuffer(vk->command_buffers[image_index], 0));
	if (!updateUniformBuffer(game, image_index)){
		return false;
	}
	if (!updateInstanceBuffer(game)){
		return false;
	}
	if (!recordCommandBuffer(game, image_index)) {
		return false;
	}

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &vk->image_available[vk->current_frame];
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk->command_buffers[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &vk->render_finished[vk->current_frame];

	VK_CHECK(vkQueueSubmit(vk->graphics_queue,
		  	       1,
			       &submit_info,
			       vk->in_flight[vk->current_frame]));

	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk->render_finished[vk->current_frame];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk->swapchain;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(vk->present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vk->framebuffer_resized) {
		return recreateSwapchain(game);
	}

	if (result != VK_SUCCESS) {
		fprintf(stderr, "vkQueuePresentKHR failed with %d\n", (int)result);
		return false;
	}

	vk->current_frame = (vk->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}



void cleanupVulkan(GameState *game){
	VulkanState *vk = &game->vk;
	uint32_t i;

	if (vk->device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(vk->device);
	}

	destroySwapchainObjects(game);

	for (i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vk->image_available[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(vk->device, vk->image_available[i], NULL);
		}
		if (vk->render_finished[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(vk->device, vk->render_finished[i], NULL);
		}
		if (vk->in_flight[i] != VK_NULL_HANDLE) {
			vkDestroyFence(vk->device, vk->in_flight[i], NULL);
		}
	}

	if (vk->command_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(vk->device, vk->command_pool, NULL);
	}

	if (vk->instance_buffer != VK_NULL_HANDLE){
		vkDestroyBuffer(vk->device, vk->instance_buffer, NULL);
	}
	if (vk->instance_buffer_memory != VK_NULL_HANDLE){
		vkFreeMemory(vk->device, vk->instance_buffer_memory, NULL);
	}

	if (vk->index_buffer != VK_NULL_HANDLE){
		vkDestroyBuffer(vk->device, vk->index_buffer, NULL);
	}
	if (vk->index_buffer_memory != VK_NULL_HANDLE){
		vkFreeMemory(vk->device, vk->index_buffer_memory, NULL);
	}

	if (vk->vertex_buffer != VK_NULL_HANDLE){
		vkDestroyBuffer(vk->device, vk->vertex_buffer, NULL);
	}
	if (vk->vertex_buffer_memory != VK_NULL_HANDLE){
		vkFreeMemory(vk->device, vk->vertex_buffer_memory, NULL);
	}

	if (vk->descriptor_set_layout != VK_NULL_HANDLE){
		vkDestroyDescriptorSetLayout(vk->device, vk->descriptor_set_layout, NULL);
	}

	if (vk->device != VK_NULL_HANDLE) {
		vkDestroyDevice(vk->device, NULL);
	}

	if (vk->surface != VK_NULL_HANDLE) {
		SDL_Vulkan_DestroySurface(vk->instance, vk->surface, NULL);
	}

	if (vk->instance != VK_NULL_HANDLE) {
		vkDestroyInstance(vk->instance, NULL);
	}
}
