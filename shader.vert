#version 450

layout(binding = 0) uniform Buffer {
  vec2 offset;
} u_buffer;

layout(location = 0) in vec2 i_position;
layout(location = 1) in vec2 i_uv;

layout(location = 0) out vec3 o_color;
layout(location = 1) out vec2 o_uv;

void main() {
  gl_Position = vec4(i_position + u_buffer.offset, 0.0, 1.0);
  o_uv = i_uv;
}
