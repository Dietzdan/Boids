#include "Boids.h"
#include <iostream>
#include "Urho3D/IO/Log.h"

float Boids::Range_FAttract = 30.0f;
float Boids::Range_FRepel = 20.0f;
float Boids::Range_FAlign = 5.0f;
float Boids::FAttract_Vmax = 5.0f;
float Boids::FAttract_Factor = 4.0f;
float Boids::FRepel_Factor = 2.0f;
float Boids::FAlign_Factor = 2.0f;
Vector<Vector<Vector<int>>> BoidsGrid;

Boids::Boids()
{
	pNode = nullptr;
	pCollsionShape = nullptr;
	pRigidBody = nullptr;
	pObject = nullptr;
}

Boids::~Boids()
{
}

void Boids::Initialise(ResourceCache * pRes, Scene * pScene)
{
	float scale = 1.5f;
	pNode = pScene->CreateChild("Cone");
	pNode->SetPosition(Vector3(Random(180.0f) - 90.0f, 20.0f,Random(180.0f) - 90.0f));
	pObject = pNode->CreateComponent<StaticModel>();
	pRigidBody = pNode->CreateComponent<RigidBody>();
	pRigidBody->SetUseGravity(false);
	pRigidBody->SetLinearVelocity(Vector3(Random(20.0f), 0.0f, Random(20.0f)));
	pRigidBody->SetMass(0.5f);
	pObject->SetModel(pRes->GetResource<Model>("Models/Cone.mdl"));
	pObject->SetMaterial(pRes->GetResource<Material>("Materials/Mushroom.xml"));
	pObject->SetCastShadows(false);
	
	
	
	
	pCollsionShape = pNode->CreateComponent<CollisionShape>();
	pCollsionShape->SetConvexHull(pObject->GetModel());
	
}

void Boids::ComputeForce(Boids * boid)
{
	Vector3 Pmean, Vmean;
	int Pn = 0;
	int Vn = 0;
	Force = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 FS, FC, FA;
	//printf("Neighbours: %i \n", NeighbourVec.Size());
	//search Neighbourhood
	//Vector3 Vec = pRigidBody->GetPosition() - boid[NeighbourVec[0]].pRigidBody->GetPosition();
	//float d = Vec.Length();
	//printf("Vec Length: %f \n" ,d);

	//TODO: Problem with boidlist element 98 y axis is not a number for some reason;
	//for (int x = 0; x < Numboids; x++)
	for(int x =0; x < NeighbourVec.Size();x++)
	{
		int Index = NeighbourVec[x];
		//the current boid?
		//if (this == &boid[x]) continue;
		//sep = vector position of this boid from current boid
		Vector3 sep = pRigidBody->GetPosition() - boid[Index].pRigidBody->GetPosition();
		float d = sep.Length();//distance of boid
		//printf("Vec Length: %f \n" ,d);
		if (d < Range_FAttract)
		{
			//with range,so is a neighbour
			Pmean += boid[Index].pRigidBody->GetPosition();
			Pn++;
		}
		
		if (d < Range_FAlign)
		{
			//with range,so is a neighbour
			Vmean += boid[Index].pRigidBody->GetLinearVelocity();
			Vn++;
		}
		if (d < Range_FRepel)
		{
			FS += sep / d;
		}

	}
	//Cohension force component
	if (Pn > 0)
	{
		//find average position = centre of mass
		Pmean /= Pn;
		Vector3 dir = (Pmean - pRigidBody->GetPosition()).Normalized();
		Vector3 vDesired = dir*FAttract_Vmax;
		FC = (vDesired - pRigidBody->GetLinearVelocity())*FAttract_Factor;
	}
	//Alligment
	if (Vn > 0)
	{
		Vmean /= Vn;
		FA = FAlign_Factor*(Vmean - pRigidBody->GetLinearVelocity());
	}

	//Seperation
	FS *= FRepel_Factor;

	Force = FA + FC + FS;
}

void Boids::Update(float Num)
{
	pRigidBody->ApplyForce(Force);
	Vector3 vel = pRigidBody->GetLinearVelocity();
	float d = vel.Length();
	if (d < 10.0f)
	{
		d = 10.0f;
		pRigidBody->SetLinearVelocity(vel.Normalized()*d);
	}
	else if (d > 50.0f)
	{
		d = 50.0f;
		pRigidBody->SetLinearVelocity(vel.Normalized()*d);
	}
	Vector3 vn = vel.Normalized();
	Vector3 cp = -vn.CrossProduct(Vector3(0.0f, 1.0f, 0.0f));
	float dp = cp.DotProduct(vn);
	pRigidBody->SetRotation(Quaternion(Acos(dp), cp));

	Vector3 p = pRigidBody->GetPosition();
	if (p.y_ < 10.0f)
	{
		p.y_ = 10.0f;
		pRigidBody->SetPosition(p);
	}
	else if (p.y_ > 50.0f)
	{
		p.y_ = 50.0f;
		pRigidBody->SetPosition(p);
	}

	
}

