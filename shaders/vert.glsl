#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;

layout(binding = 0) uniform ViewProj {
    mat4 view;
    mat4 proj;
} vp;

// 注意：这里只接收 mat4，因为 C++ 端只 push 矩阵
layout(push_constant) uniform Model {
    mat4 model;
} pc;

void main() {
    gl_Position = vp.proj * vp.view * pc.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}