#pragma once
#include "flecs.h"
#include "GameData.h"

namespace Tmpl8
{

namespace GameplayFunctions
{

void OnMoveStart(const flecs::entity entity, MoveData& moveData)
{
	moveData.reachedTarget = false;
}

void MoveEntity(const flecs::entity entity, MoveData& moveData) {

	float dist = abs(length(moveData.target - moveData.currentPos));
	float change = moveData.speed * entity.delta_time();
	if (dist > change)
	{
		float3 dir = normalize(moveData.target - moveData.currentPos);
		moveData.currentPos += dir * change;
	}
	else
	{
		moveData.currentPos = moveData.target;
		moveData.reachedTarget = true;
		entity.disable<MoveData>();
	}

	MoveSpriteTo(entity.id(), make_int3(moveData.currentPos));
}

void SpawnShotObject(const uint shootObject, const int3 location, const ShotObjectData& shotobjectData)
{
	World* world = GetWorld();
	uint spriteID = world->CloneSprite(shootObject);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);
	flecs::entity entity = ecs.entity(spriteID);
	entity.add<ShotObjectData>()
		.set<ShotObjectData>(shotobjectData);
}

void WeaponUpdate(const flecs::entity entity, WeaponData& weaponData)
{
	if (weaponData.currentReloadTime > 0)
	{
		weaponData.currentReloadTime -= entity.delta_time();
		return;
	}

	World* world = GetWorld();

	int3 currentPos = world->sprite[entity.id()]->currPos;

	auto filter = flecs::filter(ecs)
		.include<Player2>()
		.include_kind(flecs::MatchAll);

	for (auto it : ecs.filter(filter))
	{
		for (auto index : it)
		{
			int3 enemyPos = world->sprite[it.entity(index).id()]->currPos;
			float dist = abs(length(make_float3(enemyPos - currentPos)));
			if (dist < weaponData.fireRange)
			{
				MoveData moveData{ make_float3(enemyPos),weaponData.shotObjectSpeed,make_float3(currentPos),false };
				ShotObjectData shotObjectData{ moveData, it.entity(index),weaponData.dmg };
				SpawnShotObject(weaponData.shotObjectID, currentPos, shotObjectData);
				weaponData.currentReloadTime = weaponData.reloadTime;
				return;
			}
		}
	}

}


void MoveShotObject(const flecs::entity entity, ShotObjectData& shotObjectData)
{
	if (!shotObjectData.target.has<Dead>())
	{
		if (shotObjectData.moveData.reachedTarget)
		{
			GetWorld()->MoveSpriteTo(shotObjectData.target.id(), 0, 10, 0);
			shotObjectData.target.add<Dead>();
			GetWorld()->MoveSpriteTo(entity.id(), 0, 10, 0);
			entity.add<Dead>();
			entity.disable<ShotObjectData>();
		}
		else
		{
			MoveEntity(entity, shotObjectData.moveData);
		}
	}
	else
	{
		entity.disable<ShotObjectData>();
	}
}

}
}
