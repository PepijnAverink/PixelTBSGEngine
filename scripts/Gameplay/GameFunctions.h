#pragma once
#include "flecs.h"
#include "GameData.h"

namespace Tmpl8
{

namespace GameplayFunctions
{

	void OnRotationStart(const flecs::entity entity, Rotation& rotation)
	{
		rotation.reachedTarget = false;
	}

	void RotateEntity(const flecs::entity entity, Rotation& rotation)
	{
		float diff = rotation.currentRotation - rotation.target;

		float modifier = 0;
		if (diff > 0)
		{
			modifier = abs(diff) < 180 ? -1 : 1;
		}
		else
		{
			modifier = abs(diff) < 180 ? 1 : -1;
		}

		float change = rotation.speed * entity.delta_time();
		World* world = GetWorld();
		
		if (abs(diff) > change)
		{	
			rotation.currentRotation += change * modifier;

			if (rotation.currentRotation >= 180)
			{
				rotation.currentRotation = -180;
			}
			else if(rotation.currentRotation <= -180)
			{
				rotation.currentRotation = 180;
			}


			world->RotateSprite(entity.id(), 0, 1, 0, DegreesToRadians(rotation.currentRotation));
		}
		else
		{
			rotation.currentRotation = rotation.target;
			world->RotateSprite(entity.id(), 0, 1, 0, DegreesToRadians(rotation.target));
			rotation.reachedTarget = true;
			entity.disable<Rotation>();
		}
	}


void OnMoveStart(const flecs::entity entity, MoveLocation& moveData)
{
	moveData.reachedTarget = false;
}

void UnitHeight(const flecs::entity entity, float3 entityPos, float3 dir)
{
	World* world = GetWorld();


	float3 enityPosNotRounded = entityPos / 16.0f;
	int3 roundedEntityPos = make_int3(round(enityPosNotRounded.x), round(enityPosNotRounded.y), round(enityPosNotRounded.z));
	roundedEntityPos *= 16;

	int2 indexes = make_int2((roundedEntityPos.x / 16.0f) - 10, (roundedEntityPos.z / 16.0f) - 10);
	int currentTileHeight = heightMap[indexes.x * 32 + indexes.y];

	if (currentTileHeight == 3)
	{
		world->MoveSpriteTo(entity.id(), entityPos.x, 32, entityPos.z);
		return;
	}
	else if (currentTileHeight == 0)
	{
		world->MoveSpriteTo(entity.id(), entityPos.x, 16, entityPos.z);
		return;
	}
	int targetTileHeight = heightMap[(indexes.x + dir.x) * 32 + (indexes.y + dir.z)];
	
	if (currentTileHeight == targetTileHeight)
	{
		world->MoveSpriteTo(entity.id(), entityPos.x, ((currentTileHeight - 1) * 8) + 16, entityPos.z);
		return;
	}

	float3 diff = entityPos - (roundedEntityPos + (dir * 8));
	float heightDiffProcent = clamp(abs(round(length(diff))) / 16.0f, 0.0f, 1.0f);


	float yPos = 0;
	if (targetTileHeight < currentTileHeight)
	{
		yPos = ((heightDiffProcent + currentTileHeight - 1) * 8) + 16;
	}
	else
	{
		yPos = (((1 - heightDiffProcent) + currentTileHeight - 1) * 8) + 16;
	}
	world->MoveSpriteTo(entity.id(), entityPos.x, yPos, entityPos.z);
}


void MoveEntity(const flecs::entity entity, MoveLocation& moveData) {

	float dist = abs(length(moveData.target - moveData.currentPos));
	float change = moveData.speed * entity.delta_time();
	if (dist > change)
	{
		World* world = GetWorld();
		float3 dir = normalize(moveData.target - moveData.currentPos);

		if (entity.has< Rotation>())
		{
			if (!entity.get<Rotation>()->reachedTarget)
			{
				return;
			}
		}
		else
		{
			float targetRot = atan2(dir.z, dir.x);
			world->RotateSprite(entity.id(), 0, 1, 0, targetRot);
		}


		moveData.currentPos += dir * change;
		MoveSpriteTo(entity.id(), make_int3(moveData.currentPos));
		UnitHeight(entity, moveData.currentPos, dir);
	}
	else
	{
		moveData.currentPos = moveData.target;
		MoveSpriteTo(entity.id(), make_int3(moveData.currentPos));
		UnitHeight(entity, moveData.currentPos, make_float3(0,0,0));
		moveData.reachedTarget = true;
		entity.disable<MoveLocation>();
	}
}

void MoveAttackEntity(const flecs::entity entity, MoveAttack& moveAttack)
{
	if (entity.has<MoveLocation>() && !ecs.entity(moveAttack.target).has<Dead>())
	{
		World* world = GetWorld();
		MoveLocation* moveLocation = entity.get_mut<MoveLocation>();
		float3 target = make_float3(world->sprite[moveAttack.target]->currPos);
		float dist = abs(length(target - moveLocation->currentPos));
		if (dist <= moveAttack.shootingRange)
		{
			entity.disable<MoveAttack>().disable<MoveLocation>();
			return;
		}
		entity.enable<MoveLocation>();
		if (moveLocation->target != target)
		{
			moveLocation->target = target;
			entity.modified<MoveLocation>();
		}
	}
	else
	{
		entity.disable<MoveAttack>();
		return;
	}
}

void SpawnShotObject(const uint shootObject, const int3 location, const ShotObjectData& shotobjectData)
{
	World* world = GetWorld();
	uint spriteID = world->CloneSprite(shootObject);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);
	flecs::entity entity = ecs.entity(spriteID);
	entity.add<ShotObjectData>()
		.set<ShotObjectData>(shotobjectData);
	//cout << "Created Sprite: " + to_string(spriteID) << endl;
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

	for (auto it : ecs.filter(filterPlayer2))
	{
		for (auto index : it)
		{
			int3 enemyPos = world->sprite[it.entity(index).id()]->currPos;
			float dist = abs(length(make_float3(enemyPos - currentPos)));
			if (dist < weaponData.fireRange)
			{
				MoveLocation moveData{ make_float3(enemyPos),weaponData.shotObjectSpeed,make_float3(currentPos),false };
				ShotObjectData shotObjectData{ moveData, it.entity(index).id(),weaponData.dmg };
				SpawnShotObject(weaponData.shotObjectID, currentPos, shotObjectData);
				weaponData.currentReloadTime = weaponData.reloadTime;
				return;
			}
		}
	}
}


void MoveShotObject(const flecs::entity entity, ShotObjectData& shotObjectData)
{
	flecs::entity target = ecs.entity(shotObjectData.targetID);
	if (!target.has<Dead>())
	{
		if (shotObjectData.moveData.reachedTarget)
		{
			GetWorld()->DisableSprite(shotObjectData.targetID);
			target.add<Dead>();
			GetWorld()->DisableSprite(entity.id());
			entity.add<Dead>();
			entity.disable<ShotObjectData>();
			//cout << "Destory: " + to_string(entity.id()) << endl;
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

bool AABB(float2 pos1, float2 size1, float2 pos2, float2 size2)
{
	return pos1.x < pos2.x + size2.x &&
		pos1.x + size1.x > pos2.x &&
		pos1.y < pos2.y + size2.y &&
		pos1.y + size1.y > pos2.y;
}

}
}