void Boids::FindNeighbours(Boids* boid)
{
	NeighbourVec.Clear();
	//get current boid grid position
	Vector3 Pos = pRigidBody->GetPosition();

	//plus 100 to make all values positive
	int x = Pos.x_ + 100;
	int z = Pos.z_ + 100;
	//divide by 20 to get grid position
	int GridX = x / 10;
	int GridZ = z / 10;

	if (GridZ > 19 || GridX > 19 || GridX < 0 || GridZ < 0)
	{
		
		return;
	}
	//checking current grid square 
	for (int i = 0; i < BoidsGrid[GridX][GridZ].Size(); i++)
	{
		
		if (this == &boid[BoidsGrid[GridX][GridZ][i]]);
		{
			continue;
		}
		NeighbourVec.Push(BoidsGrid[GridX][GridZ][i]);
	}
	
	//checking adjacent grid squares
	if (GridX < 19)
	{
		for (int i = 0; i < BoidsGrid[GridX+1][GridZ].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX + 1][GridZ][i]);
		}
	}
	if (GridX > 0)
	{
		for (int i = 0; i < BoidsGrid[GridX - 1][GridZ].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX - 1][GridZ][i]);
		}
	}
	if (GridZ < 19)
	{
		for (int i = 0; i < BoidsGrid[GridX][GridZ + 1].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX][GridZ + 1][i]);
		}
	}
	if (GridZ > 0)
	{
		for (int i = 0; i < BoidsGrid[GridX][GridZ - 1].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX][GridZ - 1][i]);
		}
	}
	//diagonals 
	if (GridX < 19 && GridZ < 19)
	{
		for (int i = 0; i < BoidsGrid[GridX + 1][GridZ + 1].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX + 1][GridZ + 1][i]);
		}
	}
	if (GridX < 19 && GridZ > 0)
	{
		for (int i = 0; i < BoidsGrid[GridX + 1][GridZ - 1].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX + 1][GridZ - 1][i]);
		}
	}
	if (GridX > 0 && GridZ < 19)
	{
		for (int i = 0; i < BoidsGrid[GridX - 1][GridZ + 1].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX - 1][GridZ + 1][i]);
		}
	}
	if (GridX > 0 && GridZ > 0)
	{
		for (int i = 0; i < BoidsGrid[GridX - 1][GridZ - 1].Size(); i++)
		{
			NeighbourVec.Push(BoidsGrid[GridX - 1][GridZ - 1][i]);
		}
	}

}




void BoidSet::Initialise(ResourceCache * pRes, Scene * pScene)
{
	

	for (int x = 0; x < 20; x++)
	{
	
		BoidsGrid.Push(Vector<Vector<int>>());
		for (int y = 0; y < 20; y++)
		{
			
			BoidsGrid[x].Push(Vector<int>());
			
		}

	}

	for (int x = 0; x < Numboids; x++)
	{
		boidList[x].Initialise(pRes,pScene);
	
	}

	

	
}

void BoidSet::Update(float Num)
{

	GridBoids(boidList);
	
	for (int i = 0; i < Numboids; i++)
	{
		boidList[i].FindNeighbours(boidList);
		boidList[i].ComputeForce(boidList);
		boidList[i].Update(Num);
	}
	
	ClearGrid();
}

void BoidSet::GridBoids(Boids * boid)
{
	for (int i = 0; i < Numboids; i++)
	{
		//get the position of the current boid
		Vector3 Pos = boid[i].pRigidBody->GetPosition();
		//plus 100 to make all values positive
		int x = Pos.x_ + 100;
		int z = Pos.z_ + 100;
		//divide by 20 to get grid position
		int GridX = x / 10;
		int GridZ = z / 10;
		if (GridZ > 19 || GridX > 19 || GridX < 0 || GridZ < 0)
		{

		
			continue;
		}
		BoidsGrid[GridX][GridZ].Push(i);
	}
}

void BoidSet::ClearGrid()
{
	for (int x = 0; x < 20; x++)
	{
		for (int y = 0; y < 20; y++)
		{
			for (int z = 0; z < BoidsGrid[x][y].Size(); z++)
			{
				BoidsGrid[x][y].Clear();
			}
		}

	}
}






