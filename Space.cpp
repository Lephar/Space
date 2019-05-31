#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

GLFWwindow* window;
int32_t width, height;

vk::Instance instance;
vk::DispatchLoaderDynamic loader;
vk::DebugUtilsMessengerEXT messenger;
vk::SurfaceKHR surface;
vk::PhysicalDevice physicalDevice;
vk::Device device;
uint32_t queueIndex;
vk::Queue queue;

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cout << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void initBase()
{
	width = 800;
	height = 600;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Space", NULL, NULL);

	uint32_t extensionCount = 0;
	const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
	std::vector<const char*> layers{ "VK_LAYER_KHRONOS_validation" }, extensions{ extensionNames, extensionNames + extensionCount };
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

	auto devices = instance.enumeratePhysicalDevices();

	for (auto& device : devices)
	{
		auto properties = device.getProperties();

		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			auto queueFamilies = device.getQueueFamilyProperties();

			for (uint32_t i = 0; i < queueFamilies.size(); i++)
			{
				if (device.getSurfaceSupportKHR(i, surface) && (queueFamilies.at(i).queueFlags & vk::QueueFlagBits::eGraphics))
				{
					physicalDevice = device;
					queueIndex = i;
					break;
				}
			}
		}

		if (physicalDevice == device)
			break;
	}

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

void setup()
{
	initBase();
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