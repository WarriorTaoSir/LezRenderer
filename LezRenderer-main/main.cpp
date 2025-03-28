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
#include <cstdint>      // Necessary for uint32_t
#include <limits>       // Necessary for std::numeric_limits
#include <algorithm>    // Necessary for std::clamp
#include <fstream>

// ���ڳߴ糣��
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

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
    VkRenderPass renderPass;                            // ��Ⱦͨ��

    VkPipelineLayout pipelineLayout;                    // ���߲���
    VkPipeline graphicsPipeline;                        // ͼ�ι���
    std::vector<VkFramebuffer> swapChainFramebuffers;   // ������֡����
    VkCommandPool commandPool;                          // ������
    std::vector<VkCommandBuffer> commandBuffers;        // �����

    // ͬ�����
    std::vector<VkSemaphore> imageAvailableSemaphores;  // �����ж��Ƿ���Դӽ������л�ȡͼ��
    std::vector<VkSemaphore> renderFinishedSemaphores;  // �����ж���һ֡��Ⱦ�Ƿ����
    std::vector<VkFence> inFlightFences;                // ȷ��һ��ֻ��Ⱦһ֡����

    bool framebufferResized = false;                    // ֡�����Ƿ񱻸��Ĵ�С
    uint32_t currentFrame = 0;

