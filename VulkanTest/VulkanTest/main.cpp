#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>

#include <string>
using std::string;

#include "VulkanRenderer.h"

#define MAX 99;

GLFWwindow* window = nullptr;
VulkanRenderer vulkanRenderer;

void initWindow(string wName = "Vulkan", const int width = 800, const int height = 600)
{
		// Initialize GLFW
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // Glfw won't work with opengl
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

void clean()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}


int main() {

	initWindow();
	if (vulkanRenderer.init(window) == EXIT_FAILURE) return EXIT_FAILURE;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}


	clean();
	return 0;
}