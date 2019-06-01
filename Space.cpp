#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

GLFWwindow* window;
int32_t width, height;

vk::Instance instance;
vk::DispatchLoaderDynamic loader;
vk::DebugUtilsMessengerEXT messenger;
vk::SurfaceKHR surface;
uint32_t deviceIndex, queueIndex;
vk::PhysicalDevice physicalDevice;
vk::Device device;
vk::Queue queue;
vk::SwapchainKHR swapchain;
std::vector<vk::Image> swapchainImages;
std::vector<vk::ImageView> swapchainViews;

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cout << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void initializeBase()
{
	width = 800;
	height = 600;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Space", NULL, NULL);

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
	if (!window || glfwCreateWindowSurface(VkInstance(instance), window, NULL, (VkSurfaceKHR*)& surface) != VK_SUCCESS)
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
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	physicalDevice.getSurfaceFormatsKHR(surface);
	physicalDevice.getSurfacePresentModesKHR(surface);
	physicalDevice.getSurfaceSupportKHR(queueIndex, surface);

	vk::SwapchainCreateInfoKHR swapchainInfo{
		vk::SwapchainCreateFlagsKHR(),
		surface,
		surfaceCapabilities.minImageCount + 1,
		vk::Format::eB8G8R8A8Unorm,
		vk::ColorSpaceKHR::eSrgbNonlinear,
		surfaceCapabilities.currentExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		0,
		0,
		surfaceCapabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eMailbox,
		VK_TRUE,
		nullptr
	};

	swapchain = device.createSwapchainKHR(swapchainInfo);
	swapchainImages = device.getSwapchainImagesKHR(swapchain);
	for (int32_t i = 0; i < swapchainImages.size(); i++)
		swapchainViews.emplace_back(createImageView(swapchainImages.at(i),
			1, vk::Format::eB8G8R8A8Unorm, vk::ImageAspectFlagBits::eColor));
}

void setup()
{
	initializeBase();
	createSwapchain();
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