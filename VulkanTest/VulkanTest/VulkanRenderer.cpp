#include "VulkanRenderer.h"
#include <set>
#include <array>

const std::vector<const char*> VulkanRenderer::validationLayers{ "VK_LAYER_KHRONOS_validation" };

VulkanRenderer::VulkanRenderer()
{
	window = nullptr;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VulkanRenderer::~VulkanRenderer()
{
	clean();
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


int VulkanRenderer::init(GLFWwindow* windowP)
{
	window = windowP;
	try
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createRenderPass();
		createGraphicsPipeline();
	}
	catch (const std::runtime_error& e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


bool VulkanRenderer::checkInstanceExtensionSupport(const std::vector<const char*>& checkExtensions)
{
	// How many extensions vulkan supports?

	//Init amount variable and then get the amount of extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);


	// Create the vector of extensions based on the amount
	std::vector<VkExtensionProperties> extensions(extensionCount);							// A vector with a certain number of elements

	// Populate the vector
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());



	// This pattern, getting the number of elements then
	// populating a vector, is quite common with vulkan
	// 
	// Check if given extensions are in list of available extensions

	for (const auto& checkExtension : checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) return false;
	}
	return true;

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::clean()
{	
	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);

	for (auto image : swapchainImages)
	{
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}

	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);

	if (enableValidationLayers) {
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);					// Second argument is a custom de-allocator
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::createSurface()
{
	// Create a surface relatively to our window
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a vulkan surface.");
	}
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::createGraphicsPipeline()
{
	// Read shader code and format it through a shader module
	auto vertexShaderCode = readShaderFile("shaders/vert.spv");
	auto fragmentShaderCode = readShaderFile("shaders/frag.spv");
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);
	
	
	
	// -- SHADER STAGE CREATION INFO --
	// 
	// Vertex stage creation info
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Used to know which shader
	vertexShaderCreateInfo.module = vertexShaderModule;

	// Pointer to the start function in the shader
	vertexShaderCreateInfo.pName = "main";

	// Fragment stage creation info
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	// Graphics pipeline requires an array of shader create info
	VkPipelineShaderStageCreateInfo shaderStages[]{
	vertexShaderCreateInfo, fragmentShaderCreateInfo
	};


	// Create pipeline //
	
	// -- VERTEX INPUT STAGE --
	
	// TODO: Put in vertex description when resources created
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;

	// List of vertex binding desc. (data spacing, stride...)
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;

	// List of vertex attribute desc. (data format and where to bind to/from)
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;



	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	
	// How to assemble vertices
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// When you want to restart a primitive, e.g. with a strip
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;



	// -- VIEWPORT AND SCISSOR --

	// Create a viewport info struct
	VkViewport viewport{};
	viewport.x = 0.0f; // X start coordinate
	viewport.y = 0.0f; // Y start coordinate

	viewport.width = (float)swapchainExtent.width; // Width of viewport
	viewport.height = (float)swapchainExtent.height; // Height of viewport

	viewport.minDepth = 0.0f; // Min framebuffer depth
	viewport.maxDepth = 1.0f; // Max framebuffer depth

	// Create a scissor info struct, everything outside is cut
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	

	// -- DYNAMIC STATE --
	// 
	// This will be alterable, so you don't have to create an entire pipeline
	// when you want to change parameters.
	// We won't use this feature, this is an example.

	/*
	vector<VkDynamicState> dynamicStateEnables;
	// Viewport can be resized in the command buffer with
	// vkCmdSetViewport(commandBuffer, 0, 1, &newViewport);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	// Scissors can be resized in the command buffer with
	// vkCmdSetScissor(commandBuffer, 0, 1, &newScissor);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
	*/



	// -- RASTERIZER --

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	
	// Treat elements beyond the far plane like being on the far place, needs a GPU device feature
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	
	// Whether to discard data and skip rasterizer. When you want a pipeline without framebuffer.
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	
	// How to handle filling points between vertices. Here, considers things inside the polygon as
	// a fragment. VK_POLYGON_MODE_LINE will consider element inside polygones being empty (no
	// fragment). May require a device feature.
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	
	// How thick should line be when drawn
	rasterizerCreateInfo.lineWidth = 1.0f;
	
	// Culling. Do not draw back of polygons
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	
	// Widing to know the front face of a polygon
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	
	// Whether to add a depth offset to fragments. Good for stopping "shadow acne" in shadow
	// mapping. Is set, need to set 3 other values.
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	
	
	
	// -- MULTISAMPLING --

	// Not for textures, only for edges
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	
	// Enable multisample shading or not
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	
	// Number of samples to use per fragment
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	
	
	
	// -- BLENDING --
 
	// How to blend a new color being written to the fragment, with the old value
	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	
	// Alternative to usual blending calculation
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	
	// Enable blending and choose colors to apply blending to
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;

	// Blending equation:
	// (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	
	// Replace the old alpha with the new one: (1 * new alpha) + (0 * old alpha)
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;



	// -- PIPELINE LAYOUT --

	// TODO: apply future descriptorset layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	
	// Create pipeline layout
	VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}



	// -- DEPTH STENCIL TESTING --

	// TODO: Set up depth stencil testing



	// -- PASSES --

	// Passes are composed of a sequence of subpasses that can pass data from one to another
	


	// -- GRAPHICS PIPELINE CREATION --

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.layout = pipelineLayout;

	// Renderpass description the pipeline is compatible with. This pipeline will be used
	// by the render pass.
	graphicsPipelineCreateInfo.renderPass = renderPass;

	// Subpass of render pass to use with pipeline. Usually one pipeline by subpass.
	graphicsPipelineCreateInfo.subpass = 0;

	// When you want to derivate a pipeline from an other pipeline
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	// Index of pipeline being created to derive from (in case of creating multiple at once)
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	result = vkCreateGraphicsPipelines(mainDevice.logicalDevice,
		VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);

	// The handle is a cache when you want to save your pipeline to create an other later
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Cound not create a graphics pipeline");
	}


	// Destroy shader modules
	vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	
	// Conversion between pointer types with reinterpret_cast
	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create shader module.");
	}
	return shaderModule;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::createRenderPass()
{
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	// Attachement description : describe color buffer output, depth buffer output...
	// e.g. (location = 0) in the fragment shader is the first attachment
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat; // Format to use for attachment


	// Number of samples t write for multisampling
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;


	// What to do with attachement before renderer. Here, clear when we start the render pass.
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;


	// What to do with attachement after renderer. Here, store the render pass.
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;


	// What to do with stencil before renderer. Here, don't care, we don't use stencil.
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;


	// What to do with stencil after renderer. Here, don't care, we don't use stencil.
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;


	// Framebuffer images will be stored as an image, but image can have different layouts
	// to give optimal use for certain operations.
	// Image data layout before render pass starts
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;


	// Image data layout after render pass
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;


	// Attachment reference uses an attachment index that refers to index in the attachement list
	// passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentReference{};
	colorAttachmentReference.attachment = 0;


	// Layout of the subpass (between initial and final layout)
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	// Subpass description, will reference attachements
	VkSubpassDescription subpass{};


	// Pipeline type the subpass will be bound to. Could be compute pipeline, or nvidia raytracing...
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;


	// Subpass dependencies: transitions between subpasses + from the last subpass
	// to what happens after
	// Need to determine when layout transitions occur using subpass dependencies.
	// Will define implicitly layout transitions.
	std::array<VkSubpassDependency, 2> subpassDependencies;


	// -- From layout undefined to color attachment optimal
	// ---- Transition must happens after
	// External means from outside the subpasses
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL
		
		;
	// Which stage of the pipeline has to happen before
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;


	// ---- But must happens before
	// Conversion should happen before the first subpass starts
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;


	// ...and before the color attachment attempts to read or write
	subpassDependencies[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;


	// No dependency flag
	subpassDependencies[0].dependencyFlags = 0;


	// -- From layout color attachment optimal to image layout present
	// ---- Transition must happens after
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;


	// ---- But must happens before
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create render pass.");
	}
}

