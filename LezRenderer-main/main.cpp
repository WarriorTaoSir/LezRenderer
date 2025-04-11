#define GLFW_INCLUDE_VULKAN // 让GLFW自动包含Vulkan头文件
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS                   // 统一使用radiance
#define GLM_FORCE_DEPTH_ZERO_TO_ONE         // 将深度限制在0到1

#include <glm/glm.hpp>                      // 
#include <glm/gtc/matrix_transform.hpp>     // 包含模型变换等函数

// 图像读取
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      
// 模型加载
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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
#include <unordered_map>

#include <glm/glm.hpp>
#include <array>

#include <chrono>       // 公开了进行精准计时的函数。

// 窗口尺寸常量
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

// 资源路径
const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
//const std::string MODEL_PATH = "models/revolver.obj";
//const std::string TEXTURE_PATH = "textures/revolver_basecolor.png";
//const std::string MODEL_PATH = "models/python.obj";
//const std::string TEXTURE_PATH = "textures/python_basecolor.jpg";

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
    glm::vec3 pos;      // 位置属性（3D坐标，对应顶点着色器的 location=0）
    glm::vec3 color;    // 颜色属性（RGB，对应顶点着色器的 location=1）
    glm::vec2 texCoord; // UV属性

    // 生成顶点数据绑定描述（Vulkan需要此信息读取内存中的顶点数据）
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;                                 // 绑定索引（从0开始，单缓冲时通常为0）
        bindingDescription.stride = sizeof(Vertex);                     // 每个顶点数据的总字节大小（自动计算）
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;     // 按顶点步进（非实例化）

        return bindingDescription;
    }

    // 生成顶点属性描述（定义每个属性的格式和位置）
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        // 位置属性（pos）的描述
        attributeDescriptions[0].binding = 0;                               // 绑定索引（与绑定描述一致）
        attributeDescriptions[0].location = 0;                              // 顶点着色器的输入位置（location=0）
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;       // 数据格式（vec3）3个32位浮点数
        attributeDescriptions[0].offset = offsetof(Vertex, pos);            // 偏移量（pos在结构体中的起始位置）

        // 描述颜色属性（color）
        attributeDescriptions[1].binding = 0;                           // 同一绑定（数据来自同一缓冲区）
        attributeDescriptions[1].location = 1;                          // 顶点着色器的输入位置（location=1）
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // 数据格式（vec3）3个32位浮点数
        attributeDescriptions[1].offset = offsetof(Vertex, color);      // 偏移量（color的起始位置）

        // 描述UV属性（texcoord）
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

// 为自定义Vertex结构体特化标准库的哈希模板
// 用途：使Vertex对象可作为std::unordered_map的键
namespace std {
    template<> struct hash<Vertex> {
        // 组合各个属性的哈希值，生成唯一性较高的复合哈希
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}// 必须定义在std命名空间内

// 顶点属性数组
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

// 顶点索引数组
// uint16_t适合少于65535顶点数的模型
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
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

    // 模型的顶点与索引
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Vulkan缓冲句柄
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;

    // mipmap等级数
    uint32_t mipLevels;
    // Multi-Sampling
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage textureImage;
    VkImage depthImage;

    // Vulkan缓冲对应的设备内存
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;
    VkDeviceMemory textureImageMemory;
    VkDeviceMemory depthImageMemory;

    // 贴图采样相关
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkImageView depthImageView;

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
        createCommandPool();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
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

        // 销毁图形管线
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        // 销毁管线布局
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        // 销毁渲染通道
        vkDestroyRenderPass(device, renderPass, nullptr);