private:
    // ��ʼ��GLFW����
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // ��ʹ��OpenGL������

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    // ��ʼ��Vulkan�������
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    // ���¼�ѭ��
    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        // �ȴ������첽�����������ٽ�����ѭ��
        vkDeviceWaitIdle(device);
    }

    // �ͷ���Դ
    void cleanup() {
        cleanupSwapChain();

        // ����ͼ�ι���
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        // ���ٹ��߲���
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        // ������Ⱦͨ��
        vkDestroyRenderPass(device, renderPass, nullptr);

        // �����ź���
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr); // ����������

        vkDestroyDevice(device, nullptr);                   // �����߼��豸

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
    void createImageViews() {
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

    // ������Ⱦͨ��
    void createRenderPass() {
        // ������ɫ������Color Attachment��
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;          // ��Ҫ�뽻�����е�ͼ���ʽƥ��
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;        // ��ʱû�õ����ز��� 1sample����

        // ������ɫ�����ļ���/�洢����
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // ��Ⱦǰ���
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // ��Ⱦ�󱣴�

        // ����ģ�帽���ļ���/�洢��������ǰδʹ��ģ�壬��ΪDONT_CARE��
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // ������Ⱦ��ʼ�ͽ���ʱ�����Ĳ���
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // ���������ݻ�
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // ������ʾ

        // ������ɫ���������ã������ӹ��̣�
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // �����ӹ��̣�Subpass��
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // ָ��Ϊͼ�ι��ߣ��Ǽ�����ߣ�
        subpass.colorAttachmentCount = 1;                               // ʹ�õ���ɫ��������
        subpass.pColorAttachments = &colorAttachmentRef;

        
        // ������ϵ�����ý��ͣ�
        // 1. ȷ����Ⱦͨ����ʼǰ�ȴ�ͼ����ã�ͨ�� imageAvailableSemaphore��
        // 2. ����ͼ�񲼾ִ� UNDEFINED �Զ�ת��Ϊ COLOR_ATTACHMENT_OPTIMAL ��ʱ��
        // 3. ��ֹ��ͼ��δ׼����ʱ������Ⱦ���������ݾ�����

        // ������Ⱦͨ����Ϣ�ṹ��
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                // Դ����Ⱦͨ����ʼǰ����ʽ�������罻������ȡͼ��
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // Ŀ�꣺��һ����Ψһ�������̣�ʵ����Ⱦ������
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // ������Ⱦͨ����Ϣ�ṹ��
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;   // �ṹ�����ͱ�ʶ
        renderPassInfo.attachmentCount = 1;                                 // ������������ǰ����ɫ������
        renderPassInfo.pAttachments = &colorAttachment;                     // ָ�򸽼���������
        renderPassInfo.subpassCount = 1;                                    // �ӹ�����������ǰ��1����
        renderPassInfo.pSubpasses = &subpass;                               // ָ���ӹ�����������
        renderPassInfo.dependencyCount = 1;                                 // ������ϵ����
        renderPassInfo.pDependencies = &dependency;                         // ָ��������ϵ����

        // ������Ⱦͨ��
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    // ����ͼ�ι���
    void createGraphicsPipeline() {
        // 1. ��ȡԤ����Ķ����Ƭ����ɫ���ֽ��루SPIR-V��ʽ��
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        // 2. ������ɫ��ģ�飨���ڷ�װSPIR-V���룩
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // 3. ���ö�����ɫ���׶���Ϣ
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;                             // ָ��Ϊ������ɫ���׶�
        vertShaderStageInfo.module = vertShaderModule;                                      // ����������ɫ��ģ��
        vertShaderStageInfo.pName = "main";

        // 4. ����ƬԪ��ɫ���׶���Ϣ
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;                           // ָ��ΪƬ����ɫ���׶�
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // 5. ���������ɫ���׶Σ�˳���������Ⱦ����һ�£�
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // 6. ���ö���������������ǰΪ�գ������ʵ�ʶ������ݽṹ�޸ģ�
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        // 7. ��������װ�䷽ʽ���������б�
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;                       // �������б�ģʽ
        inputAssembly.primitiveRestartEnable = VK_FALSE;                                    // ����ͼԪ�����������������壩
            
        // 8. �����ӿںͲü����Σ�ʹ�ö�̬״̬��������Ⱦʱ���ã�
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 9. ���ù�դ������
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 10. ���ö��ز�������ǰ���ã�
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;                                       // ���ò�����ɫ������ݣ�
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;                         // ������=1

        // 11. ������ɫ��ϸ���������������ǰ���û�ϣ�
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        // 12. ����ȫ����ɫ���״̬
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

        // 13. ���ö�̬״̬����������ʱ�޸��ӿںͲü���
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 14. �������߲��֣���ǰ���������������ͳ�����
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // 15. ��װͼ�ι��ߴ�����Ϣ
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;                            // ��ɫ���׶�����������+Ƭ�Σ�
        pipelineInfo.pStages = shaderStages;                    // ָ����ɫ���׶�����

        // �󶨸��׶�״̬����
        pipelineInfo.pVertexInputState = &vertexInputInfo;      // ������������
        pipelineInfo.pInputAssemblyState = &inputAssembly;      // ����װ������
        pipelineInfo.pViewportState = &viewportState;           // �ӿ�״̬
        pipelineInfo.pRasterizationState = &rasterizer;         // ��դ������
        pipelineInfo.pMultisampleState = &multisampling;        // ���ز�������
        pipelineInfo.pDepthStencilState = nullptr;              // ���/ģ��������ã���ѡ��
        pipelineInfo.pColorBlendState = &colorBlending;         // ��ɫ�������
        pipelineInfo.pDynamicState = &dynamicState;             // ��̬״̬����

        // �������߲��ֺ���Ⱦͨ��
        pipelineInfo.layout = pipelineLayout;                   // ���߲��ֶ���
        pipelineInfo.renderPass = renderPass;                   // ��Ⱦͨ������
        pipelineInfo.subpass = 0;                               // ʹ�õ��ӹ�������

        // ���߼̳����ã������Ż����ߴ������ܣ�
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;       // �������߾�����޼̳У�
        pipelineInfo.basePipelineIndex = -1;                    // ���������������޼̳У�

        // 16. ����ͼ����Ⱦ����
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // 17. ������ɫ��ģ�飨Ӧ�ڹ��ߴ����ɹ���ִ�У�
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    // ����֡����
    void createFramebuffers() {
        // ���ݽ�����ͼ����������¾���֡�������
        swapChainFramebuffers.resize(swapChainImageViews.size());

        // ���δ���Frame buffer
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

    // ���������أ����ڷ���͹������������
    void createCommandPool() {
        // ��ȡ�����豸�Ķ��м���������ȷ��ͼ�ζ��м��壩
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        // ��������ش�������
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;            // �ṹ�����ͱ�ʶ
        // ������������������ã��������������أ�
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;   
        // ָ��������ͼ�ζ��м��壨����������ύ���˶��У�
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        // ���������
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    // ����������������ڼ�¼���ύ��Ⱦָ�GPU��
    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        // ������������������
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;   // �ṹ�����ͱ�ʶ
        
        // ָ�����ĸ�������з��仺����������ǰ������
        allocInfo.commandPool = commandPool;

        // ָ��Ϊ"���������"����ֱ���ύ������ִ�У��μ�������������������������
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size(); // ������������ǰ����2��������֡���д������չΪ�����

        // ��������з����������
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    // ¼����������ľ���ָ�������Ⱦ�������̣�
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        // --- �׶�1����ʼ¼��������� ---
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;  // �ṹ�����ͱ�ʶ
        beginInfo.flags = 0;                                            // ������ʹ�ñ�־����ͨ¼��ģʽ��
        beginInfo.pInheritanceInfo = nullptr;                           // �μ��������̳���Ϣ�����������������ã�

        // ��ʼ¼��������������ư���¼������
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // --- �׶�2�����ò�������Ⱦ���� ---
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;    // �ṹ�����ͱ�ʶ
        renderPassInfo.renderPass = renderPass;                             // ָ��ʹ�õ���Ⱦ����
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];     // �󶨵�ǰ֡��֡����

        renderPassInfo.renderArea.offset = { 0, 0 };                        // ��Ⱦ������㣨���Ͻǣ�
        renderPassInfo.renderArea.extent = swapChainExtent;                 // ��Ⱦ����ߴ磨�����������ڣ�

        // ���������ɫ����ɫ��RGBA=0,0,0,1��
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;                                 // ���ֵ����������ɫ������
        renderPassInfo.pClearValues = &clearColor;                          // ָ�������ɫ����

        // ������Ⱦ���̣����ƿ�ʼһ�����Ļ��ƣ�
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // --- �׶�3����ͼ�ι��� ---
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // --- �׶�4�������ӿڣ������������---
        VkViewport viewport{};                                      
        viewport.x = 0.0f;                                      // �ӿ����X����
        viewport.y = 0.0f;                                      // �ӿ����Y���� 
        viewport.width = (float)swapChainExtent.width;          // �ӿڿ�ȣ��봰��ͬ��
        viewport.height = (float)swapChainExtent.height;        // �ӿڸ߶ȣ��봰��ͬ�ߣ�
        viewport.minDepth = 0.0f;                               // ��С���ֵ��0-1��Χ��
        viewport.maxDepth = 1.0f;                               // ������ֵ
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);       // Ӧ���ӿ�����

        // --- �׶�5�����òü��������ƻ��Ʒ�Χ��---
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };                              // �ü��������
        scissor.extent = swapChainExtent;                       // �ü�����ߴ磨�����������ڣ�
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);         // Ӧ�òü�����

        // --- �׶�6����������ָ�� ---
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        // �������ͣ�
        // vertexCount: 3������3�����㣬���������Σ�
        // instanceCount: 1��������ʵ������Ⱦ��
        // firstVertex: 0���Ӷ��㻺���0λ�ÿ�ʼ��
        // firstInstance: 0��ʵ��ID��0��ʼ��

        // --- �׶�7��������Ⱦ���� ---
        vkCmdEndRenderPass(commandBuffer);

        // --- �׶�8�������������¼�� ---
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // ���¼�ƣ�����ֹͣ¼������ʱ��������Ѱ�������ָ�����У�
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
        // ��С��
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

    // ��������ͬ�����ź���
    void createSyncObjects() {
        // ��ʼ�������С
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // һ��������3���ź���
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }

    // ����֡
    void drawFrame() {
        // ����1���ȴ�ǰһ֡��Ⱦ��ɣ�������Դ������
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        // ����2���ӽ�������ȡ��һ֡ͼ��ȷ����ȾĿ�꣩��ͬʱ�жϽ������Ƿ��ʱ
        VkResult result = vkAcquireNextImageKHR(
            device,
            swapChain,
            UINT64_MAX,                                 // ��ʱʱ�䣨���޵ȴ���
            imageAvailableSemaphores[currentFrame],     // �ź�����ͼ���ȡ��ɺ󴥷�
            VK_NULL_HANDLE,                             // ��ʹ��դ�����˴����ź���ͬ����
            &imageIndex                                 // �����������ͼ�������
        );
        // �����������ʱ�����ý�������ֱ�ӷ���
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // ����3������դ��״̬��Ϊ��ǰ֡��׼����
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        
        // ����4�����ò�¼�����������������Ⱦ������
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);             // ������������������ڴ棩
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);     // ¼�ƻ���ָ��󶨹��ߡ��ӿڵȣ�

        // ����5�����������ύ��Ϣ��ͬ��GPU����˳��
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // �ȴ��׶����ź�����
        // - �ȴ� imageAvailableSemaphore��ȷ��ͼ���ѻ�ȡ��
        // - �ȴ��׶Σ���ɫ��������׶Σ�����ʼ��Ⱦǰ�ȴ���
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;        // �ӽ������л�ȡ��ͼ��-�ź���
        submitInfo.pWaitDstStageMask = waitStages;          

        // �ύ�����������������֡����Ⱦָ�
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        // �����ź�������Ⱦ��ɺ󴥷� renderFinishedSemaphore
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;    // ͼ����Ⱦ���-�ź���

        // ����6���ύ���ͼ�ζ��У�����GPU��Ⱦ��
        // - graphicsQueue: Ŀ����У�ͼ�β������У�
        // - inFlightFence: �ύ�������դ������Ⱦ��ɺ󴥷���
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // ����7������ͼ�������Ϣ���ύ�����ֶ��У������ؼ����֣�
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; // �ṹ�����ͱ�ʶ

        // �ȴ��ź�������Ⱦ����ź�����ȷ��ͼ����Ⱦ��Ϻ��ٳ��֣�
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;         // ʹ�ò���6�д�����renderFinishedSemaphore

        // ָ����������ͼ������
        VkSwapchainKHR swapChains[] = { swapChain };            // Ŀ�꽻��������ǰ����Ľ�������
        presentInfo.swapchainCount = 1;                         // ����������
        presentInfo.pSwapchains = swapChains;                   // ָ�򽻻�������
        presentInfo.pImageIndices = &imageIndex;                // Ҫ���ֵĽ�����ͼ������������3�л�ȡ�ģ�

        // �ύ�������󣨽���Ⱦ��ɵ�ͼ����ʾ����Ļ��
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // �����ֽ��
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain(); // �ؽ�������
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // ����CurrentFrame
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // ���� shader ģ��
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        // ��shader module�йصĴ�����Ϣ
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        // ����shader module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
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

    // ѡ�񽻻���չʾģʽ
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // ѡ�񽻻����ֱ���
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

    // �����ļ������ļ�
    static std::vector<char> readFile(const std::string& filename) {
        // ��ĩβ����ate������Ϊ�������ļ�
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        // ͨ����ĩβ��ָ��λ�ã�ֱ�ӻ���ļ���С�����ֽ���
        size_t fileSize = (size_t)file.tellg();
        // �����ڴ滺����
        std::vector<char> buffer(fileSize);

        // ��ȡ�ļ�����
        file.seekg(0);                          // ���ļ�ָ���ƻؿ�ͷ
        file.read(buffer.data(), fileSize);     // ��ȡȫ�����ݵ�������

        // �ر��ļ�
        file.close();

        return buffer;
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

    // ֡�����С���ġ����ص�����
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
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