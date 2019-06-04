#include <fstream>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

GLFWwindow* window;
uint32_t width, height;

vk::Instance instance;
vk::DispatchLoaderDynamic loader;
vk::DebugUtilsMessengerEXT messenger;
vk::SurfaceKHR surface;
uint32_t deviceIndex, queueIndex;
vk::PhysicalDevice physicalDevice;
vk::Device device;
vk::Queue queue;
vk::SwapchainKHR swapchain;
vk::Format swapchainFormat;
std::vector<vk::Image> swapchainImages;
std::vector<vk::ImageView> swapchainViews;
vk::RenderPass renderPass;
vk::PipelineLayout pipelineLayout;
vk::Pipeline pipeline;
std::vector<vk::Framebuffer> framebuffers;
vk::CommandPool commandPool;
std::vector<vk::CommandBuffer> commandBuffers;

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	(void)type;
	(void)severity;
	(void)pUserData;

	std::cout << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void initializeBase()
{
	width = 800;
	height = 600;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), "Space", NULL, NULL);

	uint32_t extensionCount = 0;
	const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

	std::vector<const char*> layers{ "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> extensions{ extensionNames, extensionNames + extensionCount };
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	vk::ApplicationInfo applicationInfo{
		"Space",
		VK_MAKE_VERSION(1, 0, 0),
		"Mungui Engine",
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_1
	};

	vk::DebugUtilsMessengerCreateInfoEXT messengerInfo{
		vk::DebugUtilsMessengerCreateFlagsEXT(),
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		messageCallback,
		nullptr
	};

	vk::InstanceCreateInfo instanceInfo{
		vk::InstanceCreateFlags(),
		&applicationInfo,
		static_cast<uint32_t>(layers.size()),
		layers.data(),
		static_cast<uint32_t>(extensions.size()),
		extensions.data(),
	};
	instanceInfo.setPNext(&messengerInfo);

	instance = vk::createInstance(instanceInfo);
	loader = vk::DispatchLoaderDynamic{ instance };
	messenger = instance.createDebugUtilsMessengerEXT(messengerInfo, nullptr, loader);
	if (!window || glfwCreateWindowSurface(VkInstance(instance), window, NULL, (VkSurfaceKHR*)& surface)
		!= VK_SUCCESS)
		throw vk::SurfaceLostKHRError(nullptr);

	deviceIndex = 0;
	physicalDevice = instance.enumeratePhysicalDevices().at(deviceIndex);
	physicalDevice.getQueueFamilyProperties();

	queueIndex = 0;
	float queuePriority = 1.0f;

	vk::DeviceQueueCreateInfo queueInfo{
		vk::DeviceQueueCreateFlags(),
		queueIndex,
		1,
		&queuePriority
	};

	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	vk::DeviceCreateInfo deviceInfo{
		vk::DeviceCreateFlags(),
		1,
		&queueInfo,
		0,
		nullptr,
		static_cast<uint32_t>(deviceExtensions.size()),
		deviceExtensions.data(),
		&deviceFeatures
	};

	device = physicalDevice.createDevice(deviceInfo);
	queue = device.getQueue(queueIndex, 0);
}

vk::ImageView createImageView(vk::Image image, uint32_t levels, vk::Format format, vk::ImageAspectFlags flags)
{
	vk::ImageViewCreateInfo viewInfo{
		vk::ImageViewCreateFlags(),
		image,
		vk::ImageViewType::e2D,
		format,
		vk::ComponentMapping{
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		},
		vk::ImageSubresourceRange{
			flags,
			0,
			levels,
			0,
			1,
		}
	};

	return device.createImageView(viewInfo);
}

