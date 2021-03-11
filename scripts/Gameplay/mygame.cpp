#include "precomp.h"
#include "mygame.h"
#include "GameFunctions.h"
#include <iostream>

Game* game = new MyGame();


void MyGame::Init()
{
	//ShowCursor(false);
	world = GetWorld();
	units.Init(*world);
	terrain.Init(*world);
	AddComponentsToFlecsWorld();
	SpawnWorld();

	flecs::entity tank = SpawnEntity(units.recon, 1, { 210,3,210 });
	float3 tankPos = make_float3(world->sprite[tank.id()]->currPos);
	float tankRot = 0;
	tank.add<MoveData>()
		.set<MoveData>({ tankPos, 37, tankPos })
		.disable<MoveData>()
		.add<WeaponData>()
		.set<WeaponData>({ tankRot, 100,120,500,0,units.bazooka,tankRot,5,0,100});


	SpawnEntity(units.aAirMissile, 2, { 390,3,210 });
	SpawnEntity(units.aAirMissile, 2, { 210,3,390 });
	SpawnEntity(units.aAirMissile, 2, { 390,3,390 });

}

void Tmpl8::MyGame::AddComponentsToFlecsWorld()
{
	ecs.component<MoveData>();
	ecs.component<Player1>();
	ecs.component<Player2>();
	ecs.component<Dead>();
	ecs.system<MoveData>("MoveEntity").each(GameplayFunctions::MoveEntity);
	ecs.system<MoveData>("OnMoveStart").kind(flecs::Disabled).each(GameplayFunctions::OnMoveStart);
	ecs.system<WeaponData>("WeaponUpdate").each(GameplayFunctions::WeaponUpdate);
	ecs.system<ShotObjectData>("MoveShotObject").each(GameplayFunctions::MoveShotObject);
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
			flecs::entity entity = SpawnEntity(terrain.grass, 0, make_float3(startPos.x + spriteSize * i, 1, startPos.z + spriteSize * ii));
			grid.push_back(entity.id());
		}
	}
}

void MyGame::Tick(float deltaTime)
{
	world->SetCameraMatrix(mat4::LookAt(make_float3(500, 128, 500), make_float3(300, 1, 300)));

	if (mouseClicked)
	{
		uint objectHit = HandleMouseRaycasting();
		if (objectHit > 0)
		{
			SetSelectedTanksTarget(make_float3(world->sprite[objectHit]->currPos));
		}
	}

	ecs.progress();
}

uint Tmpl8::MyGame::HandleMouseRaycasting()
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

	return world->RayCast(make_float3(500, 128, 500), dir);
}

//void Tmpl8::MyGame::SetTankTarget(float3 target)
//{
//	tank.enable<MoveData>();
//	float3 tankPos = make_float3(world->sprite[tank.id()]->currPos);
//	target.y = tankPos.y;
//	MoveData* moveData = tank.get_mut<MoveData>();
//	moveData->target = target;
//	tank.modified<MoveData>();
//}


void Tmpl8::MyGame::SetSelectedTanksTarget(float3 target)
{
}

flecs::entity Tmpl8::MyGame::SpawnEntity(uint unit, int playerID /* = 0*/, float3 location /* = float3 (0,0,0) */)
{
	uint spriteID = world->CloneSprite(unit);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);
	flecs::entity entity = ecs.entity(spriteID);
	if (playerID == 1) { entity.add<Player1>(); }
	else if (playerID == 2) { entity.add<Player2>(); }

	return entity;
}