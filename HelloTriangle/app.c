#include "app.h"
#include "validation.h"

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
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

static const int MAX_FRAMES_IN_FLIGHT = 2;

#define RESULT_ERROR(c, msg) (Result) { .code = c, .data = msg }

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
        if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
                return RESULT_ERROR(-1, "validation layers requested, but not available!");

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
        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create instance!");
        
        return RESULT_SUCCESS;
}

static const Result createSurface(App *app)
{
        VkResult result = glfwCreateWindowSurface(
                app->instance,
                app->window,
                NULL, &app->surface
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create window surface!");

        return RESULT_SUCCESS;
}

typedef struct {
        VkSurfaceCapabilitiesKHR capabilities;
        uint32_t formatCount;
        VkSurfaceFormatKHR formats[256];
        uint32_t presentModeCount;
        VkPresentModeKHR presentModes[4];
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

        bool swapchainAdequate = false;
        if (extensionsSupported) {
                SwapChainSupportDetails swapchainSupport = querySwapChainSupport(
                        device,
                        surface
                );

                swapchainAdequate =
                        swapchainSupport.formatCount != 0
                        && swapchainSupport.presentModeCount != 0;
        }

        return indicesComplete(indices)
                && extensionsSupported
                && swapchainAdequate;
}

static const Result pickPhysicalDevice(App *app)
{
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);

        if (deviceCount == 0)
                return RESULT_ERROR(-1, "failed to find GPUs with Vulkan support!");

        VkPhysicalDevice devices[deviceCount];
        vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices);

        for (int i = 0; i < deviceCount; i++) {
                if (isDeviceSuitable(devices[i], app->surface)) {
                        app->physicalDevice = devices[i];
                        break;
                }
        }

        if (app->physicalDevice == NULL)
                return RESULT_ERROR(-1, "failed to find a suitable GPU!");
        
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

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create logical device!");

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

static const VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const VkSurfaceFormatKHR *availableFormats,
        uint32_t formatCount
) {
        for (int i = 0; i < formatCount; i++) {
                if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
                        return availableFormats[i];
        }

        return availableFormats[0];
}

static const VkPresentModeKHR chooseSwapPresentMode(
        const VkPresentModeKHR *availablePresentModes,
        uint32_t presentModeCount
) {
        for (int i = 0; i < presentModeCount; i++)  {
                if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                        return availablePresentModes[i];
        }

        return VK_PRESENT_MODE_FIFO_KHR;
}

static const VkExtent2D chooseSwapExtent(
        GLFWwindow *window,
        const VkSurfaceCapabilitiesKHR capabilities
) {
        if (capabilities.currentExtent.width != UINT32_MAX)
                return capabilities.currentExtent;

        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);
        return (VkExtent2D) {
                .width = capabilities.currentExtent.width < width
                        ? capabilities.currentExtent.width
                        : (uint32_t) width,
                .height = capabilities.currentExtent.height < height
                        ? capabilities.currentExtent.height
                        :(uint32_t) height,
        };
}

static const Result createSwapChain(App *app)
{
        const SwapChainSupportDetails swapchainSupport = querySwapChainSupport(
                app->physicalDevice,
                app->surface
        );

        const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
                swapchainSupport.formats,
                swapchainSupport.formatCount
        );

        const VkPresentModeKHR presentMode = chooseSwapPresentMode(
                swapchainSupport.presentModes,
                swapchainSupport.presentModeCount
        );

        const VkExtent2D extent = chooseSwapExtent(
                app->window,
                swapchainSupport.capabilities
        );

        uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0
                && imageCount > swapchainSupport.capabilities.maxImageCount
        ) {
                
                imageCount = swapchainSupport.capabilities.maxImageCount;
        }

        QueueFamilyIndices indices = findQueueFamilies(
                app->physicalDevice,
                app->surface
        );

        uint32_t queueFamilyIndices[] = {
                indices.graphicsFamily,
                indices.presentFamily,
        };

        VkSwapchainCreateInfoKHR createInfo = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = app->surface,
                .minImageCount = imageCount,
                .imageFormat = surfaceFormat.format,
                .imageColorSpace = surfaceFormat.colorSpace,
                .imageExtent = extent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .preTransform = swapchainSupport.capabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = presentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = VK_NULL_HANDLE,
        };

        if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // optional
        }

        const VkResult result = vkCreateSwapchainKHR(
                app->device,
                &createInfo,
                NULL,
                &app->swapchain
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create swapchain!");

        vkGetSwapchainImagesKHR(app->device, app->swapchain, &imageCount, NULL);
        app->swapchainImageCount = imageCount;

        app->swapchainImages = malloc(sizeof(VkImage) * app->swapchainImageCount);
        
        vkGetSwapchainImagesKHR(
                app->device,
                app->swapchain,
                &app->swapchainImageCount,
                app->swapchainImages
        );


        app->swapchainImageFormat = surfaceFormat.format;
        app->swapchainExtent = extent;
        
        return RESULT_SUCCESS;
}

