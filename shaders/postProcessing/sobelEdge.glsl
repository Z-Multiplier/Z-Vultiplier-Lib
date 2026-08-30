#version 450

layout(binding = 0) uniform sampler2D screenTexture;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(screenTexture, 0);
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    
    // Sobel 算子
    mat3 sobelX = mat3(
        -1.0, 0.0, 1.0,
        -2.0, 0.0, 2.0,
        -1.0, 0.0, 1.0
    );
    mat3 sobelY = mat3(
        -1.0, -2.0, -1.0,
        0.0, 0.0, 0.0,
        1.0, 2.0, 1.0
    );
    
    float gx = 0.0;
    float gy = 0.0;
    
    // 采样 3x3 邻域，只取亮度
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 offset = vec2(i, j) * texelSize;
            vec3 sampleColor = texture(screenTexture, uv + offset).rgb;
            float luminance = dot(sampleColor, vec3(0.299, 0.587, 0.114));
            gx += luminance * sobelX[i+1][j+1];
            gy += luminance * sobelY[i+1][j+1];
        }
    }
    
    float edge = sqrt(gx * gx + gy * gy);
    
    // 边缘用黑色，非边缘用白色（或者保留原色）
    vec3 edgeColor = vec3(1.0 - edge);
    outColor = vec4(edgeColor, 1.0);
}