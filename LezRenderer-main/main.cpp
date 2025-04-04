#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN // 让GLFW自动包含Vulkan头文件
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLM_FORCE_RADIANS                   // 统一使用radiance
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>                      // 
#include <glm/gtc/matrix_transform.hpp>     // 包含模型变换等函数

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint>      // Necessary for uint32_t
#include <limits>       // Necessary for std::numeric_limits
#include <algorithm>    // Necessary for std::clamp
#include <fstream>

#include <glm/glm.hpp>
#include <array>

#include <chrono>       // 公开了进行精准计时的函数。

// 窗口尺寸常量
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

// 启用的验证层列表（Khronos官方验证层）
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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
    std::optional<uint32_t> presentFamily;  // 展示队列家族索引（可能不存在）

    // 检查所有必需的队列家族是否已找到
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value(); // 当前仅需图形队列即可视为“完整”
    }
};

// 交换链支持细节
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// 顶点数据格式定义（包含位置和颜色）
struct Vertex {
    glm::vec2 pos;      // 位置属性（2D坐标，对应顶点着色器的 location=0）
    glm::vec3 color;    // 颜色属性（RGB，对应顶点着色器的 location=1）

    // 生成顶点数据绑定描述（Vulkan需要此信息读取内存中的顶点数据）
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;                                 // 绑定索引（从0开始，单缓冲时通常为0）
        bindingDescription.stride = sizeof(Vertex);                     // 每个顶点数据的总字节大小（自动计算）
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;     // 按顶点步进（非实例化）

        return bindingDescription;
    }

    // 生成顶点属性描述（定义每个属性的格式和位置）
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        // 位置属性（pos）的描述
        attributeDescriptions[0].binding = 0;                           // 绑定索引（与绑定描述一致）
        attributeDescriptions[0].location = 0;                          // 顶点着色器的输入位置（location=0）
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;      // 数据格式（vec2）2个32位浮点数
        attributeDescriptions[0].offset = offsetof(Vertex, pos);        // 偏移量（pos在结构体中的起始位置）

        // 描述颜色属性（color）
        attributeDescriptions[1].binding = 0;                           // 同一绑定（数据来自同一缓冲区）
        attributeDescriptions[1].location = 1;                          // 顶点着色器的输入位置（location=1）
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // 数据格式（vec3）3个32位浮点数
        attributeDescriptions[1].offset = offsetof(Vertex, color);      // 偏移量（color的起始位置）

        return attributeDescriptions;
    }
};

// 顶点属性数组
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

// 顶点索引数组
// uint16_t适合少于65535顶点数的模型
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

// 全局变量
struct UniformBufferObject {

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
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
    GLFWwindow* window;                                 // GLFW窗口句柄
    VkInstance instance;                                // Vulkan实例
    VkDebugUtilsMessengerEXT debugMessenger;            // 调试回调信使
    VkSurfaceKHR surface;                               // 窗口表面 
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // 物理设备句柄
    VkDevice device;                                    // 逻辑设备句柄

    VkQueue graphicsQueue;                              // 图形队列
    VkQueue presentQueue;                               // 展示队列
    VkSwapchainKHR swapChain;                           // 交换链
    std::vector<VkImage> swapChainImages;               // 交换链图片

    VkFormat swapChainImageFormat;                      // 像素格式
    VkExtent2D swapChainExtent;                         // 分辨率
    std::vector<VkImageView> swapChainImageViews;       // 交换链的图像视图
    VkRenderPass renderPass;                            // 渲染通道

    VkDescriptorSetLayout descriptorSetLayout;          // 描述符集布局
    VkPipelineLayout pipelineLayout;                    // 管线布局
    VkPipeline graphicsPipeline;                        // 图形管线
    std::vector<VkFramebuffer> swapChainFramebuffers;   // 交换链帧缓冲
    VkCommandPool commandPool;                          // 命令缓冲池
    std::vector<VkCommandBuffer> commandBuffers;        // 命令缓冲

    // 同步相关
    std::vector<VkSemaphore> imageAvailableSemaphores;  // 用于判断是否可以从交换链中获取图像
    std::vector<VkSemaphore> renderFinishedSemaphores;  // 用于判断这一帧渲染是否完成
    std::vector<VkFence> inFlightFences;                // 确保一次只渲染一帧画面

    bool framebufferResized = false;                    // 帧缓冲是否被更改大小
    uint32_t currentFrame = 0;

    // Vulkan缓冲句柄
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;

    // Vulkan缓冲对应的设备内存
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;

