#pragma once
#include "encapVk.h"
#include "objLoader.h"
#include "MPipeline.h"
class MObject
{
public:
	struct MVPUniformBuffer {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 lookat;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};
	struct Transform
	{
		glm::vec3 position;
		glm::vec3 rotate;
		glm::vec3 scale;
	};
	objLoader* obj;
	Transform transform;
	glm::mat4 modelMat;
	float roughness;
	MObject(objLoader* m_obj, Transform m_transform, float m_roughness);
};

