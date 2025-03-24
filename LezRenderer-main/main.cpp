#define GLFW_INCLUDE_VULKAN // ��GLFW�Զ�����Vulkanͷ�ļ�
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>

// ���ڳߴ糣��
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// ���õ���֤���б�Khronos�ٷ���֤�㣩
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
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

    // ������б���Ķ��м����Ƿ����ҵ�
    bool isComplete() {
        return graphicsFamily.has_value(); // ��ǰ����ͼ�ζ��м�����Ϊ��������
    }
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
        pickPhysicalDevice();
        createLogicalDevice();
    }

    // ���¼�ѭ��
    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // �ͷ���Դ
    void cleanup() {
        vkDestroyDevice(device, nullptr);           // �����߼��豸

        // �����������֤��
        if (enableValidationLayers) {
            // �ݻٴ����ĵ���Messenger
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);   // �����ٵ�����ʹ
        }

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

    // �����߼��豸
    void createLogicalDevice() {
        // ��ȡ�����豸֧�ֵĶ��м�������������ͼ�ζ��м��壩
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // ���ö��д�����Ϣ�����ﴴ��ͼ�ζ��У�
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        // ���ö������ȼ�����Χ0.0~1.0��Ӱ��GPU��Դ���䣩
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // ��ʼ���豸�������󣨴˴�δ�����κ����⹦�ܣ�
        VkPhysicalDeviceFeatures deviceFeatures{};

        // �����߼��豸������Ϣ
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;

        // �����߼��豸���������豸�������������
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        // ��ȡ�豸�д�����ͼ�ζ��о�������������ύ��ͼ���
        // ����˵�����豸����, ���м�������, ����������ͬһ�����п����ж�����У���������о��
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
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

    // ����豸�Ƿ��������Ҫ�󣨶��м����Ƿ�������
    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device); // ���Ҷ��м���

        return indices.isComplete(); // �Ƿ����������б���Ķ��У�Ŀǰֻ��ͼ�ζ��У�
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
                if (indices.isComplete()) {
                    break;
                }
            }
            i++;
        }

        return indices;
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
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // �����豸���
    VkDevice device;                                    // �߼��豸���
    VkQueue graphicsQueue;                              // ͼ�ζ���
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