#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>

std::chrono::time_point<std::chrono::system_clock> epoch;

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
	std::cout << time() << (error ? " F: " : " S: ") << pCallbackData->pMessage << std::endl;
	return error ? VK_TRUE : VK_FALSE;
}

void initBase()
{
	epoch = std::chrono::system_clock::now();
}