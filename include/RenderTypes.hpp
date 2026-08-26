//MIT License

//Copyright (c) 2026 Z-Multiplier

#ifndef RENDER_TYPES_HPP
#define RENDER_TYPES_HPP

#include <glm/glm.hpp>
#include <vector>
#include "vulkan/vulkan.h"

namespace Render{
    struct Vertex{
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 uv={0,0};
    };

    struct DrawCommand{
        uint32_t firstVertex;
        uint32_t vertexCount;
        glm::mat4 transform;
        VkImageView textureView=VK_NULL_HANDLE;
        VkSampler textureSampler=VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet=VK_NULL_HANDLE;
        bool isTransparent=false;
    };

    struct TextPush{
        glm::mat4 transform;
        glm::vec4 color;
        uint32_t downsample;
        float width;
        float height;
    };
}

#endif