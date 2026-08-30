#version 450

layout(binding = 0) uniform sampler2D screenTexture;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(screenTexture, 0);
    vec3 color = texture(screenTexture, uv).rgb;
    
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    
    outColor = vec4(vec3(gray), 1.0);
}