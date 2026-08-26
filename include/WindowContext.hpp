//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef WINDOWCONTEXT_HPP
#define WINDOWCONTEXT_HPP
#include "Initializer.hpp"
#include <vector>
#include <memory>

namespace Render{
    struct Painter;
}

namespace Window{
    struct WindowContext{
        public:
            WindowContext(Core::Initializer& init,GLFWwindow* window,int width,int height);
            ~WindowContext();
            WindowContext(const WindowContext&)=delete;
            WindowContext& operator=(const WindowContext&)=delete;
            Core::Initializer* getInitializer()const{return thisinitializer;}
            VkExtent2D getExtent()const{return thisextent;}
            const std::vector<VkImageView>& getSwapchainImageViews()const{return thisswapchainImageViews;}
            VkFormat getSwapchainImageFormat()const{return thisswapchainImageFormat;}
            void drawFrame();
            void recreateSwapchain();
            const std::unique_ptr<Render::Painter>& getPainter()const{return thisPainter;}
            VkCommandPool getCommandPool()const{return thiscommandPool;}
        private:
            void createSurface();
            void createSwapchain();
            void createImageViews();
            void createCommandPool();
            void createCommandBuffers();
            void createSyncObjects();

            void cleanupSwapchain();

            Core::Initializer* thisinitializer;
            GLFWwindow* thiswindow;

            int thiswidth;
            int thisheight;
            VkExtent2D thisextent;

            VkSurfaceKHR thissurface;
            VkSwapchainKHR thisswapchain;
            std::vector<VkImage> thisswapchainImages;
            std::vector<VkImageView> thisswapchainImageViews;
            VkFormat thisswapchainImageFormat;
            VkExtent2D thisswapchainExtent;

            VkCommandPool thiscommandPool;
            std::vector<VkCommandBuffer> thiscommandBuffers;

            VkSemaphore thisimageAvailableSemaphore;
            std::vector<VkSemaphore> thisrenderFinishedSemaphores;
            VkFence thisinFlightFence;

            std::unique_ptr<Render::Painter> thisPainter;
    };
}

#endif