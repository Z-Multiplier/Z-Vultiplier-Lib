#version 450

layout(binding = 0) uniform sampler2D screenTexture;
layout(location = 0) out vec4 outColor;

// 5x5 高斯核（近似值）
const float kernel[25] = float[25](
    1.0/256.0, 4.0/256.0, 6.0/256.0, 4.0/256.0, 1.0/256.0,
    4.0/256.0, 16.0/256.0, 24.0/256.0, 16.0/256.0, 4.0/256.0,
    6.0/256.0, 24.0/256.0, 36.0/256.0, 24.0/256.0, 6.0/256.0,
    4.0/256.0, 16.0/256.0, 24.0/256.0, 16.0/256.0, 4.0/256.0,
    1.0/256.0, 4.0/256.0, 6.0/256.0, 4.0/256.0, 1.0/256.0
);

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(screenTexture, 0);
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    
    vec3 result = vec3(0.0);
    int index = 0;
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 offset = vec2(x, y) * texelSize;
            result += texture(screenTexture, uv + offset).rgb * kernel[index];
            index++;
        }
    }
    
    outColor = vec4(result, 1.0);
}