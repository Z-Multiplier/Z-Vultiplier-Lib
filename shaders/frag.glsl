#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

// 从 set=1 binding=0 采样纹理（因为你的纹理描述符集使用独立布局，只有 binding=0）
layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    outColor = fragColor * texColor;
}