        // 销毁UBO
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        // 销毁描述符集池
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        // 销毁贴图采样器 和 图像视图
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);

        // 销毁图片以及对应Device Memory
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        // 销毁描述符集布局
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        // 销毁索引Buffer以及回收对印内存
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        // 销毁顶点Buffer以及回收对印内存
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

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
                msaaSamples = getMaxUsableSampleCount();
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
        deviceFeatures.samplerAnisotropy = VK_TRUE; // 各向异性采样
        deviceFeatures.sampleRateShading = VK_TRUE; // 启用设备的sample shading

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
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    // 创建渲染通道
    void createRenderPass() {
        // 配置颜色附件（Color Attachment）
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;          // 需要与交换链中的图像格式匹配
        colorAttachment.samples = msaaSamples;                  // 多重采样

        // 定义颜色附件的加载/存储操作
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // 渲染前清空
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // 渲染后保存

        // 定义模板附件的加载/存储操作（当前未使用模板，设为DONT_CARE）
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // 定义渲染开始和结束时附件的布局
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // 不保留内容化
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;      // 用于显示

        // 配置颜色附件的引用（用于子过程）
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 用于配置深度附件
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        // 配置深度附件的引用（用于子过程）
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // 定义颜色解析附件的属性，用于多重采样反锯齿（MSAA）的解析操作
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = swapChainImageFormat;                       // 使用交换链同款格式，确保兼容呈现
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;                     // 单采样（作为MSAA解析目标）
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;            // 无需加载初始内容（每次覆盖）
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;              // 必须存储解析结果（供后续呈现使用）
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;     // 非模板附件，忽略
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;           // 初始布局无意义（可丢弃）
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;       // 最终转换为呈现优化布局

        // 配置颜色解析附件的引用（用于子过程）
        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 配置子过程（Subpass）
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // 指定为图形管线（非计算管线）
        subpass.colorAttachmentCount = 1;                               // 使用的颜色附件数量
        subpass.pColorAttachments = &colorAttachmentRef;                // 颜色附件引用
        subpass.pDepthStencilAttachment = &depthAttachmentRef;          // 深度模板附件引用
        subpass.pResolveAttachments = &colorAttachmentResolveRef;       // 颜色解析附件引用

        // 依赖关系的作用解释：
        // 1. 确保渲染通道开始前等待图像可用（通过 imageAvailableSemaphore）
        // 2. 控制图像布局从 UNDEFINED 自动转换为 COLOR_ATTACHMENT_OPTIMAL 的时机
        // 3. 防止在图像未准备好时进行渲染操作（数据竞争）

        // 创建渲染通道信息结构体
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                // 源：渲染通道开始前的隐式操作（如交换链获取图像）
        dependency.dstSubpass = 0;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
        // 创建渲染通道信息结构体
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;   // 结构体类型标识
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());// 附件数量
        renderPassInfo.pAttachments = attachments.data();                   // 指向附件描述数组
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
        // UBO 布局绑定
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;                       // 绑定位置（与着色器中的 layout(binding=0) 对应）
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // 类型：UBO
        uboLayoutBinding.descriptorCount = 1;               // UBO 数量（此处为单个，也可用于数组）
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   // 生效的着色器阶段（顶点着色器）
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // 2D 贴图采样器 布局绑定
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // 在片元着色器生效

        // 绑定数组
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

        // 创建描述符集布局 createInfo
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

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
        multisampling.sampleShadingEnable = VK_TRUE;                                       // 禁用采样着色（抗锯齿）
        multisampling.rasterizationSamples = msaaSamples;                                   // 采样数
        multisampling.minSampleShading = .2f; 

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

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
        pipelineInfo.pDepthStencilState = &depthStencil;        // 深度/模板测试配置
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

    // 创建交换链帧缓冲区的函数
    // 功能：为每个交换链图像视图创建对应的帧缓冲区，绑定颜色附件（交换链图像）和深度附件
    void createFramebuffers() {
        // 根据交换链图像视图数量调整帧缓冲区数组大小（每个交换链图像对应一个帧缓冲区）
        swapChainFramebuffers.resize(swapChainImageViews.size());

        // 遍历所有交换链图像视图，逐个创建帧缓冲区
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            // 定义帧缓冲区附件数组（必须与渲染通道描述的附件顺序一致）
            // [0] 颜色附件：当前交换链图像视图（VK_ATTACHMENT_LOAD_OP_CLEAR 操作的目标）
            // [1] 深度附件：深度图像视图（用于深度测试）
            std::array<VkImageView, 3> attachments = {
                colorImageView,
                depthImageView,
                swapChainImageViews[i]
            };

            // 配置帧缓冲区创建信息
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;                    // 关联的渲染通道（描述附件如何使用）
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());    // 附件数量
            framebufferInfo.pAttachments = attachments.data();          // 附件视图数组指针
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            // 创建帧缓冲区对象
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

    // 创建深度资源
    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    
    }

    // 创建Vulkan图像对象并分配内存
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        // 1. 配置图像创建信息
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;  // 结构体类型标识
        imageInfo.imageType = VK_IMAGE_TYPE_2D;                 // 2D纹理（还有1D/3D类型）
        imageInfo.extent.width = width;                         // 图像宽度
        imageInfo.extent.height = height;                       // 图像高度
        imageInfo.extent.depth = 1;                             // 深度（3D图像时才>1）
        imageInfo.mipLevels = mipLevels;                        // Mipmap层级数
        imageInfo.arrayLayers = 1;                              // 图像数组层数（用于纹理数组）
        imageInfo.format = format;                              // 像素格式，必须与后续视图一致
        imageInfo.tiling = tiling;                              // 内存布局方式：
                                                                // LINEAR-线性排列（方便CPU访问）
                                                                // OPTIMAL-硬件优化排列（GPU高效）
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    // 初始布局（后续会转换）
        imageInfo.usage = usage;                                // 图像用途组合（传输、采样等）
        imageInfo.samples = numSamples;                         // 多重采样数
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;      // 独占访问模式（无需跨队列共享）

        // 2. 创建图像对象
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // 3. 查询图像内存需求（获取需要分配的内存大小和对齐要求）
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        // 4. 配置内存分配信息
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        // 寻找符合要求的内存类型（如设备本地内存）
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // 5. 分配设备内存
        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        // 6. 将内存绑定到图像对象（类似OpenGL的glBindBuffer）
        vkBindImageMemory(device, image, imageMemory, 0);
    }

    // 创建纹理贴图的核心函数
    // 流程：加载图片文件 → 暂存到CPU可见缓冲区 → 复制到GPU专用图像 → 转换图像布局供着色器使用
    void createTextureImage() {
        // 1. 加载图片文件
        int texWidth, texHeight, texChannels;
        // 使用stb_image库加载JPG图片，强制转换为RGBA格式（4通道）
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        VkDeviceSize imageSize = texWidth * texHeight * 4;

        // 检查图片加载是否成功
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        // 2. 创建暂存缓冲区（Staging Buffer）
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        // 创建CPU可见的缓冲区
        // 创建CPU可见的临时缓冲区：
        // - 用途：作为传输源（TRANSFER_SRC_BIT）
        // - 内存属性：主机可见（可映射） + 主机相干（自动同步CPU/GPU内存）
        createBuffer(imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // 后续要作为传输源
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // CPU可访问
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // 自动内存同步
            stagingBuffer, stagingBufferMemory);

        // 3. 将像素数据复制到暂存缓冲区
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);// 释放STB图像数据（不再需要CPU端原始数据）

        // 4. 创建GPU专用的纹理图像：
        // - 格式：R8G8B8A8_SRGB（sRGB颜色空间）
        // - 平铺方式：OPTIMAL（GPU优化布局，CPU不可直接访问）
        // - 用途：传输目标 + 着色器采样
        // - 内存属性：DEVICE_LOCAL（GPU专用内存，访问速度快）
        createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
        
        // 5. 执行数据传输和布局转换流程：
        // (1) 转换布局：UNDEFINED → TRANSFER_DST_OPTIMAL（准备接收数据）
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        // (2) 从暂存缓冲区复制数据到纹理图像
            copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // (3) 转换布局：TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL（着色器采样优化）
        // transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

        // 生成mipmap
        generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
        // 6. 清理临时资源
        vkDestroyBuffer(device, stagingBuffer, nullptr);    // 销毁暂存缓冲区对象
        vkFreeMemory(device, stagingBufferMemory, nullptr); // 释放暂存缓冲区内存
    }

    // 用于生成纹理的Mipmap链，通过多级图像缩放(Blit)操作实现，同时管理Vulkan图像布局转换
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // 检查图像格式是否支持线性blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        //1: 获取单次命令缓冲区
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // 初始化当前处理的Mip层级尺寸
        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        //2：配置基础内存屏障参数
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;                                          // 操作的目标图像
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;          // 不转移队列所有权
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;// 颜色通道
        barrier.subresourceRange.baseArrayLayer = 0;                    // 基础数组层
        barrier.subresourceRange.layerCount = 1;                        // 单层纹理
        barrier.subresourceRange.levelCount = 1;                        // 每次处理1个Mip层级

        //3：逐级生成Mipmap
        for (uint32_t i = 1; i < mipLevels; i++) {
            // 3.1：转换前级Mip布局为TRANSFER_SRC_OPTIMAL
            barrier.subresourceRange.baseMipLevel = i - 1;              // 操作i-1级
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;       // 等待前级写入完成
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;        // 准备作为Blit源

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,                 // 等待所有传输操作完成                
                VK_PIPELINE_STAGE_TRANSFER_BIT,                 // 在传输阶段执行
                0, 0, nullptr, 0, nullptr,1, &barrier);

            // 3.2：配置Blit操作参数
            VkImageBlit blit{};
            // 源区域（当前Mip级）
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            // 目标区域（下一Mip级）
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;           // 目标Mip级
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            // 3.3：执行Blit缩放操作
            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            // 3.4：转换前级Mip为SHADER_READ布局
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,          // 在传输之后
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,   // 片段着色阶段可用
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            // 更新尺寸为下一级
            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // 4：处理最后一级Mip
        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // 5：提交命令缓冲区
        endSingleTimeCommands(commandBuffer);
    }

    // 获取最大可用采样数
    VkSampleCountFlagBits getMaxUsableSampleCount() {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    void createColorResources() {
        VkFormat colorFormat = swapChainImageFormat;

        createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
        colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    }

    // 创建纹理采样器（决定如何从纹理中获取颜色）
    // 采样器相当于GPU的"取色规则说明书"，控制纹理如何被读取和过滤
    void createTextureSampler() {
        // 获取物理设备属性
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // 纹理放大/缩小时的过滤方式（解决像素不够用的问题）
        samplerInfo.magFilter = VK_FILTER_LINEAR;       // 放大时用线性插值（平滑）
        samplerInfo.minFilter = VK_FILTER_LINEAR;       // 缩小时同样用线性插值

        // 纹理坐标超出[0,1]时的处理方式（就像决定墙纸如何铺贴）
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // U方向重复纹理
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // V方向重复纹理
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // W方向（3D纹理用）

        // 各向异性过滤配置（消除倾斜表面的纹理模糊）
        samplerInfo.anisotropyEnable = VK_TRUE;

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // 设置硬件支持的最大值
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;         // 越界区域显示为白色
        // 坐标是否归一化（true表示使用实际像素坐标，false表示0-1标准化坐标）
        samplerInfo.unnormalizedCoordinates = VK_FALSE;                     // 使用标准UV坐标（0到1范围）
        // 深度比较设置（用于阴影映射等高级特性）
        samplerInfo.compareEnable = VK_FALSE;                               // 不启用深度比较
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                       // 比较永远返回true（无效果）

        // Mipmap相关设置（多级纹理细节）
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;             // mipmap使用线性插值
        samplerInfo.mipLodBias = 0.0f;                                      // mipmap级别偏移量
        samplerInfo.minLod = 0.0f;               // 最小使用的mipmap级别
        samplerInfo.maxLod = static_cast<float>(mipLevels);                 // 最大使用的mipmap级别（0表示只用基础级别）

        // 创建采样器对象
        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    // 找到支持最优的深度贴图格式
    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    // 查找物理设备支持的图像格式
    // 用途：根据所需的平铺方式（Tiling）和特性（Features），从候选格式列表中选择第一个可用的格式
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        // 遍历所有候选格式（按调用方传入的优先级顺序）
        for (VkFormat format : candidates) {
            // 获取物理设备对该格式的支持属性
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
            // 检查线性平铺（LINEAR）模式的支持情况
            // 适用场景：需要CPU可访问的图像（如暂存图像）
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            // 检查优化平铺（OPTIMAL）模式的支持情况
            // 适用场景：GPU专用图像（如纹理、深度缓冲）
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        // 所有候选格式均不支持要求的特性，抛出错误
        throw std::runtime_error("failed to find supported format!");
    }

    // 判断该format是否有stencil部分
    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    // helper function 创建图像视图
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        // 配置图像视图的创建参数
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        // 定义图像视图的类型和格式
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        // 配置子资源范围（定义视图能访问图像的哪些部分）
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    // 创建并开始一个一次性使用的命令缓冲区
    // 适用场景：短期存在的GPU操作（如传输数据），执行后立即销毁
    VkCommandBuffer beginSingleTimeCommands() {
        // 命令缓冲区分配信息
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        // 从命令池分配命令缓冲区
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        // 命令缓冲区开始记录信息
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // 开始命令缓冲区记录状态
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;// 返回已开始记录的命令缓冲区
    }

    // 提交并销毁一次性命令缓冲区
    // 注意：包含隐式 GPU-CPU 同步（vkQueueWaitIdle），适用于低频操作
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        // 结束命令缓冲区记录
        vkEndCommandBuffer(commandBuffer);

        // 提交信息配置
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;                  // 提交1个命令缓冲区
        submitInfo.pCommandBuffers = &commandBuffer;        // 指向要提交的命令缓冲区

        // 提交到图形队列（假设 graphicsQueue 已初始化）
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);   
        // 等待队列执行完成（强制同步，确保命令完成）
        vkQueueWaitIdle(graphicsQueue);                                 
        // 释放命令缓冲区回命令池（不会销毁命令池本身）
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    // 图像布局转换函数
    // 用途：安全地改变图像的Vulkan内存布局，确保GPU操作的正确同步
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        // 获取一次性命令缓冲区（用于短期存在的操作）
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // 配置图像内存屏障
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; // 标准Vulkan结构体类型标识
        barrier.oldLayout = oldLayout;                          // 当前内存布局
        barrier.newLayout = newLayout;                          // 目标内存布局
        // 以下参数表示不转移队列所有权（同一队列家族使用）
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;                                  // 目标图像对象
        // 配置图像子资源范围（影响整个图像）
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // 定义管线阶段和访问掩码（确保正确的执行顺序和内存可见性）
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        // 情况1：从未初始化状态 -> 传输目标优化布局
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            // 访问权限配置：
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            // 管线阶段配置：
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // 情况2：从传输目标优化布局 -> 着色器只读优化布局
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // 访问权限配置：
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            // 管线阶段配置：
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            // 不支持其他布局转换（可扩展添加更多情况）
            throw std::invalid_argument("unsupported layout transition!");
        }

        // 提交管线屏障命令
        vkCmdPipelineBarrier(
            commandBuffer,                   // 命令缓冲区
            sourceStage, destinationStage,   // 源和目标管线阶段
            0,                               // 依赖标志（0表示无特殊依赖）
            0, nullptr,                      // 内存屏障数量和数据（此处未使用）
            0, nullptr,                      // 缓冲区内存屏障数量和数据（此处未使用）
            1, &barrier                      // 图像内存屏障数量和数据
        );

        // 提交并执行命令缓冲区（包含隐式等待操作）
        endSingleTimeCommands(commandBuffer);
    }

    // 将缓冲区数据复制到图像的辅助函数
    // 用途：用于将CPU/缓冲区中的像素数据（如纹理）传输到GPU图像对象
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        // 获取一次性命令缓冲区（适用于短期操作）
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // 配置缓冲区到图像的复制区域信息
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        // 定义目标图像的子资源信息
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        // 定义图像写入位置和范围
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        // 提交缓冲区到图像的复制命令
        // 注意：目标图像必须处于 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 布局
        vkCmdCopyBufferToImage(
            commandBuffer,                   // 命令缓冲区
            buffer,                          // 源缓冲区
            image,                           // 目标图像
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // 目标图像的布局（必须为此布局）
            1,                               // 区域数量
            &region                          // 区域配置指针
        );

        // 提交并等待命令执行完成（确保数据传输完毕）
        endSingleTimeCommands(commandBuffer);
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

    // 加载OBJ模型数据到顶点和索引缓冲
    // 功能：解析.obj文件，提取顶点数据，并自动去重生成索引缓冲
    void loadModel() {
        // 初始化tinyobjloader数据结构
        tinyobj::attrib_t attrib;                   // 存储顶点属性（位置/纹理坐标等）
        std::vector<tinyobj::shape_t> shapes;       // 存储网格形状数据
        std::vector<tinyobj::material_t> materials; // 材质数据（未使用）
        std::string warn, err;                      // 加载过程的警告/错误信息

        // 加载OBJ模型文件
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        // 创建顶点哈希表用于去重（需为Vertex实现哈希函数）
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        // 遍历模型中的所有形状（通常对应mesh）
        for (const auto& shape : shapes) {
            // 遍历形状的所有面索引
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                // 提取顶点位置（X/Y/Z坐标）
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                // 提取并处理纹理坐标（翻转V分量以适配Vulkan坐标系）
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]   // 翻转一下V坐标
                };

                // 设置默认白色顶点颜色（应改为从材质读取）
                vertex.color = { 1.0f, 1.0f, 1.0f };

                // 顶点去重处理：如果顶点不存在于哈希表则添加
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                // 添加索引
                indices.push_back(uniqueVertices[vertex]);
            }
        }
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
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                      // 类型：Uniform Buffer
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // 数量：每帧一个
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;              // 类型： Combined Image Sampler
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        // 创建描述符池的配置信息
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());           // 池类型数量
        poolInfo.pPoolSizes = poolSizes.data();                                     // 指向类型配置的指针
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);             // 最大可分配的描述符集数量
    
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

            // 描述符应用的贴图信息
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            // 构造描述符写入操作
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];     // 目标描述符集（当前帧）
            descriptorWrites[0].dstBinding = 0;                 // 绑定点（与着色器中的binding=0对应）
            descriptorWrites[0].dstArrayElement = 0;            // 数组索引（非数组时为0）
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 类型：UBO
            descriptorWrites[0].descriptorCount = 1;            // 绑定的描述符数量（单个）
            descriptorWrites[0].pBufferInfo = &bufferInfo;      // 缓冲区信息指针

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            // 更新描述符集（将配置提交到GPU）
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();  // 开始录制

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

        endSingleTimeCommands(commandBuffer);   // 结束录制，并提交，同步，释放
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
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;                                 // 清除值数量（仅颜色附件）
        renderPassInfo.pClearValues = clearValues.data();                          // 指向清除颜色数组

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
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

    // 清理交换链相关资源的函数
    // 用途：在交换链重建（如窗口大小变化）或程序退出时，释放与交换链关联的GPU资源
    void cleanupSwapChain() {
        // 0. 销毁颜色缓冲相关资源（深度图像视图、图像对象、设备内存）
        vkDestroyImageView(device, colorImageView, nullptr);    // 销毁深度图像视图（VkImageView）
        vkDestroyImage(device, colorImage, nullptr);            // 销毁深度图像对象（VkImage）
        vkFreeMemory(device, colorImageMemory, nullptr);        // 释放深度图像设备内存（VkDeviceMemory）

        // 1. 销毁深度缓冲相关资源（深度图像视图、图像对象、设备内存）
        vkDestroyImageView(device, depthImageView, nullptr);    // 销毁深度图像视图（VkImageView）
        vkDestroyImage(device, depthImage, nullptr);            // 销毁深度图像对象（VkImage）
        vkFreeMemory(device, depthImageMemory, nullptr);        // 释放深度图像设备内存（VkDeviceMemory）

        // 2. 销毁所有交换链帧缓冲区（Frame buffer）
        // 帧缓冲区依赖图像视图，需先于图像视图销毁
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        }
        // 3. 销毁所有交换链图像视图（Image views）
        // 图像视图依赖交换链图像，需在销毁交换链前销毁
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }

        // 4. 销毁交换链本体（Swap chain）
        // 注意：交换链图像（VkImage）由交换链自动管理，此处无需手动销毁
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    // 重建交换链的核心函数
    // 用途：在窗口大小改变、窗口恢复（如从最小化还原）或交换链失效时，重新创建交换链及其关联资源
    void recreateSwapChain() {
        // 1. 获取当前窗口的帧缓冲区尺寸（单位：像素）
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        
        // 2. 处理窗口最小化/尺寸为0的特殊情况
        // 当窗口被最小化时，持续等待直到窗口恢复有效尺寸
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        // 3. 等待设备所有队列空闲（确保没有正在使用交换链资源的操作）
        vkDeviceWaitIdle(device);// 避免在资源仍被使用时进行清理

        // 4. 清理旧交换链及其关联资源
        cleanupSwapChain();// 销毁深度缓冲、帧缓冲、图像视图和交换链本身

        // 5. 按正确顺序重建所有交换链相关资源
        createSwapChain();          // 创建新交换链（VkSwapchainKHR）
        createImageViews();         // 为交换链中的每个图像创建视图（VkImageView）
        createColorResources();
        createDepthResources();     // 重建深度缓冲图像（VkImage）和视图
        createFramebuffers();       // 为新图像视图和深度缓冲创建帧缓冲（VkFramebuffer）
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
        // ▼ 条件4：要求支持各项异性采样
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        // 所有条件必须同时满足：队列完整、扩展支持、交换链可用
        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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