static const Result createImageViews(App *app)
{
        app->swapchainImageViews = malloc(sizeof(VkImage) * app->swapchainImageCount);
        for (int i = 0; i < app->swapchainImageCount; i++) {
                VkImageViewCreateInfo createInfo = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .image = app->swapchainImages[i],
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = app->swapchainImageFormat,
                        .components = (VkComponentMapping) {
                                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                .g= VK_COMPONENT_SWIZZLE_IDENTITY,
                                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                        },
                        .subresourceRange = (VkImageSubresourceRange) {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                        },
                };

                VkResult result = vkCreateImageView(
                        app->device,
                        &createInfo,
                        NULL,
                        &app->swapchainImageViews[i]
                );

                if (result != VK_SUCCESS)
                        return RESULT_ERROR(result, "failed to create image views!");
        }
        return RESULT_SUCCESS;
}

static const Result readFile(const char *fname, uint32_t *pfsize)
{
        FILE *fp = fopen(fname, "rb+");
        if (!fp) {
                char errmsg[64];
                sprintf(errmsg, "failed to open file: %s", fname);
                return RESULT_ERROR(-1, errmsg);
        }

        fseek(fp, 0l, SEEK_END);
        *pfsize = (uint32_t) ftell(fp);
        rewind(fp);

        char *fcontent = malloc(*pfsize * sizeof(char));
        fread(fcontent, 1, *pfsize, fp);

        fclose(fp);
        return (Result) {
                .code = 0,
                .data = (char *) fcontent,
        };
}

static const Result createShaderModule(
        VkDevice device,
        const char *code,
        uint32_t fsize
) {
        printf("File size: %d\n", fsize);
        VkShaderModuleCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = fsize,
                .pCode = (const uint32_t *) code,
        };

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(
                device,
                &createInfo,
                NULL,
                &shaderModule
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create shader module!");

        return (Result) {
                .code = 0,
                .data = (VkShaderModule) shaderModule,
        };
}

static const Result createRenderPass(App *app)
{
        const VkAttachmentDescription colorAttachment = {
                .format = app->swapchainImageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        const VkAttachmentReference colorAttachmentRef = {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        const VkSubpassDescription subpass = {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachmentRef,
        };

        const VkSubpassDependency dependency = {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        const VkRenderPassCreateInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = 1,
                .pAttachments = &colorAttachment,
                .subpassCount = 1,
                .pSubpasses = &subpass,
                .dependencyCount = 1,
                .pDependencies = &dependency,
        };

        const VkResult result = vkCreateRenderPass(
                app->device,
                &renderPassInfo,
                NULL,
                &app->renderPass
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create render pass!");

        return RESULT_SUCCESS;
}

static const Result createGraphicsPipeline(App *app)
{
        uint32_t vertSize;
        uint32_t fragSize;

        const Result vertShaderResult = readFile("shaders/vert.spv", &vertSize);
        if (vertShaderResult.code != 0)
                return vertShaderResult;

        const Result fragShaderResult = readFile("shaders/frag.spv", &fragSize);
        if (fragShaderResult.code != 0)
                return fragShaderResult;

        char *vertShaderCode = vertShaderResult.data;
        char *fragShaderCode = fragShaderResult.data;

        const Result vertModuleResult = createShaderModule(
                app->device,
                vertShaderCode,
                vertSize
        );
        if (vertModuleResult.code != 0)
                return vertModuleResult;

        const Result fragModuleResult = createShaderModule(
                app->device,
                fragShaderCode,
                fragSize
        );
        if (fragModuleResult.code != 0)
                return fragModuleResult;

        VkShaderModule vertShaderModule = vertModuleResult.data;
        VkShaderModule fragShaderModule = fragModuleResult.data;

        const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertShaderModule,
                .pName = "main",
        };

        const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragShaderModule,
                .pName = "main",
        };

        const VkPipelineShaderStageCreateInfo shaderStages[] = {
                vertShaderStageInfo,
                fragShaderStageInfo,
        };

        const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 0,
                .pVertexBindingDescriptions = NULL, // optional
                .vertexAttributeDescriptionCount = 0,
                .pVertexAttributeDescriptions = NULL, // optional
        };

        const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
        };

        const VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float) app->swapchainExtent.width,
                .height = (float) app->swapchainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };

        const VkRect2D scissor = {
                .offset = { .x = 0, .y = 0 },
                .extent = app->swapchainExtent,
        };

        uint32_t dynamicStateCount = 2;
        const VkDynamicState dynamicStates[] = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };

        const VkPipelineDynamicStateCreateInfo dynamicState = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = dynamicStateCount,
                .pDynamicStates = dynamicStates,
        };

        const VkPipelineViewportStateCreateInfo viewportState = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor,
        };

        const VkPipelineRasterizationStateCreateInfo rasterizer = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .lineWidth = 1.0f,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
        };

        const VkPipelineMultisampleStateCreateInfo multisampling = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .sampleShadingEnable = VK_FALSE,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };

        const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
                .colorWriteMask =
                        VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
                .blendEnable = VK_FALSE,
        };

        const VkPipelineColorBlendStateCreateInfo colorBlending = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachment,
        };

        const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        };

        const VkResult pipelineLayoutResult = vkCreatePipelineLayout(
                app->device,
                &pipelineLayoutInfo,
                NULL,
                &app->pipelineLayout
        );

        if (pipelineLayoutResult != VK_SUCCESS) {
                return RESULT_ERROR(
                        pipelineLayoutResult,
                        "failed to create pipeline layout!"
                );
        }

        const VkGraphicsPipelineCreateInfo pipelineInfo = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = 2,
                .pStages = shaderStages,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pDepthStencilState = NULL, // optional
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicState,
                .layout = app->pipelineLayout,
                .renderPass = app->renderPass,
                .subpass = 0,
                .basePipelineHandle = NULL, // optional
                .basePipelineIndex = -1, // optional
        };

        VkResult result = vkCreateGraphicsPipelines(
                app->device,
                NULL,
                1,
                &pipelineInfo,
                NULL,
                &app->graphicsPipeline
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create graphics pipeline!");

        vkDestroyShaderModule(app->device, fragShaderModule, NULL);
        vkDestroyShaderModule(app->device, vertShaderModule, NULL);
        free(fragShaderCode);
        free(vertShaderCode);
        return RESULT_SUCCESS;
}

