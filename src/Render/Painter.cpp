//MIT License

//Copyright (c) 2026 Z-Multiplier
#define QUICK_DEBUG
#include "Painter.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

#include <stdexcept>
#include <source_location>
#include <fstream>
#include <vector>
#include <cstring>
#include <array>
#include <cmath>
#include <filesystem>
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
#define STB_TRUETYPE_IMPLEMENTATION
#include "VEFontCache/stb_truetype.h"

#include "VEFontCache/utf8.h"

static uint32_t utf8_to_codepoint(const char*& ptr){
    if(!ptr||*ptr=='\0'){
        return 0;
    }

    utf8_int32_t codepoint;
    ptr=(const char*)utf8codepoint(ptr,&codepoint);

    if(codepoint==0||codepoint==0xFFFD){
        return 0;
    }

    return static_cast<uint32_t>(codepoint);
}

static std::vector<char> readFile(const std::string& filename){
    std::filesystem::path path(filename);
    if(!std::filesystem::exists(path)){
        std::filesystem::path fallback=std::filesystem::current_path()/"build"/filename;
        if(std::filesystem::exists(fallback)){
            path=fallback;
            Core::globalLogger.traceLog(
                Core::logger::LOG_INFO,
                "Using fallback shader path: "+path.string(),
                std::source_location::current()
            );
        }
    }

    std::ifstream file(path,std::ios::ate|std::ios::binary);
    if(!file.is_open()){
        throw std::runtime_error("Failed to open file: "+path.string());
    }
    size_t fileSize=static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(),fileSize);
    return buffer;
}

namespace Render{
    Painter::Image::Image(const std::string& filepath,Painter* painter){
        this->painter=painter;
        loadFromFile(filepath);
    }

    Painter::Image::~Image(){
        cleanup();
    }

    void Painter::Image::loadFromFile(const std::string& filepath){
        FILE* test=fopen(filepath.c_str(),"rb");
        if(!test){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "File not found: "+filepath,
                std::source_location::current()
            );
            throw std::runtime_error("File not found: "+filepath);
        }
        fclose(test);

        int w,h,comp;
        unsigned char* data=stbi_load(filepath.c_str(),&w,&h,&comp,STBI_rgb_alpha);

