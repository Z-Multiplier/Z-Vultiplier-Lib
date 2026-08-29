//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef PAINTER_HPP
#define PAINTER_HPP

#include "WindowContext.hpp"
#include "RenderTypes.hpp"
#include <glm/glm.hpp>

#include "VEFontCache/stb_truetype.h"

struct stbtt_fontinfo;

namespace Render{
    using vec2=glm::vec2;
    using vec3=glm::vec3;
    using vec4=glm::vec4;
    struct Painter{
        public:
            struct Font{
                std::string name;
                float size=24.0f;
                size_t index=static_cast<size_t>(-1);
                bool isValid()const{return index!=static_cast<size_t>(-1);}
            };
            struct Image{
                public:
                    Image(const std::string& filepath,Painter* painter);
                    ~Image();

                    Image(const Image&)=delete;
                    Image& operator=(const Image&)=delete;
                    Image(Image&&)=default;
                    Image& operator=(Image&&)=default;

                    VkImageView getImageView()const{return imageView;}
                    VkSampler getSampler()const{return sampler;}
                    int getWidth()const{return width;}
                    int getHeight()const{return height;}
                    bool isValid()const{return isLoaded;}
                    void createTexture(VkDevice device,VkCommandPool pool,VkQueue queue);

                    VkDescriptorSet getDescriptorSet()const{return descriptorSet;}
                    void allocateDescriptorSet();
                    void setDescriptorSet(VkDescriptorSet set){descriptorSet=set;}

                private:
                    Painter* painter=nullptr;

                    void loadFromFile(const std::string& filepath);
                    void cleanup();

                    int width,height,channels;
                    std::vector<unsigned char> pixels;

                    VkImage textureImage=VK_NULL_HANDLE;
                    VkDeviceMemory textureImageMemory=VK_NULL_HANDLE;
                    VkImageView imageView=VK_NULL_HANDLE;
                    VkSampler sampler=VK_NULL_HANDLE;

                    VkDescriptorSet descriptorSet=VK_NULL_HANDLE;

                    bool isLoaded=false;
            };
            Painter(Window::WindowContext& context);
            ~Painter();

            void updateTextureBinding(VkImageView imageView,VkSampler sampler);

            Painter(const Painter&)=delete;
            Painter& operator=(const Painter&)=delete;
            Painter(Painter&&)=delete;
            Painter& operator=(Painter&&)=delete;

            void recordCommands(VkCommandBuffer cmdBuf,uint32_t imageIndex);
            void recreateFramebuffers();

            Font loadFont(const std::string& filepath,float size=24.0f);
            vec2 measureText(const Font& font,const std::string& text,float scale=1.0f);

            void drawLine(vec2 p1,vec2 p2,vec4 color,float width=0.001f);
            void drawPoint(vec2 p,vec4 color,float size=0.001f);
            void drawTriangle(vec2 p1,vec2 p2,vec2 p3,vec4 color);
            void drawTriangle(vec2 p1,vec2 p2,vec2 p3,vec4 c1,vec4 c2,vec4 c3);
            void drawRect(vec2 pos,vec2 size,vec4 color);
            void drawPolygon(const std::vector<vec2>& vertices,const vec4& color);
            void drawCircle(vec2 center,float radius,const vec4& color,int segments=32);
            void drawRoundedRect(vec2 pos,vec2 size,float radius,vec4 color);
            void drawEllipse(vec2 center,vec2 radius,vec4 color,int segments=32);
            void drawRegularPolygon(vec2 center,float radius,int sides,vec4 color);
            void drawStar(vec2 center,float outerRadius,float innerRadius,int points,vec4 color);
            void drawPie(vec2 center,float radius,float startAngle,float endAngle,vec4 color,int segments=32);
            void drawBezier(const std::vector<vec2>& controlPoints,vec4 color,int segments,float width=0.001f);
            void putImage(vec2 pos,vec2 size,Image& img,vec4 tint={1,1,1,1},vec2 UVmin={0,0},vec2 UVmax={1,1});
            void drawText(const Font& font,const std::string& text,vec2 pos,vec4 color={1,1,1,1},float scale=1.0f);

            Image& createImage(const std::string& filepath);

            void beginFrame();
            void endFrame();

            void setTransform(const glm::mat4& transform);
            void resetTransform();
            const glm::mat4& getTransform()const;
            void setModelMatrix(const glm::mat4& matrix);
            void setViewMatrix(const glm::mat4& matrix);
            void setProjectionMatrix(const glm::mat4& matrix);
        private:
            struct GlyphInfo{
                float u0,v0,u1,v1;
                float x0,x1,y0,y1;
                float advance;
                float bearingX;
                float bearingY;
                float width;
                float height;
            };

            struct FontData{
                std::string name;
                float size;
                std::vector<unsigned char> buffer;
                std::unique_ptr<stbtt_fontinfo> info=nullptr;
                float scale;
                int ascent,descent,lineGap;
                std::unordered_map<uint32_t,GlyphInfo> glyphs;
            };

            struct Atlas{
                uint32_t width=1024;
                uint32_t height=1024;
                uint32_t cursorX=0;
                uint32_t cursorY=0;
                uint32_t rowHeight=0;
                std::vector<unsigned char> pixels;
                bool needsUpload=false;

