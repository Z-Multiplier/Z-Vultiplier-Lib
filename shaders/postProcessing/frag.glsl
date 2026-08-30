#version 450
layout(binding = 0) uniform sampler2D screenTexture;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(screenTexture, 0);
    vec3 color = texture(screenTexture, uv).rgb;
    outColor = vec4(1.0 - color, 1.0);
}