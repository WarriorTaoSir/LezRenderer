#define GLFW_INCLUDE_VULKAN // ��GLFW�Զ�����Vulkanͷ�ļ�
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS                   // ͳһʹ��radiance
#define GLM_FORCE_DEPTH_ZERO_TO_ONE         // �����������0��1

#include <glm/glm.hpp>                      // 
#include <glm/gtc/matrix_transform.hpp>     // ����ģ�ͱ任�Ⱥ���

// ͼ���ȡ
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      
// ģ�ͼ���
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

#include <chrono>       // �����˽��о�׼��ʱ�ĺ�����

// ���ڳߴ糣��
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

// ��Դ·��
const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
//const std::string MODEL_PATH = "models/revolver.obj";
//const std::string TEXTURE_PATH = "textures/revolver_basecolor.png";
//const std::string MODEL_PATH = "models/python.obj";
//const std::string TEXTURE_PATH = "textures/python_basecolor.jpg";

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
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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

// �������ݸ�ʽ���壨����λ�ú���ɫ��
struct Vertex {
    glm::vec3 pos;      // λ�����ԣ�3D���꣬��Ӧ������ɫ���� location=0��
    glm::vec3 color;    // ��ɫ���ԣ�RGB����Ӧ������ɫ���� location=1��
    glm::vec2 texCoord; // UV����

    // ���ɶ������ݰ�������Vulkan��Ҫ����Ϣ��ȡ�ڴ��еĶ������ݣ�
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;                                 // ����������0��ʼ��������ʱͨ��Ϊ0��
        bindingDescription.stride = sizeof(Vertex);                     // ÿ���������ݵ����ֽڴ�С���Զ����㣩
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;     // �����㲽������ʵ������

        return bindingDescription;
    }

    // ���ɶ�����������������ÿ�����Եĸ�ʽ��λ�ã�
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        // λ�����ԣ�pos��������
        attributeDescriptions[0].binding = 0;                               // ���������������һ�£�
        attributeDescriptions[0].location = 0;                              // ������ɫ��������λ�ã�location=0��
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;       // ���ݸ�ʽ��vec3��3��32λ������
        attributeDescriptions[0].offset = offsetof(Vertex, pos);            // ƫ������pos�ڽṹ���е���ʼλ�ã�

        // ������ɫ���ԣ�color��
        attributeDescriptions[1].binding = 0;                           // ͬһ�󶨣���������ͬһ��������
        attributeDescriptions[1].location = 1;                          // ������ɫ��������λ�ã�location=1��
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // ���ݸ�ʽ��vec3��3��32λ������
        attributeDescriptions[1].offset = offsetof(Vertex, color);      // ƫ������color����ʼλ�ã�

        // ����UV���ԣ�texcoord��
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

// Ϊ�Զ���Vertex�ṹ���ػ���׼��Ĺ�ϣģ��
// ��;��ʹVertex�������Ϊstd::unordered_map�ļ�
namespace std {
    template<> struct hash<Vertex> {
        // ��ϸ������ԵĹ�ϣֵ������Ψһ�Խϸߵĸ��Ϲ�ϣ
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}// ���붨����std�����ռ���

// ������������
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

// ������������
// uint16_t�ʺ�����65535��������ģ��
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

// ȫ�ֱ���
struct UniformBufferObject {

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
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

    VkDescriptorSetLayout descriptorSetLayout;          // ������������
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

    // ģ�͵Ķ���������
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Vulkan������
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;

    // mipmap�ȼ���
    uint32_t mipLevels;
    // Multi-Sampling
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage textureImage;
    VkImage depthImage;

    // Vulkan�����Ӧ���豸�ڴ�
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;
    VkDeviceMemory textureImageMemory;
    VkDeviceMemory depthImageMemory;

    // ��ͼ�������
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkImageView depthImageView;

    // UBO
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    VkDescriptorPool descriptorPool;                    // ��������
    std::vector<VkDescriptorSet> descriptorSets;        // �������� ����

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

        // ����UBO
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        // ��������������
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        // ������ͼ������ �� ͼ����ͼ
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);

