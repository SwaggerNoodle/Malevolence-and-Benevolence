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
bool Helper_mat4Inverse(float out[16], const float m[16]){
	float inverted[16];
	float det;
	int i;

	inverted[0] = m[5]  * m[10] * m[15] -
		      m[5]  * m[11] * m[14] -
      		      m[9]  * m[6]  * m[15] +
		      m[9]  * m[7]  * m[14] +
		      m[13] * m[6]  * m[11] -
		      m[13] * m[7]  * m[10];

	inverted[4] = -m[4]  * m[9]  * m[15] +
      		       m[4]  * m[11] * m[14] +
                       m[8]  * m[6]  * m[15] -
                       m[8]  * m[7]  * m[14] -
                       m[12] * m[6]  * m[11] +
                       m[12] * m[7]  * m[10];

	inverted[8] = m[4]  * m[9]  * m[15] -
                      m[4]  * m[11] * m[13] -
                      m[8]  * m[5]  * m[15] +
                      m[8]  * m[7]  * m[13] +
                      m[12] * m[5]  * m[11] -
                      m[12] * m[7]  * m[9];

        inverted[12] = -m[4]  * m[9]  * m[14] +
                        m[4]  * m[10] * m[13] +
                        m[8]  * m[5]  * m[14] -
                        m[8]  * m[6]  * m[13] -
                        m[12] * m[5]  * m[10] +
                        m[12] * m[6]  * m[9];

        inverted[1] = -m[1]  * m[10] * m[15] +
                       m[1]  * m[11] * m[14] +
                       m[9]  * m[2]  * m[15] -
                       m[9]  * m[3]  * m[14] -
                       m[13] * m[2]  * m[11] +
                       m[13] * m[3]  * m[10];

        inverted[5] = m[0]  * m[10] * m[15] -
                      m[0]  * m[11] * m[14] -
                      m[8]  * m[2]  * m[15] +
                      m[8]  * m[3]  * m[14] +
                      m[12] * m[2]  * m[11] -
                      m[12] * m[3]  * m[10];

        inverted[9] = -m[0]  * m[9]  * m[15] +
                       m[0]  * m[11] * m[13] +
                       m[8]  * m[1]  * m[15] -
                       m[8]  * m[3]  * m[13] -
                       m[12] * m[1]  * m[11] +
                       m[12] * m[3]  * m[9];

        inverted[13] = m[0]  * m[9]  * m[14] -
                       m[0]  * m[10] * m[13] -
                       m[8]  * m[1]  * m[14] +
                       m[8]  * m[2]  * m[13] +
                       m[12] * m[1]  * m[10] -
                       m[12] * m[2]  * m[9];

        inverted[2] = m[1]  * m[6] * m[15] -
                      m[1]  * m[7] * m[14] -
                      m[5]  * m[2] * m[15] +
                      m[5]  * m[3] * m[14] +
                      m[13] * m[2] * m[7] -
                      m[13] * m[3] * m[6];

        inverted[6] = -m[0]  * m[6] * m[15] +
                       m[0]  * m[7] * m[14] +
                       m[4]  * m[2] * m[15] -
                       m[4]  * m[3] * m[14] -
                       m[12] * m[2] * m[7] +
                       m[12] * m[3] * m[6];

        inverted[10] = m[0]  * m[5] * m[15] -
                       m[0]  * m[7] * m[13] -
                       m[4]  * m[1] * m[15] +
                       m[4]  * m[3] * m[13] +
                       m[12] * m[1] * m[7] -
                       m[12] * m[3] * m[5];

        inverted[14] = -m[0]  * m[5] * m[14] +
                        m[0]  * m[6] * m[13] +
                        m[4]  * m[1] * m[14] -
                        m[4]  * m[2] * m[13] -
                        m[12] * m[1] * m[6] +
                        m[12] * m[2] * m[5];

        inverted[3] = -m[1] * m[6] * m[11] +
                       m[1] * m[7] * m[10] +
                       m[5] * m[2] * m[11] -
                       m[5] * m[3] * m[10] -
                       m[9] * m[2] * m[7] +
                       m[9] * m[3] * m[6];

        inverted[7] = m[0] * m[6] * m[11] -
                      m[0] * m[7] * m[10] -
                      m[4] * m[2] * m[11] +
                      m[4] * m[3] * m[10] +
                      m[8] * m[2] * m[7] -
                      m[8] * m[3] * m[6];

        inverted[11] = -m[0] * m[5] * m[11] +
                        m[0] * m[7] * m[9] +
                        m[4] * m[1] * m[11] -
                        m[4] * m[3] * m[9] -
                        m[8] * m[1] * m[7] +
                        m[8] * m[3] * m[5];

        inverted[15] = m[0] * m[5] * m[10] -
                       m[0] * m[6] * m[9] -
                       m[4] * m[1] * m[10] +
                       m[4] * m[2] * m[9] +
                       m[8] * m[1] * m[6] -
                       m[8] * m[2] * m[5];

	det = m[0] * inverted[0] + m[1] * inverted[4] + m[2] * inverted[8] + m[3] * inverted[12];

	if (fabsf(det) < 0.000001f){
		return false;
	}

	det = 1.0f / det;

	for (i = 0; i < 16; i++){
		out[i] = inverted[i] * det;
	}
	return true;
}
bool Helper_unprojectPoint(const float inv_view_proj[16],
		 	   float ndc_x,
			   float ndc_y,
			   float ndc_z,
			   float out_point[3]){
	float x;
	float y;
	float z;
	float w;

	x = inv_view_proj[0]  * ndc_x +
            inv_view_proj[4]  * ndc_y +
            inv_view_proj[8]  * ndc_z +
            inv_view_proj[12];

        y = inv_view_proj[1]  * ndc_x +
            inv_view_proj[5]  * ndc_y +
            inv_view_proj[9]  * ndc_z +
            inv_view_proj[13];

        z = inv_view_proj[2]  * ndc_x +
            inv_view_proj[6]  * ndc_y +
            inv_view_proj[10] * ndc_z +
            inv_view_proj[14];

        w = inv_view_proj[3]  * ndc_x +
            inv_view_proj[7]  * ndc_y +
            inv_view_proj[11] * ndc_z +
            inv_view_proj[15];

	if (fabsf(w) < 0.000001f){
		return false;
	}

	out_point[0] = x / w;
	out_point[1] = y / w;
	out_point[2] = z / w;

	return true;
}


