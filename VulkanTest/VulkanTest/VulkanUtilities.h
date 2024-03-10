#pragma once
#include <iostream>
#include <vector>

struct QueueFamilyIndices
{
	int graphicsFamily = -1; // Location of Graphics Queue Family
	int presentationFamily = -1; // Location of PResentation Queue Family

	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainDetails {

	// What the surface is capable of displaying, e.g. image size/extent
	VkSurfaceCapabilitiesKHR surfaceCapabilities;

	// Vector of the image formats, e.g. RGBA
	std::vector<VkSurfaceFormatKHR> formats;

	// Vector of presentation modes
	std::vector<VkPresentModeKHR> presentationModes;
};

struct SwapchainImage
{
	VkImage image;
	VkImageView imageView;
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

const std::vector<const char*> deviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };