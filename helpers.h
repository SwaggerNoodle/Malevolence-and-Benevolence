#ifndef MALEVOLENCE_AND_BENEVOLENCE_HELPERS_H
#define MALEVOLENCE_AND_BENEVOLENCE_HELPERS_H

#include "common.h"

typedef struct GameState GameState;


bool Helper_findMemoryType(GameState *game,
				uint32_t type_filter,
				VkMemoryPropertyFlags properties,
				uint32_t *memory_type_index
);

bool Helper_createBuffer(GameState *game,
				VkDeviceSize size,
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags properties,
				VkBuffer *buffer,
				VkDeviceMemory *memory
);

bool Helper_readBinaryFile(const char *filename,
				unsigned char **out_data,
				 size_t *out_size
);

bool Helper_createShaderModule(GameState *game,
					const char *filename,
					VkShaderModule *shader_module
);

void Helper_mat4Identity(float m[16]);

void Helper_mat4Multiply(float out[16],
					const float a[16],
					 const float b[16]
);

void Helper_mat4Perspective(float out[16],
					float fov_radians,
					float aspect,
					float znear,
					float zfar
);

void Helper_mat4Translate(float out[16],
					float x,
					float y,
					float z
);

void Helper_mat4RotateX(float out[16], float radians);

#endif //MALEVOLENCE_AND_BENEVOLENCE_HELPERS_H
