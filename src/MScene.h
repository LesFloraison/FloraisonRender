#pragma once
#include "encapVk.h"
#include "objLoader.h"
#include "MObject.h"
#include "JSON.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include <iostream>
#include "objLoader.h"
class MScene
{
public:
	struct pointLight {
		glm::vec3 position;
		glm::vec3 light;
	};
	struct waterLayerInfo {
		glm::vec3 position;
		glm::vec2 scale;
		float warpScale_1;
		float warpScale_2;
		glm::vec2 flow_1;
		glm::vec2 flow_2;
	};
	map<int, objLoader*> objMap;
	vector<MObject*> objectList;
	vector<glm::vec3> lightInfos;
	vector<waterLayerInfo> waterLayerInfos;
	VkBuffer TLASBuffer;
	VkDeviceMemory TLASMemory;
	VkAccelerationStructureKHR TLAS;
	vector<double> sceneVertexStream;
	vector<double> instanceStream;
	VkBuffer sceneVertexBuffer;
	VkDeviceMemory sceneVertexMemory;
	VkBuffer sceneReferenceBuffer;
	VkDeviceMemory sceneReferenceMemory;
	VkBuffer sceneInstanceBuffer;
	VkDeviceMemory sceneInstanceMemory;
	objLoader* waterLayer;

	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;
	std::vector<btRigidBody*> rigidBodyArr;
	std::vector<btCollisionShape*> shapeArr;
	btRigidBody* controllerRigidBody;
	float controllerVelocity = 0;
	int controllerJumpImpulse = 0;
	int controllerJumpStreak = 0;
	int controllerJumpRemain = 0;

	struct MyContactResultCallback : public btCollisionWorld::ContactResultCallback {
		bool& isGroundedRef;
		btRigidBody* body;

		MyContactResultCallback(btRigidBody* rb, bool& isGrounded) : isGroundedRef(isGrounded), body(rb) {}

		virtual btScalar addSingleResult(btManifoldPoint& cp,
			const btCollisionObjectWrapper* colObj0, int partId0, int index0,
			const btCollisionObjectWrapper* colObj1, int partId1, int index1) override {
			if (cp.getDistance() < 0.01f) {
				isGroundedRef = true;
			}
			return 0;
		}
	};

	inline void checkIfGrounded(btDiscreteDynamicsWorld* world, btRigidBody* capsuleRigidBody, bool& isGrounded) {
		isGrounded = false;
		MyContactResultCallback callback(capsuleRigidBody, isGrounded);
		world->contactTest(capsuleRigidBody, callback);
	}

	MScene(string path);
	~MScene();
	void sceneUpdate();
	void addObject(objLoader* m_obj, MObject::Transform m_transformInfo, float roughness);
	void addPointLight(glm::vec3 lightPosition, glm::vec3 lightScale, glm::vec3 lightColor, float lightMaxRad);
	void setDirectLight(glm::vec3 lightVec, glm::vec3 lightColor);
	void drawScene(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout);
	void drawForward(VkCommandBuffer commandBuffer, MPipeline* pipeline, VkPipelineLayout pipelineLayout);
	void createTLAS();
	void createSceneVertexBuffer();
	void createInstanceStorageBuffer();
};