        // ����ͼƬ�Լ���ӦDevice Memory
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        // ����������������
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        // ��������Buffer�Լ����ն�ӡ�ڴ�
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        // ���ٶ���Buffer�Լ����ն�ӡ�ڴ�
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

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
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;  // ԭ����ǿ��ת������Ҫ��ֱ��ȡ��ַ����
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
                msaaSamples = getMaxUsableSampleCount();
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
        deviceFeatures.samplerAnisotropy = VK_TRUE; // �������Բ���
        deviceFeatures.sampleRateShading = VK_TRUE; // �����豸��sample shading

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
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    // ������Ⱦͨ��
    void createRenderPass() {
        // ������ɫ������Color Attachment��
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;          // ��Ҫ�뽻�����е�ͼ���ʽƥ��
        colorAttachment.samples = msaaSamples;                  // ���ز���

        // ������ɫ�����ļ���/�洢����
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // ��Ⱦǰ���
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // ��Ⱦ�󱣴�

        // ����ģ�帽���ļ���/�洢��������ǰδʹ��ģ�壬��ΪDONT_CARE��
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // ������Ⱦ��ʼ�ͽ���ʱ�����Ĳ���
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // ���������ݻ�
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;      // ������ʾ

        // ������ɫ���������ã������ӹ��̣�
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // ����������ȸ���
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        // ������ȸ��������ã������ӹ��̣�
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // ������ɫ�������������ԣ����ڶ��ز�������ݣ�MSAA���Ľ�������
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = swapChainImageFormat;                       // ʹ�ý�����ͬ���ʽ��ȷ�����ݳ���
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;                     // ����������ΪMSAA����Ŀ�꣩
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;            // ������س�ʼ���ݣ�ÿ�θ��ǣ�
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;              // ����洢�������������������ʹ�ã�
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;     // ��ģ�帽��������
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;           // ��ʼ���������壨�ɶ�����
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;       // ����ת��Ϊ�����Ż�����

        // ������ɫ�������������ã������ӹ��̣�
        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // �����ӹ��̣�Subpass��
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // ָ��Ϊͼ�ι��ߣ��Ǽ�����ߣ�
        subpass.colorAttachmentCount = 1;                               // ʹ�õ���ɫ��������
        subpass.pColorAttachments = &colorAttachmentRef;                // ��ɫ��������
        subpass.pDepthStencilAttachment = &depthAttachmentRef;          // ���ģ�帽������
        subpass.pResolveAttachments = &colorAttachmentResolveRef;       // ��ɫ������������

        // ������ϵ�����ý��ͣ�
        // 1. ȷ����Ⱦͨ����ʼǰ�ȴ�ͼ����ã�ͨ�� imageAvailableSemaphore��
        // 2. ����ͼ�񲼾ִ� UNDEFINED �Զ�ת��Ϊ COLOR_ATTACHMENT_OPTIMAL ��ʱ��
        // 3. ��ֹ��ͼ��δ׼����ʱ������Ⱦ���������ݾ�����

        // ������Ⱦͨ����Ϣ�ṹ��
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                // Դ����Ⱦͨ����ʼǰ����ʽ�������罻������ȡͼ��
        dependency.dstSubpass = 0;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
        // ������Ⱦͨ����Ϣ�ṹ��
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;   // �ṹ�����ͱ�ʶ
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());// ��������
        renderPassInfo.pAttachments = attachments.data();                   // ָ�򸽼���������
        renderPassInfo.subpassCount = 1;                                    // �ӹ�����������ǰ��1����
        renderPassInfo.pSubpasses = &subpass;                               // ָ���ӹ�����������
        renderPassInfo.dependencyCount = 1;                                 // ������ϵ����
        renderPassInfo.pDependencies = &dependency;                         // ָ��������ϵ����

        // ������Ⱦͨ��
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    // ���������� �� ����
    void createDescriptorSetLayout() {
        // UBO ���ְ�
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;                       // ��λ�ã�����ɫ���е� layout(binding=0) ��Ӧ��
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // ���ͣ�UBO
        uboLayoutBinding.descriptorCount = 1;               // UBO �������˴�Ϊ������Ҳ���������飩
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   // ��Ч����ɫ���׶Σ�������ɫ����
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // 2D ��ͼ������ ���ְ�
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // ��ƬԪ��ɫ����Ч

        // ������
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

        // ���������������� createInfo
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        // ����������������
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
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

        // 6. ���ö�����������������ʵ�ʶ������ݽṹ�޸ģ�
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // ��ȡ�������ݵİ���������������
        auto bindingDescription = Vertex::getBindingDescription();          // �����������ݲ��֣�
        auto attributeDescriptions = Vertex::getAttributeDescriptions();    // ����������ÿ���ֶε�ϸ�ڣ�

        // ���ö�������״̬
        vertexInputInfo.vertexBindingDescriptionCount = 1;                                  // ������������������������
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());  // �����������˴�Ϊ2��
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;                   // ����������ָ��
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();        // ������������ָ��

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
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 10. ���ö��ز�������ǰ���ã�
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;                                       // ���ò�����ɫ������ݣ�
        multisampling.rasterizationSamples = msaaSamples;                                   // ������
        multisampling.minSampleShading = .2f; 

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

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
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

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
        pipelineInfo.pDepthStencilState = &depthStencil;        // ���/ģ���������
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

