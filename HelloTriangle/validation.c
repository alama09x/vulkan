#include "validation.h"
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <string.h>

const uint32_t VALIDATION_LAYER_COUNT = 1;
const char *const VALIDATION_LAYERS[16] = {
        "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
        const bool ENABLE_VALIDATION_LAYERS = false;
#else
        const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const bool checkValidationLayerSupport()
{
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);

        VkLayerProperties availableLayers[layerCount];
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

        for (int i = 0; i < VALIDATION_LAYER_COUNT; i++) {
                bool layerFound = false;

                for (int j = 0; j < layerCount; j++) {
                        if (strncmp(
                                VALIDATION_LAYERS[i],
                                availableLayers[j].layerName,
                                64
                        ) == 0) {
                                layerFound = true;
                                break;
                        }
                }

                if (!layerFound)
                        return false;
        }

        return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData
) {
        fprintf(stderr, "Validation layer: %s\n", pCallbackData->pMessage);

        return VK_FALSE;
}

static const VkResult createDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger
) {
        #define T PFN_vkCreateDebugUtilsMessengerEXT
        T f = (T) vkGetInstanceProcAddr(
                instance,
                "vkCreateDebugUtilsMessengerEXT"
        );
        #undef T

        if (f)
                return f(instance, pCreateInfo, pAllocator, pDebugMessenger);
        else
                return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator
) {
        #define T PFN_vkDestroyDebugUtilsMessengerEXT
        T f = (T) vkGetInstanceProcAddr(
                instance,
                "vkDestroyDebugUtilsMessengerEXT"
        );
        #undef T

        if (f)
                f(instance, debugMessenger, pAllocator);
}

const VkDebugUtilsMessengerCreateInfoEXT setupDebugMessengerCreateInfo()
{

        return (VkDebugUtilsMessengerCreateInfoEXT) {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType =
                        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = debugCallback,
        };
}

const Result setupDebugMessenger(App *app)
{
        if (!ENABLE_VALIDATION_LAYERS)
                return RESULT_SUCCESS;

        VkDebugUtilsMessengerCreateInfoEXT createInfo =
                setupDebugMessengerCreateInfo();

        const VkResult res = createDebugUtilsMessengerEXT(
                app->instance,
                &createInfo,
                NULL,
                &app->debugMessenger
        );

        if (res != VK_SUCCESS) {
                return (Result) {
                        .code = res,
                        .data = "failed to set up debug messenger!",
                };
        }
        return RESULT_SUCCESS;
}

