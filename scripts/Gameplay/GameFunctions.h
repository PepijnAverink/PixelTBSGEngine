#pragma once
#include "flecs.h"
#include "GameData.h"

namespace Tmpl8
{

namespace GameplayFunctions
{
	void RotateEntity(const flecs::entity& entity, Rotation& rotation)
	{
		if (!rotation.reachedTarget)
		{
			float diff = rotation.currentRotation - rotation.target;
			float modifier = diff > 0 ? abs(diff) < 180 ? -1 : 1 : abs(diff) < 180 ? 1 : -1;
			float change = rotation.speed * entity.delta_time();
			World* world = GetWorld();

			if (abs(diff) > change)
			{
				rotation.currentRotation += change * modifier;

				if (rotation.currentRotation >= 180)
				{
					rotation.currentRotation = -180;
				}
				else if (rotation.currentRotation <= -180)
				{
					rotation.currentRotation = 180;
				}
			}
			else
			{
				rotation.currentRotation = rotation.target;
				rotation.reachedTarget = true;
			}
			world->RotateSprite(entity.id(), 0, 1, 0, DegreesToRadians(rotation.currentRotation));
		}
	}

void UnitHeight(const flecs::entity& entity, float3 entityPos)
{
	int currentTileHeight = heightMap[GridPosToIndex(GetIndexes(entityPos),mapSize.x)];
	GetWorld()->MoveSpriteTo(entity.id(), entityPos.x, 16 + currentTileHeight, entityPos.z);
}


void MoveEntity(const flecs::entity& entity, MoveLocation& moveData) {

	if (!moveData.reachedTarget)
	{
		World* world = GetWorld();
		if (entity.has<Rotation>())
		{
			if (!entity.get<Rotation>()->reachedTarget)
			{
				return;
			}
		}
		else
		{
			float3 dir = normalize(moveData.targetPos - moveData.startPos);
			float targetRot = atan2(dir.z, dir.x);
			world->RotateSprite(entity.id(), 0, 1, 0, targetRot);
		}
		float dist = abs(length(moveData.targetPos - moveData.startPos));
		//std::cout << to_string(entity.delta_time()) << std::endl;
		moveData.progress += (moveData.speed * entity.delta_time() / (dist * 0.01f));
		if (moveData.progress > 1)
		{
			moveData.progress = 1;
			moveData.reachedTarget = true;
		}
		moveData.currentPos = lerp(moveData.startPos, moveData.targetPos, moveData.progress);
		MoveSpriteTo(entity.id(), make_int3(moveData.currentPos));
		UnitHeight(entity, moveData.currentPos);
	}
}

void MoveCurveEntity(const flecs::entity& entity, MoveLocation& moveData) {

	if (!moveData.reachedTarget)
	{
		World* world = GetWorld();
		float dist = abs(length(moveData.targetPos - moveData.startPos));
		moveData.progress += (moveData.speed * entity.delta_time() / (dist * 0.01f));
		if (moveData.progress > 1)
		{
			moveData.progress = 1;
			moveData.reachedTarget = true;
		}
		float3 controlPoint = moveData.targetPos - (moveData.startPos * 0.5f);
		float3 bezierPoint = CalculateBezierPoint(moveData.progress, moveData.startPos, controlPoint, moveData.targetPos);

		float3 dir = normalize(bezierPoint - moveData.currentPos);
		float targetRot = atan2(dir.z, dir.x);
		world->RotateSprite(entity.id(), 0, 1, 0, targetRot);
		moveData.currentPos = bezierPoint;
		world->MoveSpriteTo(entity.id(), moveData.currentPos.x, moveData.currentPos.y, moveData.currentPos.z);
		UnitHeight(entity, moveData.currentPos);
	}
}

void MoveAttackEntity(const flecs::entity& entity, MoveAttack& moveAttack)
{
	if (moveAttack.target != 0)
	{
		if (entity.has<MoveLocation>() && !ecs.entity(moveAttack.target).has<Dead>())
		{
			World* world = GetWorld();
			const MoveLocation* moveLocation = entity.get<MoveLocation>();
			float3 target = make_float3(world->sprite[moveAttack.target]->currPos);
			float dist = abs(length(target - moveLocation->currentPos));
			if (dist > moveAttack.shootingRange)
			{
				//recalculate point to go
				int3 indexes = make_int3(round(target.x / 16), 0, round(target.z / 16));
				float3 newTarget = make_float3(indexes.x * 16, target.y, indexes.z * 16);
				MovePathFinding* movePathFinding = entity.get_mut<MovePathFinding>();
				if (movePathFinding->target != newTarget)
				{
					movePathFinding->target = newTarget;
					movePathFinding->reachedTarget = false;
					entity.modified<MovePathFinding>();
				}
				return;
			}
			MovePathFinding* movePathFinding = entity.get_mut<MovePathFinding>();
			moveAttack.target = 0;
			movePathFinding->target = moveLocation->targetPos;
		}
	}
}

void MoveUnitOverPath(const flecs::entity& entity, MovePathFinding& movePathFinding)
{
	MoveLocation* moveLocation = entity.get_mut<MoveLocation>();
	if (!movePathFinding.reachedTarget && moveLocation->reachedTarget)
	{
		if (!movePathFinding.setOldPosUnitCost)
		{
			//pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(movePathFinding.oldPos), mapSize.x));
			movePathFinding.setOldPosUnitCost = true;
		}
		if (make_int3(moveLocation->currentPos) == make_int3(movePathFinding.target))
		{
			pathfinder.SetUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x));
			movePathFinding.reachedTarget = true;
			return;
		}
		//pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x));
		movePathFinding.flowField = pathfinder.GetFlowFlield(GetIndexes(movePathFinding.target));
		int targetIndex = movePathFinding.flowField[GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x)];
		//pathfinder.SetUnitInUnitFieldWithAmount(GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x), 10);

