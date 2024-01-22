#ifndef APP_H
#define APP_H

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct app {
        GLFWwindow *window;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkSwapchainKHR swapchain;
        uint32_t swapchainImageCount;
        VkImage *swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        VkImageView *swapchainImageViews;
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        VkFramebuffer *swapchainFramebuffers;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
} App;

typedef struct result {
        int code;
        void *data;
} Result;

extern const Result RESULT_SUCCESS;

const Result appRun(struct app *app);

#endif
