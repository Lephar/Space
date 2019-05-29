cmake_minimum_required (VERSION 3.8)

project(Space)
add_executable(Space Space.cpp)
add_dependencies(Space Shaders)

find_package(Vulkan REQUIRED)
target_link_libraries(Space Vulkan::Vulkan ${PROJECT_SOURCE_DIR}/../GLFW/lib-vc2019/glfw3.lib)
target_include_directories(Space PRIVATE Vulkan::Vulkan ${PROJECT_SOURCE_DIR}/../GLFW/include ${PROJECT_SOURCE_DIR}/../GLM)

add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/data/vert.spv COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/shader.vert -o ${PROJECT_SOURCE_DIR}/data/vert.spv -O)
add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/data/frag.spv COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/shader.frag -o ${PROJECT_SOURCE_DIR}/data/frag.spv -O)
add_custom_target(Shaders DEPENDS ${PROJECT_SOURCE_DIR}/data/vert.spv ${PROJECT_SOURCE_DIR}/data/frag.spv)

file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${PROJECT_BINARY_DIR})