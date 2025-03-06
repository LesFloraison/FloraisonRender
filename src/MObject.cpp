#include "MObject.h"

MObject::MObject(objLoader* m_obj, Transform m_transform, float m_roughness)
{
	obj = m_obj;
	transform = m_transform;
	roughness = m_roughness;
	glm::mat4 T = glm::translate(glm::mat4(1), m_transform.position);
	glm::mat4 R = glm::rotate(glm::mat4(1), glm::radians(m_transform.rotate.x), glm::vec3(1.0f, 0.0f, 0.0f));
	R = glm::rotate(R, glm::radians(m_transform.rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
	R = glm::rotate(R, glm::radians(m_transform.rotate.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 S = glm::scale(glm::mat4(1), m_transform.scale);
	modelMat = T * R * S;
}