static const Result createFramebuffers(App *app)
{
        app->swapchainFramebuffers = malloc(
                app->swapchainImageCount * sizeof(VkFramebuffer)
        );

        for (int i = 0; i < app->swapchainImageCount; i++) {
                const VkImageView attachments[] = {
                        app->swapchainImageViews[i],
                };

                const VkFramebufferCreateInfo createInfo = {
                        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                        .renderPass = app->renderPass,
                        .attachmentCount = 1,
                        .pAttachments = attachments,
                        .width = app->swapchainExtent.width,
                        .height = app->swapchainExtent.height,
                        .layers = 1,
                };

                VkResult result = vkCreateFramebuffer(
                        app->device,
                        &createInfo,
                        NULL,
                        &app->swapchainFramebuffers[i]
                );

                if (result != VK_SUCCESS)
                        return RESULT_ERROR(result, "failed to create framebuffer!");
        }

        return RESULT_SUCCESS;
}

static const Result createCommandPool(App *app)
{
        const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(
                app->physicalDevice,
                app->surface
        );

        const VkCommandPoolCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = queueFamilyIndices.graphicsFamily,
        };

        const VkResult result = vkCreateCommandPool(
                app->device,
                &createInfo,
                NULL,
                &app->commandPool
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to create command pool");
        
        return RESULT_SUCCESS;
}

static const Result createCommandBuffers(App *app)
{
        app->commandBuffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
        const VkCommandBufferAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = app->commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
        };

        const VkResult result = vkAllocateCommandBuffers(
                app->device,
                &allocInfo,
                app->commandBuffers
        );

        if (result != VK_SUCCESS)
                return RESULT_ERROR(result, "failed to allocate command buffers!");

        return RESULT_SUCCESS;
}

static const Result recordCommandBuffer(
        App *app,
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex
) {
        const VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = 0, // optional
                .pInheritanceInfo = NULL, // optional
        };

        const VkResult commandBufferBeginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (commandBufferBeginResult != VK_SUCCESS) {
                return RESULT_ERROR(
                        commandBufferBeginResult,
                        "failed to begin recording command buffer!"
                );
        }

        const VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
        VkRenderPassBeginInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = app->renderPass,
                .framebuffer = app->swapchainFramebuffers[imageIndex],
                .renderArea = {
                        .offset = { .x = 0, .y = 0 },
                        .extent = app->swapchainExtent,
                },
                .clearValueCount = 1,
                .pClearValues = &clearColor,
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline);

        const VkViewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = (float) app->swapchainExtent.width,
                .height = (float) app->swapchainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        const VkRect2D scissor = {
                .offset = { .x = 0, .y = 0 },
                .extent = app->swapchainExtent,
        };

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        const VkResult cmdBufResult = vkEndCommandBuffer(commandBuffer);
        if (cmdBufResult != VK_SUCCESS)
                return RESULT_ERROR(cmdBufResult, "failed to record command buffer!");
        
        return RESULT_SUCCESS;
}

