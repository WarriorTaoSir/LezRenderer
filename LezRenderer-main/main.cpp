#define GLFW_INCLUDE_VULKAN // 让GLFW自动包含Vulkan头文件
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>

// 窗口尺寸常量
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// 启用的验证层列表（Khronos官方验证层）
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// 根据编译模式决定是否启用验证层
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// 代理函数：动态加载并调用vkCreateDebugUtilsMessengerEXT
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // 通过vkGetInstanceProcAddr获取函数指针
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // 如果成功获取函数指针，调用实际函数
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        // 扩展未启用或驱动不支持
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// 代理函数：动态加载并调用vkDestroyDebugUtilsMessengerEXT
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// 用于存储物理设备支持的队列家族索引
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // 图形队列家族索引（可能不存在）

    // 检查所有必需的队列家族是否已找到
    bool isComplete() {
        return graphicsFamily.has_value(); // 当前仅需图形队列即可视为“完整”
    }
};

// 主应用程序类
class HelloTriangleApplication {
public:
    void run() {
        initWindow();   // 初始化GLFW窗口
        initVulkan();   // 初始化Vulkan资源
        mainLoop();     // 主循环处理事件
        cleanup();      // 清理资源
    }

private:
    // 初始化GLFW窗口
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 不使用OpenGL上下文
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // 禁用窗口缩放
        // the window(width, height, title, screenIndex
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    // 初始化Vulkan核心组件
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    // 主事件循环
    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // 释放资源
    void cleanup() {
        vkDestroyDevice(device, nullptr);           // 销毁逻辑设备

        // 如果启用了验证层
        if (enableValidationLayers) {
            // 摧毁创建的调试Messenger
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);   // 先销毁调试信使
        }

        vkDestroyInstance(instance, nullptr);       // 销毁Vulkan实例

        glfwDestroyWindow(window);                  // 销毁窗口

        glfwTerminate();                            // 终止GLFW
    }

    // 创建Vulkan实例
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        // 应用信息配置（非必需但建议提供）
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // 明确该结构体所存储的信息类型
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // 实例创建配置
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // 获取所需扩展（窗口系统扩展 + 调试扩展）
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // 调试Messenger创建信息
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        // 如果启用了验证层
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // 填充调试信使的配置（消息级别、回调函数等）
            populateDebugMessengerCreateInfo(debugCreateInfo);

            // 将调试配置链接到实例创建信息中
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;  // 原代码强制转换不必要，直接取地址即可
        }
        else {
            // 不启用调试时，清空相关配置
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // 创建Vulkan实例
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // 填充调试回调配置
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // 接收错误、警告和冗余信息（调试时可根据需要调整）
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // 接收验证和性能相关的消息
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    // 配置调试Messenger
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // 检查所有验证层是否收到支持
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // 检查ValidationLayers是否都在availableLayers里，如果不在，那么有的验证层就不支持
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    // 获取实例所需的扩展列表
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // 获取GLFW所需的扩展（如窗口系统集成）
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // 调试模式下添加调试扩展
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    // 选择物理设备
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        // 没有Vulkan支持的设备
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // 初始化设备个数
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());


        // 遍历设备
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        // 选择合适的设备
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // 创建逻辑设备
    void createLogicalDevice() {
        // 获取物理设备支持的队列家族索引（例如图形队列家族）
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // 配置队列创建信息（这里创建图形队列）
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        // 设置队列优先级（范围0.0~1.0，影响GPU资源分配）
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // 初始化设备功能需求（此处未启用任何特殊功能）
        VkPhysicalDeviceFeatures deviceFeatures{};

        // 配置逻辑设备创建信息
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;

        // 创建逻辑设备（与物理设备关联的软件抽象）
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        // 获取设备中创建的图形队列句柄（后续用于提交绘图命令）
        // 参数说明：设备对象, 队列家族索引, 队列索引（同一家族中可能有多个队列），输出队列句柄
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }

    // 计算设备合适度——自定义选择物理设备的过程
    int rateDeviceSuitability(VkPhysicalDevice device) {
        // 获取设备的硬件属性（如名称、类型、驱动版本等）
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        // 获取设备支持的图形功能（如几何着色器、多重采样等）
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        // 规则1：独立显卡（性能更强）加1000分
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // 规则2：支持的2D纹理最大尺寸越大，得分越高（影响贴图分辨率）
        score += deviceProperties.limits.maxImageDimension2D;

        // 规则3：必须支持几何着色器，否则直接0分淘汰
        if (!deviceFeatures.geometryShader) {
            return 0;
        }

        return score;
    }

    // 检查设备是否满足最低要求（队列家族是否完整）
    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device); // 查找队列家族

        return indices.isComplete(); // 是否找齐了所有必须的队列（目前只需图形队列）
    }

    // 遍历设备的所有队列家族，记录支持的队列索引
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        
        uint32_t queueFamilyCount = 0;
        // 获取队列家族数量
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        // 获取所有队列家族的属性
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        // 遍历每个队列家族，检查是否支持图形操作
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
                if (indices.isComplete()) {
                    break;
                }
            }
            i++;
        }

        return indices;
    }

    // 调试回调函数（静态成员函数）
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // 消息严重程度
        VkDebugUtilsMessageTypeFlagsEXT messageType,                // 消息类型
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  // 具体信息内容
        void* pUserData) {                                          // 用户自定义数据
        // 将错误信息输出到标准错误流
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        // 返回VK_FALSE表示“不终止程序运行”
        return VK_FALSE;
    }
private:
    GLFWwindow* window;                                 // GLFW窗口句柄
    VkInstance instance;                                // Vulkan实例
    VkDebugUtilsMessengerEXT debugMessenger;            // 调试回调信使
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // 物理设备句柄
    VkDevice device;                                    // 逻辑设备句柄
    VkQueue graphicsQueue;                              // 图形队列
};

// 程序入口
int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl; // catch the runtime error
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}