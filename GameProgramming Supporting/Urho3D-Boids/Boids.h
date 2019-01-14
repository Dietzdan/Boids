#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
static const int Numboids = 200;

namespace Urho3D
{
	class Node;
	class Scene;
	class RigidBody;
	class CollisionShape;
	class ResourceCache;
}
// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

class Boids
{
	static float Range_FAttract;
	static float Range_FRepel;
	static float Range_FAlign;
	static float FAttract_Factor;
	static float FRepel_Factor;
	static float FAlign_Factor;
	static float FAttract_Vmax;

public:
	Node* pNode;
	RigidBody* pRigidBody;
	CollisionShape* pCollsionShape;
	StaticModel* pObject;
	Vector3 Force;
	Vector<int> NeighbourVec; 

	Boids();
	~Boids();
	
	void Initialise(ResourceCache* pRes, Scene* pScene);
	void ComputeForce(Boids* boid);
	void Update(float Num);
	void FindNeighbours(Boids* boid);
	
};

class BoidSet
{
public:
	Boids boidList[Numboids];
	
	//Vector<Vector<Vector<int>>> BoidsGrid;
	BoidSet() {};

	void Initialise(ResourceCache* pRes, Scene* pScene);
	void Update(float Num);
	void GridBoids(Boids* boid);
	void ClearGrid();
	

};