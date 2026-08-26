//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef INITIALIZER_HPP
#define INITIALIZER_HPP

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <string>
#include <set>

namespace Core{
    extern void errCallback(int,const char*);
    struct Initializer{
        private:
            VkDevice thisDevice=VK_NULL_HANDLE;
            VkInstance thisInstance=VK_NULL_HANDLE;
            VkPhysicalDevice thisPhysicalDevice=VK_NULL_HANDLE;

            VkQueue thisGraphicsQueue;
            VkQueue thisPresentQueue;
            
            uint32_t graphicsQueueFamilyIndex=UINT32_MAX;
            uint32_t presentQueueFamilyIndex=UINT32_MAX;

            bool glfwInitialized=false;
            bool deviceCreated=false;

            VkDebugUtilsMessengerEXT debugMessenger=VK_NULL_HANDLE;
        public:
            Initializer(std::string);
            ~Initializer();
            Initializer(const Initializer& other)=delete;
            Initializer(Initializer&& other)=delete;
            Initializer& operator=(const Initializer& other)=delete;
            Initializer& operator=(Initializer&& other)=delete;

            VkInstance getInstance()const{return thisInstance;}
            VkDevice getDevice()const{return thisDevice;}
            VkPhysicalDevice getPhysicalDevice()const{return thisPhysicalDevice;}
            VkQueue getGraphicsQueue()const{return thisGraphicsQueue;}
            VkQueue getPresentQueue()const{return thisPresentQueue;}
            bool isGlfwInitialized()const{return glfwInitialized;}
            uint32_t getGraphicsQueueFamilyIndex()const{return graphicsQueueFamilyIndex;}
            uint32_t getPresentQueueFamilyIndex()const{return presentQueueFamilyIndex;}
    };
}

#endif // INITIALIZER_HPP