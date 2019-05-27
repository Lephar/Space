cmake_minimum_required (VERSION 3.8)

add_executable(Space "Space.cpp")

if(UNIX)
	target_link_libraries(Space -lglm -lglfw -lvulkan)
elseif(WIN32)
	find_package(Vulkan REQUIRED)
	target_link_libraries(Space Vulkan::Vulkan "../GLFW/lib-vc2019/glfw3.lib")
	target_include_directories(Space PRIVATE Vulkan::Vulkan "../GLFW/include" "../GLM")
endif()