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
		e.disable<MoveData>();
	}

	MoveSpriteTo(e.id(), make_int3(tank.currentPos));
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

	if (mouseClicked)
	{
		HandleMouseRaycasting();
	}

	ecs.progress();
}

void Tmpl8::MyGame::HandleMouseRaycasting()
{
	float2 screenResolution = make_float2(1280,800);
	float fovX = PI/3.0f;

	float mx = (float)(((screenResolution.x - mousePos.x) - screenResolution.x * 0.5) * (1.0 / screenResolution.x) * fovX * 0.5);
	float my = (float)(((screenResolution.y - mousePos.y) - screenResolution.y * 0.5) * (1.0 / screenResolution.y) * fovX * 0.5);
	float3 cameraForwar = world->GetCameraViewDir();
	float3 cameraRight = cross(cameraForwar, make_float3(0, 1, 0));
	float3 dx = cameraRight * mx;
	float3 dy = make_float3(0,1,0) * my;

	float3 dir = normalize(world->GetCameraViewDir() + (dx + dy) * 2.0);

	uint hit = world->RayCast(make_float3(500, 128, 500), dir);

	if (hit > 0)
	{
		tank.enable<MoveData>();
		float3 targetPos = make_float3(world->sprite[hit]->currPos);
		float3 tankPos = make_float3(world->sprite[tank.id()]->currPos);
		targetPos.y = tankPos.y;
		MoveData* moveData = tank.get_mut<MoveData>();
		moveData->target = targetPos;
		tank.modified<MoveData>();
	}
}
flecs::entity Tmpl8::MyGame::SpawnEntity(uint unit, float3 location /* = float3 (0,0,0) */)
{
	uint spriteID = world->CloneSprite(unit);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);

	return ecs.entity(spriteID);
}