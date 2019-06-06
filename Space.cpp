#include <fstream>
#include <iostream>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
};

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
vk::CommandPool commandPool;
vk::SwapchainKHR swapchain;
vk::Format swapchainFormat;
vk::Rect2D swapchainArea;
std::vector<vk::Image> swapchainImages;
std::vector<vk::ImageView> swapchainViews;
vk::RenderPass renderPass;
vk::ShaderModule vertexShader, fragmentShader;
vk::PipelineLayout pipelineLayout;
vk::Pipeline pipeline;
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;
vk::Buffer vertexBuffer, indexBuffer;
vk::DeviceMemory vertexMemory, indexMemory;
std::vector<vk::Framebuffer> framebuffers;
std::vector<vk::CommandBuffer> commandBuffers;
uint32_t syncLimit;
std::vector<vk::Fence> frameFences;
std::vector<vk::Semaphore> imageSemaphores, renderSemaphores;

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
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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
	if (!window || glfwCreateWindowSurface(static_cast<VkInstance>(instance), window,
		NULL, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS)
		throw vk::SurfaceLostKHRError(nullptr);

	deviceIndex = 0;
	queueIndex = 0;
	float queuePriority = 1.0f;
	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;

	vk::DeviceQueueCreateInfo queueInfo{
		vk::DeviceQueueCreateFlags(),
		queueIndex,
		1,
		&queuePriority
	};

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

	vk::CommandPoolCreateInfo poolInfo{
		vk::CommandPoolCreateFlags(),
		queueIndex
	};

	physicalDevice = instance.enumeratePhysicalDevices().at(deviceIndex);
	physicalDevice.getQueueFamilyProperties();
	device = physicalDevice.createDevice(deviceInfo);
	queue = device.getQueue(queueIndex, 0);
	commandPool = device.createCommandPool(poolInfo);
}

vk::ImageView createImageView(vk::Image image, uint32_t levels, vk::Format format, vk::ImageAspectFlags flags)
{
	vk::ComponentMapping components{
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity
	};

	vk::ImageSubresourceRange subresourceRange{
		flags,
		0,
		levels,
		0,
		1,
	};

	vk::ImageViewCreateInfo viewInfo{
		vk::ImageViewCreateFlags(),
		image,
		vk::ImageViewType::e2D,
		format,
		components,
		subresourceRange
	};

	return device.createImageView(viewInfo);
}

