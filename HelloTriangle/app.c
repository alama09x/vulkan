#include "app.h"
#include "validation.h"

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define handle(result) \
        res = result; \
        if (res.code != 0) \
                return res;

static const uint32_t WIDTH = 800;
static const uint32_t HEIGHT = 600;

const Result RESULT_SUCCESS = (Result) {
        .code = 0,
        .data = NULL,
};

#define DEVICE_EXTENSION_COUNT 1
const char *const DEVICE_EXTENSIONS[DEVICE_EXTENSION_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const Result initWindow(App *app)
{
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        app->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
        return RESULT_SUCCESS;
}

const char **getRequiredExtensions(uint32_t *extCount)
{
        const char **extensions;
        extensions = glfwGetRequiredInstanceExtensions(extCount);

        if (ENABLE_VALIDATION_LAYERS)
                extensions[(*extCount)++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        return extensions;
}

static void checkExtensions(const char **extensions, uint32_t extCount)
{
        uint32_t availableExtCount = 0;
        vkEnumerateInstanceExtensionProperties(NULL, &availableExtCount, NULL);
        VkExtensionProperties availableExts[availableExtCount];
        vkEnumerateInstanceExtensionProperties(NULL, &availableExtCount, availableExts);

        printf("Enabled extensions:\n");
        for (int i = 0; i < extCount; i++) {
                printf("\t%s\n", extensions[i]);
                bool included = false;
                for (int j = 0; j < availableExtCount; j++) {
                        if (strncmp(extensions[i], availableExts[j].extensionName, 64) == 0) {
                                included = true;
                                break;
                        }
                }
                if (!included)
                        fprintf(stderr, "WARN: extension, %s, not included.", extensions[i]);
        }

        printf("Available extensions:\n");
        for (int i = 0; i < availableExtCount; i++)
                printf("\t%s\n", availableExts[i].extensionName);
}


static const Result createInstance(App *app)
{
        printf("In debug mode: %s\n", ENABLE_VALIDATION_LAYERS ? "true" : "false");
        if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
                return (Result) {
                        .code = -1,
                        .data = "validation layers requested, but not available!",
                };
        }

        const VkApplicationInfo appInfo = {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = "Hello Triangle",
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = "No Engine",
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VK_API_VERSION_1_0,
        };

        uint32_t extCount = 0;
        const char **extensions = getRequiredExtensions(&extCount);
        checkExtensions(extensions, extCount);

        VkInstanceCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo = &appInfo,
                .enabledExtensionCount = extCount,
                .ppEnabledExtensionNames = extensions,
                .enabledLayerCount = 0,
        };

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (ENABLE_VALIDATION_LAYERS) {
                debugCreateInfo = setupDebugMessengerCreateInfo();
                createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
                createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)
                        &debugCreateInfo;
        }

        VkResult result = vkCreateInstance(&createInfo, NULL, &app->instance);
        if (result != VK_SUCCESS) {
                return (Result) {
                        .code = result,
                        .data = "failed to create instance!",
                };
        }
        
        return RESULT_SUCCESS;
}

static const Result createSurface(App *app)
{
        VkResult result = glfwCreateWindowSurface(
                app->instance,
                app->window,
                NULL, &app->surface
        );

        if (result != VK_SUCCESS) {
                return (Result) {
                        .code = result,
                        .data = "failed to create window surface!",
                };
        }

        return RESULT_SUCCESS;
}

typedef struct {
        VkSurfaceCapabilitiesKHR capabilities;
        uint32_t formatCount;
        VkSurfaceFormatKHR formats[256];
        uint32_t presentModeCount;
        VkPresentModeKHR presentModes[256];
} SwapChainSupportDetails;

static const SwapChainSupportDetails querySwapChainSupport(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                device,
                surface,
                &details.capabilities
        );

        vkGetPhysicalDeviceSurfaceFormatsKHR(
                device,
                surface,
                &details.formatCount,
                NULL
        );

        if (details.formatCount != 0) {
                vkGetPhysicalDeviceSurfaceFormatsKHR(
                        device,
                        surface,
                        &details.formatCount,
                        details.formats
                );
        }

        vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                surface,
                &details.presentModeCount,
                NULL
        );

        if (details.presentModeCount != 0) {
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                        device,
                        surface,
                        &details.presentModeCount,
                        details.presentModes
                );
        }

        return details;
}

typedef struct {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
} QueueFamilyIndices;

static const bool indicesComplete(QueueFamilyIndices indices) {
        return indices.graphicsFamily != -1 && indices.presentFamily != -1;
}

static const QueueFamilyIndices findQueueFamilies(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
) {
        QueueFamilyIndices indices = {
                .graphicsFamily = -1,
                .presentFamily = -1,
        };

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

        VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(
                device,
                &queueFamilyCount,
                queueFamilies
        );

        VkBool32 presentSupport = false;
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        indices.graphicsFamily = i;

                vkGetPhysicalDeviceSurfaceSupportKHR(
                        device,
                        i,
                        surface,
                        &presentSupport
                );

                if (presentSupport)
                        indices.presentFamily = i;

                if (indicesComplete(indices))
                        break;
        }

        return indices;
}

