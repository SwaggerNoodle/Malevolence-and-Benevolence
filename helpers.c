#include "game.h"
#include "helpers.h"

bool Helper_findMemoryType(GameState *game,
				  uint32_t type_filter,
				  VkMemoryPropertyFlags properties,
				  uint32_t *memory_type_index
				  ){

	VkPhysicalDeviceMemoryProperties mem_properties;
	uint32_t i;

	vkGetPhysicalDeviceMemoryProperties(game->vk.physical_device, &mem_properties);

	for (i = 0; i < mem_properties.memoryTypeCount; i++){
		if ((type_filter & (1u << i)) &&
		    (mem_properties.memoryTypes[i].propertyFlags & properties) == properties){
			*memory_type_index = i;
			return true;
		}
	}
	return false;
}
bool Helper_createBuffer(GameState *game,
				VkDeviceSize size,
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags properties,
				VkBuffer *buffer,
				VkDeviceMemory *memory
				){

	VkBufferCreateInfo buffer_info = {0};
	VkMemoryRequirements mem_requirements;
	VkMemoryAllocateInfo alloc_info = {0};
	uint32_t memory_type_index = 0;

	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(game->vk.device, &buffer_info, NULL, buffer));
	vkGetBufferMemoryRequirements(game->vk.device, *buffer, &mem_requirements);

	if (!Helper_findMemoryType(game, mem_requirements.memoryTypeBits, properties, &memory_type_index)){
		fprintf(stderr, "Failed to find a suitable memory type\n");
		return false;
	}

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = memory_type_index;

	VK_CHECK(vkAllocateMemory(game->vk.device, &alloc_info, NULL, memory));
	VK_CHECK(vkBindBufferMemory(game->vk.device, *buffer, *memory, 0));

	return true;
}
bool Helper_readBinaryFile(const char *filename, unsigned char **out_data, size_t *out_size){
	FILE *f;
	long size;
	unsigned char *data;

	f = fopen(filename, "rb");
	if (!f){
		fprintf(stderr, "Failed to open file: %s\n", filename);
		return false;
	}
	if (fseek(f, 0, SEEK_END) != 0){
		fclose(f);
		return false;
	}
	size = ftell(f);
	if (size <= 0){
		fclose(f);
		return false;
	}

	rewind(f);
	data = (unsigned char *)malloc((size_t)size);
	if (!data){
		fclose(f);
		return false;
	}
	if (fread(data, 1, (size_t)size, f) != (size_t)size){
		free(data);
		fclose(f);
		return false;
	}
	fclose(f);
	*out_data = data;
	*out_size = (size_t)size;
	return true;
}
bool Helper_createShaderModule(GameState *game,
				      const char *filename,
				      VkShaderModule *shader_module){

	unsigned char *code = NULL;
	size_t code_size = 0;
	VkShaderModuleCreateInfo create_info = {0};

	if (!Helper_readBinaryFile(filename, &code, &code_size)){
		return false;
	}

	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code_size;
	create_info.pCode = (const uint32_t *)code;

	VK_CHECK(vkCreateShaderModule(game->vk.device, &create_info, NULL, shader_module));

	free(code);
	return true;
}
void Helper_mat4Identity(float m[16]){
	memset(m, 0, sizeof(float) * 16);
	m[0] = 1.0f;
	m[5] = 1.0f;
	m[10] = 1.0f;
	m[15] = 1.0f;
}
void Helper_mat4Multiply(float out[16], const float a[16], const float b[16]){
	int row;
	int column;
	int k;
	float temp[16];

	for (column = 0; column < 4; column++){
		for (row = 0; row < 4; row++){
			temp[column * 4 + row] = 0.0f;

			for (k = 0; k < 4; k++){
				temp[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];
			}
		}
	}
	memcpy(out, temp, sizeof(temp));
}
void Helper_mat4Perspective(float out[16], float fov_radians, float aspect, float znear, float zfar){
	float f = 1.0f / tanf(fov_radians * 0.5f);

	memset(out, 0, sizeof(float) * 16);
	out[0] = f / aspect;
	out[5] = -f;
	out[10] = zfar / (znear - zfar);
	out[11] = -1.0f;
	out[14] = (znear * zfar) / (znear - zfar);
}
void Helper_mat4Translate(float out[16], float x, float y, float z){
	Helper_mat4Identity(out);
	out[12] = x;
	out[13] = y;
	out[14] = z;
}
void Helper_mat4RotateX(float out[16], float radians){
	float c = cosf(radians);
	float s = sinf(radians);

	Helper_mat4Identity(out);
	out[5] = c;
	out[6] = s;
	out[9] = -s;
	out[10] = c;
}
bool Helper_mouseToWorldOnZPlane(GameState *game,
				 int mouse_x,
				 int mouse_y,
				 float plane_z,
				 float *out_x,
				 float *out_y){

	int pixel_w = 0;
	int pixel_h = 0;

	float aspect;
	float fov;
	float tan_half_fov;

	float ndc_x;
	float ndc_y;

	float ray_camera_x;
	float ray_camera_y;
	float ray_camera_z;

	float pitch_rad;
	float c;
	float s;

	float ray_world_x;
	float ray_world_y;
	float ray_world_z;

	float t;

	if (SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)){
		pixel_w = game->window_width;
		pixel_h = game->window_height;
	}
	if (pixel_w <= 0){
		pixel_w = 1;
	}
	if (pixel_h <= 0){
		pixel_h =1;
	}

	aspect = (float)pixel_w / (float)pixel_h;

	fov = 60.0f * (3.14159265f / 180.0f);
	tan_half_fov = tanf(fov *0.5f);

	ndc_x = ((float)mouse_x / (float)pixel_w) * 2.0f - 1.0f;
	ndc_y = ((float)mouse_y / (float)pixel_h) * 2.0f - 1.0f;

	ray_camera_x = ndc_x * aspect * tan_half_fov;
	ray_camera_y = ndc_y * tan_half_fov;
	ray_camera_z = -1.0f;

	pitch_rad = game->camera.pitch * (3.14159265f / 180.0f);
	c = cosf(pitch_rad);
	s = sinf(pitch_rad);

	ray_world_x = ray_camera_x;
	ray_world_y = ray_camera_y * c - ray_camera_z * s;
	ray_world_z = ray_camera_y * s + ray_camera_z * c;

	if (fabsf(ray_world_z) < 0.0001f){
		return false;
	}

	t = (plane_z - game->camera.z) / ray_world_z;

	if (t < 0.0f){
		return false;
	}

	*out_x = game->camera.x + ray_world_x * t;
	*out_y = game->camera.y + ray_world_y * t;

	return true;
}




