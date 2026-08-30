#version 450

layout(binding = 0) uniform sampler2D screenTexture;
layout(location = 0) out vec4 outColor;


void main() {
    float pixelSize = 8.0; // 可通过 push constant 传入
    vec2 uv = gl_FragCoord.xy / textureSize(screenTexture, 0);
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    
    // 对像素坐标取整，实现块状化
    vec2 pixelatedCoord = floor(gl_FragCoord.xy / pixelSize) * pixelSize + pixelSize * 0.5;
    vec2 uvPixelated = pixelatedCoord * texelSize;
    
    vec3 color = texture(screenTexture, uvPixelated).rgb;
    outColor = vec4(color, 1.0);
}