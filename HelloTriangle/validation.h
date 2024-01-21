#ifndef VALIDATION_H
#define VALIDATION_H

#include "app.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

extern const bool ENABLE_VALIDATION_LAYERS;

extern const uint32_t VALIDATION_LAYER_COUNT;
extern const char *const VALIDATION_LAYERS[16];

void destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator
);
const struct result setupDebugMessenger(App *app);
const VkDebugUtilsMessengerCreateInfoEXT setupDebugMessengerCreateInfo();
const bool checkValidationLayerSupport();

#endif
