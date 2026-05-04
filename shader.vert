#version 450

layout(location = 0) in vec3 in_pos;

layout(location = 1) in vec3 in_instance_pos;
layout(location = 2) in vec3 in_instance_scale;
layout(location = 3) in vec3 in_instance_color;

layout(location = 0) out vec3 frag_color;

layout(set = 0, binding = 0) uniform CameraUBO {
	mat4 view_proj;
} ubo;

void main(){
	vec3 world_pos = in_instance_pos + (in_pos * in_instance_scale);
	gl_Position = ubo.view_proj * vec4(world_pos, 1.0);
	frag_color = in_instance_color;
}

