//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "WindowContext.hpp"
#include "Painter.hpp"
#include "Logger.hpp"
#include <stdexcept>
#include <algorithm>
#include <source_location>

namespace Window{
    WindowContext::WindowContext(Core::Initializer& init,GLFWwindow* window,int width,int height)
        :thisinitializer(&init)
        ,thiswindow(window)
        ,thiswidth(width)
        ,thisheight(height){
        createSurface();

        int fbWidth,fbHeight;
        glfwGetFramebufferSize(thiswindow,&fbWidth,&fbHeight);
        thisextent.width=static_cast<uint32_t>(fbWidth);
        thisextent.height=static_cast<uint32_t>(fbHeight);

        if(thisextent.width==0||thisextent.height==0){
            thisextent.width=1;
            thisextent.height=1;
        }

        createSwapchain();
        createImageViews();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();

        thisPainter=std::make_unique<Render::Painter>(*this);

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "WindowContext created. Extent:"+std::to_string(thisextent.width)+"x"+std::to_string(thisextent.height),
            std::source_location::current()
        );
    }
    WindowContext::~WindowContext(){
        vkDeviceWaitIdle(thisinitializer->getDevice());

        thisPainter.reset();

        VkDevice device=thisinitializer->getDevice();
        if(thisimageAvailableSemaphore!=VK_NULL_HANDLE){
            vkDestroySemaphore(device,thisimageAvailableSemaphore,nullptr);
            thisimageAvailableSemaphore=VK_NULL_HANDLE;
        }
        if(thisinFlightFence!=VK_NULL_HANDLE){
            vkDestroyFence(device,thisinFlightFence,nullptr);
            thisinFlightFence=VK_NULL_HANDLE;
        }

        cleanupSwapchain();

        if(thiscommandPool!=VK_NULL_HANDLE){
            vkDestroyCommandPool(thisinitializer->getDevice(),thiscommandPool,nullptr);
            thiscommandPool=VK_NULL_HANDLE;
            Core::globalLogger.traceLog(
                Core::logger::LOG_INFO,
                "Command pool destroyed.",
                std::source_location::current()
            );
        }

        if(thissurface!=VK_NULL_HANDLE){
            vkDestroySurfaceKHR(thisinitializer->getInstance(),thissurface,nullptr);
            thissurface=VK_NULL_HANDLE;
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "WindowContext destroyed.",
            std::source_location::current()
        );
    }
    void WindowContext::createSurface(){
        if(glfwCreateWindowSurface(
                thisinitializer->getInstance(),
                thiswindow,
                nullptr,
                &thissurface)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create Vulkan surface!");
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Vulkan surface created.",
            std::source_location::current()
        );
    }
    void WindowContext::createSwapchain(){
        if(glfwGetWindowAttrib(thiswindow,GLFW_ICONIFIED)){
            Core::globalLogger.traceLog(
                Core::logger::LOG_WARNING,
                "Window is minimized,skipping swapchain creation.",
                std::source_location::current()
            );
            return;
        }

        VkPhysicalDevice physicalDevice=thisinitializer->getPhysicalDevice();
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,thissurface,&capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,thissurface,&formatCount,nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,thissurface,&formatCount,formats.data());

        VkSurfaceFormatKHR chosenFormat=formats[0];
        for(const auto& format:formats){
            if(format.format==VK_FORMAT_B8G8R8A8_SRGB&&
               format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
                chosenFormat=format;
                break;
            }
        }
        thisswapchainImageFormat=chosenFormat.format;

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,thissurface,&presentModeCount,nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,thissurface,&presentModeCount,presentModes.data());

        VkPresentModeKHR chosenPresentMode=VK_PRESENT_MODE_FIFO_KHR;
        for(const auto& mode:presentModes){
            if(mode==VK_PRESENT_MODE_MAILBOX_KHR){
               chosenPresentMode=mode;
                break;
            }
        }

        uint32_t imageCount=capabilities.minImageCount+1;
        if(capabilities.maxImageCount>0&&imageCount>capabilities.maxImageCount){
            imageCount=capabilities.maxImageCount;
        }

        VkExtent2D extent=thisextent;
        if(capabilities.currentExtent.width!=UINT32_MAX){
            extent=capabilities.currentExtent;
        }
        else{
            extent.width=std::clamp(extent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
            extent.height=std::clamp(extent.height,
                                    capabilities.minImageExtent.height,
                                    capabilities.maxImageExtent.height);
        }
        thisswapchainExtent=extent;

        VkSurfaceTransformFlagBitsKHR preTransform=
            (capabilities.supportedTransforms&VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                ?VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                :capabilities.currentTransform;

        VkCompositeAlphaFlagBitsKHR compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if(!(capabilities.supportedCompositeAlpha&VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)){
            if(capabilities.supportedCompositeAlpha&VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR){
                compositeAlpha=VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
            }
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface=thissurface;
        createInfo.minImageCount=imageCount;
        createInfo.imageFormat=thisswapchainImageFormat;
        createInfo.imageColorSpace=chosenFormat.colorSpace;
        createInfo.imageExtent=extent;
        createInfo.imageArrayLayers=1;
        createInfo.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform=preTransform;
        createInfo.compositeAlpha=compositeAlpha;
        createInfo.presentMode=chosenPresentMode;
        createInfo.clipped=VK_TRUE;
        createInfo.oldSwapchain=VK_NULL_HANDLE;

        uint32_t graphicsFamily=thisinitializer->getGraphicsQueueFamilyIndex();
        uint32_t presentFamily=thisinitializer->getPresentQueueFamilyIndex();

        if(graphicsFamily!=presentFamily){
            uint32_t queueFamilyIndices[]={graphicsFamily,presentFamily};
            createInfo.imageSharingMode=VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount=2;
            createInfo.pQueueFamilyIndices=queueFamilyIndices;
        }
        else{
            createInfo.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount=0;
            createInfo.pQueueFamilyIndices=nullptr;
        }

        if(vkCreateSwapchainKHR(thisinitializer->getDevice(),&createInfo,nullptr,&thisswapchain)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create swapchain!");
        }

        vkGetSwapchainImagesKHR(thisinitializer->getDevice(),thisswapchain,&imageCount,nullptr);
        thisswapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(thisinitializer->getDevice(),thisswapchain,&imageCount,thisswapchainImages.data());

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Swapchain created. Images: "+std::to_string(imageCount) +
            ",Extent: "+std::to_string(extent.width)+"x"+std::to_string(extent.height),
            std::source_location::current()
        );
    }
    void WindowContext::createImageViews(){
        VkDevice device=thisinitializer->getDevice();

        thisswapchainImageViews.resize(thisswapchainImages.size());

        for(size_t i=0;i<thisswapchainImages.size();++i){
            VkImageViewCreateInfo createInfo{};
            createInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image=thisswapchainImages[i];
            createInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format=thisswapchainImageFormat;
            createInfo.components.r=VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g=VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b=VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a=VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel=0;
            createInfo.subresourceRange.levelCount=1;
            createInfo.subresourceRange.baseArrayLayer=0;
            createInfo.subresourceRange.layerCount=1;

            if(vkCreateImageView(device,&createInfo,nullptr,&thisswapchainImageViews[i])!=VK_SUCCESS){
                throw std::runtime_error("Failed to create image view for swapchain image "+std::to_string(i));
            }
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Image views created. Count: "+std::to_string(thisswapchainImageViews.size()),
            std::source_location::current()
        );
    }
    void WindowContext::createCommandPool(){
        VkDevice device=thisinitializer->getDevice();

        uint32_t graphicsFamily=thisinitializer->getGraphicsQueueFamilyIndex();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex=graphicsFamily;

        if(vkCreateCommandPool(device,&poolInfo,nullptr,&thiscommandPool)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create command pool!");
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Command pool created.",
            std::source_location::current()
        );
    }
    void WindowContext::createCommandBuffers(){
        VkDevice device=thisinitializer->getDevice();

        thiscommandBuffers.resize(thisswapchainImages.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool=thiscommandPool;
        allocInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount=static_cast<uint32_t>(thiscommandBuffers.size());

        if(vkAllocateCommandBuffers(device,&allocInfo,thiscommandBuffers.data())!=VK_SUCCESS){
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Command buffers created. Count: "+std::to_string(thiscommandBuffers.size()),
            std::source_location::current()
        );
    }
    void WindowContext::createSyncObjects(){
        VkDevice device=thisinitializer->getDevice();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags=VK_FENCE_CREATE_SIGNALED_BIT;

        if(vkCreateSemaphore(device,&semaphoreInfo,nullptr,&thisimageAvailableSemaphore)!=VK_SUCCESS||
            vkCreateFence(device,&fenceInfo,nullptr,&thisinFlightFence)!=VK_SUCCESS){
            throw std::runtime_error("Failed to create synchronization objects!");
        }
        thisrenderFinishedSemaphores.resize(thisswapchainImages.size());
        for(size_t i=0;i<thisswapchainImages.size();i++){
            vkCreateSemaphore(device,&semaphoreInfo,nullptr,&thisrenderFinishedSemaphores[i]);
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Sync objects created (semaphores+fence).",
            std::source_location::current()
        );
    }
    void WindowContext::cleanupSwapchain(){
        VkDevice device=thisinitializer->getDevice();

        for(auto sem:thisrenderFinishedSemaphores){
            if(sem!=VK_NULL_HANDLE){
                vkDestroySemaphore(device,sem,nullptr);
            }
        }
        thisrenderFinishedSemaphores.clear();

        if(thisimageAvailableSemaphore!=VK_NULL_HANDLE){
            vkDestroySemaphore(device,thisimageAvailableSemaphore,nullptr);
            thisimageAvailableSemaphore=VK_NULL_HANDLE;
        }
        if(thisinFlightFence!=VK_NULL_HANDLE){
            vkDestroyFence(device,thisinFlightFence,nullptr);
            thisinFlightFence=VK_NULL_HANDLE;
        }

        if(!thiscommandBuffers.empty()){
            vkFreeCommandBuffers(device,thiscommandPool,
                                static_cast<uint32_t>(thiscommandBuffers.size()),
                                thiscommandBuffers.data());
            thiscommandBuffers.clear();
        }

        if(!thisswapchainImageViews.empty()){
            for(auto view:thisswapchainImageViews){
                if(view!=VK_NULL_HANDLE){
                    vkDestroyImageView(device,view,nullptr);
                }
            }
            thisswapchainImageViews.clear();
        }

        if(thisswapchain!=VK_NULL_HANDLE){
            vkDestroySwapchainKHR(device,thisswapchain,nullptr);
            thisswapchain=VK_NULL_HANDLE;
        }

        thisswapchainImages.clear();

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Swapchain cleaned up.",
            std::source_location::current()
        );
    }
    void WindowContext::drawFrame(){
        if(glfwGetWindowAttrib(thiswindow,GLFW_ICONIFIED)){
            return;
        }

        if(thisswapchain==VK_NULL_HANDLE){
            recreateSwapchain();
            return;
        }
        VkDevice device=thisinitializer->getDevice();
        VkQueue graphicsQueue=thisinitializer->getGraphicsQueue();
        VkQueue presentQueue=thisinitializer->getPresentQueue();

        vkWaitForFences(device,1,&thisinFlightFence,VK_TRUE,UINT64_MAX);
        vkResetFences(device,1,&thisinFlightFence);

        uint32_t imageIndex;
        VkResult result=vkAcquireNextImageKHR(device,thisswapchain,UINT64_MAX,
                                                thisimageAvailableSemaphore,VK_NULL_HANDLE,&imageIndex);

        if(result==VK_ERROR_OUT_OF_DATE_KHR||result==VK_SUBOPTIMAL_KHR){
            recreateSwapchain();
            return;
        }
        else if(result!=VK_SUCCESS){
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        if(thisPainter){
            thisPainter->recordCommands(thiscommandBuffers[imageIndex],imageIndex);
        }
        else{
            Core::globalLogger.traceLog(Core::logger::LOG_ERROR,"thisPainter does not exist.",std::source_location::current());
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]={thisimageAvailableSemaphore};
        VkPipelineStageFlags waitStages[]={VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount=1;
        submitInfo.pWaitSemaphores=waitSemaphores;
        submitInfo.pWaitDstStageMask=waitStages;

        submitInfo.commandBufferCount=1;
        submitInfo.pCommandBuffers=&thiscommandBuffers[imageIndex];

        VkSemaphore signalSemaphores[]={thisrenderFinishedSemaphores[imageIndex]};
        submitInfo.signalSemaphoreCount=1;
        submitInfo.pSignalSemaphores=signalSemaphores;

        if(vkQueueSubmit(graphicsQueue,1,&submitInfo,thisinFlightFence)!=VK_SUCCESS){
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount=1;
        presentInfo.pWaitSemaphores=signalSemaphores;

        VkSwapchainKHR swapchains[]={thisswapchain};
        presentInfo.swapchainCount=1;
        presentInfo.pSwapchains=swapchains;
        presentInfo.pImageIndices=&imageIndex;

        result=vkQueuePresentKHR(presentQueue,&presentInfo);

        if(result==VK_ERROR_OUT_OF_DATE_KHR||result==VK_SUBOPTIMAL_KHR){
            recreateSwapchain();
        }
        else if(result!=VK_SUCCESS){
            throw std::runtime_error("Failed to present swapchain image!");
        }
    }
    void WindowContext::recreateSwapchain(){
        int fbWidth,fbHeight;
        glfwGetFramebufferSize(thiswindow,&fbWidth,&fbHeight);
        if(fbWidth==0||fbHeight==0){
            Core::globalLogger.traceLog(Core::logger::LOG_WARNING,
                "Recreate swapchain skipped due to zero size.",
                std::source_location::current());
            cleanupSwapchain();
            return;
        }

        VkDevice device=thisinitializer->getDevice();

        vkDeviceWaitIdle(device);

        if(thisimageAvailableSemaphore!=VK_NULL_HANDLE){
            vkDestroySemaphore(device,thisimageAvailableSemaphore,nullptr);
            thisimageAvailableSemaphore=VK_NULL_HANDLE;
        }
        if(thisinFlightFence!=VK_NULL_HANDLE){
            vkDestroyFence(device,thisinFlightFence,nullptr);
            thisinFlightFence=VK_NULL_HANDLE;
        }
        for(auto sem:thisrenderFinishedSemaphores){
            if(sem!=VK_NULL_HANDLE){
                vkDestroySemaphore(device,sem,nullptr);
            }
        }
        thisrenderFinishedSemaphores.clear();

        cleanupSwapchain();

        thisextent.width=static_cast<uint32_t>(fbWidth);
        thisextent.height=static_cast<uint32_t>(fbHeight);
        if(thisextent.width==0||thisextent.height==0){
            thisextent.width=1;
            thisextent.height=1;
        }

        createSwapchain();

        createImageViews();

        if(!thiscommandBuffers.empty()){
            vkFreeCommandBuffers(device,thiscommandPool,
                                static_cast<uint32_t>(thiscommandBuffers.size()),
                                thiscommandBuffers.data());
            thiscommandBuffers.clear();
        }
        createCommandBuffers();

        createSyncObjects();

        if(thisPainter){
            thisPainter->recreateFramebuffers();
        }

        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "Swapchain recreated. New extent: "+std::to_string(thisextent.width)+"x"+std::to_string(thisextent.height),
            std::source_location::current()
        );
    }
}