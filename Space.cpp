#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

GLFWwindow* window;
int32_t width, height;
std::chrono::time_point<std::chrono::system_clock> epoch;

vk::Instance instance;
vk::DispatchLoaderDynamic loader;
vk::DebugUtilsMessengerEXT messenger;
vk::SurfaceKHR surface;

std::string time()
{
	auto now = std::chrono::system_clock::now() - epoch;
	auto sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
	auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(now).count() - sec * 1000;
	auto usec = std::chrono::duration_cast<std::chrono::microseconds>(now).count() - msec * 1000 - sec * 1000000;

	std::ostringstream stream;
	stream << std::setfill('0') << std::setw(3) << sec << ':' << std::setw(3) << msec << ':' << std::setw(3) << usec;
	return stream.str();
}

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	auto error = severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	std::cout << time() << (error ? " F:" : " S:") << pCallbackData->pMessage << std::endl;
	return error ? VK_TRUE : VK_FALSE;
}

void initBase()
{
	epoch = std::chrono::system_clock::now();

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	width = 800;
	height = 600;

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
	window = glfwCreateWindow(width, height, "Space", NULL, NULL);
	if (!window || glfwCreateWindowSurface(VkInstance(instance), window, NULL, (VkSurfaceKHR*)& surface) != VK_SUCCESS)
		throw vk::SurfaceLostKHRError(nullptr);
}

void pickDevice()
{

}

void setup()
{
	initBase();
	pickDevice();
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