        if(!data){
            const char* reason=stbi_failure_reason();
            std::string errMsg="Failed to load image: "+filepath;
            if(reason){
                errMsg+=" (stb_image reason: "+std::string(reason)+")";
            }
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                errMsg,
                std::source_location::current()
            );
            throw std::runtime_error(errMsg);
        }

        if(w<=0||h<=0){
            stbi_image_free(data);
            throw std::runtime_error("Invalid image dimensions: "+std::to_string(w)+"x"+std::to_string(h));
        }

        width=w;
        height=h;
        channels=4;
        pixels.resize(width*height*channels);
        memcpy(pixels.data(),data,pixels.size());

        stbi_image_free(data);

        isLoaded=true;

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Image loaded: "+filepath+" ("+std::to_string(width)+"x"+std::to_string(height)+")",
            std::source_location::current()
        );
    }

    void Painter::Image::createTexture(VkDevice device,VkCommandPool pool,VkQueue queue){
        if(!isLoaded){
            throw std::runtime_error("Cannot create texture: image not loaded!");
        }
        if(textureImage!=VK_NULL_HANDLE){
            return;
        }

        VkPhysicalDevice physicalDevice=painter->thiscontext->getInitializer()->getPhysicalDevice();

        VkDeviceSize imageSize=width*height*4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size=imageSize;
        bufferInfo.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device,&bufferInfo,nullptr,&stagingBuffer)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create staging buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device,stagingBuffer,&memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize=memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProperties);

        uint32_t memoryTypeIndex=UINT32_MAX;
        for(uint32_t i=0;i<memProperties.memoryTypeCount;++i){
            if((memRequirements.memoryTypeBits&(1<<i))&&
                (memProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)&&
                (memProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                memoryTypeIndex=i;
                break;
            }
        }
        if(memoryTypeIndex==UINT32_MAX){
            throw std::runtime_error("Failed to find suitable memory type for staging buffer!");
        }
        allocInfo.memoryTypeIndex=memoryTypeIndex;

        if(vkAllocateMemory(device,&allocInfo,nullptr,&stagingBufferMemory)!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate staging buffer memory!");
        }

        vkBindBufferMemory(device,stagingBuffer,stagingBufferMemory,0);

        void* data;
        vkMapMemory(device,stagingBufferMemory,0,imageSize,0,&data);
        memcpy(data,pixels.data(),imageSize);
        vkUnmapMemory(device,stagingBufferMemory);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType=VK_IMAGE_TYPE_2D;
        imageInfo.extent.width=static_cast<uint32_t>(width);
        imageInfo.extent.height=static_cast<uint32_t>(height);
        imageInfo.extent.depth=1;
        imageInfo.mipLevels=1;
        imageInfo.arrayLayers=1;
        imageInfo.format=VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling=VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateImage(device,&imageInfo,nullptr,&textureImage)!=VK_SUCCESS){
            vkDestroyBuffer(device,stagingBuffer,nullptr);
            vkFreeMemory(device,stagingBufferMemory,nullptr);
            throw std::runtime_error("Failed to create texture image!");
        }

        vkGetImageMemoryRequirements(device,textureImage,&memRequirements);

        allocInfo.allocationSize=memRequirements.size;
        memoryTypeIndex=UINT32_MAX;
        for(uint32_t i=0;i<memProperties.memoryTypeCount;++i){
            if((memRequirements.memoryTypeBits&(1<<i))&&
                (memProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
                memoryTypeIndex=i;
                break;
            }
        }
        if(memoryTypeIndex==UINT32_MAX){
            vkDestroyImage(device,textureImage,nullptr);
            vkDestroyBuffer(device,stagingBuffer,nullptr);
            vkFreeMemory(device,stagingBufferMemory,nullptr);
            throw std::runtime_error("Failed to find suitable memory type for texture!");
        }
        allocInfo.memoryTypeIndex=memoryTypeIndex;

        if(vkAllocateMemory(device,&allocInfo,nullptr,&textureImageMemory)!=VK_SUCCESS){
            vkDestroyImage(device,textureImage,nullptr);
            vkDestroyBuffer(device,stagingBuffer,nullptr);
            vkFreeMemory(device,stagingBufferMemory,nullptr);
            throw std::runtime_error("Failed to allocate texture memory!");
        }

        vkBindImageMemory(device,textureImage,textureImageMemory,0);

        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool=pool;
        cmdAllocInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount=1;

        if(vkAllocateCommandBuffers(device,&cmdAllocInfo,&commandBuffer)!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate command buffer!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer,&beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=textureImage;
        barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel=0;
        barrier.subresourceRange.levelCount=1;
        barrier.subresourceRange.baseArrayLayer=0;
        barrier.subresourceRange.layerCount=1;
        barrier.srcAccessMask=0;
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);

        VkBufferImageCopy region{};
        region.bufferOffset=0;
        region.bufferRowLength=0;
        region.bufferImageHeight=0;
        region.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel=0;
        region.imageSubresource.baseArrayLayer=0;
        region.imageSubresource.layerCount=1;
        region.imageOffset={0,0,0};
        region.imageExtent={static_cast<uint32_t>(width),static_cast<uint32_t>(height),1};

        vkCmdCopyBufferToImage(commandBuffer,stagingBuffer,textureImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);

        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount=1;
        submitInfo.pCommandBuffers=&commandBuffer;

        vkQueueSubmit(queue,1,&submitInfo,VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkDestroyBuffer(device,stagingBuffer,nullptr);
        vkFreeMemory(device,stagingBufferMemory,nullptr);
        vkFreeCommandBuffers(device,pool,1,&commandBuffer);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image=textureImage;
        viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format=VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel=0;
        viewInfo.subresourceRange.levelCount=1;
        viewInfo.subresourceRange.baseArrayLayer=0;
        viewInfo.subresourceRange.layerCount=1;

        if(vkCreateImageView(device,&viewInfo,nullptr,&imageView)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create image view!");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter=VK_FILTER_LINEAR;
        samplerInfo.minFilter=VK_FILTER_LINEAR;
        samplerInfo.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable=VK_TRUE;
        samplerInfo.maxAnisotropy=16.0f;
        samplerInfo.borderColor=VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates=VK_FALSE;
        samplerInfo.compareEnable=VK_FALSE;
        samplerInfo.compareOp=VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias=0.0f;
        samplerInfo.minLod=0.0f;
        samplerInfo.maxLod=0.0f;

        if(vkCreateSampler(device,&samplerInfo,nullptr,&sampler)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create sampler!");
        }

        pixels.clear();
        pixels.shrink_to_fit();

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Texture created: "+std::to_string(width)+"x"+std::to_string(height),
            std::source_location::current()
        );
    }

    void Painter::Image::allocateDescriptorSet(){
        if(descriptorSet!=VK_NULL_HANDLE) return;

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "allocateDescriptorSet: pool="+std::to_string((uintptr_t)painter->thisDescriptorPool)+
            ",textureLayout="+std::to_string((uintptr_t)painter->thisTextureDescriptorSetLayout),
            std::source_location::current()
        );

        VkDevice device=painter->thiscontext->getInitializer()->getDevice();

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool=painter->thisDescriptorPool;
        allocInfo.descriptorSetCount=1;
        allocInfo.pSetLayouts=&painter->thisTextureDescriptorSetLayout;

        VkResult result=vkAllocateDescriptorSets(device,&allocInfo,&descriptorSet);
        if(result!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate descriptor set! error: "+std::to_string(result));
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView=imageView;
        imageInfo.sampler=sampler;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet=descriptorSet;
        writeSet.dstBinding=0;
        writeSet.dstArrayElement=0;
        writeSet.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount=1;
        writeSet.pImageInfo=&imageInfo;

        vkUpdateDescriptorSets(device,1,&writeSet,0,nullptr);
    }

    void Painter::Image::cleanup(){
        VkDevice device=painter?painter->thiscontext->getInitializer()->getDevice():VK_NULL_HANDLE;
        if(device==VK_NULL_HANDLE) return;

        if(descriptorSet!=VK_NULL_HANDLE){
            descriptorSet=VK_NULL_HANDLE;
        }
        if(sampler!=VK_NULL_HANDLE){
            vkDestroySampler(device,sampler,nullptr);
            sampler=VK_NULL_HANDLE;
        }
        if(imageView!=VK_NULL_HANDLE){
            vkDestroyImageView(device,imageView,nullptr);
            imageView=VK_NULL_HANDLE;
        }
        if(textureImage!=VK_NULL_HANDLE){
            vkDestroyImage(device,textureImage,nullptr);
            textureImage=VK_NULL_HANDLE;
        }
        if(textureImageMemory!=VK_NULL_HANDLE){
            vkFreeMemory(device,textureImageMemory,nullptr);
            textureImageMemory=VK_NULL_HANDLE;
        }
        isLoaded=false;
    }
    static std::vector<vec2> generateRoundedRectVertices(vec2 pos,vec2 size,float radius,int segments=12){
        std::vector<vec2> vertices;
        float halfW=size.x*0.5f;
        float halfH=size.y*0.5f;
        radius=std::min(radius,std::min(halfW,halfH));

        if(radius<0.001f){
            vertices.push_back(pos);
            vertices.push_back({pos.x+size.x,pos.y});
            vertices.push_back({pos.x+size.x,pos.y+size.y});
            vertices.push_back({pos.x,pos.y+size.y});
            return vertices;
        }

        vec2 centers[4]={
            {pos.x+radius,pos.y+radius},
            {pos.x+size.x-radius,pos.y+radius},
            {pos.x+size.x-radius,pos.y+size.y-radius},
            {pos.x+radius,pos.y+size.y-radius}
        };

        auto addArc=[&](const vec2& center,float startDeg,float endDeg){
            float startRad=startDeg*3.14159265f/180.0f;
            float endRad=endDeg*3.14159265f/180.0f;
            float step=(endRad-startRad)/segments;
            for(int i=0;i<=segments;++i){
                float angle=startRad+i*step;
                vertices.push_back(center+vec2(radius*cosf(angle),radius*sinf(angle)));
            }
        };

        addArc(centers[0],180.0f,270.0f);
        vertices.push_back({pos.x+size.x-radius,pos.y});
        addArc(centers[1],270.0f,360.0f);
        vertices.push_back({pos.x+size.x,pos.y+size.y-radius});
        addArc(centers[2],0.0f,90.0f);
        vertices.push_back({pos.x+radius,pos.y+size.y});
        addArc(centers[3],90.0f,180.0f);

        return vertices;
    }
    VkShaderModule Render::Painter::createShaderModule(const std::vector<char>& code){
        VkDevice device=thiscontext->getInitializer()->getDevice();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize=code.size();
        createInfo.pCode=reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if(vkCreateShaderModule(device,&createInfo,nullptr,&shaderModule)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create shader module!");
        }
        return shaderModule;
    }
    Painter::Painter(Window::WindowContext& context)
        :thiscontext(&context){
        createRenderPass();
        createFramebuffers();

        createUniformBuffer();
        createDescriptorSetLayout();

        createGraphicsPipeline();
        createVertexBuffer();

        createDefaultTexture();

        initTextSystem();

        thisProjectionMatrix=glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Painter created.",
            std::source_location::current()
        );
    }
    Painter::~Painter(){
        cleanup();

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Painter destroyed.",
            std::source_location::current()
        );
    }

    void Painter::updateTextureBinding(VkImageView imageView,VkSampler sampler){
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView=imageView;
        imageInfo.sampler=sampler;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet=thisDescriptorSet;
        writeSet.dstBinding=1;
        writeSet.dstArrayElement=0;
        writeSet.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount=1;
        writeSet.pImageInfo=&imageInfo;

        vkUpdateDescriptorSets(thiscontext->getInitializer()->getDevice(),1,&writeSet,0,nullptr);
    }

    void Painter::createDefaultTexture(){
        VkDevice device=thiscontext->getInitializer()->getDevice();
        VkPhysicalDevice physicalDevice=thiscontext->getInitializer()->getPhysicalDevice();

        uint32_t whitePixel=0xFFFFFFFF;
        VkDeviceSize imageSize=sizeof(uint32_t);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size=imageSize;
        bufferInfo.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device,&bufferInfo,nullptr,&stagingBuffer);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device,stagingBuffer,&memReqs);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize=memReqs.size;
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProps);
        uint32_t memType=UINT32_MAX;
        for(uint32_t i=0;i<memProps.memoryTypeCount;++i){
            if((memReqs.memoryTypeBits&(1<<i))&&
                (memProps.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)&&
                (memProps.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                memType=i;
                break;
            }
        }
        allocInfo.memoryTypeIndex=memType;
        vkAllocateMemory(device,&allocInfo,nullptr,&stagingMemory);
        vkBindBufferMemory(device,stagingBuffer,stagingMemory,0);

        void* data;
        vkMapMemory(device,stagingMemory,0,imageSize,0,&data);
        memcpy(data,&whitePixel,imageSize);
        vkUnmapMemory(device,stagingMemory);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType=VK_IMAGE_TYPE_2D;
        imageInfo.extent={1,1,1};
        imageInfo.mipLevels=1;
        imageInfo.arrayLayers=1;
        imageInfo.format=VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling=VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(device,&imageInfo,nullptr,&defaultTextureImage);

        vkGetImageMemoryRequirements(device,defaultTextureImage,&memReqs);
        allocInfo.allocationSize=memReqs.size;
        for(uint32_t i=0;i<memProps.memoryTypeCount;++i){
            if((memReqs.memoryTypeBits&(1<<i))&&
                (memProps.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
                memType=i;
                break;
            }
        }
        allocInfo.memoryTypeIndex=memType;
        vkAllocateMemory(device,&allocInfo,nullptr,&defaultTextureMemory);
        vkBindImageMemory(device,defaultTextureImage,defaultTextureMemory,0);

        VkCommandBuffer cmdBuf;
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool=thiscontext->getCommandPool();
        cmdAllocInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount=1;
        vkAllocateCommandBuffers(device,&cmdAllocInfo,&cmdBuf);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmdBuf,&beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=defaultTextureImage;
        barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        barrier.srcAccessMask=0;
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmdBuf,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,0,nullptr,0,nullptr,1,&barrier);

        VkBufferImageCopy region{};
        region.bufferOffset=0;
        region.bufferRowLength=0;
        region.bufferImageHeight=0;
        region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        region.imageOffset={0,0,0};
        region.imageExtent={1,1,1};
        vkCmdCopyBufferToImage(cmdBuf,stagingBuffer,defaultTextureImage,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);

        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmdBuf,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0,0,nullptr,0,nullptr,1,&barrier);

        vkEndCommandBuffer(cmdBuf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount=1;
        submitInfo.pCommandBuffers=&cmdBuf;
        vkQueueSubmit(thiscontext->getInitializer()->getGraphicsQueue(),1,&submitInfo,VK_NULL_HANDLE);
        vkQueueWaitIdle(thiscontext->getInitializer()->getGraphicsQueue());

        vkFreeCommandBuffers(device,thiscontext->getCommandPool(),1,&cmdBuf);
        vkDestroyBuffer(device,stagingBuffer,nullptr);
        vkFreeMemory(device,stagingMemory,nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image=defaultTextureImage;
        viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format=VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        vkCreateImageView(device,&viewInfo,nullptr,&defaultTextureView);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter=VK_FILTER_NEAREST;
        samplerInfo.minFilter=VK_FILTER_NEAREST;
        samplerInfo.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device,&samplerInfo,nullptr,&defaultTextureSampler);

        VkDescriptorSetAllocateInfo descAllocInfo{};
        descAllocInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool=thisDescriptorPool;
        descAllocInfo.descriptorSetCount=1;
        descAllocInfo.pSetLayouts=&thisTextureDescriptorSetLayout;
        vkAllocateDescriptorSets(device,&descAllocInfo,&defaultDescriptorSet);

        VkDescriptorImageInfo imageInfoDesc{};
        imageInfoDesc.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfoDesc.imageView=defaultTextureView;
        imageInfoDesc.sampler=defaultTextureSampler;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet=defaultDescriptorSet;
        writeSet.dstBinding=0;
        writeSet.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount=1;
        writeSet.pImageInfo=&imageInfoDesc;
        vkUpdateDescriptorSets(device,1,&writeSet,0,nullptr);
    }

    void Painter::createRenderPass(){
        VkDevice device=thiscontext->getInitializer()->getDevice();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format=thiscontext->getSwapchainImageFormat();
        colorAttachment.samples=VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription stencilAttachment{};
        stencilAttachment.format=VK_FORMAT_D24_UNORM_S8_UINT;
        stencilAttachment.samples=VK_SAMPLE_COUNT_1_BIT;
        stencilAttachment.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        stencilAttachment.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        stencilAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        stencilAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        stencilAttachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        stencilAttachment.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentDescription,2> attachments={colorAttachment,stencilAttachment};

        VkAttachmentReference colorRef{};
        colorRef.attachment=0;
        colorRef.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference stencilRef{};
        stencilRef.attachment=1;
        stencilRef.layout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;
        subpass.pColorAttachments=&colorRef;
        subpass.pDepthStencilAttachment=&stencilRef;

        VkSubpassDependency dep{};
        dep.srcSubpass=VK_SUBPASS_EXTERNAL;
        dep.dstSubpass=0;
        dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask=0;
        dep.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount=static_cast<uint32_t>(attachments.size());
        rpInfo.pAttachments=attachments.data();
        rpInfo.subpassCount=1;
        rpInfo.pSubpasses=&subpass;
        rpInfo.dependencyCount=1;
        rpInfo.pDependencies=&dep;

        if(vkCreateRenderPass(device,&rpInfo,nullptr,&thisrenderPass)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create render pass with stencil!");
        }
    }

    void Painter::createStencilImages(){
        VkDevice device=thiscontext->getInitializer()->getDevice();
        VkPhysicalDevice physicalDevice=thiscontext->getInitializer()->getPhysicalDevice();
        VkExtent2D extent=thiscontext->getExtent();

        size_t count=thiscontext->getSwapchainImageViews().size();
        stencilImages.resize(count);
        stencilImageMemories.resize(count);
        stencilImageViews.resize(count);

        VkImageCreateInfo imgInfo{};
        imgInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType=VK_IMAGE_TYPE_2D;
        imgInfo.format=VK_FORMAT_D24_UNORM_S8_UINT;
        imgInfo.extent={extent.width,extent.height,1};
        imgInfo.mipLevels=1;
        imgInfo.arrayLayers=1;
        imgInfo.samples=VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling=VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imgInfo.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        VkMemoryRequirements memReqs;
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProps);

        for(size_t i=0;i<count;++i){
            if(vkCreateImage(device,&imgInfo,nullptr,&stencilImages[i])!=VK_SUCCESS){
                throw std::runtime_error("Failed to create stencil image");
            }
            vkGetImageMemoryRequirements(device,stencilImages[i],&memReqs);
            allocInfo.allocationSize=memReqs.size;
            uint32_t memType=0;
            for(uint32_t j=0;j<memProps.memoryTypeCount;++j){
                if((memReqs.memoryTypeBits&(1<<j))&&(memProps.memoryTypes[j].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
                    memType=j;
                    break;
                }
            }
            allocInfo.memoryTypeIndex=memType;
            if(vkAllocateMemory(device,&allocInfo,nullptr,&stencilImageMemories[i])!=VK_SUCCESS){
                throw std::runtime_error("Failed to allocate stencil image memory");
            }
            vkBindImageMemory(device,stencilImages[i],stencilImageMemories[i],0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image=stencilImages[i];
            viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format=VK_FORMAT_D24_UNORM_S8_UINT;
            viewInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
            viewInfo.subresourceRange.baseMipLevel=0;
            viewInfo.subresourceRange.levelCount=1;
            viewInfo.subresourceRange.baseArrayLayer=0;
            viewInfo.subresourceRange.layerCount=1;
            if(vkCreateImageView(device,&viewInfo,nullptr,&stencilImageViews[i])!=VK_SUCCESS){
                throw std::runtime_error("Failed to create stencil image view");
            }
        }
    }

    void Painter::createFramebuffers(){
        VkDevice device=thiscontext->getInitializer()->getDevice();
        const auto& imageViews=thiscontext->getSwapchainImageViews();

        if(stencilImages.empty()){
            createStencilImages();
        }

        thisframebuffers.resize(imageViews.size());
        for(size_t i=0;i<imageViews.size();++i){
            VkImageView attachments[]={imageViews[i],stencilImageViews[i]};

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass=thisrenderPass;
            fbInfo.attachmentCount=2;
            fbInfo.pAttachments=attachments;
            fbInfo.width=thiscontext->getExtent().width;
            fbInfo.height=thiscontext->getExtent().height;
            fbInfo.layers=1;

            if(vkCreateFramebuffer(device,&fbInfo,nullptr,&thisframebuffers[i])!=VK_SUCCESS){
                throw std::runtime_error("Failed to create framebuffer "+std::to_string(i));
            }
        }
    }

    void Painter::cleanup(){
        vkDeviceWaitIdle(thiscontext->getInitializer()->getDevice());

        VkDevice device=thiscontext->getInitializer()->getDevice();

        images.clear();

        fonts.clear();
        if(textAtlas.descriptorSet!=VK_NULL_HANDLE){
            vkFreeDescriptorSets(device,thisDescriptorPool,1,&textAtlas.descriptorSet);
            textAtlas.descriptorSet=VK_NULL_HANDLE;
        }
        if(textAtlas.sampler!=VK_NULL_HANDLE){
            vkDestroySampler(device,textAtlas.sampler,nullptr);
            textAtlas.sampler=VK_NULL_HANDLE;
        }
        if(textAtlas.imageView!=VK_NULL_HANDLE){
            vkDestroyImageView(device,textAtlas.imageView,nullptr);
            textAtlas.imageView=VK_NULL_HANDLE;
        }
        if(textAtlas.image!=VK_NULL_HANDLE){
            vkDestroyImage(device,textAtlas.image,nullptr);
            textAtlas.image=VK_NULL_HANDLE;
        }
        if(textAtlas.memory!=VK_NULL_HANDLE){
            vkFreeMemory(device,textAtlas.memory,nullptr);
            textAtlas.memory=VK_NULL_HANDLE;
        }
        textAtlas.pixels.clear();

        cleanupBuffers();
        if(defaultDescriptorSet!=VK_NULL_HANDLE){
            defaultDescriptorSet=VK_NULL_HANDLE;
        }
        if(defaultTextureSampler!=VK_NULL_HANDLE){
            vkDestroySampler(device,defaultTextureSampler,nullptr);
            defaultTextureSampler=VK_NULL_HANDLE;
        }
        if(defaultTextureView!=VK_NULL_HANDLE){
            vkDestroyImageView(device,defaultTextureView,nullptr);
            defaultTextureView=VK_NULL_HANDLE;
        }
        if(defaultTextureImage!=VK_NULL_HANDLE){
            vkDestroyImage(device,defaultTextureImage,nullptr);
            defaultTextureImage=VK_NULL_HANDLE;
        }
        if(defaultTextureMemory!=VK_NULL_HANDLE){
            vkFreeMemory(device,defaultTextureMemory,nullptr);
            defaultTextureMemory=VK_NULL_HANDLE;
        }

        if(thisopaquePipeline!=VK_NULL_HANDLE){
            vkDestroyPipeline(device,thisopaquePipeline,nullptr);
            thisopaquePipeline=VK_NULL_HANDLE;
        }
        if(thistransparentPipeline!=VK_NULL_HANDLE){
            vkDestroyPipeline(device,thistransparentPipeline,nullptr);
            thistransparentPipeline=VK_NULL_HANDLE;
        }

        if(thisopaquePipelineLayout!=VK_NULL_HANDLE){
            vkDestroyPipelineLayout(device,thisopaquePipelineLayout,nullptr);
            thisopaquePipelineLayout=VK_NULL_HANDLE;
        }
        if(thistransparentPipelineLayout!=VK_NULL_HANDLE){
            vkDestroyPipelineLayout(device,thistransparentPipelineLayout,nullptr);
            thistransparentPipelineLayout=VK_NULL_HANDLE;
        }

        if(thisTextureDescriptorSetLayout!=VK_NULL_HANDLE){
            vkDestroyDescriptorSetLayout(device,thisTextureDescriptorSetLayout,nullptr);
            thisTextureDescriptorSetLayout=VK_NULL_HANDLE;
        }

        if(thisvertShaderModule!=VK_NULL_HANDLE){
            vkDestroyShaderModule(device,thisvertShaderModule,nullptr);
            thisvertShaderModule=VK_NULL_HANDLE;
        }
        if(thisfragShaderModule!=VK_NULL_HANDLE){
            vkDestroyShaderModule(device,thisfragShaderModule,nullptr);
            thisfragShaderModule=VK_NULL_HANDLE;
        }

        for(auto fb:thisframebuffers){
            if(fb!=VK_NULL_HANDLE){
                vkDestroyFramebuffer(device,fb,nullptr);
            }
        }
        thisframebuffers.clear();

        for(auto& imageView:stencilImageViews){
            if(imageView!=VK_NULL_HANDLE){
                vkDestroyImageView(device,imageView,nullptr);
            }
        }
        stencilImageViews.clear();

        for(auto& image:stencilImages){
            if(image!=VK_NULL_HANDLE){
                vkDestroyImage(device,image,nullptr);
            }
        }
        stencilImages.clear();

        for(auto& memory:stencilImageMemories){
            if(memory!=VK_NULL_HANDLE){
                vkFreeMemory(device,memory,nullptr);
            }
        }
        stencilImageMemories.clear();

        if(thisrenderPass!=VK_NULL_HANDLE){
            vkDestroyRenderPass(device,thisrenderPass,nullptr);
            thisrenderPass=VK_NULL_HANDLE;
        }

        if(thisUniformMappedData!=nullptr){
            vkUnmapMemory(device,thisUniformBufferMemory);
            thisUniformMappedData=nullptr;
        }
        if(thisUniformBuffer!=VK_NULL_HANDLE){
            vkDestroyBuffer(device,thisUniformBuffer,nullptr);
            thisUniformBuffer=VK_NULL_HANDLE;
        }
        if(thisUniformBufferMemory!=VK_NULL_HANDLE){
            vkFreeMemory(device,thisUniformBufferMemory,nullptr);
            thisUniformBufferMemory=VK_NULL_HANDLE;
        }
        if(thisDescriptorPool!=VK_NULL_HANDLE){
            vkDestroyDescriptorPool(device,thisDescriptorPool,nullptr);
            thisDescriptorPool=VK_NULL_HANDLE;
        }
        if(thisDescriptorSetLayout!=VK_NULL_HANDLE){
            vkDestroyDescriptorSetLayout(device,thisDescriptorSetLayout,nullptr);
            thisDescriptorSetLayout=VK_NULL_HANDLE;
        }
    }

    void Painter::createPipeline(VkPipeline& pipeline,
                             VkPipelineLayout& pipelineLayout,
                             const VkPipelineColorBlendAttachmentState& blendAttachment,
                             VkShaderModule vertModule,
                             VkShaderModule fragModule,
                             VkDescriptorSetLayout descriptorSetLayout,
                             const VkPushConstantRange* customPushRange,
                             uint32_t customPushRangeCount,
                             const VkVertexInputAttributeDescription* customAttributes,
                             uint32_t customAttributeCount){
        VkDevice device=thiscontext->getInitializer()->getDevice();

        VkPushConstantRange defaultPushRange{};
        defaultPushRange.stageFlags=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
        defaultPushRange.offset=0;
        defaultPushRange.size=sizeof(TextPush);

        const VkPushConstantRange* pRange=(customPushRange&&customPushRangeCount>0)?customPushRange:&defaultPushRange;
        uint32_t rangeCount=(customPushRange&&customPushRangeCount>0)?customPushRangeCount:1;

        VkPipelineShaderStageCreateInfo vertStageInfo{};
        vertStageInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage=VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module=vertModule;
        vertStageInfo.pName="main";

        VkPipelineShaderStageCreateInfo fragStageInfo{};
        fragStageInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage=VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module=fragModule;
        fragStageInfo.pName="main";

        VkPipelineShaderStageCreateInfo shaderStages[]={vertStageInfo,fragStageInfo};

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding=0;
        bindingDescription.stride=sizeof(Vertex);
        bindingDescription.inputRate=VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attributeDescriptions[3]{};
        attributeDescriptions[0].binding=0;
        attributeDescriptions[0].location=0;
        attributeDescriptions[0].format=VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset=0;

        attributeDescriptions[1].binding=0;
        attributeDescriptions[1].location=1;
        attributeDescriptions[1].format=VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset=sizeof(glm::vec2);

        attributeDescriptions[2].binding=0;
        attributeDescriptions[2].location=2;
        attributeDescriptions[2].format=VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset=offsetof(Vertex,uv);

        const VkVertexInputAttributeDescription* pAttributes=(customAttributes&&customAttributeCount>0)?customAttributes:attributeDescriptions;
        uint32_t attributeCount=(customAttributes&&customAttributeCount>0)?customAttributeCount:3;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount=1;
        vertexInputInfo.pVertexBindingDescriptions=&bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount=attributeCount;
        vertexInputInfo.pVertexAttributeDescriptions=pAttributes;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable=VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount=1;
        viewportState.scissorCount=1;

        VkDynamicState dynamicStates[]={
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount=2;
        dynamicState.pDynamicStates=dynamicStates;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable=VK_FALSE;
        rasterizer.rasterizerDiscardEnable=VK_FALSE;
        rasterizer.polygonMode=VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth=1.0f;
        rasterizer.cullMode=VK_CULL_MODE_NONE;
        rasterizer.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable=VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable=VK_FALSE;
        multisampling.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable=VK_FALSE;
        colorBlending.logicOp=VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount=1;
        colorBlending.pAttachments=&blendAttachment;
        colorBlending.blendConstants[0]=0.0f;
        colorBlending.blendConstants[1]=0.0f;
        colorBlending.blendConstants[2]=0.0f;
        colorBlending.blendConstants[3]=0.0f;

        std::array<VkDescriptorSetLayout,2> setLayouts={
            descriptorSetLayout,
            thisTextureDescriptorSetLayout
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount=static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts=setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount=rangeCount;
        pipelineLayoutInfo.pPushConstantRanges=pRange;

        if(vkCreatePipelineLayout(device,&pipelineLayoutInfo,nullptr,&pipelineLayout)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencilState{};
        depthStencilState.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.depthTestEnable=VK_FALSE;
        depthStencilState.depthWriteEnable=VK_FALSE;
        depthStencilState.depthCompareOp=VK_COMPARE_OP_ALWAYS;
        depthStencilState.depthBoundsTestEnable=VK_FALSE;
        depthStencilState.stencilTestEnable=VK_FALSE;


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount=2;
        pipelineInfo.pStages=shaderStages;
        pipelineInfo.pVertexInputState=&vertexInputInfo;
        pipelineInfo.pInputAssemblyState=&inputAssembly;
        pipelineInfo.pViewportState=&viewportState;
        pipelineInfo.pRasterizationState=&rasterizer;
        pipelineInfo.pMultisampleState=&multisampling;
        pipelineInfo.pColorBlendState=&colorBlending;
        pipelineInfo.pDynamicState=&dynamicState;
        pipelineInfo.layout=pipelineLayout;
        pipelineInfo.renderPass=thisrenderPass;
        pipelineInfo.subpass=0;
        pipelineInfo.basePipelineHandle=VK_NULL_HANDLE;
        pipelineInfo.pDepthStencilState=&depthStencilState;

        if(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&pipeline)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create graphics pipeline!");
        }
    }

    void Painter::createVertexBuffer(){
        VkDevice device=thiscontext->getInitializer()->getDevice();
        VkPhysicalDevice physicalDevice=thiscontext->getInitializer()->getPhysicalDevice();

        VkDeviceSize bufferSize=sizeof(Vertex)*thismaxVertexCount;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size=bufferSize;
        bufferInfo.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device,&bufferInfo,nullptr,&thisvertexBuffer)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device,thisvertexBuffer,&memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize=memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProperties);

        uint32_t memoryTypeIndex=UINT32_MAX;
        for(uint32_t i=0;i<memProperties.memoryTypeCount;++i){
            if((memRequirements.memoryTypeBits&(1<<i))&&
                (memProperties.memoryTypes[i].propertyFlags&
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))){
                memoryTypeIndex=i;
                break;
            }
        }
        if(memoryTypeIndex==UINT32_MAX){
            throw std::runtime_error("Failed to find suitable memory type for vertex buffer!");
        }
        allocInfo.memoryTypeIndex=memoryTypeIndex;

        if(vkAllocateMemory(device,&allocInfo,nullptr,&thisvertexBufferMemory)!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(device,thisvertexBuffer,thisvertexBufferMemory,0);

        vkMapMemory(device,thisvertexBufferMemory,0,bufferSize,0,&thismappedData);

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Vertex buffer created. Max vertices: "+std::to_string(thismaxVertexCount),
            std::source_location::current()
        );
    }

    void Painter::cleanupBuffers(){
        VkDevice device=thiscontext->getInitializer()->getDevice();

        if(thismappedData!=nullptr){
            vkUnmapMemory(device,thisvertexBufferMemory);
            thismappedData=nullptr;
        }

        if(thisvertexBuffer!=VK_NULL_HANDLE){
            vkDestroyBuffer(device,thisvertexBuffer,nullptr);
            thisvertexBuffer=VK_NULL_HANDLE;
        }

        if(thisvertexBufferMemory!=VK_NULL_HANDLE){
            vkFreeMemory(device,thisvertexBufferMemory,nullptr);
            thisvertexBufferMemory=VK_NULL_HANDLE;
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Vertex buffers cleaned up.",
            std::source_location::current()
        );
    }

    void Painter::beginFrame(){
        thisvertices.clear();
        thisDrawCommands.clear();
        thisframeStarted=true;
    }

    void Painter::endFrame(){
        thisframeStarted=false;
    }

    void Painter::drawLine(vec2 p1,vec2 p2,vec4 color,float width){
        if(!thisframeStarted) return;
        if(thisvertices.size()+4>thismaxVertexCount){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Vertex buffer overflow!",
                std::source_location::current()
            );
            return;
        }

        vec2 dir=p2-p1;
        float len=glm::length(dir);
        if(len<1e-6f){
            drawPoint(p1,color);
            return;
        }
        vec2 dirNorm=dir/len;
        vec2 perp=vec2(-dirNorm.y,dirNorm.x)*width;

        vec2 v1=p1-perp;
        vec2 v2=p1+perp;
        vec2 v3=p2-perp;
        vec2 v4=p2+perp;

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        thisvertices.push_back({v1,color});
        thisvertices.push_back({v3,color});
        thisvertices.push_back({v2,color});

        thisvertices.push_back({v2,color});
        thisvertices.push_back({v3,color});
        thisvertices.push_back({v4,color});

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(color.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});
    }

    void Painter::drawPoint(vec2 p,vec4 color,float size){
        if(!thisframeStarted) return;
        if(thisvertices.size()+4>thismaxVertexCount){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Vertex buffer overflow!",
                std::source_location::current()
            );
            return;
        }

        float half=size*0.5f;
        vec2 p1=p-vec2(half,half);
        vec2 p2=p+vec2(half,-half);
        vec2 p3=p+vec2(-half,half);
        vec2 p4=p+vec2(half,half);

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        thisvertices.push_back({p1,color});
        thisvertices.push_back({p2,color});
        thisvertices.push_back({p3,color});

        thisvertices.push_back({p2,color});
        thisvertices.push_back({p4,color});
        thisvertices.push_back({p3,color});

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(color.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});
    }

    void Painter::drawTriangle(vec2 p1,vec2 p2,vec2 p3,vec4 color){
        if(!thisframeStarted) return;
        if(thisvertices.size()+3>thismaxVertexCount){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Vertex buffer overflow!",
                std::source_location::current()
            );
            return;
        }

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        thisvertices.push_back({p1,color});
        thisvertices.push_back({p2,color});
        thisvertices.push_back({p3,color});

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(color.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});
    }

    void Painter::drawTriangle(vec2 p1,vec2 p2,vec2 p3,vec4 c1,vec4 c2,vec4 c3){
        if(!thisframeStarted) return;
        if(thisvertices.size()+3>thismaxVertexCount){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Vertex buffer overflow!",
                std::source_location::current()
            );
            return;
        }

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        thisvertices.push_back({p1,c1});
        thisvertices.push_back({p2,c2});
        thisvertices.push_back({p3,c3});

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(c1.a<0.999f)||(c2.a<0.999f)||(c3.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});
    }

    void Painter::drawRect(vec2 pos,vec2 size,vec4 color){
        if(!thisframeStarted) return;
        if(thisvertices.size()+4>thismaxVertexCount){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Vertex buffer overflow!",
                std::source_location::current()
            );
            return;
        }
        vec2 p1=pos;
        vec2 p2={pos.x+size.x,pos.y};
        vec2 p3={pos.x,pos.y+size.y};
        vec2 p4={pos.x+size.x,pos.y+size.y};

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        thisvertices.push_back({p1,color});
        thisvertices.push_back({p2,color});
        thisvertices.push_back({p3,color});

        thisvertices.push_back({p2,color});
        thisvertices.push_back({p4,color});
        thisvertices.push_back({p3,color});

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(color.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});
    }

    void Painter::updateVertexBuffer(){
        if(thisvertices.empty()) return;
        size_t dataSize=thisvertices.size()*sizeof(Vertex);
        memcpy(thismappedData,thisvertices.data(),dataSize);
    }

    void Painter::recreateFramebuffers(){
        VkDevice device=thiscontext->getInitializer()->getDevice();

        for(auto fb:thisframebuffers){
            if(fb!=VK_NULL_HANDLE){
                vkDestroyFramebuffer(device,fb,nullptr);
            }
        }
        thisframebuffers.clear();

        for(auto& imageView:stencilImageViews){
            if(imageView!=VK_NULL_HANDLE){
                vkDestroyImageView(device,imageView,nullptr);
            }
        }
        stencilImageViews.clear();

        for(auto& image:stencilImages){
            if(image!=VK_NULL_HANDLE){
                vkDestroyImage(device,image,nullptr);
            }
        }
        stencilImages.clear();

        for(auto& memory:stencilImageMemories){
            if(memory!=VK_NULL_HANDLE){
                vkFreeMemory(device,memory,nullptr);
            }
        }
        stencilImageMemories.clear();

        createStencilImages();

        createFramebuffers();
    }

    void Painter::createGraphicsPipeline(){
        auto vertCode=readFile("shaders/vert.spv");
        auto fragCode=readFile("shaders/frag.spv");
        thisvertShaderModule=createShaderModule(vertCode);
        thisfragShaderModule=createShaderModule(fragCode);

        VkPipelineColorBlendAttachmentState opaqueBlend{};
        opaqueBlend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|
                                    VK_COLOR_COMPONENT_G_BIT|
                                    VK_COLOR_COMPONENT_B_BIT|
                                    VK_COLOR_COMPONENT_A_BIT;
        opaqueBlend.blendEnable=VK_FALSE;
        createPipeline(thisopaquePipeline,thisopaquePipelineLayout,
                    opaqueBlend,thisvertShaderModule,thisfragShaderModule,
                    thisDescriptorSetLayout);

        VkPipelineColorBlendAttachmentState transparentBlend{};
        transparentBlend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|
                                        VK_COLOR_COMPONENT_G_BIT|
                                        VK_COLOR_COMPONENT_B_BIT|
                                        VK_COLOR_COMPONENT_A_BIT;
        transparentBlend.blendEnable=VK_TRUE;
        transparentBlend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
        transparentBlend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        transparentBlend.colorBlendOp=VK_BLEND_OP_ADD;
        transparentBlend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
        transparentBlend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
        transparentBlend.alphaBlendOp=VK_BLEND_OP_ADD;
        createPipeline(thistransparentPipeline,thistransparentPipelineLayout,
                    transparentBlend,thisvertShaderModule,thisfragShaderModule,
                    thisDescriptorSetLayout);
    }
    void Painter::drawPolygon(const std::vector<vec2>& vertices,const vec4& color){
        if(!thisframeStarted) return;
        if(vertices.size()<3) return;

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        auto triangles=Utils::earClipTriangulate(vertices);
        for(const auto& tri:triangles){
            thisvertices.push_back({tri[0],color});
            thisvertices.push_back({tri[1],color});
            thisvertices.push_back({tri[2],color});
        }

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(color.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});

        if(thisvertices.size()>thismaxVertexCount){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Polygon triangulation overflowed vertex buffer!",
                std::source_location::current()
            );
        }
    }
    void Painter::drawCircle(vec2 center,float radius,const vec4& color,int segments){
        if(!thisframeStarted) return;
        if(segments<3) segments=3;

        std::vector<vec2> vertices;
        vertices.reserve(segments);
        for(int i=0;i<segments;++i){
            float angle=2.0f*3.14159265f*i/segments;
            vertices.push_back({
                center.x+radius*cosf(angle),
                center.y+radius*sinf(angle)
            });
        }
        drawPolygon(vertices,color);
    }
    void Painter::createUniformBuffer(){
        VkDevice device=thiscontext->getInitializer()->getDevice();
        VkPhysicalDevice physicalDevice=thiscontext->getInitializer()->getPhysicalDevice();

        VkDeviceSize bufferSize=sizeof(glm::mat4)*3;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size=bufferSize;
        bufferInfo.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device,&bufferInfo,nullptr,&thisUniformBuffer)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create uniform buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device,thisUniformBuffer,&memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize=memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProperties);

        uint32_t memoryTypeIndex=UINT32_MAX;
        for(uint32_t i=0;i<memProperties.memoryTypeCount;++i){
            if((memRequirements.memoryTypeBits&(1<<i))&&
                (memProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)&&
                (memProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                memoryTypeIndex=i;
                break;
            }
        }
        if(memoryTypeIndex==UINT32_MAX){
            throw std::runtime_error("Failed to find suitable memory type for uniform buffer!");
        }
        allocInfo.memoryTypeIndex=memoryTypeIndex;

        if(vkAllocateMemory(device,&allocInfo,nullptr,&thisUniformBufferMemory)!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate uniform buffer memory!");
        }

        vkBindBufferMemory(device,thisUniformBuffer,thisUniformBufferMemory,0);

        vkMapMemory(device,thisUniformBufferMemory,0,bufferSize,0,&thisUniformMappedData);

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Uniform buffer created.",
            std::source_location::current()
        );
    }
    void Painter::createDescriptorSetLayout(){
        VkDevice device=thiscontext->getInitializer()->getDevice();

        VkDescriptorSetLayoutBinding layoutBindings[2]{};
        layoutBindings[0].binding=0;
        layoutBindings[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindings[0].descriptorCount=1;
        layoutBindings[0].stageFlags=VK_SHADER_STAGE_VERTEX_BIT;

        layoutBindings[1].binding=1;
        layoutBindings[1].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[1].descriptorCount=1;
        layoutBindings[1].stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount=2;
        layoutInfo.pBindings=layoutBindings;

        if(vkCreateDescriptorSetLayout(device,&layoutInfo,nullptr,&thisDescriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create descriptor set layout!");
        }

        VkDescriptorPoolSize poolSizes[2]{};
        poolSizes[0].type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount=1024;

        poolSizes[1].type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount=1024;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount=2;
        poolInfo.pPoolSizes=poolSizes;
        poolInfo.maxSets=2048;
        poolInfo.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if(vkCreateDescriptorPool(device,&poolInfo,nullptr,&thisDescriptorPool)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create descriptor pool!");
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool=thisDescriptorPool;
        allocInfo.descriptorSetCount=1;
        allocInfo.pSetLayouts=&thisDescriptorSetLayout;

        VkResult result=VK_NOT_READY;
        int retries=3;
        while(retries-->0&&result!=VK_SUCCESS){
            result=vkAllocateDescriptorSets(device,&allocInfo,&this->thisDescriptorSet);
            if(result!=VK_SUCCESS){
                Core::globalLogger.traceLog(
                    Core::logger::LOG_INFO,
                    "Failed to allocate descriptor set,retry...",
                    std::source_location::current()
                );
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            else{
                break;
            }
        }

        if(result!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate descriptor set! error: "+std::to_string(result));
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer=thisUniformBuffer;
        bufferInfo.offset=0;
        bufferInfo.range=sizeof(glm::mat4)*3;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet=thisDescriptorSet;
        writeSet.dstBinding=0;
        writeSet.dstArrayElement=0;
        writeSet.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSet.descriptorCount=1;
        writeSet.pBufferInfo=&bufferInfo;

        vkUpdateDescriptorSets(device,1,&writeSet,0,nullptr);

        VkDescriptorSetLayoutBinding texBinding{};
        texBinding.binding=0;
        texBinding.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texBinding.descriptorCount=1;
        texBinding.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo texLayoutInfo{};
        texLayoutInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        texLayoutInfo.bindingCount=1;
        texLayoutInfo.pBindings=&texBinding;

        if(vkCreateDescriptorSetLayout(device,&texLayoutInfo,nullptr,&thisTextureDescriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create texture descriptor set layout!");
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Descriptor pool created: pool="+std::to_string((uintptr_t)thisDescriptorPool),
            std::source_location::current()
        );
    }
    void Painter::setModelMatrix(const glm::mat4& matrix){
        thisModelMatrix=matrix;
    }

    void Painter::setViewMatrix(const glm::mat4& matrix){
        thisViewMatrix=matrix;
    }

    void Painter::setProjectionMatrix(const glm::mat4& matrix){
        thisProjectionMatrix=matrix;
    }
    
    void Painter::setTransform(const glm::mat4& transform){
        thisTransform=transform;
    }
    void Painter::resetTransform(){
        thisTransform=glm::mat4(1.0f);
    }
    const glm::mat4& Painter::getTransform()const{
        return thisTransform;
    }
    void Painter::putImage(vec2 pos,vec2 size,Image& image,vec4 tint,vec2 UVmin,vec2 UVmax){
        if(!thisframeStarted) return;

        if(!image.isValid()){
            VkDevice device=thiscontext->getInitializer()->getDevice();
            VkCommandPool pool=thiscontext->getCommandPool();
            VkQueue queue=thiscontext->getInitializer()->getGraphicsQueue();
            image.createTexture(device,pool,queue);
        }

        if(!image.isValid()) return;

        if(image.getDescriptorSet()==VK_NULL_HANDLE){
            image.allocateDescriptorSet();
            if(image.getDescriptorSet()==VK_NULL_HANDLE){
                Core::globalLogger.traceLog(
                    Core::logger::LOG_ERROR,
                    "Failed to allocate descriptor set for image!",
                    std::source_location::current()
                );
                return;
            }
        }

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        vec2 p1=pos;
        vec2 p2={pos.x+size.x,pos.y};
        vec2 p3={pos.x,pos.y+size.y};
        vec2 p4={pos.x+size.x,pos.y+size.y};

        thisvertices.push_back({p1,tint,{UVmin.x,UVmax.y}});
        thisvertices.push_back({p2,tint,{UVmax.x,UVmax.y}});
        thisvertices.push_back({p3,tint,{UVmin.x,UVmin.y}});
        thisvertices.push_back({p2,tint,{UVmax.x,UVmax.y}});
        thisvertices.push_back({p4,tint,{UVmax.x,UVmin.y}});
        thisvertices.push_back({p3,tint,{UVmin.x,UVmin.y}});

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;

        thisDrawCommands.push_back({
            start,
            count,
            thisTransform,
            image.getImageView(),
            image.getSampler(),
            image.getDescriptorSet(),
            true
        });
    }
    void Painter::drawRoundedRect(vec2 pos,vec2 size,float radius,vec4 color){
        if(!thisframeStarted) return;
        auto outline=generateRoundedRectVertices(pos,size,radius);
        if(outline.size()<3) return;

        uint32_t start=static_cast<uint32_t>(thisvertices.size());

        const vec2& center=outline[0];
        for(size_t i=1;i<outline.size()-1;++i){
            thisvertices.push_back({center,color});
            thisvertices.push_back({outline[i],color});
            thisvertices.push_back({outline[i+1],color});
        }

        uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;
        bool transparent=(color.a<0.999f);
        thisDrawCommands.push_back({start,count,thisTransform,VK_NULL_HANDLE,VK_NULL_HANDLE,defaultDescriptorSet,transparent});
    }

    void Painter::drawEllipse(vec2 center,vec2 radius,vec4 color,int segments){
        if(!thisframeStarted) return;
        if(segments<3) segments=3;
        std::vector<vec2> vertices;
        vertices.reserve(segments);
        for(int i=0;i<segments;++i){
            float angle=2.0f*3.14159265f*i/segments;
            vertices.push_back(center+vec2(radius.x*cosf(angle),radius.y*sinf(angle)));
        }
        drawPolygon(vertices,color);
    }
    Painter::Image& Painter::createImage(const std::string& filepath){
        auto image=std::make_unique<Image>(filepath,this);
        if(!image->isValid()){
            throw std::runtime_error("Failed to load image: "+filepath);
        }
        VkDevice device=thiscontext->getInitializer()->getDevice();
        VkCommandPool pool=thiscontext->getCommandPool();
        VkQueue queue=thiscontext->getInitializer()->getGraphicsQueue();
        image->createTexture(device,pool,queue);
        auto& ref=*image;
        images.push_back(std::move(image));
        return ref;
    }

    void Painter::recordCommands(VkCommandBuffer cmdBuf,uint32_t imageIndex){
        uploadAtlasToGPU();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags=VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        if(vkBeginCommandBuffer(cmdBuf,&beginInfo)!=VK_SUCCESS){
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkClearValue clearValues[2]{};
        clearValues[0].color={{1.0f,1.0f,1.0f,1.0f}};
        clearValues[1].depthStencil={1.0f,0};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass=thisrenderPass;
        renderPassInfo.framebuffer=thisframebuffers[imageIndex];
        renderPassInfo.renderArea.offset={0,0};
        renderPassInfo.renderArea.extent=thiscontext->getExtent();
        renderPassInfo.clearValueCount=2;
        renderPassInfo.pClearValues=clearValues;

        updateVertexBuffer();

        vkCmdBeginRenderPass(cmdBuf,&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x=0.0f;
        viewport.y=0.0f;
        viewport.width=(float)thiscontext->getExtent().width;
        viewport.height=(float)thiscontext->getExtent().height;
        viewport.minDepth=0.0f;
        viewport.maxDepth=1.0f;
        vkCmdSetViewport(cmdBuf,0,1,&viewport);

        VkRect2D scissor{};
        scissor.offset={0,0};
        scissor.extent=thiscontext->getExtent();
        vkCmdSetScissor(cmdBuf,0,1,&scissor);

        struct ViewProj {
            glm::mat4 view;
            glm::mat4 proj;
        }vp;
        vp.view=thisViewMatrix;
        vp.proj=thisProjectionMatrix;
        memcpy(thisUniformMappedData,&vp,sizeof(vp));

        VkPipelineLayout layout=thistransparentPipelineLayout;

        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout,0,1,
                                &thisDescriptorSet,0,nullptr);

        VkDeviceSize offsets[]={0};
        vkCmdBindVertexBuffers(cmdBuf,0,1,&thisvertexBuffer,offsets);

        for(const auto& drawCmd:thisDrawCommands){
            VkDescriptorSet descSet=drawCmd.descriptorSet;
            if(descSet==VK_NULL_HANDLE){
                descSet=defaultDescriptorSet;
            }
            vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,1,1,&descSet,0,nullptr);

            VkPipeline pipeline=drawCmd.isTransparent?thistransparentPipeline:thisopaquePipeline;
            vkCmdBindPipeline(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);

            TextPush push{};
            push.transform=drawCmd.transform;
            push.color=glm::vec4(1.0f);
            push.downsample=0;
            vkCmdPushConstants(cmdBuf,layout,
                                VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                0,sizeof(glm::mat4),&drawCmd.transform);

            vkCmdDraw(cmdBuf,drawCmd.vertexCount,1,drawCmd.firstVertex,0);
        }

        vkCmdEndRenderPass(cmdBuf);

        if(vkEndCommandBuffer(cmdBuf)!=VK_SUCCESS){
            throw std::runtime_error("Failed to end command buffer!");
        }
    }
    void Painter::drawRegularPolygon(vec2 center,float radius,int sides,vec4 color){
        if(!thisframeStarted) return;
        if(sides<3) sides=3;

        std::vector<vec2> vertices;
        vertices.reserve(sides);
        for(int i=0;i<sides;++i){
            float angle=2.0f*3.14159265f*i/sides-3.14159265f*0.5f;
            vertices.push_back(center+vec2(radius*cosf(angle),radius*sinf(angle)));
        }
        drawPolygon(vertices,color);
    }
    void Painter::drawStar(vec2 center,float outerRadius,float innerRadius,int points,vec4 color){
        if(!thisframeStarted) return;
        if(points<3) points=3;

        std::vector<vec2> vertices;
        vertices.reserve(points*2);
        for(int i=0;i<points*2;++i){
            float angle=2.0f*3.14159265f*i/(points*2)-3.14159265f*0.5f;
            float r=(i%2==0)?outerRadius:innerRadius;
            vertices.push_back(center+vec2(r*cosf(angle),r*sinf(angle)));
        }
        drawPolygon(vertices,color);
    }
    void Painter::drawPie(vec2 center,float radius,float startAngle,float endAngle,vec4 color,int segments){
        if(!thisframeStarted) return;
        if(segments<3) segments=3;

        if(endAngle<=startAngle) endAngle+=2.0f*3.14159265f;

        std::vector<vec2> vertices;
        vertices.reserve(segments+2);
        vertices.push_back(center);
        for(int i=0;i<=segments;++i){
            float t=(float)i/segments;
            float angle=startAngle+t*(endAngle-startAngle);
            vertices.push_back(center+vec2(radius*cosf(angle),radius*sinf(angle)));
        }
        drawPolygon(vertices,color);
    }
    static vec2 deCasteljau(const std::vector<vec2>& points,float t){
        std::vector<vec2> pts=points;
        while(pts.size()>1){
            std::vector<vec2> next;
            next.reserve(pts.size()-1);
            for(size_t i=0;i<pts.size()-1;++i){
                vec2 p=pts[i]+t*(pts[i+1]-pts[i]);
                next.push_back(p);
            }
            pts=std::move(next);
        }
        return pts[0];
    }

    void Painter::drawBezier(const std::vector<vec2>& controlPoints,vec4 color,int segments,float width){
        if(!thisframeStarted) return;
        if(controlPoints.size()<2) return;
        if(segments<2) segments=2;

        vec2 prev=controlPoints[0];
        for(int i=1;i<=segments;++i){
            float t=(float)i/segments;
            vec2 p=deCasteljau(controlPoints,t);
            drawLine(prev,p,color,width);
            prev=p;
        }
    }

    bool Painter::initTextSystem(){
        if(textSystemInitialized) return true;
        VkDevice device=thiscontext->getInitializer()->getDevice();

        textAtlas.init(1024,1024);

        VkPhysicalDevice physicalDevice=thiscontext->getInitializer()->getPhysicalDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType=VK_IMAGE_TYPE_2D;
        imageInfo.extent={textAtlas.width,textAtlas.height,1};
        imageInfo.mipLevels=1;
        imageInfo.arrayLayers=1;
        imageInfo.format=VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling=VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateImage(device,&imageInfo,nullptr,&textAtlas.image)!=VK_SUCCESS){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "Failed to create atlas image!",
                std::source_location::current()
            );
            return false;
        }

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device,textAtlas.image,&memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize=memReqs.size;

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProps);

        uint32_t memType=UINT32_MAX;
        for(uint32_t i=0;i<memProps.memoryTypeCount;++i){
            if((memReqs.memoryTypeBits&(1<<i))&&
                (memProps.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
                memType=i;
                break;
            }
        }
        if(memType==UINT32_MAX){
            vkDestroyImage(device,textAtlas.image,nullptr);
            textAtlas.image=VK_NULL_HANDLE;
            return false;
        }
        allocInfo.memoryTypeIndex=memType;

        if(vkAllocateMemory(device,&allocInfo,nullptr,&textAtlas.memory)!=VK_SUCCESS){
            vkDestroyImage(device,textAtlas.image,nullptr);
            textAtlas.image=VK_NULL_HANDLE;
            return false;
        }

        vkBindImageMemory(device,textAtlas.image,textAtlas.memory,0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image=textAtlas.image;
        viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format=VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel=0;
        viewInfo.subresourceRange.levelCount=1;
        viewInfo.subresourceRange.baseArrayLayer=0;
        viewInfo.subresourceRange.layerCount=1;

        if(vkCreateImageView(device,&viewInfo,nullptr,&textAtlas.imageView)!=VK_SUCCESS){
            vkDestroyImage(device,textAtlas.image,nullptr);
            vkFreeMemory(device,textAtlas.memory,nullptr);
            textAtlas.image=VK_NULL_HANDLE;
            textAtlas.memory=VK_NULL_HANDLE;
            return false;
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter=VK_FILTER_LINEAR;
        samplerInfo.minFilter=VK_FILTER_LINEAR;
        samplerInfo.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable=VK_FALSE;
        samplerInfo.borderColor=VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates=VK_FALSE;
        samplerInfo.compareEnable=VK_FALSE;
        samplerInfo.compareOp=VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if(vkCreateSampler(device,&samplerInfo,nullptr,&textAtlas.sampler)!=VK_SUCCESS){
            vkDestroyImageView(device,textAtlas.imageView,nullptr);
            vkDestroyImage(device,textAtlas.image,nullptr);
            vkFreeMemory(device,textAtlas.memory,nullptr);
            textAtlas.imageView=VK_NULL_HANDLE;
            textAtlas.image=VK_NULL_HANDLE;
            textAtlas.memory=VK_NULL_HANDLE;
            return false;
        }

        VkDescriptorSetAllocateInfo descAllocInfo{};
        descAllocInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool=thisDescriptorPool;
        descAllocInfo.descriptorSetCount=1;
        descAllocInfo.pSetLayouts=&thisTextureDescriptorSetLayout;

        if(vkAllocateDescriptorSets(device,&descAllocInfo,&textAtlas.descriptorSet)!=VK_SUCCESS){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "Failed to allocate descriptor set for text atlas!",
                std::source_location::current()
            );
            vkDestroySampler(device,textAtlas.sampler,nullptr);
            vkDestroyImageView(device,textAtlas.imageView,nullptr);
            vkDestroyImage(device,textAtlas.image,nullptr);
            vkFreeMemory(device,textAtlas.memory,nullptr);
            textAtlas.sampler=VK_NULL_HANDLE;
            textAtlas.imageView=VK_NULL_HANDLE;
            textAtlas.image=VK_NULL_HANDLE;
            textAtlas.memory=VK_NULL_HANDLE;
            return false;
        }

        VkDescriptorImageInfo imageInfoDesc{};
        imageInfoDesc.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfoDesc.imageView=textAtlas.imageView;
        imageInfoDesc.sampler=textAtlas.sampler;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet=textAtlas.descriptorSet;
        writeSet.dstBinding=0;
        writeSet.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount=1;
        writeSet.pImageInfo=&imageInfoDesc;

        vkUpdateDescriptorSets(device,1,&writeSet,0,nullptr);

        textSystemInitialized=true;

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Text system initialized (atlas: "+std::to_string(textAtlas.width)+"x"+std::to_string(textAtlas.height)+")",
            std::source_location::current()
        );

        return true;
    }
    Painter::Font Painter::loadFont(const std::string& filepath,float size){
        if(!textSystemInitialized){
            if(!initTextSystem()){
                return {};
            }
        }

        FILE* file=fopen(filepath.c_str(),"rb");
        if(!file){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "Failed to open font: "+filepath,
                std::source_location::current()
            );
            return {};
        }
        fseek(file,0,SEEK_END);
        size_t fileSize=ftell(file);
        fseek(file,0,SEEK_SET);
        
        std::vector<unsigned char> buffer(fileSize);
        fread(buffer.data(),1,fileSize,file);
        fclose(file);

        FontData fontData;
        fontData.name=filepath;
        fontData.size=size;
        fontData.buffer=std::move(buffer);

        fontData.info=std::make_unique<stbtt_fontinfo>();
        if(!stbtt_InitFont(fontData.info.get(),fontData.buffer.data(),0)){
            delete fontData.info.get();
            fontData.info=nullptr;
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "Failed to init font: "+filepath,
                std::source_location::current()
            );
            return {};
        }

        fontData.scale=stbtt_ScaleForPixelHeight(fontData.info.get(),size);
        stbtt_GetFontVMetrics(fontData.info.get(),&fontData.ascent,&fontData.descent,&fontData.lineGap);

        size_t index=fonts.size();
        fonts.push_back(std::move(fontData));

        Font font;
        font.name=filepath;
        font.size=size;
        font.index=index;

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Font loaded: "+filepath+" ("+std::to_string(size)+"px)",
            std::source_location::current()
        );

        return font;
    }

    Painter::GlyphInfo Painter::rasterizeGlyph(FontData& fontData,uint32_t codepoint){
        GlyphInfo info={};

        int glyphIndex=stbtt_FindGlyphIndex(fontData.info.get(),codepoint);
        if(glyphIndex==0){
            info.advance=fontData.size*0.5f;
            info.width=0;
            info.height=0;
            return info;
        }

        int advance,bearingX,bearingY,x0,y0,x1,y1;
        stbtt_GetGlyphHMetrics(fontData.info.get(),glyphIndex,&advance,&bearingX);
        stbtt_GetGlyphBitmapBox(fontData.info.get(),glyphIndex,fontData.scale,fontData.scale,&x0,&y0,&x1,&y1);

        info.advance=advance*fontData.scale;
        info.bearingX=bearingX*fontData.scale;
        info.bearingY=y0*fontData.scale;
        info.width=(float)(x1-x0);
        info.height=(float)(y1-y0);

        if(info.width<=0||info.height<=0){
            return info;
        }

        int w=(int)info.width;
        int h=(int)info.height;
        std::vector<unsigned char> bitmap(w*h);
        stbtt_MakeGlyphBitmap(fontData.info.get(),bitmap.data(),w,h,w,fontData.scale,fontData.scale,glyphIndex);

        uint32_t px,py;
        if(!textAtlas.pack((uint32_t)w,(uint32_t)h,px,py)){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Font atlas full! Consider increasing atlas size.",
                std::source_location::current()
            );
            return info;
        }

        for(uint32_t y=0;y<(uint32_t)h;++y){
            uint32_t dstOffset=((py+y)*textAtlas.width+px)*4;
            for(uint32_t x=0;x<(uint32_t)w;++x){
                unsigned char val=bitmap[y*w+x];
                textAtlas.pixels[dstOffset+x*4+0]=255;
                textAtlas.pixels[dstOffset+x*4+1]=255;
                textAtlas.pixels[dstOffset+x*4+2]=255;
                textAtlas.pixels[dstOffset+x*4+3]=val;
            }
        }
        textAtlas.needsUpload=true;

        info.u0=(float)px/textAtlas.width;
        info.v0=(float)py/textAtlas.height;
        info.u1=(float)(px+w)/textAtlas.width;
        info.v1=(float)(py+h)/textAtlas.height;

        info.y0=y0;
        info.y1=y1;
        info.x0=x0;
        info.x1=x1;

        return info;
    }
    void Painter::drawText(const Font& font,const std::string& text,
                           vec2 pos,vec4 color,float scale){
        if(!font.isValid()||font.index>=fonts.size()) return;
        if(!thisframeStarted) return;

        FontData& fontData=fonts[font.index];
        float x=pos.x;
        float y=pos.y;

        const char* ptr=text.c_str();
        while(*ptr){
            uint32_t codepoint;
            codepoint=utf8_to_codepoint(ptr);
            if(codepoint==0){
                if(*ptr=='\0') break;
                continue;
            }

            auto it=fontData.glyphs.find(codepoint);
            if(it==fontData.glyphs.end()){
                GlyphInfo info=rasterizeGlyph(fontData,codepoint);
                fontData.glyphs[codepoint]=info;
                it=fontData.glyphs.find(codepoint);
            }

            const GlyphInfo& glyph=it->second;
            if(glyph.width>0&&glyph.height>0){
                float left=x+glyph.x0*scale;
                float right=x+glyph.x1*scale;
                float topY=y-glyph.y0*scale;
                float bottomY=y-glyph.y1*scale;

                uint32_t start=static_cast<uint32_t>(thisvertices.size());

                thisvertices.push_back({{left,topY},color,{glyph.u0,glyph.v0}});
                thisvertices.push_back({{right,topY},color,{glyph.u1,glyph.v0}});
                thisvertices.push_back({{left,bottomY},color,{glyph.u0,glyph.v1}});
                thisvertices.push_back({{right,topY},color,{glyph.u1,glyph.v0}});
                thisvertices.push_back({{right,bottomY},color,{glyph.u1,glyph.v1}});
                thisvertices.push_back({{left,bottomY},color,{glyph.u0,glyph.v1}});

                uint32_t count=static_cast<uint32_t>(thisvertices.size())-start;

                thisDrawCommands.push_back({
                    start,
                    count,
                    thisTransform,
                    textAtlas.imageView,
                    textAtlas.sampler,
                    textAtlas.descriptorSet,
                    true
                });
            }

            x+=glyph.advance*scale;
        }
    }
    void Painter::uploadAtlasToGPU(){
        if(!textAtlas.needsUpload) return;
        if(textAtlas.pixels.empty()) return;

        VkDevice device=thiscontext->getInitializer()->getDevice();
        VkPhysicalDevice physicalDevice=thiscontext->getInitializer()->getPhysicalDevice();
        VkCommandPool pool=thiscontext->getCommandPool();
        VkQueue queue=thiscontext->getInitializer()->getGraphicsQueue();

        VkDeviceSize imageSize=textAtlas.width*textAtlas.height*4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size=imageSize;
        bufferInfo.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device,&bufferInfo,nullptr,&stagingBuffer)!=VK_SUCCESS){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "Failed to create staging buffer for atlas!",
                std::source_location::current()
            );
            return;
        }

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device,stagingBuffer,&memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize=memReqs.size;

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProps);

        uint32_t memType=UINT32_MAX;
        for(uint32_t i=0;i<memProps.memoryTypeCount;++i){
            if((memReqs.memoryTypeBits&(1<<i))&&
                (memProps.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)&&
                (memProps.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                memType=i;
                break;
            }
        }
        if(memType==UINT32_MAX){
            vkDestroyBuffer(device,stagingBuffer,nullptr);
            return;
        }
        allocInfo.memoryTypeIndex=memType;

        if(vkAllocateMemory(device,&allocInfo,nullptr,&stagingMemory)!=VK_SUCCESS){
            vkDestroyBuffer(device,stagingBuffer,nullptr);
            return;
        }

        vkBindBufferMemory(device,stagingBuffer,stagingMemory,0);

        void* data;
        vkMapMemory(device,stagingMemory,0,imageSize,0,&data);
        memcpy(data,textAtlas.pixels.data(),imageSize);
        vkUnmapMemory(device,stagingMemory);

        VkCommandBuffer cmdBuf;
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool=pool;
        cmdAllocInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount=1;

        if(vkAllocateCommandBuffers(device,&cmdAllocInfo,&cmdBuf)!=VK_SUCCESS){
            vkDestroyBuffer(device,stagingBuffer,nullptr);
            vkFreeMemory(device,stagingMemory,nullptr);
            return;
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmdBuf,&beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=textAtlas.image;
        barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel=0;
        barrier.subresourceRange.levelCount=1;
        barrier.subresourceRange.baseArrayLayer=0;
        barrier.subresourceRange.layerCount=1;
        barrier.srcAccessMask=0;
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);

        VkBufferImageCopy region{};
        region.bufferOffset=0;
        region.bufferRowLength=0;
        region.bufferImageHeight=0;
        region.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel=0;
        region.imageSubresource.baseArrayLayer=0;
        region.imageSubresource.layerCount=1;
        region.imageOffset={0,0,0};
        region.imageExtent={textAtlas.width,textAtlas.height,1};

        vkCmdCopyBufferToImage(cmdBuf,stagingBuffer,textAtlas.image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);

        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);

        vkEndCommandBuffer(cmdBuf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount=1;
        submitInfo.pCommandBuffers=&cmdBuf;

        vkQueueSubmit(queue,1,&submitInfo,VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkDestroyBuffer(device,stagingBuffer,nullptr);
        vkFreeMemory(device,stagingMemory,nullptr);
        vkFreeCommandBuffers(device,pool,1,&cmdBuf);

        textAtlas.needsUpload=false;

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Atlas uploaded to GPU: "+std::to_string(textAtlas.width)+"x"+std::to_string(textAtlas.height),
            std::source_location::current()
        );
    }
    vec2 Painter::measureText(const Font& font,const std::string& text,float scale){
        if(!font.isValid()||font.index>=fonts.size()) return {0,0};

        FontData& fontData=fonts[font.index];
        float width=0.0f;
        float maxHeight=0.0f;

        const char* ptr=text.c_str();
        while(*ptr){
            uint32_t codepoint=utf8_to_codepoint(ptr);
            if(codepoint==0){
                if(*ptr=='\0') break;
                continue;
            }

            auto it=fontData.glyphs.find(codepoint);
            if(it==fontData.glyphs.end()){
                GlyphInfo info=rasterizeGlyph(fontData,codepoint);
                fontData.glyphs[codepoint]=info;
                it=fontData.glyphs.find(codepoint);
            }

            const GlyphInfo& glyph=it->second;
            width+=glyph.advance*scale;
            if(glyph.height>maxHeight) maxHeight=glyph.height*scale;
        }

        return {width,maxHeight};
    }
}