static const bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

        VkExtensionProperties availableExtensions[extensionCount];
        vkEnumerateDeviceExtensionProperties(
                device,
                NULL,
                &extensionCount,
                availableExtensions
        );

        bool missingExt = true;
        for (int i = 0; i < DEVICE_EXTENSION_COUNT; i++) {
                for (int j = 0; j < extensionCount; j++) {
                        if (strncmp(
                                DEVICE_EXTENSIONS[i],
                                availableExtensions[j].extensionName,
                                64
                        ) == 0) {
                                missingExt = false;
                                break;
                        }
                }

                if (missingExt)
                        break;
        }

        return !missingExt;
}

static const bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
        const QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(
                        device,
                        surface
                );

                swapChainAdequate =
                        swapChainSupport.formatCount != 0
                        && swapChainSupport.presentModeCount != 0;
        }

        return indicesComplete(indices)
                && extensionsSupported
                && swapChainAdequate;
}

static const Result pickPhysicalDevice(App *app)
{
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);

        if (deviceCount == 0) {
                return (Result) {
                        .code = -1,
                        .data = "failed to find GPUs with Vulkan support!",
                };
        }

        VkPhysicalDevice devices[deviceCount];
        vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices);

        for (int i = 0; i < deviceCount; i++) {
                if (isDeviceSuitable(devices[i], app->surface)) {
                        app->physicalDevice = devices[i];
                        break;
                }
        }

        if (app->physicalDevice == NULL) {
                return (Result) {
                        .code = -1,
                        .data = "failed to find a suitable GPU!",
                };
        }
        
        return RESULT_SUCCESS;
}

static const Result createLogicalDevice(App *app)
{
        const QueueFamilyIndices indices = findQueueFamilies(
                app->physicalDevice,
                app->surface
        );

        #define QUEUE_COUNT 2
        VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_COUNT];
        uint32_t queueFamilies[QUEUE_COUNT] = {
                indices.graphicsFamily,
                indices.presentFamily,
        };

        const float queuePriority = 1.0;
        uint32_t uniqueCount = 0;
        for (int i = 0; i < QUEUE_COUNT; i++) {
                bool unique = true;
                for (int j = 0; j < uniqueCount; j++) {
                        if (queueFamilies[i] ==
                                queueCreateInfos[j].queueFamilyIndex
                        ) {
                                unique = false;
                                break;
                        }
                }

                if (!unique)
                        continue;

                queueCreateInfos[uniqueCount++] = (VkDeviceQueueCreateInfo) {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = queueFamilies[i],
                        .queueCount = 1,
                        .pQueuePriorities = &queuePriority,
                };
        }

        const VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = uniqueCount,
                .pQueueCreateInfos = queueCreateInfos,
                .pEnabledFeatures = &deviceFeatures,
                .enabledExtensionCount = DEVICE_EXTENSION_COUNT,
                .ppEnabledExtensionNames = DEVICE_EXTENSIONS,
                .enabledLayerCount = 0,
        };

        // Technically ignored but good practice for compatability
        if (ENABLE_VALIDATION_LAYERS) {
                createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
                createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
        }

        VkResult result = vkCreateDevice(
                app->physicalDevice,
                &createInfo,
                NULL,
                &app->device
        );

        if (result != VK_SUCCESS) {
                return (Result) {
                        .code = result,
                        .data = "failed to create logical device!",
                };
        }

        vkGetDeviceQueue(
                app->device,
                indices.graphicsFamily,
                0, // single queue
                &app->graphicsQueue
        );

        vkGetDeviceQueue(
                app->device,
                indices.presentFamily,
                0, // single queue
                &app->presentQueue
        );

        return RESULT_SUCCESS;
}

static const Result initVulkan(App *app)
{
        Result res;
        handle(createInstance(app));
        handle(setupDebugMessenger(app));
        handle(createSurface(app));
        handle(pickPhysicalDevice(app));
        handle(createLogicalDevice(app));
        return RESULT_SUCCESS;
}

static const Result mainLoop(App *app)
{
        while (!glfwWindowShouldClose(app->window)) {
                glfwPollEvents();
        }
        return RESULT_SUCCESS;
}

static const Result cleanUp(App *app)
{
        if (ENABLE_VALIDATION_LAYERS) {
                destroyDebugUtilsMessengerEXT(
                        app->instance,
                        app->debugMessenger,
                        NULL
                );
        }
        vkDestroyDevice(app->device, NULL);
        vkDestroySurfaceKHR(app->instance, app->surface, NULL);
        vkDestroyInstance(app->instance, NULL);
        glfwDestroyWindow(app->window);
        glfwTerminate();
        return RESULT_SUCCESS;
}

const Result appRun(App *app)
{
        Result res;
        handle(initWindow(app));
        handle(initVulkan(app));
        handle(mainLoop(app));
        handle(cleanUp(app));
        return RESULT_SUCCESS;
}
