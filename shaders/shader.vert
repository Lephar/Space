#version 460
#extension GL_ARB_separate_shader_objects: enable

layout(binding = 0) uniform Transformation {
    mat4 model;
    mat4 view;
    mat4 projection;
} transformation;

layout(location = 0) in vec3 inputPosition;
layout(location = 1) in vec3 inputColor;

layout(location = 0) out vec3 outputColor;

void main()
{
    gl_Position = transformation.projection * transformation.view * transformation.model * vec4(inputPosition, 1.0);
    outputColor = inputColor;
}