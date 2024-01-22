#ifndef APP_H
#define APP_H

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
} App;

typedef struct result {
        int code;
        void *data;
} Result;

extern const Result RESULT_SUCCESS;

const Result appRun(struct app *app);

#endif
