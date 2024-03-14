#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLFW_DLL

#include <GLFW/glfw3.h>
#include "VulkanUtilities.h"
#include <stdexcept>

struct 
{
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
} mainDevice;

class VulkanRenderer
{
public:

#ifdef NDEBUG
	static const bool enableValidationLayers = false;
#else
	static const bool enableValidationLayers = true;
#endif
	static const std::vector<const char*> validationLayers;

	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* windowP);
	bool checkInstanceExtensionSupport(const std::vector<const char*>& checkExtensions);
	
	void clean();

private:

	GLFWwindow* window;
	VkInstance instance;

	VkQueue presentationQueue;
	VkQueue graphicsQueue;

	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	VkPipelineLayout pipelineLayout;

	void createRenderPass();
	VkRenderPass renderPass;

	VkPipeline graphicsPipeline;

	std::vector<SwapchainImage> swapchainImages;

	void createSwapchain();
	void createSurface();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkDebugUtilsMessengerEXT debugMessenger;
	void setupDebugMessenger();

	void createInstance();

	VkResult createDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void createLogicalDevice();

	SwapchainDetails getSwapchainDetails(VkPhysicalDevice device);

	void getPhysicalDevice();
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

	std::vector<const char*> getRequiredExtensions();
};