    // UBO
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    VkDescriptorPool descriptorPool;                    // 描述符池
    std::vector<VkDescriptorSet> descriptorSets;        // 描述符集 数组

private:
    // 初始化GLFW窗口
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 不使用OpenGL上下文

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    // 初始化Vulkan核心组件
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    // 主事件循环
    void mainLoop() {

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        // 等待所有异步操作结束后，再结束主循环
        vkDeviceWaitIdle(device);
    }

    // 释放资源
    void cleanup() {
        cleanupSwapChain();

        // 销毁UBO
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        // 销毁描述符集池
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        // 销毁描述符集布局
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        // 销毁索引Buffer以及回收对印内存
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        // 销毁顶点Buffer以及回收对印内存
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        // 销毁图形管线
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        // 销毁管线布局
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        // 销毁渲染通道
        vkDestroyRenderPass(device, renderPass, nullptr);

        // 销毁信号量
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr); // 销毁命令缓冲池

        vkDestroyDevice(device, nullptr);                   // 销毁逻辑设备

        // 如果启用了验证层
        if (enableValidationLayers) {
            // 摧毁创建的调试Messenger
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);   // 先销毁调试信使
        }

        vkDestroySurfaceKHR(instance, surface, nullptr); // 销毁窗口表面

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
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;  // 原代码强制转换不必要，直接取地址即可
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

    // 创建窗口表面
    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
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

    // 创建Vulkan逻辑设备（Logical Device），配置所需队列家族，并获取图形和呈现队列句柄，为后续渲染和显示操作提供接口
    void createLogicalDevice() {
        // 获取物理设备支持的队列家族索引（图形和展示队列）
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // 配置队列创建信息（这里创建图形队列）
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        // 使用set自动去重，避免重复创建同一队列家族的多个队列
        std::set<uint32_t> uniqueQueueFamilies =
        {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        // 为每个队列家族分配优先级并创建配置
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // 初始化设备功能需求（此处未启用任何特殊功能）
        VkPhysicalDeviceFeatures deviceFeatures{};

        // 配置逻辑设备创建信息
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        // 启用设备扩展（如交换链扩展）
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // 启用验证层（调试用）
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }


        // 创建逻辑设备（与物理设备关联的软件抽象）
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // 获取图形队列句柄（用于提交渲染命令）
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        // 获取呈现队列句柄（用于显示图像到窗口）
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    // 创建Vulkan交换链（Swapchain），管理渲染图像队列以实现屏幕显示
    void createSwapChain() {
        // 获取物理设备对交换链的支持详情（能力、格式、呈现模式）
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // 选择最佳交换链配置参数
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // 计算交换链图像数量（在设备支持的范围内）
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // 最小值 + 1
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount; // 截断
        }

        // 配置交换链创建参数
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; // 结构体类型标识
        createInfo.surface = surface; // 关联的窗口表面（由GLFW或其他窗口库创建）

        // 图像参数配置
        createInfo.minImageCount = imageCount;                      // 交换链图像数量
        createInfo.imageFormat = surfaceFormat.format;              // 像素格式（如VK_FORMAT_B8G8R8A8_SRGB）
        createInfo.imageColorSpace = surfaceFormat.colorSpace;      // 颜色空间（如VK_COLOR_SPACE_SRGB_NONLINEAR_KHR）
        createInfo.imageExtent = extent;                            // 图像分辨率（宽高）
        createInfo.imageArrayLayers = 1;                            // 图像层数（3D应用可能需要多层，此处设为1）
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// 图像用途：作为颜色附件（渲染目标）

        // 处理队列家族共享模式（图形队列 vs 呈现队列）
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        // 如果图形队列和呈现队列属于不同家族，需配置为并发共享模式
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   // 并发模式：多队列家族共享图像
            createInfo.queueFamilyIndexCount = 2;                       // 参与的队列家族数量
            createInfo.pQueueFamilyIndices = queueFamilyIndices;        // 队列家族索引数组
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;    // 独占模式：单队列家族独占访问（性能更优）
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        // 配置图像变换和透明度（默认无变换且不透明）
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // 设置呈现模式和裁剪行为
        createInfo.presentMode = presentMode;       // 选择的呈现模式（如VK_PRESENT_MODE_MAILBOX_KHR）
        createInfo.clipped = VK_TRUE;               // 启用裁剪：忽略被遮挡的像素（优化性能）
        createInfo.oldSwapchain = VK_NULL_HANDLE;   // 旧交换链句柄（重建交换链时可用于优化）

        // 创建交换链对象
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");   // 失败时抛出异常
        }

        // 获取交换链中的图像列表（拿到所有“画布”）
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);                   // 先问有多少张图
        swapChainImages.resize(imageCount);                                                 // 准备好容器
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());    // 拿到所有图的句柄

        // 保存配置参数(后续渲染要用)
        swapChainImageFormat = surfaceFormat.format;    // 记住像素格式
        swapChainExtent = extent;                       // 记住分辨率
    }

    // 为交换链中的每个图像创建图像视图（ImageView），以便在渲染管线中访问图像数据
    void createImageViews() {
        // 根据交换链中的图像数量，调整图像视图容器的大小
        swapChainImageViews.resize(swapChainImages.size());

        // 遍历交换链中的每个图像，为其创建对应的图像视图
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            // 配置图像视图的创建参数
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;    // 结构体类型标识
            createInfo.image = swapChainImages[i];                          // 绑定到交换链中的第i个图像

            // 定义图像视图的类型和格式
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    // 视图类型：2D纹理（也支持1D、3D、CubeMap等）
            createInfo.format = swapChainImageFormat;       // 图像格式（需与交换链的格式一致）


            // 配置颜色通道映射（此处保持默认RGBA顺序）
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // 配置子资源范围（定义视图能访问图像的哪些部分）
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 视图用于颜色数据
            createInfo.subresourceRange.baseMipLevel = 0;                       // 起始Mipmap级别（0表示基础级别）
            createInfo.subresourceRange.levelCount = 1;                         // Mipmap层级数量（此处仅1级，无多级渐远）
            createInfo.subresourceRange.baseArrayLayer = 0;                     // 起始数组层（0表示基础层）
            createInfo.subresourceRange.layerCount = 1;                         // 数组层数量（此处仅1层，非纹理数组）

            // 创建图像视图对象
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    // 创建渲染通道
    void createRenderPass() {
        // 配置颜色附件（Color Attachment）
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;          // 需要与交换链中的图像格式匹配
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;        // 暂时没用到多重采样 1sample就行

        // 定义颜色附件的加载/存储操作
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // 渲染前清空
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // 渲染后保存

        // 定义模板附件的加载/存储操作（当前未使用模板，设为DONT_CARE）
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // 定义渲染开始和结束时附件的布局
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // 不保留内容化
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // 用于显示

        // 配置颜色附件的引用（用于子过程）
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 配置子过程（Subpass）
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // 指定为图形管线（非计算管线）
        subpass.colorAttachmentCount = 1;                               // 使用的颜色附件数量
        subpass.pColorAttachments = &colorAttachmentRef;


        // 依赖关系的作用解释：
        // 1. 确保渲染通道开始前等待图像可用（通过 imageAvailableSemaphore）
        // 2. 控制图像布局从 UNDEFINED 自动转换为 COLOR_ATTACHMENT_OPTIMAL 的时机
        // 3. 防止在图像未准备好时进行渲染操作（数据竞争）

        // 创建渲染通道信息结构体
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                // 源：渲染通道开始前的隐式操作（如交换链获取图像）
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // 目标：第一个（唯一）子流程（实际渲染操作）
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // 创建渲染通道信息结构体
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;   // 结构体类型标识
        renderPassInfo.attachmentCount = 1;                                 // 附件数量（当前仅颜色附件）
        renderPassInfo.pAttachments = &colorAttachment;                     // 指向附件描述数组
        renderPassInfo.subpassCount = 1;                                    // 子过程数量（当前仅1个）
        renderPassInfo.pSubpasses = &subpass;                               // 指向子过程描述数组
        renderPassInfo.dependencyCount = 1;                                 // 依赖关系数量
        renderPassInfo.pDependencies = &dependency;                         // 指向依赖关系数组

        // 创建渲染通道
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    // 创建描述符 集 布局
    void createDescriptorSetLayout() {
        // 描述符绑定配置（对应着色器中的 binding=0）
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;                       // 绑定位置（与着色器中的 layout(binding=0) 对应）
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // 类型：UBO
        uboLayoutBinding.descriptorCount = 1;               // UBO 数量（此处为单个，也可用于数组）
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   // 生效的着色器阶段（顶点着色器）
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // 创建描述符集布局 createInfo
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        // 创建描述符集布局
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
    }

    // 创建图形管线
    void createGraphicsPipeline() {
        // 1. 读取预编译的顶点和片段着色器字节码（SPIR-V格式）
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        // 2. 创建着色器模块（用于封装SPIR-V代码）
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // 3. 配置顶点着色器阶段信息
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;                             // 指定为顶点着色器阶段
        vertShaderStageInfo.module = vertShaderModule;                                      // 关联顶点着色器模块
        vertShaderStageInfo.pName = "main";

        // 4. 配置片元着色器阶段信息
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;                           // 指定为片段着色器阶段
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // 5. 组合所有着色器阶段（顺序必须与渲染流程一致）
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // 6. 配置顶点输入描述（根据实际顶点数据结构修改）
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // 获取顶点数据的绑定描述和属性描述
        auto bindingDescription = Vertex::getBindingDescription();          // 绑定描述（数据布局）
        auto attributeDescriptions = Vertex::getAttributeDescriptions();    // 属性描述（每个字段的细节）

        // 配置顶点输入状态
        vertexInputInfo.vertexBindingDescriptionCount = 1;                                  // 绑定描述的数量（单缓冲区）
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());  // 属性数量（此处为2）
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;                   // 绑定描述数组指针
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();        // 属性描述数组指针

        // 7. 配置输入装配方式（三角形列表）
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;                       // 三角形列表模式
        inputAssembly.primitiveRestartEnable = VK_FALSE;                                    // 禁用图元重启（用于索引缓冲）

        // 8. 配置视口和裁剪矩形（使用动态状态，需在渲染时设置）
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 9. 配置光栅化参数
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 10. 配置多重采样（当前禁用）
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;                                       // 禁用采样着色（抗锯齿）
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;                         // 采样数=1

        // 11. 配置颜色混合附件（单附件，当前禁用混合）
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        // 12. 配置全局颜色混合状态
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 13. 配置动态状态（允许运行时修改视口和裁剪）
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 14. 创建管线布局（当前无描述符集或推送常量）
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // 15. 组装图形管线创建信息
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;                            // 着色器阶段数量（顶点+片段）
        pipelineInfo.pStages = shaderStages;                    // 指向着色器阶段数组

        // 绑定各阶段状态配置
        pipelineInfo.pVertexInputState = &vertexInputInfo;      // 顶点输入配置
        pipelineInfo.pInputAssemblyState = &inputAssembly;      // 输入装配配置
        pipelineInfo.pViewportState = &viewportState;           // 视口状态
        pipelineInfo.pRasterizationState = &rasterizer;         // 光栅化配置
        pipelineInfo.pMultisampleState = &multisampling;        // 多重采样配置
        pipelineInfo.pDepthStencilState = nullptr;              // 深度/模板测试配置（可选）
        pipelineInfo.pColorBlendState = &colorBlending;         // 颜色混合配置
        pipelineInfo.pDynamicState = &dynamicState;             // 动态状态配置

        // 关联管线布局和渲染通道
        pipelineInfo.layout = pipelineLayout;                   // 管线布局对象
        pipelineInfo.renderPass = renderPass;                   // 渲染通道对象
        pipelineInfo.subpass = 0;                               // 使用的子过程索引

        // 管线继承配置（用于优化管线创建性能）
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;       // 基础管线句柄（无继承）
        pipelineInfo.basePipelineIndex = -1;                    // 基础管线索引（无继承）

        // 16. 创建图形渲染管线
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // 17. 销毁着色器模块（应在管线创建成功后执行）
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    // 创建帧缓冲
    void createFramebuffers() {
        // 根据交换链图像个数来重新决定帧缓冲个数
        swapChainFramebuffers.resize(swapChainImageViews.size());

        // 依次创建Frame buffer
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    // 创建命令缓冲池（用于分配和管理命令缓冲区）
    void createCommandPool() {
        // 获取物理设备的队列家族索引（确定图形队列家族）
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        // 配置命令池创建参数
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;            // 结构体类型标识
        // 允许单个命令缓冲区被重置（无需重置整个池）
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        // 指定关联的图形队列家族（命令缓冲区将提交到此队列）
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        // 创建命令池
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    // 创建Buffer的 工具函数
    // 参数说明：缓冲区大小（字节）、缓冲区用途（顶点缓冲、传输缓冲）、内存属性、输出的缓冲区对象、输出的设备内存
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;         // 独占访问（仅一个队列家族使用）

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);    // 查询内存需求

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);                // 绑定内存到缓冲区
    }

    // 创建顶点缓冲区（用来存储顶点数据，如位置、颜色等）
    void createVertexBuffer() {
        // 计算顶点数据总字节数 = 单个顶点大小 × 顶点数量
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // 1. 创建暂存缓冲区（Staging Buffer）
        // -----------------------------------------------
        // - 作用：临时存储CPU端数据，用于向GPU高效内存传输数据
        // - 内存属性：CPU可见（HOST_VISIBLE） + 自动同步（HOST_COHERENT）
        // - 用途：作为传输源（TRANSFER_SRC），后续将数据复制到设备本地缓冲区
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        // 2. 将顶点数据从CPU复制到暂存缓冲区
        // -----------------------------------------------
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);    // 映射内存，让CPU能访问（类似“锁定”内存）
        memcpy(data, vertices.data(), (size_t)bufferSize);                   // 复制顶点数据到映射的内存
        vkUnmapMemory(device, stagingBufferMemory);                                                      // 解除映射（可选立即让CPU不可见，但因为COHERENT，数据已自动同步）

        // 3. 创建设备本地顶点缓冲区（最终存储位置）
        // -----------------------------------------------
        // - 作用：存储最终顶点数据，供GPU高效访问
        // - 内存属性：设备本地（DEVICE_LOCAL），通常位于显存，CPU不可见
        // - 用途：作为传输目标（TRANSFER_DST） + 顶点数据（VERTEX_BUFFER）
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,   // 用途：数据传输目标 + 顶点数据
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                                    // GPU高效内存
            vertexBuffer,
            vertexBufferMemory
        );

        // 4. 将数据从暂存缓冲区复制到设备本地缓冲区
        // -----------------------------------------------
        // - 使用GPU命令（vkCmdCopyBuffer）在设备内部完成高效传输
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // 5. 销毁暂存资源（数据已转移，不再需要）
        // -----------------------------------------------
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

    }

    // 创建索引缓冲区（用于存储顶点索引数据，描述顶点连接顺序）
    void createIndexBuffer() {
        // 计算索引数据总字节数 = 单个索引大小 × 索引数量
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        // 1. 创建暂存缓冲区（Staging Buffer）
        // -----------------------------------------------
        // - 作用：临时存储CPU端索引数据，用于中转至GPU高效内存
        // - 内存属性：CPU可见（HOST_VISIBLE） + 自动同步（HOST_COHERENT）
        // - 用途：数据传输源（TRANSFER_SRC）
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // 2. 将索引数据从CPU复制到暂存缓冲区
        // -----------------------------------------------
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        // 3. 创建设备本地索引缓冲区（最终存储位置）
        // -----------------------------------------------
        // - 作用：存储最终索引数据，供GPU高效访问
        // - 内存属性：设备本地（DEVICE_LOCAL），通常位于显存
        // - 用途：数据传输目标（TRANSFER_DST） + 索引数据（INDEX_BUFFER）
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |      // 用途：数据传输目标
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,       // 用途：索引缓冲区
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // GPU高效内存
            indexBuffer,
            indexBufferMemory
        );

        // 4. 将数据从暂存缓冲区复制到设备本地缓冲区
        // -----------------------------------------------
        // - 内部使用vkCmdCopyBuffer命令，由GPU执行异步高速复制
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        // 5. 销毁暂存资源（数据已转移，不再需要）
// -----------------------------------------------
        vkDestroyBuffer(device, stagingBuffer, nullptr);        // 销毁暂存缓冲区对象
        vkFreeMemory(device, stagingBufferMemory, nullptr);     // 释放暂存内存
    }

    // 创建Uniform Buffer
    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    // 创建描述符 池
    void createDescriptorPool() {
        // 定义池中描述符的类型和数量
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 类型：Uniform Buffer
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // 数量：每帧一个

        // 创建描述符池的配置信息
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;                     // 池类型数量（此处只有一种类型）
        poolInfo.pPoolSizes = &poolSize;                // 指向类型配置的指针
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // 最大可分配的描述符集数量
    
        // 创建描述符池
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    // 创建描述符 集
    void createDescriptorSets() {
        // 为每帧分配相同布局的描述符集
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;              // 从哪个池分配
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // 分配数量（每帧一个）
        allocInfo.pSetLayouts = layouts.data();                 // 描述符集布局数组

        // 分配描述符集（每个描述符集对应一帧）
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        // 配置每个描述符集（绑定Uniform Buffer）
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // 描述符引用的Uniform Buffer信息
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];          // 当前帧的Uniform Buffer
            bufferInfo.offset = 0;                          // 数据起始偏移量
            bufferInfo.range = sizeof(UniformBufferObject); // 数据总大小（或VK_WHOLE_SIZE）

            // 构造描述符写入操作
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];     // 目标描述符集（当前帧）
            descriptorWrite.dstBinding = 0;                 // 绑定点（与着色器中的binding=0对应）
            descriptorWrite.dstArrayElement = 0;            // 数组索引（非数组时为0）
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 类型：UBO
            descriptorWrite.descriptorCount = 1;            // 绑定的描述符数量（单个）
            descriptorWrite.pBufferInfo = &bufferInfo;      // 缓冲区信息指针

            //descriptorWrite.pImageInfo = nullptr;           // Optional
            //descriptorWrite.pTexelBufferView = nullptr;     // Optional

            // 更新描述符集（将配置提交到GPU）
            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    // 创建命令缓冲区（用于记录并提交渲染指令到GPU）
    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        // 配置命令缓冲区分配参数
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;   // 结构体类型标识

        // 指定从哪个命令池中分配缓冲区（需提前创建）
        allocInfo.commandPool = commandPool;

        // 指定为"主命令缓冲区"（可直接提交到队列执行，次级缓冲区需依附于主缓冲区）
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size(); // 分配数量（当前仅需2个，若多帧并行处理可扩展为多个）

        // 从命令池中分配命令缓冲区
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    // 复制缓冲区数据（从源缓冲区到目标缓冲区）
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        // 1. 分配命令缓冲区
        // -----------------------------------------------
        // - 作用：用于录制并提交缓冲区复制命令
        // - 级别：主命令缓冲区（VK_COMMAND_BUFFER_LEVEL_PRIMARY，可直接提交到队列）
        // - 命令池：指定分配命令缓冲区的池（需与目标队列家族兼容）
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;                  // 主命令缓冲区（独立使用）
        allocInfo.commandPool = commandPool;                                // 关联的命令池
        allocInfo.commandBufferCount = 1;                                   // 分配1个命令缓冲区

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);       // 分配命令缓冲区

        // 2. 开始录制命令
        // -----------------------------------------------
        // - 作用：准备录制GPU命令
        // - 标志：ONE_TIME_SUBMIT_BIT（命令缓冲区仅提交一次，执行后自动重置）
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;      // 单次提交模式

        // 3. 录制缓冲区复制命令
        // -----------------------------------------------
        // - 作用：将数据从源缓冲区复制到目标缓冲区
        // - 参数：
        //   - srcOffset/dstOffset：偏移量（设为0表示从头开始复制）
        //   - size：复制的字节数
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;       // 源缓冲区起始偏移（可选，默认0）
        copyRegion.dstOffset = 0;       // 目标缓冲区起始偏移（可选，默认0）
        copyRegion.size = size;         // 复制的数据总大小

        vkCmdCopyBuffer(
            commandBuffer,       // 当前命令缓冲区
            srcBuffer,           // 源缓冲区（数据来源）
            dstBuffer,           // 目标缓冲区（数据去向）
            1,                   // 复制的区域数量
            &copyRegion          // 复制区域配置
        );

        vkEndCommandBuffer(commandBuffer); // 结束录制

        // 4. 提交命令到图形队列并等待完成
        // -----------------------------------------------
        // - 作用：将命令缓冲区提交到队列执行，并阻塞CPU直到操作完成
        // - 注意：使用vkQueueWaitIdle是简单的同步方式，但会降低性能，实际应用中建议使用栅栏（Fence）或信号量（Semaphore）
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);   // 提交到图形队列
        vkQueueWaitIdle(graphicsQueue);                                 // 等待队列执行完成（阻塞CPU）

        // 5. 释放命令缓冲区
        // -----------------------------------------------
        // - 作用：命令缓冲区执行完成后，将其回收至命令池
        // - 注意：此处未销毁命令池，仅释放缓冲区（命令池可复用分配其他缓冲区）
        vkFreeCommandBuffers(
            device,
            commandPool,      // 所属命令池
            1,                // 释放的缓冲区数量
            &commandBuffer    // 待释放的缓冲区指针
        );
    }

    // 录制命令缓冲区的具体指令（定义渲染操作流程）
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        // --- 阶段1：开始录制命令缓冲区 ---
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;  // 结构体类型标识
        beginInfo.flags = 0;                                            // 无特殊使用标志（普通录制模式）
        beginInfo.pInheritanceInfo = nullptr;                           // 次级缓冲区继承信息（主缓冲区无需设置）

        // 开始录制命令缓冲区（类似按下录音键）
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // --- 阶段2：配置并启动渲染流程 ---
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;    // 结构体类型标识
        renderPassInfo.renderPass = renderPass;                             // 指定使用的渲染流程
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];     // 绑定当前帧的帧缓冲

        renderPassInfo.renderArea.offset = { 0, 0 };                        // 渲染区域起点（左上角）
        renderPassInfo.renderArea.extent = swapChainExtent;                 // 渲染区域尺寸（覆盖整个窗口）

        // 定义清除颜色（黑色，RGBA=0,0,0,1）
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;                                 // 清除值数量（仅颜色附件）
        renderPassInfo.pClearValues = &clearColor;                          // 指向清除颜色数组

        // 启动渲染流程（类似开始一幅画的绘制）
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // --- 阶段3：绑定图形管线 ---
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // --- 阶段4：配置视口（画面输出区域）---
        VkViewport viewport{};
        viewport.x = 0.0f;                                      // 视口起点X坐标
        viewport.y = 0.0f;                                      // 视口起点Y坐标 
        viewport.width = (float)swapChainExtent.width;          // 视口宽度（与窗口同宽）
        viewport.height = (float)swapChainExtent.height;        // 视口高度（与窗口同高）
        viewport.minDepth = 0.0f;                               // 最小深度值（0-1范围）
        viewport.maxDepth = 1.0f;                               // 最大深度值
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);       // 应用视口设置

        // --- 阶段5：配置裁剪区域（限制绘制范围）---
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };                              // 裁剪区域起点
        scissor.extent = swapChainExtent;                       // 裁剪区域尺寸（覆盖整个窗口）
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);         // 应用裁剪设置

        // 将顶点缓存绑定
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        // 将索引缓存绑定
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // 绑定描述符集
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);


        // --- 阶段6：发出绘制指令 ---
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        // 参数解释：
        // vertexCount: 顶点缓存的尺寸
        // instanceCount: 1（不启用实例化渲染）
        // firstVertex: 0（从顶点缓冲的0位置开始）
        // firstInstance: 0（实例ID从0开始）

        // --- 阶段7：结束渲染流程 ---
        vkCmdEndRenderPass(commandBuffer);

        // --- 阶段8：结束命令缓冲区录制 ---
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // 完成录制（类似停止录音，此时命令缓冲区已包含完整指令序列）
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        // 最小化
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    // 创建用于同步的信号量
    void createSyncObjects() {
        // 初始化数组大小
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // 一口气创建3个信号量
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }

    // 
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);    // 返回设备的全局内存信息

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // 检查内存类型是否支持且满足属性要求
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    // 更新Uniform Buffer数据（每帧调用）
    void updateUniformBuffer(uint32_t currentImage) {
        // 记录初始时间（static确保只初始化一次，后续调用保留值）
        static auto startTime = std::chrono::high_resolution_clock::now();

        // 获取当前时间，并计算从初始时间到现在的总秒数
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // 创建Uniform Buffer Object (UBO) 并填充数据
        UniformBufferObject ubo{};
        // 模型矩阵：绕Z轴随时间旋转（每秒旋转90度）
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // 视图矩阵：摄像机位于(2,2,2)，看向原点(0,0,0)，上方向为Z轴
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // 投影矩阵：45度透视投影，宽高比适配交换链分辨率，近平面0.1，远平面10
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        // 修正Vulkan的Y轴坐标系（与OpenGL相反，翻转Y轴投影矩阵分量）
        ubo.proj[1][1] *= -1;

        // 将UBO数据直接复制到当前帧的映射内存中（无需映射/解映射操作）
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    // 绘制帧
    void drawFrame() {
        // 步骤1：等待前一帧渲染完成（避免资源竞争）
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        // 步骤2：从交换链获取下一帧图像（确定渲染目标），同时判断交换链是否过时
        VkResult result = vkAcquireNextImageKHR(
            device,
            swapChain,
            UINT64_MAX,                                 // 超时时间（无限等待）
            imageAvailableSemaphores[currentFrame],     // 信号量：图像获取完成后触发
            VK_NULL_HANDLE,                             // 不使用栅栏（此处用信号量同步）
            &imageIndex                                 // 输出：交换链图像的索引
        );
        // 如果交换链过时则重置交换链并直接返回
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // 更新UBO
        updateUniformBuffer(currentFrame);

        // 步骤3：重置栅栏状态（为当前帧做准备）
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        // 步骤4：重置并录制命令缓冲区（定义渲染操作）
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);             // 重置命令缓冲区（复用内存）
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);     // 录制绘制指令（绑定管线、视口等）

        // 步骤5：配置命令提交信息（同步GPU操作顺序）
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // 等待阶段与信号量：
        // - 等待 imageAvailableSemaphore（确保图像已获取）
        // - 等待阶段：颜色附件输出阶段（即开始渲染前等待）
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;        // 从交换链中获取到图像-信号量
        submitInfo.pWaitDstStageMask = waitStages;

        // 提交的命令缓冲区（包含本帧的渲染指令）
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        // 触发信号量：渲染完成后触发 renderFinishedSemaphore
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;    // 图像渲染完成-信号量

        // 步骤6：提交命令到图形队列（启动GPU渲染）
        // - graphicsQueue: 目标队列（图形操作队列）
        // - inFlightFence: 提交后关联的栅栏（渲染完成后触发）
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // 步骤7：配置图像呈现信息并提交到呈现队列（新增关键部分）
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; // 结构体类型标识

        // 等待信号量：渲染完成信号量（确保图像渲染完毕后再呈现）
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;         // 使用步骤6中触发的renderFinishedSemaphore

        // 指定交换链和图像索引
        VkSwapchainKHR swapChains[] = { swapChain };            // 目标交换链（当前激活的交换链）
        presentInfo.swapchainCount = 1;                         // 交换链数量
        presentInfo.pSwapchains = swapChains;                   // 指向交换链数组
        presentInfo.pImageIndices = &imageIndex;                // 要呈现的交换链图像索引（步骤3中获取的）

        // 提交呈现请求（将渲染完成的图像显示到屏幕）
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // 检查呈现结果
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain(); // 重建交换链
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // 更新CurrentFrame
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // 创建 shader 模组
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        // 与shader module有关的创建信息
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        // 创建shader module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
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

    // 综合判断物理设备是否满足应用需求（队列/扩展/交换链支持）
    bool isDeviceSuitable(VkPhysicalDevice device) {
        // ▼ 条件1：检查设备是否支持必要的队列家族（图形队列和呈现队列）
        QueueFamilyIndices indices = findQueueFamilies(device);

        // ▼ 条件2：检查设备是否支持所有要求的扩展（如交换链扩展）
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // ▼ 条件3：检查交换链支持是否足够（至少有一个格式和一个呈现模式）
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // 所有条件必须同时满足：队列完整、扩展支持、交换链可用
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    // 检查物理设备（GPU) 是否支持所有必须的扩展
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        // 获取设备支持的扩展总数
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr); // 获取count
        // 获取所有可用扩展的详细信息
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // 将需要的扩展列表转换为集合，便于快速查找和删除
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // 遍历所有可用扩展，从需求集合中移除已找到的扩展
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName); // 删除匹配的扩展名
        }

        // 若需求集合为空，说明所有扩展都支持；否则存在缺失
        return requiredExtensions.empty();
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

                // 检查当前队列家族是否支持呈现（Present）到窗口表面
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

                // 如果支持呈现，则记录呈现队列家族索引
                if (presentSupport) {
                    indices.presentFamily = i;
                }

                // 如果已找到所有需要的队列家族，提前退出循环
                if (indices.isComplete()) {
                    break;
                }
            }
            i++;
        }

        return indices;
    }

    // 查询物理设备对交换链（Swapchain）的支持细节，包括表面能力、格式和呈现模式
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        // 获取表面基础能力（如图像数量范围、最小/最大尺寸）
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // 获取支持的表面格式（如颜色空间、像素格式）
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount); // 预分配内存
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // 获取支持的呈现模式（如垂直同步、立即模式）
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details; // 返回交换链支持的三类信息
    }

    // 选择交换链表面模式
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    // 选择交换链展示模式
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // 选择交换链分辨率
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // 情况1：当前范围已被明确指定（直接使用） 
        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
            return capabilities.currentExtent;
        }
        else {// 情况2：窗口管理器允许自定义范围（需手动计算）
            // 获取窗口的像素分辨率（例如 Retina 显示屏的实际像素可能更高）
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // 将分辨率限制在硬件支持的范围内（避免越界）
            actualExtent.width = std::clamp(actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);

            actualExtent.height = std::clamp(actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // 按照文件名打开文件
    static std::vector<char> readFile(const std::string& filename) {
        // 从末尾读（ate），读为二进制文件
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        // 通过在末尾的指针位置，直接获得文件大小，总字节数
        size_t fileSize = (size_t)file.tellg();
        // 分配内存缓冲区
        std::vector<char> buffer(fileSize);

        // 读取文件内容
        file.seekg(0);                          // 将文件指针移回开头
        file.read(buffer.data(), fileSize);     // 读取全部内容到缓冲区

        // 关闭文件
        file.close();

        return buffer;
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

    // 帧缓冲大小更改——回调函数
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
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