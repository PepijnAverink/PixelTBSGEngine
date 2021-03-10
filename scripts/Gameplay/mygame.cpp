#include "precomp.h"
#include "mygame.h"
#include <iostream>

Game* game = new MyGame();


void Move(const flecs::entity e, MoveData& tank) {

	float dist = abs(length(tank.target - tank.currentPos));
	float change = tank.speed * e.delta_time();
	if (dist > change)
	{
		float3 dir = normalize(tank.target - tank.currentPos);
		tank.currentPos += dir * change;
	}
	else
	{
		tank.currentPos = tank.target;
		e.remove<MoveData>();
	}

	MoveSpriteTo(e.id(), make_int3(tank.currentPos));
	std::cout << "Worked" << std::endl;
}

void MyGame::Init()
{
	//ShowCursor(false);
	world = GetWorld();
	units.Init(*world);
	terrain.Init(*world);
	AddComponentsToFlecsWorld();
	SpawnWorld();

	tank = SpawnEntity(units.recon, { 210,1,210 });
	float3 targetPos = make_float3(world->sprite[grid[(gridXSize - 1)]]->currPos);
	float3 tankPos = make_float3(world->sprite[tank.id()]->currPos);
	targetPos.y = tankPos.y;
	tank.add<MoveData>()
		.set<MoveData>({ targetPos, 37, tankPos });
}

void Tmpl8::MyGame::AddComponentsToFlecsWorld()
{
	ecs.component<MoveData>();
	ecs.system<MoveData>("Move").each(Move);
}

void Tmpl8::MyGame::SpawnWorld()
{
	uint spriteSize = 20;
	uint3 startPos{ 210,1,210 };
	grid = vector<uint>();
	for (int i = 0; i < gridXSize; ++i)
	{
		for (int ii = 0; ii < gridZSize; ++ii)
		{
		 	flecs::entity entity = SpawnEntity(terrain.grass, make_float3(startPos.x + spriteSize * i, 1, startPos.z + spriteSize * ii));
			grid.push_back(entity.id());
		}
	}
}

void MyGame::Tick(float deltaTime)
{
	world->SetCameraMatrix(mat4::LookAt(make_float3(500, 128, 500), make_float3(300, 1, 300)));

	if (keys[GLFW_KEY_G] && index == 0)
	{
		index++;
		float3 targetPos = make_float3(world->sprite[grid[grid.size()-1]]->currPos);
		float3 tankPos = make_float3(world->sprite[tank.id()]->currPos);
		targetPos.y = tankPos.y;
		tank.add<MoveData>()
		.set<MoveData>({ targetPos, 37, tankPos });
	}

	HandleMouseRaycasting();
	//MoveData* tankData = tank.get_mut<MoveData>();
	//float dist = abs(length(targets[index] - tankData->currentPos));
	//if (dist < 1)
	//{
	//	index++;
	//	if (index > 3)
	//	{
	//		index = 0;
	//	}
	//	tankData->target = make_float3(targets[index]);
	//}

	ecs.progress();
}

float GetAngle(float xa, float ya, float xb, float yb)
{
	return acos(xa * xb + ya * yb) / (sqrt(pow(xa, 2) + pow(ya, 2)) * sqrt(pow(xb, 2) + pow(yb, 2)));
}


void Tmpl8::MyGame::HandleMouseRaycasting()
{
	float3 mouseDir = normalize(make_float3(mousePos.x - 640, (800 - mousePos.y) - 400, 0));
	float angle = GetAngle(0,1, mouseDir.x, mouseDir.y) * 180 / PI;
	//std::cout << std::to_string(mouseDir.x) + " / " + std::to_string(mouseDir.y) << std::endl;
	std::cout << angle << std::endl;
	float3 dir = (world->GetCameraViewDir() * cos(angle)) + (cross(make_float3(0, 1, 0), world->GetCameraViewDir() * sin(angle)) + (make_float3(0, 1, 0) * dot(make_float3(0, 1, 0), world->GetCameraViewDir())) * (1 - cos(angle)));
	uint hit = world->RayCast(make_float3(500, 128, 500), dir);
	MoveSpriteTo(10, make_int3(make_float3(500, 128, 500) + (normalize(dir) * 100)));

	if (hit != -1)
	{
		std::cout << "NICE IT works" + hit << std::endl;
	}
}
flecs::entity Tmpl8::MyGame::SpawnEntity(uint unit, float3 location /* = float3 (0,0,0) */)
{
	uint spriteID = world->CloneSprite(unit);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);

	return ecs.entity(spriteID);
}