void VulkanRenderer::createSwapchain()
{
	// We will pick best settings for the swapchain
	SwapchainDetails swapchainDetails = getSwapchainDetails(mainDevice.physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapchainDetails.formats);
	VkPresentModeKHR presentationMode = chooseBestPresentationMode(swapchainDetails.presentationModes);
	VkExtent2D extent = chooseSwapExtent(swapchainDetails.surfaceCapabilities);


	// Setup the swapchain info
	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.presentMode = presentationMode;
	swapchainCreateInfo.imageExtent = extent;


	// Minimal number of image in our swapchain.
	// We will use one more than the minimum to enable triple-buffering.
	uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount + 1;
	if (swapchainDetails.surfaceCapabilities.maxImageCount > 0 // Not limitless
		&& swapchainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapchainDetails.surfaceCapabilities.maxImageCount;
	}

	swapchainCreateInfo.minImageCount = imageCount;

	// Number of layers for each image in swapchain
	swapchainCreateInfo.imageArrayLayers = 1;

	// What attachment go with the image (e.g. depth, stencil...). Here, just color.
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Transform to perform on swapchain images
	swapchainCreateInfo.preTransform = swapchainDetails.surfaceCapabilities.currentTransform;

	// Handles blending with other windows. Here we don't blend.
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	// Whether to clip parts of the image not in view (e.g. when an other window overlaps)
	swapchainCreateInfo.clipped = VK_TRUE;

	// Queue management
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// If graphics and presentation families are different, share images between them
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[]{ (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentationFamily };
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}


	// When you want to pass old swapchain responsibilities when destroying it,
	// e.g. when you want to resize window, use this
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create swapchain
	VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain");
	}

	// Store for later use
	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	// Get the swapchain images
	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, images.data());

	for (VkImage image : images) // We are using handles, not values
	{
		SwapchainImage swapchainImage{};
		swapchainImage.image = image;

		// Create image view
		swapchainImage.imageView = createImageView(image, swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		swapchainImages.push_back(swapchainImage);
	}

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// We will use RGBA 32bits normalized and SRGG non linear colorspace
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		// All formats available by convention
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (auto& format : formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}


	// Return first format if we have not our chosen format
	return formats[0];

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::setupDebugMessenger()
{
	if (!enableValidationLayers) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to set up debug messenger.");
	}
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	// We will use mail box presentation mode
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}


	// Part of the Vulkan spec, so have to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// Rigid extents
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}

	// Extents can vary
	else
	{
		// Create new extent using window size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D newExtent{};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// Sarface also defines max and min, so make sure we are within boundaries
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));
		return newExtent;
	}
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::createInstance()
{

	/*															  -- 1 --																*/


														  // APP INFO STRUCT

	// Information about the application
	// This data is for developer convenience

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;											// Type of the app info
	appInfo.pApplicationName = "Vulkan App";													// Name of the app
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);										// Version of the application
	appInfo.pEngineName = "No Engine";															// Custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);											// Custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_1;													// Vulkan version (here 1.1)



	/*															  -- 2 --																*/


													  // CREATE INSTANCE INFO STRUCT

	// Everything we create will be created with a createInfo
	// Here, info about the vulkan creation

	VkInstanceCreateInfo instCreateInfo{};
	instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;								// Type of the create info
	// createInfo.pNext																			// Extended information
	// createInfo.flags																			// Flags with bitfield
	instCreateInfo.pApplicationInfo = &appInfo;													// Application info from above



	/*															  -- 3 --																*/


															  // EXTENSIONS

	// Setup extensions instance will use

	std::vector<const char*> instanceExtensions = getRequiredExtensions();
	
	// Check extensions
	if (!checkInstanceExtensionSupport(instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}


	instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	instCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();



	/*															  -- 4 --																*/


														     // VALIDATION LAYERS

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}
	if (enableValidationLayers)
	{
		instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instCreateInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo); // N
		instCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		instCreateInfo.enabledLayerCount = 0;
		instCreateInfo.ppEnabledLayerNames = nullptr;
	}



	/*															  -- 5 --																*/


															// CREATE INSTANCE

	VkResult result = vkCreateInstance(&instCreateInfo, nullptr, &instance);
	// Second argument serves to choose where to allocate memory,
	// if you're interested with memory management
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance");
	}
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VkResult VulkanRenderer::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;


	// Other formats can be used for cubemaps etc.
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	// Can be used for depth for instance
	viewCreateInfo.format = format;

	// Swizzle used to remap color values. Here we keep the same.
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;


	// Subresources allow the view to view only a part of an image
	// Here we want to see the image under the aspect of colors
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

	// Start mipmap level to view from
	viewCreateInfo.subresourceRange.baseMipLevel = 0;

	// Number of mipmap level to view
	viewCreateInfo.subresourceRange.levelCount = 1;

	// Start array level to view from
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;

	// Number of array levels to view
	viewCreateInfo.subresourceRange.layerCount = 1;

	// Create image view
	VkImageView imageView;
	VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create the image view.");
	}
	return imageView;

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}

}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::createLogicalDevice()
{
/*															-- 1 --															*/
//																															//
//													// P.D GOT QUEUES INFO													//
//																															//
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues the logical device needs to create and info to do so.
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;

		// Vulkan needs to know how to handle multiple queues. It uses priorities.
		// 1 is the highest priority.
		queueCreateInfo.pQueuePriorities = &priority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
//																															//
//																															//
//																															//
/*							--								-- 2 --								--							*/
//																															//
//													  // L.D CREATE INFO													//
//																															//
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;



/*															-- 3 --															*/
//																															//
//												  // L.D QUEUES CREATE INFO													//
//																															//
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();


/*															-- 4 --															*/
//																															//
//													 // L.D EXTENSIONS INFO													//
//																															//
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
//																															//
//																															//
//																															//
/*							--  							-- 5 --								--							*/
//																															//
//														 // FEATURES														//
//																															//
	VkPhysicalDeviceFeatures deviceFeatures{};					// For now, no device features (tessellation etc.)
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
//																															//
//																															//
//																															//
/*							--								-- 6 --								--							*/
//																															//
//											 // LINK CREATED L.D WITH P.D INFOS ABOVE											//
//																															//
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create the logical device.");
	}
