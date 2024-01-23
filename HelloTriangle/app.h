#ifndef APP_H
#define APP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>

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
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkCommandBuffer *commandBuffers;
        VkSemaphore *imageAvailableSemaphores;
        VkSemaphore *renderFinishedSemaphores;
        VkFence *inFlightFences;
        bool framebufferResized;
} App;

typedef struct result {
        int code;
        void *data;
} Result;

extern const Result RESULT_SUCCESS;

const Result appRun(struct app *app);

#endif