		if (targetIndex > 0)
		{
			int2 newTargetGridPos = IndexToGridPos(targetIndex, mapSize.x);
			float3 newTarget = make_float3((newTargetGridPos.x + 10) * 16, movePathFinding.target.y, (newTargetGridPos.y + 10) * 16);
			if (pathfinder.unitField[GridPosToIndex(GetIndexes(newTarget), mapSize.x)] < 5)
			{
				pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x));
				pathfinder.SetUnitInUnitFieldWithAmount(GridPosToIndex(GetIndexes(newTarget), mapSize.x), 10);
				movePathFinding.oldPos = moveLocation->currentPos;
				movePathFinding.setOldPosUnitCost = false;
				moveLocation->startPos = moveLocation->currentPos;
				moveLocation->targetPos = newTarget;
				moveLocation->reachedTarget = false;
				moveLocation->progress = 0;
				entity.modified<MoveLocation>();

				//Rotation
				float3 dir = normalize(moveLocation->targetPos - moveLocation->startPos);
				float targetRot = atan2(dir.z, dir.x);
				Rotation* rotation = entity.get_mut<Rotation>();
				rotation->target = RadiansToDegrees(targetRot);
				rotation->reachedTarget = false;
				entity.modified<Rotation>();

				if (entity.has<ChildData>())
				{
					ChildData* childData = entity.get_mut<ChildData>();
					flecs::entity child = ecs.entity(childData->childID);
					Rotation* childRot = child.get_mut<Rotation>();
					childRot->target = RadiansToDegrees(targetRot);
					childRot->reachedTarget = false;
					child.modified<Rotation>();
				}
			}
		}
	}
}

void SpawnTankBullet(const uint shootObject, const int3 location, const ShotObjectData& shotobjectData)
{
	World* world = GetWorld();
	uint spriteID = world->CloneSprite(shootObject);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);
	flecs::entity entity = ecs.entity(spriteID);
	entity.add<ShotObjectData>()
		.set<ShotObjectData>(shotobjectData)
		.add<TankBullet>();
}