void createSwapchain()
{
	auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	physicalDevice.getSurfaceFormatsKHR(surface);
	physicalDevice.getSurfaceSupportKHR(queueIndex, surface);

	width = std::clamp(surfaceCapabilities.currentExtent.width,
		surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
	height = std::clamp(surfaceCapabilities.currentExtent.height,
		surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

	bool mailbox = false;
	for (auto& presentMode : presentModes)
		if (presentMode == vk::PresentModeKHR::eMailbox)
			mailbox = true;
	swapchainFormat = vk::Format::eB8G8R8A8Unorm;

	vk::SwapchainCreateInfoKHR swapchainInfo{
		vk::SwapchainCreateFlagsKHR(),
		surface,
		surfaceCapabilities.minImageCount + 1,
		swapchainFormat,
		vk::ColorSpaceKHR::eSrgbNonlinear,
		surfaceCapabilities.currentExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		0,
		0,
		surfaceCapabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		mailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate,
		VK_TRUE,
		nullptr
	};

	swapchain = device.createSwapchainKHR(swapchainInfo);
	swapchainImages = device.getSwapchainImagesKHR(swapchain);
	for (uint32_t i = 0; i < swapchainImages.size(); i++)
		swapchainViews.emplace_back(createImageView(swapchainImages.at(i),
			1, swapchainFormat, vk::ImageAspectFlagBits::eColor));
}

void createRenderPass()
{
	vk::AttachmentReference colorReference{
		0,
		vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::AttachmentDescription colorAttachment{
		vk::AttachmentDescriptionFlags(),
		swapchainFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	};

	vk::SubpassDescription subpass{
		vk::SubpassDescriptionFlags(),
		vk::PipelineBindPoint::eGraphics,
		0,
		nullptr,
		1,
		&colorReference,
		nullptr,
		nullptr,
		0,
		nullptr
	};

	vk::RenderPassCreateInfo renderPassInfo{
		vk::RenderPassCreateFlags(),
		1,
		&colorAttachment,
		1,
		&subpass,
		0,
		nullptr
	};

	renderPass = device.createRenderPass(renderPassInfo);
}

vk::ShaderModule initializeShaderModule(std::string path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<uint32_t> data(size / sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(data.data()), size);

	vk::ShaderModuleCreateInfo shaderInfo{
		vk::ShaderModuleCreateFlags(),
		static_cast<size_t>(size),
		data.data()
	};

	return device.createShaderModule(shaderInfo);
}

void createGraphicsPipeline()
{
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
		vk::PipelineVertexInputStateCreateFlags(),
		0,
		nullptr,
		0,
		nullptr
	};

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList,
		VK_FALSE
	};

	vk::Viewport viewport{
		0.0f,
		0.0f,
		static_cast<float>(width),
		static_cast<float>(height),
		0.0f,
		1.0f
	};

	vk::Rect2D scissor{
		vk::Offset2D{
			0,
			0
		},
		vk::Extent2D{
			width,
			height
		}
	};

	vk::PipelineViewportStateCreateInfo viewportInfo{
		vk::PipelineViewportStateCreateFlags(),
		1,
		&viewport,
		1,
		&scissor
	};

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo{
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
		vk::PolygonMode::eLine,
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eClockwise,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};

	vk::PipelineMultisampleStateCreateInfo multisamplingInfo{
		vk::PipelineMultisampleStateCreateFlags(),
		vk::SampleCountFlagBits::e1,
		VK_FALSE,
		0.0f,
		nullptr,
		VK_FALSE,
		VK_FALSE
	};

	vk::PipelineColorBlendAttachmentState colorBlending{
		VK_FALSE,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eOne,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eOne,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA,
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo{
		vk::PipelineColorBlendStateCreateFlags(),
		VK_FALSE,
		vk::LogicOp::eClear,
		1,
		&colorBlending,
		std::array<float, 4>{
			0,
			0,
			0,
			0
		}
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		vk::PipelineLayoutCreateFlags(),
		0,
		nullptr,
		0,
		nullptr
	};

	pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

	vk::ShaderModule vertexShader = initializeShaderModule("data/vert.spv");
	vk::PipelineShaderStageCreateInfo vertexInfo{
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eVertex,
		vertexShader,
		"main",
		nullptr
	};

	vk::ShaderModule fragmentShader = initializeShaderModule("data/frag.spv");
	vk::PipelineShaderStageCreateInfo fragmentInfo{
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eFragment,
		fragmentShader,
		"main",
		nullptr
	};

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
		vertexInfo,
		fragmentInfo
	};

	vk::GraphicsPipelineCreateInfo graphicsPipelineInfo{
		vk::PipelineCreateFlags(),
		static_cast<uint32_t>(shaderStages.size()),
		shaderStages.data(),
		&vertexInputInfo,
		&inputAssemblyInfo,
		nullptr,
		&viewportInfo,
		&rasterizerInfo,
		&multisamplingInfo,
		nullptr,
		&colorBlendInfo,
		nullptr,
		pipelineLayout,
		renderPass,
		0,
		nullptr,
		0
	};

	pipeline = device.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
	device.destroyShaderModule(fragmentShader);
	device.destroyShaderModule(vertexShader);
}

void createFramebuffers()
{
	for (uint32_t i = 0; i < swapchainViews.size(); i++)
	{
		vk::FramebufferCreateInfo framebufferInfo{
			vk::FramebufferCreateFlags(),
			renderPass,
			1,
			&swapchainViews.at(i),
			width,
			height,
			1
		};

		framebuffers.emplace_back(device.createFramebuffer(framebufferInfo));
	}
}

void createCommandPool()
{
	vk::CommandPoolCreateInfo poolInfo{
		vk::CommandPoolCreateFlags(),
		queueIndex
	};

	commandPool = device.createCommandPool(poolInfo);


}

void createCommandBuffers()
{
	vk::CommandBufferAllocateInfo allocationInfo{
		commandPool,
		vk::CommandBufferLevel::ePrimary,
		static_cast<uint32_t>(framebuffers.size())
	};

	commandBuffers = device.allocateCommandBuffers(allocationInfo);

	for (uint32_t i = 0; i < commandBuffers.size(); i++)
	{
		vk::CommandBufferBeginInfo commandBufferBegin{
			vk::CommandBufferUsageFlagBits::eSimultaneousUse,
			nullptr
		};

		vk::ClearValue clearColor{
			vk::ClearColorValue{
				std::array<float, 4>{
					0.0f,
					0.0f,
					0.0f,
					1.0f
				}
			}
		};

		vk::RenderPassBeginInfo renderPassBegin{
			renderPass,
			framebuffers.at(i),
			vk::Rect2D{
				vk::Offset2D{
					0,
					0
				},
				vk::Extent2D{
					width,
					height
				}
			},
			1,
			&clearColor
		};

		commandBuffers.at(i).begin(commandBufferBegin);
		commandBuffers.at(i).beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
		commandBuffers.at(i).bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		commandBuffers.at(i).draw(3, 1, 0, 0);
		commandBuffers.at(i).endRenderPass();
		commandBuffers.at(i).end();
	}
}

void setup()
{
	initializeBase();
	createSwapchain();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
}

void draw()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void clean()
{
	device.destroyCommandPool(commandPool);
	for (auto& framebuffer : framebuffers)
		device.destroyFramebuffer(framebuffer);
	device.destroyPipeline(pipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);
	for (auto& swapchainView : swapchainViews)
		device.destroyImageView(swapchainView);
	device.destroySwapchainKHR(swapchain);
	device.destroy();
	instance.destroySurfaceKHR(surface);
	instance.destroyDebugUtilsMessengerEXT(messenger, nullptr, loader);
	instance.destroy();
	glfwDestroyWindow(window);
	glfwTerminate();
}

int main()
{
	setup();
	draw();
	clean();
}
