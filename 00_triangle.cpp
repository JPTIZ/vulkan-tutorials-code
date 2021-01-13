#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

struct Size {
    int width;
    int height;
};

std::string vk_result_error_message(VkResult errorCode)
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

// Validation Layers
#ifdef NDEBUG
constexpr auto enable_validation_layers = false;
#else
constexpr auto enable_validation_layers = true;
#endif

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation",
};

auto check_validation_layer_support() {
    auto layer_count = uint32_t{0};
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    auto available_layers = std::vector<VkLayerProperties>(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const auto& layer_name : validation_layers) {
        auto found = false;

        for (const auto& layer_properties : available_layers) {
            if (std::string{layer_name} == layer_properties.layerName) {
                found = true;
                break;
            }
        }

        if (not found) {
            return false;
        }
    }

    return true;
}

// Message callbacks
auto get_required_extensions() {
    auto glfw_extension_count = uint32_t{0};
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    auto extensions = std::vector<const char*>(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static VKAPI_ATTR auto VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* data
) -> VkBool32 {
    std::cerr << "Validation layer: " << callback_data->pMessage << std::endl;

    return VK_FALSE;
}

template <typename FunctionType>
auto load_vk_function(VkInstance instance, const std::string& name) -> FunctionType {
    auto void_function = vkGetInstanceProcAddr(instance, name.c_str());
    return reinterpret_cast<FunctionType>(void_function);
}

auto create_debug_utils_messenger_ext(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* messenger
) {
    auto function = load_vk_function<PFN_vkCreateDebugUtilsMessengerEXT>(instance, "vkCreateDebugUtilsMessengerEXT");

    if (function) {
        return function(instance, create_info, allocator, messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

auto destroy_debug_utils_messenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* allocator
) {
    auto function = load_vk_function<PFN_vkDestroyDebugUtilsMessengerEXT>(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (function) {
        function(instance, debug_messenger, nullptr);
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
        create_instance();
        setup_debug_messenger();
    }

    void create_instance() {
        if (enable_validation_layers and not check_validation_layer_support()) {
            throw std::runtime_error("Validation layers requested but not supported");
        }

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

        if (enable_validation_layers) {
            auto extensions = get_required_extensions();
            instance_info.enabledLayerCount = static_cast<uint32_t>(extensions.size());
            instance_info.ppEnabledExtensionNames = extensions.data();
        }

        auto glfw_extension_count = uint32_t{0};
        const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        instance_info.enabledExtensionCount = glfw_extension_count;
        instance_info.ppEnabledExtensionNames = glfw_extensions;

        instance_info.enabledLayerCount = 0;

        if (auto result = vkCreateInstance(&instance_info, nullptr, &instance)) {
            throw std::runtime_error("Failed to create vulkan instance: " + vk_result_error_message(result));
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

    void setup_debug_messenger() {
        if (not enable_validation_layers) {
            return;
        }

        auto create_info = VkDebugUtilsMessengerCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
            .pUserData = nullptr,
        };

        if (auto r = create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger);
            r != VK_SUCCESS) {
            auto msg = vk_result_error_message(r);
            throw std::runtime_error(std::string{"Failed to setup debug messenger: "} + msg);
        }
    }

    void main_loop() {
        while (not glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        if (enable_validation_layers) {
            destroy_debug_utils_messenger(instance, debug_messenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    constexpr static auto DEFAULT_SIZE = Size{800, 600};

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
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