                VkImage image=VK_NULL_HANDLE;
                VkDeviceMemory memory=VK_NULL_HANDLE;
                VkImageView imageView=VK_NULL_HANDLE;
                VkSampler sampler=VK_NULL_HANDLE;
                VkDescriptorSet descriptorSet=VK_NULL_HANDLE;

                void init(uint32_t w,uint32_t h){
                    width=w;
                    height=h;
                    pixels.resize(width*height*4,0);
                    cursorX=0;
                    cursorY=0;
                    rowHeight=0;
                    needsUpload=false;
                }

                void cleanup(VkDevice device){
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
                    if(image!=VK_NULL_HANDLE){
                        vkDestroyImage(device,image,nullptr);
                        image=VK_NULL_HANDLE;
                    }
                    if(memory!=VK_NULL_HANDLE){
                        vkFreeMemory(device,memory,nullptr);
                        memory=VK_NULL_HANDLE;
                    }
                    pixels.clear();
                }

                bool pack(uint32_t w,uint32_t h,uint32_t& outX,uint32_t& outY){
                    if(cursorX+w>width){
                        cursorX=0;
                        cursorY+=rowHeight+1;
                        rowHeight=0;
                    }
                    if(cursorY+h>height){
                        return false;
                    }
                    outX=cursorX;
                    outY=cursorY;
                    cursorX+=w+1;
                    if(h>rowHeight) rowHeight=h;
                    needsUpload=true;
                    return true;
                }
            };

            std::vector<FontData> fonts;
            Atlas textAtlas;
            bool textSystemInitialized=false;

            bool initTextSystem();
            GlyphInfo rasterizeGlyph(FontData& fontData,uint32_t codepoint);
            VkDescriptorSet getAtlasDescriptorSet()const{return textAtlas.descriptorSet;}
            void uploadAtlasToGPU();

            std::vector<std::unique_ptr<Image>> images;
            std::vector<DrawCommand> thisDrawCommands;
            std::vector<VkImage> stencilImages;
            std::vector<VkDeviceMemory> stencilImageMemories;
            std::vector<VkImageView> stencilImageViews;

            VkImage defaultTextureImage=VK_NULL_HANDLE;
            VkDeviceMemory defaultTextureMemory=VK_NULL_HANDLE;
            VkImageView defaultTextureView=VK_NULL_HANDLE;
            VkSampler defaultTextureSampler=VK_NULL_HANDLE;
            VkDescriptorSet defaultDescriptorSet=VK_NULL_HANDLE;

            void createDefaultTexture();

            void createRenderPass();
            void createFramebuffers();
            void createStencilImages();
            void createGraphicsPipeline();
            void createPipeline(VkPipeline& pipeline,
                                VkPipelineLayout& pplineLayout,
                                const VkPipelineColorBlendAttachmentState& blendAttachment,
                                VkShaderModule vertModule,
                                VkShaderModule fragModule,
                                VkDescriptorSetLayout descriptorSetLayout,
                                const VkPushConstantRange* customPushRange=nullptr,
                                uint32_t customPushRangeCount=0,
                                const VkVertexInputAttributeDescription* customAttributes=nullptr,
                                uint32_t customAttributeCount=0);

            void cleanup();

            void createVertexBuffer();
            void updateVertexBuffer();
            void cleanupBuffers();

            glm::mat4 thisTransform=glm::mat4(1.0f);

            VkShaderModule createShaderModule(const std::vector<char>& code);

            Window::WindowContext* thiscontext;

            VkRenderPass thisrenderPass=VK_NULL_HANDLE;

            std::vector<VkFramebuffer> thisframebuffers;

            VkShaderModule thisvertShaderModule=VK_NULL_HANDLE;
            VkShaderModule thisfragShaderModule=VK_NULL_HANDLE;

            VkPipeline thisopaquePipeline=VK_NULL_HANDLE;
            VkPipeline thistransparentPipeline=VK_NULL_HANDLE;
            VkPipelineLayout thisopaquePipelineLayout=VK_NULL_HANDLE;
            VkPipelineLayout thistransparentPipelineLayout=VK_NULL_HANDLE;
            VkDescriptorSetLayout thisTextureDescriptorSetLayout=VK_NULL_HANDLE;
            VkPipeline thiscurrentPipeline=VK_NULL_HANDLE;

            VkBuffer thisvertexBuffer=VK_NULL_HANDLE;
            VkDeviceMemory thisvertexBufferMemory=VK_NULL_HANDLE;
            void* thismappedData=nullptr;
            size_t thismaxVertexCount=100000;

            std::vector<Vertex> thisvertices;
            bool thisframeStarted=false;

            VkBuffer thisUniformBuffer=VK_NULL_HANDLE;
            VkDeviceMemory thisUniformBufferMemory=VK_NULL_HANDLE;
            void* thisUniformMappedData=nullptr;

            VkDescriptorSetLayout thisDescriptorSetLayout=VK_NULL_HANDLE;
            VkDescriptorPool thisDescriptorPool=VK_NULL_HANDLE;
            VkDescriptorSet thisDescriptorSet=VK_NULL_HANDLE;

            glm::mat4 thisModelMatrix=glm::mat4(1.0f);
            glm::mat4 thisViewMatrix=glm::mat4(1.0f);
            glm::mat4 thisProjectionMatrix=glm::mat4(1.0f);

            bool thistextureDescriptorUpdated=false;
            bool fontAtlasInitialized=false;

            void createUniformBuffer();
            void createDescriptorSetLayout();
    };
}

#endif