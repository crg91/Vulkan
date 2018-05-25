#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

vec3 lightVector = vec3(0.0f, 0.0f, -1.0f);
void main()
{
	outColor = vec4(max(0.0f, dot(lightVector, normal)) * fragColor, 1.0f) + vec4(fragColor, 1.0f) * 0.3f;
}