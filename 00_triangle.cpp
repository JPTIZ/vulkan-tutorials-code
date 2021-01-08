#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

struct Size {
    int width;
    int height;
};

std::string errorString(VkResult errorCode)
{
    switch (errorCode)
    {
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
        return "UNKNOWN_ERROR";
    }
}

class HelloTriangleApp {
public:
    void run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:
    void init_window() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(DEFAULT_SIZE.width,
                                  DEFAULT_SIZE.height,
                                  "Vulkan",
                                  nullptr,
                                  nullptr);
    }

    void init_vulkan() {
        createInstance();
    }

    void createInstance() {
        auto app_info = VkApplicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = VK_API_VERSION_1_0,
        };

        auto instance_info = VkInstanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
        };

        auto glfw_extension_count = uint32_t{0};
        const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        instance_info.enabledExtensionCount = glfw_extension_count;
        instance_info.ppEnabledExtensionNames = glfw_extensions;

        instance_info.enabledLayerCount = 0;

        if (auto result = vkCreateInstance(&instance_info, nullptr, &instance)) {
            throw std::runtime_error("Failed to create vulkan instance: " + errorString(result));
        }

        auto extension_count = uint32_t{0};
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        auto extensions = std::vector<VkExtensionProperties>(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

        std::cout << "Available extensions:\n";
        for (const auto& extension: extensions) {
            std::cout << "    :: " << extension.extensionName << '\n';
        }
    }

    void main_loop() {
        while (not glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    constexpr static auto DEFAULT_SIZE = Size{800, 600};

    GLFWwindow* window;
    VkInstance instance;
};


int main() {
    auto app = HelloTriangleApp{};

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
