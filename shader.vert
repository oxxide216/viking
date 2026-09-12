#version 450

layout(binding = 0) uniform UniformBufferObject {
  vec2 offset;
} ubo;

layout(location = 0) in vec2 i_position;
layout(location = 1) in vec3 i_color;

layout(location = 0) out vec3 color;

void main() {
  gl_Position = vec4(i_position + ubo.offset, 0.0, 1.0);
  color = i_color;
}
