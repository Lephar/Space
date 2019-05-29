#include <iostream>
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

GLFWwindow* window;
int width, height;

vk::Instance instance;
vk::DebugUtilsMessengerEXT messenger;
vk::SurfaceKHR surface;

void createWindow()
{
	width = 800;
	height = 600;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Space", NULL, NULL);
}

void createInstance()
{
	vk::ApplicationInfo applicationInfo{
		"Space",
		VK_MAKE_VERSION(1, 0, 0),
		"Mungui Engine",
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_1
	};

	std::vector<const char*> layers{ "VK_LAYER_KHRONOS_validation" };

	uint32_t extensionCount = 0;
	const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
	std::vector<const char*> extensions{ extensionNames, extensionNames + extensionCount };
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	vk::InstanceCreateInfo instanceInfo{
		vk::InstanceCreateFlags(),
		&applicationInfo,
		static_cast<uint32_t>(layers.size()),
		layers.data(),
		static_cast<uint32_t>(extensions.size()),
		extensions.data()
	};

	instance = vk::createInstance(instanceInfo);
}

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cout << pCallbackData->pMessage << std::endl;
	return false;
}

void registerMessenger()
{
	messenger = instance.createDebugUtilsMessengerEXT(
		vk::DebugUtilsMessengerCreateInfoEXT{
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
		},
		nullptr,
		vk::DispatchLoaderDynamic{
			instance,
			vkGetInstanceProcAddr
		}
	);
}

void bindSurface()
{
	VkSurfaceKHR temporarySurface;
	if (glfwCreateWindowSurface(VkInstance(instance), window, NULL, &temporarySurface) == VK_SUCCESS)
		surface = vk::SurfaceKHR{ temporarySurface };
	else
		throw vk::SurfaceLostKHRError(nullptr);
}

void setup()
{
	createWindow();
	createInstance();
	registerMessenger();
	bindSurface();
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
	instance.destroySurfaceKHR(surface);
	instance.destroyDebugUtilsMessengerEXT(messenger, nullptr, vk::DispatchLoaderDynamic{ instance, vkGetInstanceProcAddr });
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