    // ����������֡�������ĺ���
    // ���ܣ�Ϊÿ��������ͼ����ͼ������Ӧ��֡������������ɫ������������ͼ�񣩺���ȸ���
    void createFramebuffers() {
        // ���ݽ�����ͼ����ͼ��������֡�����������С��ÿ��������ͼ���Ӧһ��֡��������
        swapChainFramebuffers.resize(swapChainImageViews.size());

        // �������н�����ͼ����ͼ���������֡������
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            // ����֡�������������飨��������Ⱦͨ�������ĸ���˳��һ�£�
            // [0] ��ɫ��������ǰ������ͼ����ͼ��VK_ATTACHMENT_LOAD_OP_CLEAR ������Ŀ�꣩
            // [1] ��ȸ��������ͼ����ͼ��������Ȳ��ԣ�
            std::array<VkImageView, 3> attachments = {
                colorImageView,
                depthImageView,
                swapChainImageViews[i]
            };

            // ����֡������������Ϣ
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;                    // ��������Ⱦͨ���������������ʹ�ã�
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());    // ��������
            framebufferInfo.pAttachments = attachments.data();          // ������ͼ����ָ��
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            // ����֡����������
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

    // ���������Դ
    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    
    }

    // ����Vulkanͼ����󲢷����ڴ�
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        // 1. ����ͼ�񴴽���Ϣ
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;  // �ṹ�����ͱ�ʶ
        imageInfo.imageType = VK_IMAGE_TYPE_2D;                 // 2D��������1D/3D���ͣ�
        imageInfo.extent.width = width;                         // ͼ����
        imageInfo.extent.height = height;                       // ͼ��߶�
        imageInfo.extent.depth = 1;                             // ��ȣ�3Dͼ��ʱ��>1��
        imageInfo.mipLevels = mipLevels;                        // Mipmap�㼶��
        imageInfo.arrayLayers = 1;                              // ͼ����������������������飩
        imageInfo.format = format;                              // ���ظ�ʽ�������������ͼһ��
        imageInfo.tiling = tiling;                              // �ڴ沼�ַ�ʽ��
                                                                // LINEAR-�������У�����CPU���ʣ�
                                                                // OPTIMAL-Ӳ���Ż����У�GPU��Ч��
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    // ��ʼ���֣�������ת����
        imageInfo.usage = usage;                                // ͼ����;��ϣ����䡢�����ȣ�
        imageInfo.samples = numSamples;                         // ���ز�����
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;      // ��ռ����ģʽ���������й���

        // 2. ����ͼ�����
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // 3. ��ѯͼ���ڴ����󣨻�ȡ��Ҫ������ڴ��С�Ͷ���Ҫ��
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        // 4. �����ڴ������Ϣ
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        // Ѱ�ҷ���Ҫ����ڴ����ͣ����豸�����ڴ棩
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // 5. �����豸�ڴ�
        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        // 6. ���ڴ�󶨵�ͼ���������OpenGL��glBindBuffer��
        vkBindImageMemory(device, image, imageMemory, 0);
    }

    // ����������ͼ�ĺ��ĺ���
    // ���̣�����ͼƬ�ļ� �� �ݴ浽CPU�ɼ������� �� ���Ƶ�GPUר��ͼ�� �� ת��ͼ�񲼾ֹ���ɫ��ʹ��
    void createTextureImage() {
        // 1. ����ͼƬ�ļ�
        int texWidth, texHeight, texChannels;
        // ʹ��stb_image�����JPGͼƬ��ǿ��ת��ΪRGBA��ʽ��4ͨ����
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        VkDeviceSize imageSize = texWidth * texHeight * 4;

        // ���ͼƬ�����Ƿ�ɹ�
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        // 2. �����ݴ滺������Staging Buffer��
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        // ����CPU�ɼ��Ļ�����
        // ����CPU�ɼ�����ʱ��������
        // - ��;����Ϊ����Դ��TRANSFER_SRC_BIT��
        // - �ڴ����ԣ������ɼ�����ӳ�䣩 + ������ɣ��Զ�ͬ��CPU/GPU�ڴ棩
        createBuffer(imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // ����Ҫ��Ϊ����Դ
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // CPU�ɷ���
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // �Զ��ڴ�ͬ��
            stagingBuffer, stagingBufferMemory);

        // 3. ���������ݸ��Ƶ��ݴ滺����
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);// �ͷ�STBͼ�����ݣ�������ҪCPU��ԭʼ���ݣ�

        // 4. ����GPUר�õ�����ͼ��
        // - ��ʽ��R8G8B8A8_SRGB��sRGB��ɫ�ռ䣩
        // - ƽ�̷�ʽ��OPTIMAL��GPU�Ż����֣�CPU����ֱ�ӷ��ʣ�
        // - ��;������Ŀ�� + ��ɫ������
        // - �ڴ����ԣ�DEVICE_LOCAL��GPUר���ڴ棬�����ٶȿ죩
        createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
        
        // 5. ִ�����ݴ���Ͳ���ת�����̣�
        // (1) ת�����֣�UNDEFINED �� TRANSFER_DST_OPTIMAL��׼���������ݣ�
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        // (2) ���ݴ滺�����������ݵ�����ͼ��
            copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        // (3) ת�����֣�TRANSFER_DST_OPTIMAL �� SHADER_READ_ONLY_OPTIMAL����ɫ�������Ż���
        // transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

        // ����mipmap
        generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
        // 6. ������ʱ��Դ
        vkDestroyBuffer(device, stagingBuffer, nullptr);    // �����ݴ滺��������
        vkFreeMemory(device, stagingBufferMemory, nullptr); // �ͷ��ݴ滺�����ڴ�
    }

    // �������������Mipmap����ͨ���༶ͼ������(Blit)����ʵ�֣�ͬʱ����Vulkanͼ�񲼾�ת��
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // ���ͼ���ʽ�Ƿ�֧������blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        //1: ��ȡ�����������
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // ��ʼ����ǰ�����Mip�㼶�ߴ�
        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        //2�����û����ڴ����ϲ���
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;                                          // ������Ŀ��ͼ��
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;          // ��ת�ƶ�������Ȩ
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;// ��ɫͨ��
        barrier.subresourceRange.baseArrayLayer = 0;                    // ���������
        barrier.subresourceRange.layerCount = 1;                        // ��������
        barrier.subresourceRange.levelCount = 1;                        // ÿ�δ���1��Mip�㼶

        //3��������Mipmap
        for (uint32_t i = 1; i < mipLevels; i++) {
            // 3.1��ת��ǰ��Mip����ΪTRANSFER_SRC_OPTIMAL
            barrier.subresourceRange.baseMipLevel = i - 1;              // ����i-1��
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;       // �ȴ�ǰ��д�����
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;        // ׼����ΪBlitԴ

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,                 // �ȴ����д���������                
                VK_PIPELINE_STAGE_TRANSFER_BIT,                 // �ڴ���׶�ִ��
                0, 0, nullptr, 0, nullptr,1, &barrier);

            // 3.2������Blit��������
            VkImageBlit blit{};
            // Դ���򣨵�ǰMip����
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            // Ŀ��������һMip����
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;           // Ŀ��Mip��
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            // 3.3��ִ��Blit���Ų���
            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            // 3.4��ת��ǰ��MipΪSHADER_READ����
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,          // �ڴ���֮��
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,   // Ƭ����ɫ�׶ο���
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            // ���³ߴ�Ϊ��һ��
            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // 4���������һ��Mip
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

        // 5���ύ�������
        endSingleTimeCommands(commandBuffer);
    }

    // ��ȡ�����ò�����
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

    // ���������������������δ������л�ȡ��ɫ��
    // �������൱��GPU��"ȡɫ����˵����"������������α���ȡ�͹���
    void createTextureSampler() {
        // ��ȡ�����豸����
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // ����Ŵ�/��Сʱ�Ĺ��˷�ʽ��������ز����õ����⣩
        samplerInfo.magFilter = VK_FILTER_LINEAR;       // �Ŵ�ʱ�����Բ�ֵ��ƽ����
        samplerInfo.minFilter = VK_FILTER_LINEAR;       // ��Сʱͬ�������Բ�ֵ

        // �������곬��[0,1]ʱ�Ĵ���ʽ���������ǽֽ���������
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // U�����ظ�����
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // V�����ظ�����
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // W����3D�����ã�

        // �������Թ������ã�������б���������ģ����
        samplerInfo.anisotropyEnable = VK_TRUE;

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // ����Ӳ��֧�ֵ����ֵ
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;         // Խ��������ʾΪ��ɫ
        // �����Ƿ��һ����true��ʾʹ��ʵ���������꣬false��ʾ0-1��׼�����꣩
        samplerInfo.unnormalizedCoordinates = VK_FALSE;                     // ʹ�ñ�׼UV���꣨0��1��Χ��
        // ��ȱȽ����ã�������Ӱӳ��ȸ߼����ԣ�
        samplerInfo.compareEnable = VK_FALSE;                               // ��������ȱȽ�
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                       // �Ƚ���Զ����true����Ч����

        // Mipmap������ã��༶����ϸ�ڣ�
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;             // mipmapʹ�����Բ�ֵ
        samplerInfo.mipLodBias = 0.0f;                                      // mipmap����ƫ����
        samplerInfo.minLod = 0.0f;               // ��Сʹ�õ�mipmap����
        samplerInfo.maxLod = static_cast<float>(mipLevels);                 // ���ʹ�õ�mipmap����0��ʾֻ�û�������

        // ��������������
        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    // �ҵ�֧�����ŵ������ͼ��ʽ
    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    // ���������豸֧�ֵ�ͼ���ʽ
    // ��;�����������ƽ�̷�ʽ��Tiling�������ԣ�Features�����Ӻ�ѡ��ʽ�б���ѡ���һ�����õĸ�ʽ
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        // �������к�ѡ��ʽ�������÷���������ȼ�˳��
        for (VkFormat format : candidates) {
            // ��ȡ�����豸�Ըø�ʽ��֧������
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
            // �������ƽ�̣�LINEAR��ģʽ��֧�����
            // ���ó�������ҪCPU�ɷ��ʵ�ͼ�����ݴ�ͼ��
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            // ����Ż�ƽ�̣�OPTIMAL��ģʽ��֧�����
            // ���ó�����GPUר��ͼ����������Ȼ��壩
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        // ���к�ѡ��ʽ����֧��Ҫ������ԣ��׳�����
        throw std::runtime_error("failed to find supported format!");
    }

    // �жϸ�format�Ƿ���stencil����
    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    // helper function ����ͼ����ͼ
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        // ����ͼ����ͼ�Ĵ�������
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        // ����ͼ����ͼ�����ͺ͸�ʽ
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        // ��������Դ��Χ��������ͼ�ܷ���ͼ�����Щ���֣�
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

    // ��������ʼһ��һ����ʹ�õ��������
    // ���ó��������ڴ��ڵ�GPU�������紫�����ݣ���ִ�к���������
    VkCommandBuffer beginSingleTimeCommands() {
        // �������������Ϣ
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        // ������ط����������
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        // ���������ʼ��¼��Ϣ
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // ��ʼ���������¼״̬
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;// �����ѿ�ʼ��¼���������
    }

    // �ύ������һ�����������
    // ע�⣺������ʽ GPU-CPU ͬ����vkQueueWaitIdle���������ڵ�Ƶ����
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        // �������������¼
        vkEndCommandBuffer(commandBuffer);

        // �ύ��Ϣ����
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;                  // �ύ1���������
        submitInfo.pCommandBuffers = &commandBuffer;        // ָ��Ҫ�ύ���������

        // �ύ��ͼ�ζ��У����� graphicsQueue �ѳ�ʼ����
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);   
        // �ȴ�����ִ����ɣ�ǿ��ͬ����ȷ��������ɣ�
        vkQueueWaitIdle(graphicsQueue);                                 
        // �ͷ��������������أ�������������ر���
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    // ͼ�񲼾�ת������
    // ��;����ȫ�ظı�ͼ���Vulkan�ڴ沼�֣�ȷ��GPU��������ȷͬ��
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        // ��ȡһ����������������ڶ��ڴ��ڵĲ�����
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // ����ͼ���ڴ�����
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; // ��׼Vulkan�ṹ�����ͱ�ʶ
        barrier.oldLayout = oldLayout;                          // ��ǰ�ڴ沼��
        barrier.newLayout = newLayout;                          // Ŀ���ڴ沼��
        // ���²�����ʾ��ת�ƶ�������Ȩ��ͬһ���м���ʹ�ã�
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;                                  // Ŀ��ͼ�����
        // ����ͼ������Դ��Χ��Ӱ������ͼ��
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // ������߽׶κͷ������루ȷ����ȷ��ִ��˳����ڴ�ɼ��ԣ�
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        // ���1����δ��ʼ��״̬ -> ����Ŀ���Ż�����
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            // ����Ȩ�����ã�
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            // ���߽׶����ã�
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // ���2���Ӵ���Ŀ���Ż����� -> ��ɫ��ֻ���Ż�����
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // ����Ȩ�����ã�
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            // ���߽׶����ã�
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            // ��֧����������ת��������չ��Ӹ��������
            throw std::invalid_argument("unsupported layout transition!");
        }

        // �ύ������������
        vkCmdPipelineBarrier(
            commandBuffer,                   // �������
            sourceStage, destinationStage,   // Դ��Ŀ����߽׶�
            0,                               // ������־��0��ʾ������������
            0, nullptr,                      // �ڴ��������������ݣ��˴�δʹ�ã�
            0, nullptr,                      // �������ڴ��������������ݣ��˴�δʹ�ã�
            1, &barrier                      // ͼ���ڴ���������������
        );

        // �ύ��ִ�����������������ʽ�ȴ�������
        endSingleTimeCommands(commandBuffer);
    }

    // �����������ݸ��Ƶ�ͼ��ĸ�������
    // ��;�����ڽ�CPU/�������е��������ݣ����������䵽GPUͼ�����
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        // ��ȡһ������������������ڶ��ڲ�����
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        // ���û�������ͼ��ĸ���������Ϣ
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        // ����Ŀ��ͼ�������Դ��Ϣ
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        // ����ͼ��д��λ�úͷ�Χ
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        // �ύ��������ͼ��ĸ�������
        // ע�⣺Ŀ��ͼ����봦�� VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ����
        vkCmdCopyBufferToImage(
            commandBuffer,                   // �������
            buffer,                          // Դ������
            image,                           // Ŀ��ͼ��
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Ŀ��ͼ��Ĳ��֣�����Ϊ�˲��֣�
            1,                               // ��������
            &region                          // ��������ָ��
        );

        // �ύ���ȴ�����ִ����ɣ�ȷ�����ݴ�����ϣ�
        endSingleTimeCommands(commandBuffer);
    }
    // ����Buffer�� ���ߺ���
    // ����˵������������С���ֽڣ�����������;�����㻺�塢���仺�壩���ڴ����ԡ�����Ļ���������������豸�ڴ�
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;         // ��ռ���ʣ���һ�����м���ʹ�ã�

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);    // ��ѯ�ڴ�����

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);                // ���ڴ浽������
    }

    // ����OBJģ�����ݵ��������������
    // ���ܣ�����.obj�ļ�����ȡ�������ݣ����Զ�ȥ��������������
    void loadModel() {
        // ��ʼ��tinyobjloader���ݽṹ
        tinyobj::attrib_t attrib;                   // �洢�������ԣ�λ��/��������ȣ�
        std::vector<tinyobj::shape_t> shapes;       // �洢������״����
        std::vector<tinyobj::material_t> materials; // �������ݣ�δʹ�ã�
        std::string warn, err;                      // ���ع��̵ľ���/������Ϣ

        // ����OBJģ���ļ�
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        // ���������ϣ������ȥ�أ���ΪVertexʵ�ֹ�ϣ������
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        // ����ģ���е�������״��ͨ����Ӧmesh��
        for (const auto& shape : shapes) {
            // ������״������������
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                // ��ȡ����λ�ã�X/Y/Z���꣩
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                // ��ȡ�������������꣨��תV����������Vulkan����ϵ��
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]   // ��תһ��V����
                };

                // ����Ĭ�ϰ�ɫ������ɫ��Ӧ��Ϊ�Ӳ��ʶ�ȡ��
                vertex.color = { 1.0f, 1.0f, 1.0f };

                // ����ȥ�ش���������㲻�����ڹ�ϣ�������
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                // �������
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    // �������㻺�����������洢�������ݣ���λ�á���ɫ�ȣ�
    void createVertexBuffer() {
        // ���㶥���������ֽ��� = ���������С �� ��������
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // 1. �����ݴ滺������Staging Buffer��
        // -----------------------------------------------
        // - ���ã���ʱ�洢CPU�����ݣ�������GPU��Ч�ڴ洫������
        // - �ڴ����ԣ�CPU�ɼ���HOST_VISIBLE�� + �Զ�ͬ����HOST_COHERENT��
        // - ��;����Ϊ����Դ��TRANSFER_SRC�������������ݸ��Ƶ��豸���ػ�����
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        // 2. ���������ݴ�CPU���Ƶ��ݴ滺����
        // -----------------------------------------------
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);    // ӳ���ڴ棬��CPU�ܷ��ʣ����ơ��������ڴ棩
        memcpy(data, vertices.data(), (size_t)bufferSize);                   // ���ƶ������ݵ�ӳ����ڴ�
        vkUnmapMemory(device, stagingBufferMemory);                                                      // ���ӳ�䣨��ѡ������CPU���ɼ�������ΪCOHERENT���������Զ�ͬ����

        // 3. �����豸���ض��㻺���������մ洢λ�ã�
        // -----------------------------------------------
        // - ���ã��洢���ն������ݣ���GPU��Ч����
        // - �ڴ����ԣ��豸���أ�DEVICE_LOCAL����ͨ��λ���Դ棬CPU���ɼ�
        // - ��;����Ϊ����Ŀ�꣨TRANSFER_DST�� + �������ݣ�VERTEX_BUFFER��
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,   // ��;�����ݴ���Ŀ�� + ��������
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                                    // GPU��Ч�ڴ�
            vertexBuffer,
            vertexBufferMemory
        );

        // 4. �����ݴ��ݴ滺�������Ƶ��豸���ػ�����
        // -----------------------------------------------
        // - ʹ��GPU���vkCmdCopyBuffer�����豸�ڲ���ɸ�Ч����
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // 5. �����ݴ���Դ��������ת�ƣ�������Ҫ��
        // -----------------------------------------------
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

    }

    // �������������������ڴ洢�����������ݣ�������������˳��
    void createIndexBuffer() {
        // ���������������ֽ��� = ����������С �� ��������
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        // 1. �����ݴ滺������Staging Buffer��
        // -----------------------------------------------
        // - ���ã���ʱ�洢CPU���������ݣ�������ת��GPU��Ч�ڴ�
        // - �ڴ����ԣ�CPU�ɼ���HOST_VISIBLE�� + �Զ�ͬ����HOST_COHERENT��
        // - ��;�����ݴ���Դ��TRANSFER_SRC��
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // 2. ���������ݴ�CPU���Ƶ��ݴ滺����
        // -----------------------------------------------
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        // 3. �����豸�������������������մ洢λ�ã�
        // -----------------------------------------------
        // - ���ã��洢�����������ݣ���GPU��Ч����
        // - �ڴ����ԣ��豸���أ�DEVICE_LOCAL����ͨ��λ���Դ�
        // - ��;�����ݴ���Ŀ�꣨TRANSFER_DST�� + �������ݣ�INDEX_BUFFER��
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |      // ��;�����ݴ���Ŀ��
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,       // ��;������������
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // GPU��Ч�ڴ�
            indexBuffer,
            indexBufferMemory
        );

        // 4. �����ݴ��ݴ滺�������Ƶ��豸���ػ�����
        // -----------------------------------------------
        // - �ڲ�ʹ��vkCmdCopyBuffer�����GPUִ���첽���ٸ���
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        // 5. �����ݴ���Դ��������ת�ƣ�������Ҫ��
// -----------------------------------------------
        vkDestroyBuffer(device, stagingBuffer, nullptr);        // �����ݴ滺��������
        vkFreeMemory(device, stagingBufferMemory, nullptr);     // �ͷ��ݴ��ڴ�
    }

    // ����Uniform Buffer
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

    // ���������� ��
    void createDescriptorPool() {
        // ������������������ͺ�����
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                      // ���ͣ�Uniform Buffer
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // ������ÿ֡һ��
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;              // ���ͣ� Combined Image Sampler
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        // �����������ص�������Ϣ
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());           // ����������
        poolInfo.pPoolSizes = poolSizes.data();                                     // ָ���������õ�ָ��
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);             // ���ɷ����������������
    
        // ������������
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    // ���������� ��
    void createDescriptorSets() {
        // Ϊÿ֡������ͬ���ֵ���������
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;              // ���ĸ��ط���
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // ����������ÿ֡һ����
        allocInfo.pSetLayouts = layouts.data();                 // ����������������

        // ��������������ÿ������������Ӧһ֡��
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        // ����ÿ��������������Uniform Buffer��
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // ���������õ�Uniform Buffer��Ϣ
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];          // ��ǰ֡��Uniform Buffer
            bufferInfo.offset = 0;                          // ������ʼƫ����
            bufferInfo.range = sizeof(UniformBufferObject); // �����ܴ�С����VK_WHOLE_SIZE��

            // ������Ӧ�õ���ͼ��Ϣ
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            // ����������д�����
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];     // Ŀ��������������ǰ֡��
            descriptorWrites[0].dstBinding = 0;                 // �󶨵㣨����ɫ���е�binding=0��Ӧ��
            descriptorWrites[0].dstArrayElement = 0;            // ����������������ʱΪ0��
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // ���ͣ�UBO
            descriptorWrites[0].descriptorCount = 1;            // �󶨵�������������������
            descriptorWrites[0].pBufferInfo = &bufferInfo;      // ��������Ϣָ��

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            // ���������������������ύ��GPU��
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

    // ���ƻ��������ݣ���Դ��������Ŀ�껺������
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();  // ��ʼ¼��

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;       // Դ��������ʼƫ�ƣ���ѡ��Ĭ��0��
        copyRegion.dstOffset = 0;       // Ŀ�껺������ʼƫ�ƣ���ѡ��Ĭ��0��
        copyRegion.size = size;         // ���Ƶ������ܴ�С

        vkCmdCopyBuffer(
            commandBuffer,       // ��ǰ�������
            srcBuffer,           // Դ��������������Դ��
            dstBuffer,           // Ŀ�껺����������ȥ��
            1,                   // ���Ƶ���������
            &copyRegion          // ������������
        );

        endSingleTimeCommands(commandBuffer);   // ����¼�ƣ����ύ��ͬ�����ͷ�
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
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;                                 // ���ֵ����������ɫ������
        renderPassInfo.pClearValues = clearValues.data();                          // ָ�������ɫ����

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

        // �����㻺���
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        // �����������
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // ����������
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);


        // --- �׶�6����������ָ�� ---
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        // �������ͣ�
        // vertexCount: ���㻺��ĳߴ�
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

    // �������������Դ�ĺ���
    // ��;���ڽ������ؽ����細�ڴ�С�仯��������˳�ʱ���ͷ��뽻����������GPU��Դ
    void cleanupSwapChain() {
        // 0. ������ɫ���������Դ�����ͼ����ͼ��ͼ������豸�ڴ棩
        vkDestroyImageView(device, colorImageView, nullptr);    // �������ͼ����ͼ��VkImageView��
        vkDestroyImage(device, colorImage, nullptr);            // �������ͼ�����VkImage��
        vkFreeMemory(device, colorImageMemory, nullptr);        // �ͷ����ͼ���豸�ڴ棨VkDeviceMemory��

        // 1. ������Ȼ��������Դ�����ͼ����ͼ��ͼ������豸�ڴ棩
        vkDestroyImageView(device, depthImageView, nullptr);    // �������ͼ����ͼ��VkImageView��
        vkDestroyImage(device, depthImage, nullptr);            // �������ͼ�����VkImage��
        vkFreeMemory(device, depthImageMemory, nullptr);        // �ͷ����ͼ���豸�ڴ棨VkDeviceMemory��

        // 2. �������н�����֡��������Frame buffer��
        // ֡����������ͼ����ͼ��������ͼ����ͼ����
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        }
        // 3. �������н�����ͼ����ͼ��Image views��
        // ͼ����ͼ����������ͼ���������ٽ�����ǰ����
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }

        // 4. ���ٽ��������壨Swap chain��
        // ע�⣺������ͼ��VkImage���ɽ������Զ������˴������ֶ�����
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    // �ؽ��������ĺ��ĺ���
    // ��;���ڴ��ڴ�С�ı䡢���ڻָ��������С����ԭ���򽻻���ʧЧʱ�����´������������������Դ
    void recreateSwapChain() {
        // 1. ��ȡ��ǰ���ڵ�֡�������ߴ磨��λ�����أ�
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        
        // 2. ��������С��/�ߴ�Ϊ0���������
        // �����ڱ���С��ʱ�������ȴ�ֱ�����ڻָ���Ч�ߴ�
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        // 3. �ȴ��豸���ж��п��У�ȷ��û������ʹ�ý�������Դ�Ĳ�����
        vkDeviceWaitIdle(device);// ��������Դ�Ա�ʹ��ʱ��������

        // 4. ����ɽ��������������Դ
        cleanupSwapChain();// ������Ȼ��塢֡���塢ͼ����ͼ�ͽ���������

        // 5. ����ȷ˳���ؽ����н����������Դ
        createSwapChain();          // �����½�������VkSwapchainKHR��
        createImageViews();         // Ϊ�������е�ÿ��ͼ�񴴽���ͼ��VkImageView��
        createColorResources();
        createDepthResources();     // �ؽ���Ȼ���ͼ��VkImage������ͼ
        createFramebuffers();       // Ϊ��ͼ����ͼ����Ȼ��崴��֡���壨VkFramebuffer��
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

    // 
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);    // �����豸��ȫ���ڴ���Ϣ

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // ����ڴ������Ƿ�֧������������Ҫ��
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    // ����Uniform Buffer���ݣ�ÿ֡���ã�
    void updateUniformBuffer(uint32_t currentImage) {
        // ��¼��ʼʱ�䣨staticȷ��ֻ��ʼ��һ�Σ��������ñ���ֵ��
        static auto startTime = std::chrono::high_resolution_clock::now();

        // ��ȡ��ǰʱ�䣬������ӳ�ʼʱ�䵽���ڵ�������
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // ����Uniform Buffer Object (UBO) ���������
        UniformBufferObject ubo{};
        // ģ�;�����Z����ʱ����ת��ÿ����ת90�ȣ�
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // ��ͼ���������λ��(2,2,2)������ԭ��(0,0,0)���Ϸ���ΪZ��
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // ͶӰ����45��͸��ͶӰ����߱����佻�����ֱ��ʣ���ƽ��0.1��Զƽ��10
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        // ����Vulkan��Y������ϵ����OpenGL�෴����תY��ͶӰ���������
        ubo.proj[1][1] *= -1;

        // ��UBO����ֱ�Ӹ��Ƶ���ǰ֡��ӳ���ڴ��У�����ӳ��/��ӳ�������
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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

        // ����UBO
        updateUniformBuffer(currentFrame);

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
        // �� ����4��Ҫ��֧�ָ������Բ���
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        // ������������ͬʱ���㣺������������չ֧�֡�����������
        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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