#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN // ��GLFW�Զ�����Vulkanͷ�ļ�
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

// ���ڳߴ糣��
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// ���õ���֤���б�Khronos�ٷ���֤�㣩
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};


const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// ���ݱ���ģʽ�����Ƿ�������֤��
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// ����������̬���ز�����vkCreateDebugUtilsMessengerEXT
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // ͨ��vkGetInstanceProcAddr��ȡ����ָ��
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // ����ɹ���ȡ����ָ�룬����ʵ�ʺ���
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        // ��չδ���û�������֧��
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// ����������̬���ز�����vkDestroyDebugUtilsMessengerEXT
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// ���ڴ洢�����豸֧�ֵĶ��м�������
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // ͼ�ζ��м������������ܲ����ڣ�
    std::optional<uint32_t> presentFamily;  // չʾ���м������������ܲ����ڣ�

    // ������б���Ķ��м����Ƿ����ҵ�
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value(); // ��ǰ����ͼ�ζ��м�����Ϊ��������
    }
};

// ������֧��ϸ��
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


// ��Ӧ�ó�����
class HelloTriangleApplication {
public:
    void run() {
        initWindow();   // ��ʼ��GLFW����
        initVulkan();   // ��ʼ��Vulkan��Դ
        mainLoop();     // ��ѭ�������¼�
        cleanup();      // ������Դ
    }

private:
    // ��ʼ��GLFW����
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // ��ʹ��OpenGL������
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // ���ô�������
        // the window(width, height, title, screenIndex
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    // ��ʼ��Vulkan�������
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageView();
    }

    // ���¼�ѭ��
    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // �ͷ���Դ
    void cleanup() {
        // �ͷ�ͼ����ͼ
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr); // ���ٽ�����

        vkDestroyDevice(device, nullptr);           // �����߼��豸

        // �����������֤��
        if (enableValidationLayers) {
            // �ݻٴ����ĵ���Messenger
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);   // �����ٵ�����ʹ
        }

        vkDestroySurfaceKHR(instance, surface, nullptr); // ���ٴ��ڱ���

        vkDestroyInstance(instance, nullptr);       // ����Vulkanʵ��

        glfwDestroyWindow(window);                  // ���ٴ���

        glfwTerminate();                            // ��ֹGLFW
    }

    // ����Vulkanʵ��
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        // Ӧ����Ϣ���ã��Ǳ��赫�����ṩ��
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // ��ȷ�ýṹ�����洢����Ϣ����
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // ʵ����������
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // ��ȡ������չ������ϵͳ��չ + ������չ��
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // ����Messenger������Ϣ
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        // �����������֤��
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // ��������ʹ�����ã���Ϣ���𡢻ص������ȣ�
            populateDebugMessengerCreateInfo(debugCreateInfo);

            // �������������ӵ�ʵ��������Ϣ��
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;  // ԭ����ǿ��ת������Ҫ��ֱ��ȡ��ַ����
        }
        else {
            // �����õ���ʱ������������
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // ����Vulkanʵ��
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // �����Իص�����
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // ���մ��󡢾����������Ϣ������ʱ�ɸ�����Ҫ������
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // ������֤��������ص���Ϣ
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    // ���õ���Messenger
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // �������ڱ���
    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    // ���������֤���Ƿ��յ�֧��
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // ���ValidationLayers�Ƿ���availableLayers�������ڣ���ô�е���֤��Ͳ�֧��
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

    // ��ȡʵ���������չ�б�
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // ��ȡGLFW�������չ���細��ϵͳ���ɣ�
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // ����ģʽ����ӵ�����չ
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    // ѡ�������豸
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        // û��Vulkan֧�ֵ��豸
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // ��ʼ���豸����
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());


        // �����豸
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        // ѡ����ʵ��豸
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // ����Vulkan�߼��豸��Logical Device��������������м��壬����ȡͼ�κͳ��ֶ��о����Ϊ������Ⱦ����ʾ�����ṩ�ӿ�
    void createLogicalDevice() {
        // ��ȡ�����豸֧�ֵĶ��м���������ͼ�κ�չʾ���У�
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // ���ö��д�����Ϣ�����ﴴ��ͼ�ζ��У�
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        // ʹ��set�Զ�ȥ�أ������ظ�����ͬһ���м���Ķ������
        std::set<uint32_t> uniqueQueueFamilies = 
        {   
            indices.graphicsFamily.value(), 
            indices.presentFamily.value() 
        };

        // Ϊÿ�����м���������ȼ�����������
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // ��ʼ���豸�������󣨴˴�δ�����κ����⹦�ܣ�
        VkPhysicalDeviceFeatures deviceFeatures{};

        // �����߼��豸������Ϣ
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        // �����豸��չ���罻������չ��
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // ������֤�㣨�����ã�
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } 
        else {
            createInfo.enabledLayerCount = 0;
        }


        // �����߼��豸���������豸�������������
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        
        // ��ȡͼ�ζ��о���������ύ��Ⱦ���
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        // ��ȡ���ֶ��о����������ʾͼ�񵽴��ڣ�
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    // ����Vulkan��������Swapchain����������Ⱦͼ�������ʵ����Ļ��ʾ
    void createSwapChain() {
        // ��ȡ�����豸�Խ�������֧�����飨��������ʽ������ģʽ��
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // ѡ����ѽ��������ò���
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // ���㽻����ͼ�����������豸֧�ֵķ�Χ�ڣ�
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // ��Сֵ + 1
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount; // �ض�
        }

        // ���ý�������������
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; // �ṹ�����ͱ�ʶ
        createInfo.surface = surface; // �����Ĵ��ڱ��棨��GLFW���������ڿⴴ����

        // ͼ���������
        createInfo.minImageCount = imageCount;                      // ������ͼ������
        createInfo.imageFormat = surfaceFormat.format;              // ���ظ�ʽ����VK_FORMAT_B8G8R8A8_SRGB��
        createInfo.imageColorSpace = surfaceFormat.colorSpace;      // ��ɫ�ռ䣨��VK_COLOR_SPACE_SRGB_NONLINEAR_KHR��
        createInfo.imageExtent = extent;                            // ͼ��ֱ��ʣ���ߣ�
        createInfo.imageArrayLayers = 1;                            // ͼ�������3DӦ�ÿ�����Ҫ��㣬�˴���Ϊ1��
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// ͼ����;����Ϊ��ɫ��������ȾĿ�꣩

        // ������м��干��ģʽ��ͼ�ζ��� vs ���ֶ��У�
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        // ���ͼ�ζ��кͳ��ֶ������ڲ�ͬ���壬������Ϊ��������ģʽ
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   // ����ģʽ������м��干��ͼ��
            createInfo.queueFamilyIndexCount = 2;                       // ����Ķ��м�������
            createInfo.pQueueFamilyIndices = queueFamilyIndices;        // ���м�����������
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;    // ��ռģʽ�������м����ռ���ʣ����ܸ��ţ�
            createInfo.queueFamilyIndexCount = 0;                                 
            createInfo.pQueueFamilyIndices = nullptr; 
        }

        // ����ͼ��任��͸���ȣ�Ĭ���ޱ任�Ҳ�͸����
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // ���ó���ģʽ�Ͳü���Ϊ
        createInfo.presentMode = presentMode;       // ѡ��ĳ���ģʽ����VK_PRESENT_MODE_MAILBOX_KHR��
        createInfo.clipped = VK_TRUE;               // ���òü������Ա��ڵ������أ��Ż����ܣ�
        createInfo.oldSwapchain = VK_NULL_HANDLE;   // �ɽ�����������ؽ�������ʱ�������Ż���

        // ��������������
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");   // ʧ��ʱ�׳��쳣
        }

        // ��ȡ�������е�ͼ���б��õ����С���������
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);                   // �����ж�����ͼ
        swapChainImages.resize(imageCount);                                                 // ׼��������
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());    // �õ�����ͼ�ľ��

        // �������ò���(������ȾҪ��)
        swapChainImageFormat = surfaceFormat.format;    // ��ס���ظ�ʽ
        swapChainExtent = extent;                       // ��ס�ֱ���
    }

    // Ϊ�������е�ÿ��ͼ�񴴽�ͼ����ͼ��ImageView�����Ա�����Ⱦ�����з���ͼ������
    void createImageView() {
        // ���ݽ������е�ͼ������������ͼ����ͼ�����Ĵ�С
        swapChainImageViews.resize(swapChainImages.size());

        // �����������е�ÿ��ͼ��Ϊ�䴴����Ӧ��ͼ����ͼ
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            // ����ͼ����ͼ�Ĵ�������
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;    // �ṹ�����ͱ�ʶ
            createInfo.image = swapChainImages[i];                          // �󶨵��������еĵ�i��ͼ��
 
            // ����ͼ����ͼ�����ͺ͸�ʽ
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    // ��ͼ���ͣ�2D����Ҳ֧��1D��3D��CubeMap�ȣ�
            createInfo.format = swapChainImageFormat;       // ͼ���ʽ�����뽻�����ĸ�ʽһ�£�


            // ������ɫͨ��ӳ�䣨�˴�����Ĭ��RGBA˳��
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // ��������Դ��Χ��������ͼ�ܷ���ͼ�����Щ���֣�
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // ��ͼ������ɫ����
            createInfo.subresourceRange.baseMipLevel = 0;                       // ��ʼMipmap����0��ʾ��������
            createInfo.subresourceRange.levelCount = 1;                         // Mipmap�㼶�������˴���1�����޶༶��Զ��
            createInfo.subresourceRange.baseArrayLayer = 0;                     // ��ʼ����㣨0��ʾ�����㣩
            createInfo.subresourceRange.layerCount = 1;                         // ������������˴���1�㣬���������飩

            // ����ͼ����ͼ����
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    // �����豸���ʶȡ����Զ���ѡ�������豸�Ĺ���
    int rateDeviceSuitability(VkPhysicalDevice device) {
        // ��ȡ�豸��Ӳ�����ԣ������ơ����͡������汾�ȣ�
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        // ��ȡ�豸֧�ֵ�ͼ�ι��ܣ��缸����ɫ�������ز����ȣ�
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        // ����1�������Կ������ܸ�ǿ����1000��
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // ����2��֧�ֵ�2D�������ߴ�Խ�󣬵÷�Խ�ߣ�Ӱ����ͼ�ֱ��ʣ�
        score += deviceProperties.limits.maxImageDimension2D;

        // ����3������֧�ּ�����ɫ��������ֱ��0����̭
        if (!deviceFeatures.geometryShader) {
            return 0;
        }

        return score;
    }

    // �ۺ��ж������豸�Ƿ�����Ӧ�����󣨶���/��չ/������֧�֣�
    bool isDeviceSuitable(VkPhysicalDevice device) {
        // �� ����1������豸�Ƿ�֧�ֱ�Ҫ�Ķ��м��壨ͼ�ζ��кͳ��ֶ��У�
        QueueFamilyIndices indices = findQueueFamilies(device); 

        // �� ����2������豸�Ƿ�֧������Ҫ�����չ���罻������չ��
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // �� ����3����齻����֧���Ƿ��㹻��������һ����ʽ��һ������ģʽ��
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // ������������ͬʱ���㣺������������չ֧�֡�����������
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }
    
    // ��������豸��GPU) �Ƿ�֧�����б������չ
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        // ��ȡ�豸֧�ֵ���չ����
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr); // ��ȡcount
        // ��ȡ���п�����չ����ϸ��Ϣ
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // ����Ҫ����չ�б�ת��Ϊ���ϣ����ڿ��ٲ��Һ�ɾ��
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // �������п�����չ�������󼯺����Ƴ����ҵ�����չ
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName); // ɾ��ƥ�����չ��
        }

        // �����󼯺�Ϊ�գ�˵��������չ��֧�֣��������ȱʧ
        return requiredExtensions.empty();
    }

    // �����豸�����ж��м��壬��¼֧�ֵĶ�������
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        
        uint32_t queueFamilyCount = 0;
        // ��ȡ���м�������
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        // ��ȡ���ж��м��������
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        // ����ÿ�����м��壬����Ƿ�֧��ͼ�β���
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;

                // ��鵱ǰ���м����Ƿ�֧�ֳ��֣�Present�������ڱ���
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

                // ���֧�ֳ��֣����¼���ֶ��м�������
                if (presentSupport) {
                    indices.presentFamily = i;
                }

                // ������ҵ�������Ҫ�Ķ��м��壬��ǰ�˳�ѭ��
                if (indices.isComplete()) {
                    break;
                }
            }
            i++;
        }

        return indices;
    }

    // ��ѯ�����豸�Խ�������Swapchain����֧��ϸ�ڣ�����������������ʽ�ͳ���ģʽ
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        // ��ȡ���������������ͼ��������Χ����С/���ߴ磩
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // ��ȡ֧�ֵı����ʽ������ɫ�ռ䡢���ظ�ʽ��
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount); // Ԥ�����ڴ�
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // ��ȡ֧�ֵĳ���ģʽ���紹ֱͬ��������ģʽ��
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details; // ���ؽ�����֧�ֵ�������Ϣ
    }

    // ѡ�񽻻�������ģʽ
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    // ������չʾģʽ
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // ���1����ǰ��Χ�ѱ���ȷָ����ֱ��ʹ�ã� 
        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
            return capabilities.currentExtent;
        }
        else {// ���2�����ڹ����������Զ��巶Χ�����ֶ����㣩
            // ��ȡ���ڵ����طֱ��ʣ����� Retina ��ʾ����ʵ�����ؿ��ܸ��ߣ�
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // ���ֱ���������Ӳ��֧�ֵķ�Χ�ڣ�����Խ�磩
            actualExtent.width = std::clamp(actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);

            actualExtent.height = std::clamp(actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // ���Իص���������̬��Ա������
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // ��Ϣ���س̶�
        VkDebugUtilsMessageTypeFlagsEXT messageType,                // ��Ϣ����
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  // ������Ϣ����
        void* pUserData) {                                          // �û��Զ�������
        // ��������Ϣ�������׼������
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        // ����VK_FALSE��ʾ������ֹ�������С�
        return VK_FALSE;
    }
private:
    GLFWwindow* window;                                 // GLFW���ھ��
    VkInstance instance;                                // Vulkanʵ��
    VkDebugUtilsMessengerEXT debugMessenger;            // ���Իص���ʹ
    VkSurfaceKHR surface;                               // ���ڱ��� 
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // �����豸���
    VkDevice device;                                    // �߼��豸���
    VkQueue graphicsQueue;                              // ͼ�ζ���
    VkQueue presentQueue;                               // չʾ����
    VkSwapchainKHR swapChain;                           // ������
    std::vector<VkImage> swapChainImages;               // ������ͼƬ
    VkFormat swapChainImageFormat;                      // ���ظ�ʽ
    VkExtent2D swapChainExtent;                         // �ֱ���
    std::vector<VkImageView> swapChainImageViews;       // ��������ͼ����ͼ
};

// �������
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