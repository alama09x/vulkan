#include <cglm/common.h>
#include <cglm/ivec3.h>
#include <cglm/simd/sse2/mat3.h>
#include <cglm/types.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/call.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

        uint32_t extCount = 0;

        vkEnumerateInstanceExtensionProperties(NULL, &extCount, NULL);
        printf("%d extensions supported\n", extCount);

        mat4 matrix;
        vec4 vec;
        glmc_translate(matrix, vec);

        while (!glfwWindowShouldClose(window))
                glfwPollEvents();

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_SUCCESS;
}
