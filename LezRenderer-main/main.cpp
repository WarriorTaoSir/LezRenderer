#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN // 让GLFW自动包含Vulkan头文件
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

// 窗口尺寸常量
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageView();
    }

    // 主事件循环
    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // 释放资源
    void cleanup() {
        // 释放图像视图
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr); // 销毁交换链

        vkDestroyDevice(device, nullptr);           // 销毁逻辑设备

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
    void createImageView() {
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

    // 交换链展示模式
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

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