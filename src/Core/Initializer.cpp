//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "Initializer.hpp"
#include "Logger.hpp"
#include <vector>
#include <cstring>

namespace Core{
    inline void errCallback(int err,const char* descr){
        Core::globalLogger.traceLog(Core::logger::LOG_ERROR,std::string(descr),std::source_location::current());
    }
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData){
        if(messageSeverity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,std::string(pCallbackData->pMessage),std::source_location::current());
        }
        else if(messageSeverity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
            Core::globalLogger.traceLog(Core::logger::LOG_WARNING,std::string(pCallbackData->pMessage),std::source_location::current());
        }
        else{
            Core::globalLogger.traceLog(Core::logger::LOG_INFO,std::string(pCallbackData->pMessage),std::source_location::current());
        }

        return VK_FALSE;
    }
    static bool checkValidationLayerSupport(){
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount,nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount,availableLayers.data());
        const char* layerName="VK_LAYER_KHRONOS_validation";
        for(const auto& layerProperties:availableLayers){
            if(strcmp(layerName,layerProperties.layerName)==0){
                return true;
            }
        }
        return false;
    }
    Initializer::Initializer(std::string appName){
        glfwSetErrorCallback(errCallback);
        if(!glfwInit()){
            throw std::runtime_error("Failed to init glfw");
        }
        glfwInitialized=true;
        Core::globalLogger.traceLog(Core::logger::LOG_INFO,"GLFW initialized",std::source_location::current());

        VkApplicationInfo appInfo{};
        appInfo.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName=appName.c_str();
        appInfo.applicationVersion=VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName="Z-VultiplierLib";
        appInfo.engineVersion=VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion=VK_API_VERSION_1_3;

        uint32_t glfwExtensionCount=0;
        const char** glfwExtensions=glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions,glfwExtensions+glfwExtensionCount);

        auto contains=[&](const char* ext){
            for(auto* e:extensions){
                if(strcmp(e,ext)==0) return true;
            }
            return false;
        };

        if(!contains(VK_KHR_SURFACE_EXTENSION_NAME)){
            extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        }
        #ifdef _WIN32
        if(!contains(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)){
            extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        }
        #endif

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        bool enableValidationLayers=true;

        std::vector<const char*> enabledLayers;
        if(enableValidationLayers&&checkValidationLayerSupport()){
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
            Core::globalLogger.traceLog(Core::logger::LOG_INFO,"Validation layers enabled.",std::source_location::current());
        }
        else{
            Core::globalLogger.traceLog(Core::logger::LOG_WARNING,"Validation layers not available,continuing without.",std::source_location::current());
        }

        const char* validationLayer="VK_LAYER_KHRONOS_validation";

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo=&appInfo;
        instanceCreateInfo.enabledLayerCount=static_cast<uint32_t>(enabledLayers.size());
        instanceCreateInfo.ppEnabledLayerNames=enabledLayers.data();
        instanceCreateInfo.enabledExtensionCount=static_cast<uint32_t>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames=extensions.data();

        if(vkCreateInstance(&instanceCreateInfo,nullptr,&thisInstance)!=VK_SUCCESS){
            glfwTerminate();
            glfwInitialized=false;
            throw std::runtime_error("Failed to create Vulkan instance!");
        }

        if(enableValidationLayers&&checkValidationLayerSupport()){
            auto vkCreateDebugUtilsMessengerEXT=(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(thisInstance,"vkCreateDebugUtilsMessengerEXT");
            if(vkCreateDebugUtilsMessengerEXT!=nullptr){
                VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
                messengerCreateInfo.sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                messengerCreateInfo.messageSeverity=
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
                messengerCreateInfo.messageType=
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                messengerCreateInfo.pfnUserCallback=debugCallback;
                messengerCreateInfo.pUserData=nullptr;
                if(vkCreateDebugUtilsMessengerEXT(thisInstance,&messengerCreateInfo,nullptr,&debugMessenger)!=VK_SUCCESS){
                    Core::globalLogger.traceLog(Core::logger::LOG_WARNING,"Failed to create debug messenger!",std::source_location::current());
                }
                else{
                    Core::globalLogger.traceLog(Core::logger::LOG_INFO,"Debug messenger created.",std::source_location::current());
                }
            }
            else{
                Core::globalLogger.traceLog(Core::logger::LOG_WARNING,"vkCreateDebugUtilsMessengerEXT not available.",std::source_location::current());
            }
        }

        globalLogger.traceLog(Core::logger::LOG_INFO,"Vulkan initialized.",std::source_location::current());
        

        uint32_t deviceCount=0;
        vkEnumeratePhysicalDevices(thisInstance,&deviceCount,nullptr);
        if(deviceCount==0){
            throw std::runtime_error("No Vulkan-capable GPUs found!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(thisInstance,&deviceCount,devices.data());

        for(const auto& device:devices){
            uint32_t queueFamilyCount=0;
            vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount,nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount,queueFamilies.data());

            for(uint32_t i=0;i<queueFamilyCount;++i){
                if(queueFamilies[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){
                    graphicsQueueFamilyIndex=i;
                    break;
                }
            }

            if(graphicsQueueFamilyIndex!=UINT32_MAX){
                thisPhysicalDevice=device;
                break;
            }
        }

        if(thisPhysicalDevice==VK_NULL_HANDLE){
            throw std::runtime_error("No suitable GPU found!");
        }

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(thisPhysicalDevice,&props);
        globalLogger.traceLog(Core::logger::LOG_INFO,
            "Selected GPU: "+std::string(props.deviceName),
            std::source_location::current());


        glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
        GLFWwindow* tempWindow=glfwCreateWindow(1,1,"Temp",nullptr,nullptr);
        if(!tempWindow){
            throw std::runtime_error("Failed to create temporary window for device creation!");
        }

        VkSurfaceKHR tempSurface=VK_NULL_HANDLE;
        if(glfwCreateWindowSurface(thisInstance,tempWindow,nullptr,&tempSurface)!=VK_SUCCESS){
            glfwDestroyWindow(tempWindow);
            throw std::runtime_error("Failed to create temporary surface!");
        }

        uint32_t queueFamilyCount=0;
        vkGetPhysicalDeviceQueueFamilyProperties(thisPhysicalDevice,&queueFamilyCount,nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(thisPhysicalDevice,&queueFamilyCount,queueFamilies.data());

        bool foundPresent=false;
        for(uint32_t i=0;i<queueFamilyCount;++i){
            VkBool32 presentSupport=VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(thisPhysicalDevice,i,tempSurface,&presentSupport);
            if(presentSupport){
                presentQueueFamilyIndex=i;
                foundPresent=true;
                break;
            }
        }

        if(!foundPresent){
            vkDestroySurfaceKHR(thisInstance,tempSurface,nullptr);
            glfwDestroyWindow(tempWindow);
            throw std::runtime_error("No queue family supports present!");
        }

        std::set<uint32_t> uniqueQueueFamilies={
            graphicsQueueFamilyIndex,
            presentQueueFamilyIndex
        };

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority=1.0f;

        for(uint32_t family:uniqueQueueFamilies){
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex=family;
            queueInfo.queueCount=1;
            queueInfo.pQueuePriorities=&queuePriority;
            queueCreateInfos.push_back(queueInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy=VK_TRUE;

        std::vector<const char*> deviceExtensions={
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount=static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos=queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures=&deviceFeatures;
        deviceCreateInfo.enabledExtensionCount=static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames=deviceExtensions.data();

        if(vkCreateDevice(thisPhysicalDevice,&deviceCreateInfo,nullptr,&thisDevice)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(thisDevice,graphicsQueueFamilyIndex,0,&thisGraphicsQueue);
        vkGetDeviceQueue(thisDevice,presentQueueFamilyIndex,0,&thisPresentQueue);

        globalLogger.traceLog(Core::logger::LOG_INFO,
                              "Logical device created successfully.",
                              std::source_location::current());

        vkDestroySurfaceKHR(thisInstance,tempSurface,nullptr);
        glfwDestroyWindow(tempWindow);
        glfwWindowHint(GLFW_VISIBLE,GLFW_TRUE);

        globalLogger.traceLog(Core::logger::LOG_INFO,"Intialization Done.",std::source_location::current());
    }
    Initializer::~Initializer(){
        if(debugMessenger!=VK_NULL_HANDLE){
            auto vkDestroyDebugUtilsMessengerEXT=(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(thisInstance,"vkDestroyDebugUtilsMessengerEXT");
            if(vkDestroyDebugUtilsMessengerEXT!=nullptr){
                vkDestroyDebugUtilsMessengerEXT(thisInstance,debugMessenger,nullptr);
            }
            debugMessenger=VK_NULL_HANDLE;
        }

        if(thisDevice!=VK_NULL_HANDLE){
            vkDestroyDevice(thisDevice,nullptr);
            thisDevice=VK_NULL_HANDLE;
            globalLogger.traceLog(Core::logger::LOG_INFO,"Logical device destroyed.",std::source_location::current());
        }

        if(thisInstance!=VK_NULL_HANDLE){
            vkDestroyInstance(thisInstance,nullptr);
            thisInstance=VK_NULL_HANDLE;
        }
        glfwTerminate();
        glfwInitialized=false;

        globalLogger.traceLog(Core::logger::LOG_INFO,"Initializer destroyed.",std::source_location::current());
    }
}