//																															//
//																															//
//																															//
/*							--								-- 7 --								--							*/
//																															//
//												   // ENSURE QUEUES ACCESS													//
//																															//
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


SwapchainDetails VulkanRenderer::getSwapchainDetails(VkPhysicalDevice device)
{
	SwapchainDetails swapchainDetails;

	// Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchainDetails.surfaceCapabilities);

	// Formats
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		swapchainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapchainDetails.formats.data());
	}

	// Presentation modes
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	if (presentationCount != 0)
	{
		swapchainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,&presentationCount, swapchainDetails.presentationModes.data());
	}


	return swapchainDetails;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


void VulkanRenderer::getPhysicalDevice()
{
	// Get the number of physical devices then populate the physical device vector
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// If no devices available
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find any GPU that supports vulkan");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// Get device valid for what we want to do
	for (const auto& device : devices)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	// Information about the device itself (ID, name, type, vendor, etc.)
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(device, &deviceProperties);


	// Information about what the device can do (geom shader, tesselation, wide lines...)
	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


	// For now we do nothing with this info
	QueueFamilyIndices indices = getQueueFamilies(device);
	bool extensionSupported = checkDeviceExtensionSupport(device);

	bool swapchainValid = false;
	if (extensionSupported)
	{
		SwapchainDetails swapchainDetails = getSwapchainDetails(device);
		swapchainValid = !swapchainDetails.presentationModes.empty() && !swapchainDetails.formats.empty();
	}


	return indices.isValid() && extensionSupported && swapchainValid;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	if (extensionCount == 0)
	{
		return false;
	}


	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	for (const auto& deviceExtension : deviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) return false;
	}
	return true;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


bool VulkanRenderer::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


	// Check if all of the layers in validation layers exist in the available layers
	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound) return false;
	}
	return true;

}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;																		// int of the queue index + a IsValid() func
	uint32_t queueFamilyCount = 0;																	// int of the nb of queue families/types
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);					// Calculates the nb of queue families for the refered P.D
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);							// Vector of Queue family properties of the size of queue family count
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());		// Populating of the vector queueFamilies with the refered P.D queue families




	// Go through each queue family and check it has at least one required type of queue

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		// Check there is at least graphics queue
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		// Check if queue family support presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
		}

		if (indices.isValid()) break;
		++i;
	}

	return indices;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/


std::vector<const char*> VulkanRenderer::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	//										   ^			   ^				  ^
	//									   ,___|			   |______,___________|
	//									   |						  |
	/*vector (vector&& x);vector (vector&& x, const allocator_type& alloc);
	//
	//		Moove Constructor(and moving with allocator)
	//			--> Constructs a container that acquires the elements of x.
	//
	//			--> If alloc is specified and is different from x's allocator, 
	//				the elements are moved. Otherwise, no elements are constructed
	//				(their ownership is directly transferred).
	//
	//			--> x is left in an unspecified but valid state 
	*/

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}


/*------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------*/
