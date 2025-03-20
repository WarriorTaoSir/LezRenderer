#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Initialize the window
    void initWindow() {
        glfwInit();
        // Do not create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // disable the resize
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        // the window(width, height, title, screenIndex, someting relevant to OpenGL)
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }
    // allocate the resources
    void initVulkan() {

    }

    // 
    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) {
            std::cout << "hello" << std::endl;
            glfwPollEvents();
        }
    }

    // deallocate the resources
    void cleanup() {
        glfwDestroyWindow(window);

        glfwTerminate();
    }
private:
    GLFWwindow* window; // the window to show
};

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