bool Helper_mouseToWorldOnZPlane(GameState *game,
				 int mouse_x,
				 int mouse_y,
				 float plane_z,
				 float *out_x,
				 float *out_y){

	int pixel_w = 0;
	int pixel_h = 0;

	float ndc_x;
	float ndc_y;

	CameraUBO ubo;
	float inv_view_proj[16];

	float near_point[3];
	float far_point[3];

	float ray_x;
	float ray_y;
	float ray_z;

	float t;

	if (game->vk.swapchain_extent.width > 0 && 
	    game->vk.swapchain_extent.height > 0){
		pixel_w = (int)game->vk.swapchain_extent.width;
		pixel_h = (int)game->vk.swapchain_extent.height;
	}
	else if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)){
		pixel_w = game->window_width;
		pixel_h = game->window_height;
	}
	if (pixel_w <= 0){
		pixel_w = 1;
	}
	if (pixel_h <= 0){
		pixel_h =1;
	}

	ndc_x = ((float)mouse_x / (float)pixel_w) * 2.0f - 1.0f;
	ndc_y = ((float)mouse_y / (float)pixel_h) * 2.0f - 1.0f;

	buildCameraMatrix(game, &ubo);

	if (!Helper_mat4Inverse(inv_view_proj, ubo.view_proj)){
		return false;
	}
	if (!Helper_unprojectPoint(inv_view_proj, ndc_x, ndc_y, 0.0f, near_point) ||
	    !Helper_unprojectPoint(inv_view_proj, ndc_x, ndc_y, 1.0f, far_point)){
		return false;
	}

	ray_x = far_point[0] - near_point[0];
	ray_y = far_point[1] - near_point[1];
	ray_z = far_point[2] - near_point[2];

	if (fabsf(ray_z) < 0.000001f){
		return false;
	}

	t = (plane_z - near_point[2]) / ray_z;

	if (t < 0.0f){
		return false;
	}

	*out_x = near_point[0] + ray_x * t;
	*out_y = near_point[1] + ray_y * t;

	return true;
}






















