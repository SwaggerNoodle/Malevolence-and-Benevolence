#ifndef MALEVOLENCE_AND_BENEVOLENCE_RENDERING_H
#define MALEVOLENCE_AND_BENEVOLENCE_RENDERING_H

#include "common.h"


typedef struct GameState GameState;
bool createSurface(GameState *game);


typedef struct CameraUBO {
    float view_proj[16];
} CameraUBO;
typedef struct Vertex {
    float x;
    float y;
    float z;
} Vertex;
typedef struct RenderObject {
    float x;
    float y;
    float z;

    float size_x;
    float size_y;
    float size_z;

    float r;
    float g;
    float b;
} RenderObject;
typedef struct InstanceData {
    float px;
    float py;
    float pz;

    float sx;
    float sy;
    float sz;

    float r;
    float g;
    float b;
} InstanceData;

typedef struct QueueFamilyIndices {
    bool has_graphics;
    bool has_present;
    uint32_t graphics_family;
    uint32_t present_family;
} QueueFamilyIndices;
typedef struct SwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR formats[32];
    uint32_t format_count;
    VkPresentModeKHR present_modes[32];
    uint32_t present_mode_count;
} SwapchainSupportInfo;


typedef struct VulkanState {
    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
    VkDevice device;

    uint32_t graphics_family;
    uint32_t present_family;
    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    uint32_t swapchain_image_count;

    VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
    VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGES];

    VkRenderPass render_pass;

    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets[MAX_SWAPCHAIN_IMAGES];

    VkCommandPool command_pool;
    VkCommandBuffer command_buffers[MAX_SWAPCHAIN_IMAGES];

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    uint32_t index_count;

    VkBuffer instance_buffer;
    VkDeviceMemory instance_buffer_memory;

    VkBuffer uniform_buffers[MAX_SWAPCHAIN_IMAGES];
    VkDeviceMemory uniform_buffer_memories[MAX_SWAPCHAIN_IMAGES];

    VkSemaphore image_available[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore render_finished[MAX_FRAMES_IN_FLIGHT];
    VkFence in_flight[MAX_FRAMES_IN_FLIGHT];

    bool framebuffer_resized;
    uint32_t current_frame;
} VulkanState;


extern const Vertex g_cube_vertices[MAX_CUBE_VERTICES];

extern const uint16_t g_cube_indices[MAX_CUBE_INDICES];


bool createGraphicsPipeline(GameState *game);

void pushRenderCube(GameState *game,
               float x,  float y,  float z,
               float sx, float sy, float sz,
               float r,  float g,  float b);

bool projectWorldToScreen(GameState *game,
                 float wx, float wy, float wz,
                 int *screen_x, int *screen_y);

void buildCameraMatrix(GameState *game, CameraUBO *ubo);

void buildRenderList(GameState *game);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

bool createMeshBuffers(GameState *game);

bool createInstanceBuffer(GameState *game);

bool updateInstanceBuffer(GameState *game);

bool createUniformBuffers(GameState *game);

void updateUniformBuffers(GameState *game);

bool createDescriptorSetLayout(GameState *game);

bool createDescriptorPool(GameState *game);

bool createDescriptorSets(GameState *game);

bool querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface, SwapchainSupportInfo *info);

bool deviceSupportsSwapchain(VkPhysicalDevice device);

bool pickPhysicalDevice(GameState *game);

bool createLogicalDevice(GameState *game);

VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupportInfo *support);

VkPresentModeKHR choosePresentMode(const SwapchainSupportInfo *support);

VkExtent2D chooseExtent(GameState *game, const VkSurfaceCapabilitiesKHR *caps);

void destroySwapchainObjects(GameState *game);

bool createSwapchain(GameState *game);

bool createImageViews(GameState *game);

bool createRenderPass(GameState *game);

bool createFramebuffers(GameState *game);

bool createCommandPool(GameState *game);

bool createCommandBuffers(GameState *game);

bool createSyncObjects(GameState *game);

bool recreateSwapchain(GameState *game);

bool createInstance(GameState *game);

bool createGraphicsPipeline(GameState *game);

bool recordCommandBuffer(GameState *game, uint32_t image_index);

bool drawFrame(GameState *game);

void cleanupVulkan(GameState *game);
#endif //MALEVOLENCE_AND_BENEVOLENCE_RENDERING_H