void SpawnArtilleryBullet(const uint shootObject, const int3 location, const ShotObjectData& shotobjectData)
{
	World* world = GetWorld();
	uint spriteID = world->CloneSprite(shootObject);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);
	flecs::entity entity = ecs.entity(spriteID);
	entity.add<ShotObjectData>()
		.set<ShotObjectData>(shotobjectData)
		.add<ArtilleryBullet>();
}

int GetTarget(float fireRange, int3 currentPos)
{
	for (auto it : ecs.filter(filterPlayer2))
	{
		for (auto index : it)
		{
			int enemyID = it.entity(index).id();
			if (GetWorld()->sprite[enemyID] != nullptr)
			{
				int3 enemyPos = GetWorld()->sprite[enemyID]->currPos;
				float dist = abs(length(make_float3(enemyPos - currentPos)));
				if (dist < fireRange)
				{
					return enemyID;
				}
			}
		}
	}
	return -1;
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
	int enemyID = GetTarget(weaponData.fireRange,currentPos);
	

	if (enemyID > world->sprite.size() || enemyID < 0)
	{
		return;
	}

	int3 enemyPos = world->sprite[enemyID]->currPos;
	if (entity.has< Rotation>())
	{
		float3 dir = normalize(make_float3(enemyPos - currentPos));
		float targetRot = atan2(dir.z, dir.x);
		Rotation* rotation = entity.get_mut<Rotation>();
		if (RadiansToDegrees(targetRot) != rotation->target)
		{
			rotation->target = RadiansToDegrees(targetRot);
			rotation->reachedTarget = false;
			entity.modified<Rotation>();
			return;
		}
		else if (!entity.get<Rotation>()->reachedTarget)
		{
			return;
		}
	}
	else
	{
		float3 dir = normalize(make_float3(enemyPos - currentPos));
		float targetRot = atan2(dir.z, dir.x);
		world->RotateSprite(entity.id(), 0, 1, 0, targetRot);
	}

	MoveLocation moveData{ weaponData.shotObjectSpeed,make_float3(currentPos),make_float3(currentPos), make_float3(enemyPos),false,0 };
	ShotObjectData shotObjectData{ moveData, enemyID};
	switch (weaponData.bulletType)
	{
		case BulletType::Bullet_Tank:
		{
			SpawnTankBullet(weaponData.shotObjectID, currentPos, shotObjectData);
			break;
		}
		case BulletType::Bullet_Artillery:
		{
			SpawnArtilleryBullet(weaponData.shotObjectID, currentPos, shotObjectData);
			break;
		}
	}
	weaponData.currentReloadTime = weaponData.reloadTime;
}


void MoveTankBullet(const flecs::entity entity, TankBullet& tankBullet, ShotObjectData& shotObjectData)
{
	flecs::entity target = ecs.entity(shotObjectData.targetID);
	if (!target.has<Dead>())
	{
		MoveEntity(entity, shotObjectData.moveData);
		if (shotObjectData.moveData.reachedTarget)
		{
			GetWorld()->DisableSprite(shotObjectData.targetID);
			target.add<Dead>();
			GetWorld()->DisableSprite(entity.id());
			entity.add<Dead>();
		}
	}
	else
	{
		GetWorld()->DisableSprite(entity.id());
		entity.add<Dead>();
	}
}

void MoveArtilleryBullet(const flecs::entity entity, ArtilleryBullet& artilleryBullet, ShotObjectData& shotObjectData)
{
	MoveCurveEntity(entity, shotObjectData.moveData);
	if (shotObjectData.moveData.reachedTarget)
	{
		GetWorld()->DisableSprite(entity.id());
		entity.add<Dead>();
	}
}

void HandleChilds(const flecs::entity entity, ChildData& childData)
{
	int3 parentPos = GetWorld()->sprite[entity.id()]->currPos;
	float3 newPos = parentPos + childData.offset;
	if (make_int3(newPos) != GetWorld()->sprite[childData.childID]->currPos)
	{
		GetWorld()->MoveSpriteTo(childData.childID, newPos.x, newPos.y, newPos.z);
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