void createSwapchain()
{
	glfwGetFramebufferSize(window, reinterpret_cast<int*>(&width), reinterpret_cast<int*>(&height));

	auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	physicalDevice.getSurfaceFormatsKHR(surface);
	physicalDevice.getSurfaceSupportKHR(queueIndex, surface);

	swapchainFormat = vk::Format::eB8G8R8A8Unorm;
	swapchainArea = vk::Rect2D{
		vk::Offset2D{
			0,
			0
		},
		vk::Extent2D{
			width,
			height
		}
	};

	bool mailbox = false;
	for (auto& presentMode : presentModes)
		if (presentMode == vk::PresentModeKHR::eMailbox)
			mailbox = true;

	vk::SwapchainCreateInfoKHR swapchainInfo{
		vk::SwapchainCreateFlagsKHR(),
		surface,
		surfaceCapabilities.minImageCount + 1,
		swapchainFormat,
		vk::ColorSpaceKHR::eSrgbNonlinear,
		swapchainArea.extent,
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
	swapchainViews.resize(swapchainImages.size());
	for (uint32_t i = 0; i < swapchainViews.size(); i++)
		swapchainViews.at(i) = createImageView(swapchainImages.at(i),
			1, swapchainFormat, vk::ImageAspectFlagBits::eColor);
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

	vk::SubpassDependency dependency{
		VK_SUBPASS_EXTERNAL,
		0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::AccessFlags(),
		vk::AccessFlagBits::eColorAttachmentRead |
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::DependencyFlags()
	};

	vk::RenderPassCreateInfo renderPassInfo{
		vk::RenderPassCreateFlags(),
		1,
		&colorAttachment,
		1,
		&subpass,
		1,
		&dependency
	};

	renderPass = device.createRenderPass(renderPassInfo);
}

vk::ShaderModule loadShader(std::string path)
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

void createShaderModules()
{
	vertexShader = loadShader("data/vert.spv");
	fragmentShader = loadShader("data/frag.spv");
}

void createGraphicsPipeline()
{
	vk::VertexInputBindingDescription bindingDescription{
		0,
		sizeof(Vertex),
		vk::VertexInputRate::eVertex
	};

	std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{
		vk::VertexInputAttributeDescription{
			0,
			0,
			vk::Format::eR32G32B32Sfloat,
			0
		},
		vk::VertexInputAttributeDescription{
			1,
			0,
			vk::Format::eR32G32B32Sfloat,
			sizeof(glm::vec3)
		},
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
		vk::PipelineVertexInputStateCreateFlags(),
		1,
		&bindingDescription,
		static_cast<uint32_t>(attributeDescriptions.size()),
		attributeDescriptions.data()
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

	vk::PipelineViewportStateCreateInfo viewportInfo{
		vk::PipelineViewportStateCreateFlags(),
		1,
		&viewport,
		1,
		&swapchainArea
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
		2.0f
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

	vk::PipelineShaderStageCreateInfo vertexInfo{
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eVertex,
		vertexShader,
		"main",
		nullptr
	};

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
}

void createFramebuffers()
{
	framebuffers.resize(swapchainViews.size());

	for (uint32_t i = 0; i < framebuffers.size(); i++)
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

		framebuffers.at(i) = device.createFramebuffer(framebufferInfo);
	}
}

uint32_t getMemoryIndex(uint32_t filter, vk::MemoryPropertyFlags flags)
{
	auto memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		if ((filter & (1 << i)) && (flags & memoryProperties.memoryTypes[i].propertyFlags) == flags)
			return i;

	return std::numeric_limits<uint32_t>::max();
}

vk::CommandBuffer beginSingleTimeCommand()
{
	vk::CommandBufferAllocateInfo allocationInfo{
		commandPool,
		vk::CommandBufferLevel::ePrimary,
		1
	};

	vk::CommandBufferBeginInfo commandBufferBegin{
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit
	};

	auto commandBuffer = device.allocateCommandBuffers(allocationInfo).at(0);
	commandBuffer.begin(commandBufferBegin);
	return commandBuffer;
}

void endSingleTimeCommand(vk::CommandBuffer& commandBuffer)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo{
		0,
		nullptr,
		nullptr,
		1,
		&commandBuffer,
		0,
		nullptr
	};

	queue.submit(1, &submitInfo, nullptr);
	queue.waitIdle();
	device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void copyBuffer(vk::Buffer& source, vk::Buffer& destination, vk::DeviceSize size)
{
	auto commandBuffer = beginSingleTimeCommand();

	vk::BufferCopy region{
		0,
		0,
		size
	};

	commandBuffer.copyBuffer(source, destination, 1, &region);
	endSingleTimeCommand(commandBuffer);
}

void createBuffer(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize size,
	vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	vk::BufferCreateInfo bufferInfo{
		vk::BufferCreateFlags(),
		size,
		usage,
		vk::SharingMode::eExclusive,
		0,
		nullptr
	};

	buffer = device.createBuffer(bufferInfo);
	auto requirements = device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocationInfo{
		requirements.size,
		getMemoryIndex(requirements.memoryTypeBits, properties)
	};

	memory = device.allocateMemory(allocationInfo);
	device.bindBufferMemory(buffer, memory, 0);
}

void createVertexBuffers()
{
	vertices.emplace_back(Vertex{ { 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} });
	vertices.emplace_back(Vertex{ { 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} });
	vertices.emplace_back(Vertex{ {-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} });

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;
	auto size = vertices.size() * sizeof(Vertex);

	createBuffer(stagingBuffer, stagingMemory, size, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	createBuffer(vertexBuffer, vertexMemory, size, vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	auto data = device.mapMemory(stagingMemory, 0, size);
	std::memcpy(data, vertices.data(), size);
	device.unmapMemory(stagingMemory);

	copyBuffer(stagingBuffer, vertexBuffer, size);
	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingMemory);
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
			swapchainArea,
			1,
			&clearColor
		};

		vk::DeviceSize offset = 0;

		commandBuffers.at(i).begin(commandBufferBegin);
		commandBuffers.at(i).beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
		commandBuffers.at(i).bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		commandBuffers.at(i).bindVertexBuffers(0, 1, &vertexBuffer, &offset);
		commandBuffers.at(i).draw(vertices.size(), 1, 0, 0);
		commandBuffers.at(i).endRenderPass();
		commandBuffers.at(i).end();
	}
}

void createSyncObject()
{
	syncLimit = 2;
	frameFences.resize(syncLimit);
	imageSemaphores.resize(syncLimit);
	renderSemaphores.resize(syncLimit);

	vk::FenceCreateInfo fenceInfo{
		vk::FenceCreateFlagBits::eSignaled
	};

	vk::SemaphoreCreateInfo semaphoreInfo{
		vk::SemaphoreCreateFlags()
	};

	for (uint32_t i = 0; i < syncLimit; i++)
	{
		frameFences.at(i) = device.createFence(fenceInfo);
		imageSemaphores.at(i) = device.createSemaphore(semaphoreInfo);
		renderSemaphores.at(i) = device.createSemaphore(semaphoreInfo);
	}
}

void cleanupSwapchain()
{
	device.freeCommandBuffers(commandPool, commandBuffers.size(), commandBuffers.data());
	for (auto& framebuffer : framebuffers)
		device.destroyFramebuffer(framebuffer);
	device.destroyPipeline(pipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);
	for (auto& swapchainView : swapchainViews)
		device.destroyImageView(swapchainView);
	device.destroySwapchainKHR(swapchain);
}

void recreateSwapchain()
{
	device.waitIdle();
	cleanupSwapchain();
	createSwapchain();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandBuffers();
}

void setup()
{
	initializeBase();
	createSwapchain();
	createRenderPass();
	createShaderModules();
	createGraphicsPipeline();
	createFramebuffers();
	createVertexBuffers();
	createCommandBuffers();
	createSyncObject();
}

void clean()
{
	cleanupSwapchain();
	for (uint32_t i = 0; i < syncLimit; i++)
	{
		device.destroySemaphore(renderSemaphores.at(i));
		device.destroySemaphore(imageSemaphores.at(i));
		device.destroyFence(frameFences.at(i));
	}
	device.destroyShaderModule(fragmentShader);
	device.destroyShaderModule(vertexShader);
	device.destroyBuffer(vertexBuffer);
	device.freeMemory(vertexMemory);
	device.destroyCommandPool(commandPool);
	device.destroy();
	instance.destroySurfaceKHR(surface);
	instance.destroyDebugUtilsMessengerEXT(messenger, nullptr, loader);
	instance.destroy();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void draw()
{
	uint32_t imageIndex, syncIndex = 0;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		device.waitForFences(1, &frameFences.at(syncIndex), VK_TRUE, std::numeric_limits<uint64_t>::max());
		auto acquireResult = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(),
			imageSemaphores.at(syncIndex), nullptr);

		if (acquireResult.result == vk::Result::eSuboptimalKHR ||
			acquireResult.result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapchain();
			continue;
		}

		imageIndex = acquireResult.value;

		vk::PipelineStageFlags waitStages[]{
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		};

		vk::SubmitInfo submitInfo{
			1,
			&imageSemaphores.at(syncIndex),
			waitStages,
			1,
			&commandBuffers.at(imageIndex),
			1,
			&renderSemaphores.at(syncIndex)
		};

		vk::PresentInfoKHR presentInfo{
			1,
			&renderSemaphores.at(syncIndex),
			1,
			&swapchain,
			&imageIndex,
			nullptr
		};

		device.resetFences(1, &frameFences.at(syncIndex));
		queue.submit(1, &submitInfo, frameFences.at(syncIndex));

		try {
			queue.presentKHR(presentInfo);
		}
		catch (vk::OutOfDateKHRError error) {
			recreateSwapchain();
		}

		syncIndex = ++syncIndex % syncLimit;
	}

	device.waitIdle();
}

int main()
{
	setup();
	draw();
	clean();
}
