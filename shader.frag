#version 450

layout(binding = 1) uniform sampler2D u_texture;

layout(location = 0) in vec3 i_color;
layout(location = 1) in vec2 i_uv;

layout(location = 0) out vec4 o_color;

void main() {
    o_color = texture(u_texture, i_uv);
}
