vk::SurfaceKHR surface;
vk::PhysicalDevice physicalDevice;
int32_t queueCount, graphicsIndex, presentIndex; 

void pickDevice()
{
	int32_t deviceIndex, maxScore = -1;
	auto devices = instance.enumeratePhysicalDevices();

	for (int32_t i = 0; i < devices.size(); i++)
	{
		int32_t graphicsQueue = -1, presentQueue = -1, swapchainSupport = 0;
		auto deviceFeatures = devices.at(i).getFeatures();
		auto deviceProperties = devices.at(i).getProperties();
		auto queueFamilies = devices.at(i).getQueueFamilyProperties();
		auto deviceExtensions = devices.at(i).enumerateDeviceExtensionProperties();
		auto surfaceFormats = devices.at(i).getSurfaceFormatsKHR(surface);
		auto presentModes = devices.at(i).getSurfacePresentModesKHR(surface);

		for (int32_t j = 0; j < queueFamilies.size(); j++)
		{
			if (!queueFamilies.at(j).queueCount)
				continue;
			if (queueFamilies.at(j).queueFlags & vk::QueueFlagBits::eGraphics)
				graphicsQueue = j;
			if (devices.at(i).getSurfaceSupportKHR(j, surface))
				presentQueue = j;
			if (presentQueue != -1 && graphicsQueue != -1)
				break;
		}

		for (int32_t j = 0; j < deviceExtensions.size(); j++)
			if (swapchainSupport = !std::string(deviceExtensions.at(j).extensionName).compare(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
				break;

		if (graphicsQueue != -1 && presentQueue != -1 && swapchainSupport && surfaceFormats.size() && presentModes.size()
			&& deviceFeatures.fillModeNonSolid && deviceFeatures.samplerAnisotropy && deviceFeatures.sampleRateShading)
		{
			int32_t deviceScore = deviceExtensions.size() + (surfaceFormats.size() + presentModes.size()) * 4;
			if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				deviceScore *= 2;

			if (maxScore < deviceScore)
			{
				maxScore = deviceScore;
				deviceIndex = i;
				graphicsIndex = graphicsQueue;
				presentIndex = presentQueue;
				queueCount = graphicsIndex == presentIndex ? 1 : 2;
			}
		}
	}

	if (maxScore == -1)
		throw vk::DeviceLostError("lol");

	physicalDevice = devices.at(deviceIndex);
}