static const Result createSyncObjects(App *app)
{
        app->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
        app->renderFinishedSemaphores= malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
        app->inFlightFences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                const VkResult imageAvailableSemaphoreResult = vkCreateSemaphore(
                        app->device,
                        &semaphoreInfo,
                        NULL,
                        &app->imageAvailableSemaphores[i]
                );
                const VkResult renderFinishedSemaphoreResult = vkCreateSemaphore(
                        app->device,
                        &semaphoreInfo,
                        NULL,
                        &app->renderFinishedSemaphores[i]
                );
                const VkResult inFlightFenceResult = vkCreateFence(
                        app->device,
                        &fenceInfo,
                        NULL,
                        &app->inFlightFences[i]
                );

                if (imageAvailableSemaphoreResult != VK_SUCCESS
                        && renderFinishedSemaphoreResult != VK_SUCCESS
                        && inFlightFenceResult != VK_SUCCESS
                ) {
                        return RESULT_ERROR(
                                inFlightFenceResult,
                                "failed to create semaphores and fence!"
                        );
                }
        }

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
        handle(createSwapChain(app));
        handle(createImageViews(app));
        handle(createRenderPass(app));
        handle(createGraphicsPipeline(app));
        handle(createFramebuffers(app));
        handle(createCommandPool(app));
        handle(createCommandBuffers(app));
        handle(createSyncObjects(app));
        return RESULT_SUCCESS;
}

static const Result drawFrame(App *app, uint32_t *pCurrentFrame)
{
        vkWaitForFences(app->device, 1, &app->inFlightFences[*pCurrentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(app->device, 1, &app->inFlightFences[*pCurrentFrame]);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(
                app->device,
                app->swapchain,
                UINT64_MAX,
                app->imageAvailableSemaphores[*pCurrentFrame],
                NULL,
                &imageIndex
        );

        vkResetCommandBuffer(app->commandBuffers[*pCurrentFrame], 0);
        recordCommandBuffer(app, app->commandBuffers[*pCurrentFrame], imageIndex);

        const VkSemaphore waitSemaphores[] = { app->imageAvailableSemaphores[*pCurrentFrame] };
        const VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };

        const VkSemaphore signalSemaphores[] = { app->renderFinishedSemaphores[*pCurrentFrame] };

        const VkSubmitInfo submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = waitSemaphores,
                .pWaitDstStageMask = waitStages,
                .commandBufferCount = 1,
                .pCommandBuffers = &app->commandBuffers[*pCurrentFrame],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = signalSemaphores,
        };

        const VkResult submitResult = vkQueueSubmit(
                app->graphicsQueue,
                1,
                &submitInfo,
                app->inFlightFences[*pCurrentFrame]
        );

        if (submitResult != VK_SUCCESS)
                return RESULT_ERROR(submitResult, "failed to submit draw command buffer!");

        const VkSwapchainKHR swapchains[] = { app->swapchain };
        const VkPresentInfoKHR presentInfo = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = signalSemaphores,
                .swapchainCount = 1,
                .pSwapchains = swapchains,
                .pImageIndices = &imageIndex,
                .pResults = NULL, // optional
        };

        vkQueuePresentKHR(app->presentQueue, &presentInfo);

        *pCurrentFrame = (*pCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return RESULT_SUCCESS;
}

static const Result mainLoop(App *app)
{
        while (!glfwWindowShouldClose(app->window)) {
                glfwPollEvents();
                static uint32_t currentFrame = 0;
                drawFrame(app, &currentFrame);
        }

        vkDeviceWaitIdle(app->device);
        return RESULT_SUCCESS;
}

static const Result cleanUp(App *app)
{
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(app->device, app->imageAvailableSemaphores[i], NULL);
                vkDestroySemaphore(app->device, app->renderFinishedSemaphores[i], NULL);
                vkDestroyFence(app->device, app->inFlightFences[i], NULL);
        }

        free(app->imageAvailableSemaphores);
        free(app->renderFinishedSemaphores);
        free(app->inFlightFences);

        vkDestroyCommandPool(app->device, app->commandPool, NULL);

        for (int i = 0; i < app->swapchainImageCount; i++)
                vkDestroyFramebuffer(app->device, app->swapchainFramebuffers[i], NULL);

        free(app->swapchainFramebuffers);

        vkDestroyPipeline(app->device, app->graphicsPipeline, NULL);
        vkDestroyPipelineLayout(app->device, app->pipelineLayout, NULL);
        vkDestroyRenderPass(app->device, app->renderPass, NULL);

        for (int i = 0; i < app->swapchainImageCount; i++)
                vkDestroyImageView(app->device, app->swapchainImageViews[i], NULL);

        free(app->swapchainImages);

        vkDestroySwapchainKHR(app->device, app->swapchain, NULL);

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
