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
			float modifier = diff > 0 ? abs(diff) < 180 ? -1 : 1 : abs(diff) <= 180 ? 1 : -1;
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

void OnAddMoveLocation(const flecs::entity& entity, MoveLocation& moveData)
{
	UnitHeight(entity, make_float3(GetWorld()->sprite[entity.id()]->currPos));
}

void MoveEntity(const flecs::entity& entity, MoveLocation& moveData) 
{
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
			world->RotateSprite(entity.id(), 0, 1, 0, DegreesToRadians(targetRot));
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
		float3 diff = moveData.targetPos - moveData.startPos;
		diff.y = 0;
		float dist = abs(length(diff));
		moveData.progress += (moveData.speed * entity.delta_time() / (dist * 0.01f));
		if (moveData.progress > 1)
		{
			moveData.progress = 1;
			moveData.reachedTarget = true;
		}
		float3 controlPoint = moveData.targetPos - (diff * 0.5f);
		controlPoint.y = 50;
		float3 bezierPoint = CalculateBezierPoint(moveData.progress, moveData.startPos, controlPoint, moveData.targetPos);

		float3 dir = normalize(bezierPoint - moveData.currentPos);
		float targetRot = atan2(dir.z, dir.x);
		moveData.currentPos = bezierPoint;
		world->MoveSpriteTo(entity.id(), moveData.currentPos.x, moveData.currentPos.y, moveData.currentPos.z);
		world->RotateSprite(entity.id(), 0, 1, 0, targetRot);
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
					movePathFinding->setOldPosUnitCost = true;
					entity.modified<MovePathFinding>();
				}
				return;
			}
			MovePathFinding* movePathFinding = entity.get_mut<MovePathFinding>();
			int3 indexes = make_int3(round(moveLocation->currentPos.x / 16), 0, round(moveLocation->currentPos.z / 16));
			float3 newTarget = make_float3(indexes.x * 16, target.y, indexes.z * 16);
			moveAttack.target = 0;
			movePathFinding->target = newTarget;
			entity.modified<MovePathFinding>();
		}
	}
}

void MoveUnitOverPath(const flecs::entity& entity, MovePathFinding& movePathFinding)
{
	MoveLocation* moveLocation = entity.get_mut<MoveLocation>();
	if (!movePathFinding.reachedTarget && moveLocation->reachedTarget && !entity.has<Dead>())
	{
		if (!movePathFinding.setOldPosUnitCost)
		{
			pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(movePathFinding.oldPos), mapSize.x));
			movePathFinding.setOldPosUnitCost = true;
		}

		float indexCurrentPos = GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x);

		if (make_int3(moveLocation->currentPos) == make_int3(movePathFinding.target))
		{
			pathfinder.SetUnitInUnitField(indexCurrentPos);
			movePathFinding.reachedTarget = true;
			movePathFinding.setOldPosUnitCost = true;
			return;
		}
		pathfinder.RemoveUnitInUnitField(indexCurrentPos);
		vector<int> flowField = pathfinder.GetFlowFlield(GetIndexes(movePathFinding.target));
		int targetIndex = flowField[indexCurrentPos];
		pathfinder.SetUnitInUnitFieldWithAmount(indexCurrentPos, 4);

		if (targetIndex > 0)
		{
			int2 newTargetGridPos = IndexToGridPos(targetIndex, mapSize.x);
			float3 newTarget = make_float3((newTargetGridPos.x + 10) * 16, movePathFinding.target.y, (newTargetGridPos.y + 10) * 16);
			if (pathfinder.unitField[targetIndex] == 0)
			{
				//pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x));
				pathfinder.SetUnitInUnitFieldWithAmount(targetIndex, 4);
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

int GetTarget(float fireRange, int3 currentPos, uint playerID = 0)
{
	for (auto it : ecs.filter(playerID == 1 ? filterPlayer2 : filterPlayer1))
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
	return 0;
}

void AddScore(flecs::entity currentEntity)
{
	if (currentEntity.has<Player2>())
	{
		if (currentEntity.has<Building>())
		{
			score += 100;
		}
		else
		{
			score += 50;
		}
	}
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
	if (weaponData.target == 0)
	{
		weaponData.target = GetTarget(weaponData.fireRange, currentPos, weaponData.playerID);
	}
	else
	{
		flecs::entity targetEntity = ecs.entity(weaponData.target);
		if (targetEntity.has<Dead>())
		{
			weaponData.target = GetTarget(weaponData.fireRange, currentPos, weaponData.playerID);
		}
	}
	

	if (weaponData.target > world->sprite.size() || weaponData.target <= 0)
	{
		return;
	}


	int3 enemyPos = world->sprite[weaponData.target]->currPos;
	float dist = abs(length(make_float3(currentPos - enemyPos)));

	if (dist <= weaponData.fireRange)
	{
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
		ShotObjectData shotObjectData{ moveData, weaponData.target, weaponData.playerID };
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
}

void RemoveUnitFromUnitField(const flecs::entity entity)
{
	if (entity.has<MovePathFinding>() && entity.has<MoveLocation>())
	{
		const MoveLocation* moveLocation = entity.get< MoveLocation>();
		pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->startPos), mapSize.x));
		pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->currentPos), mapSize.x));
		pathfinder.RemoveUnitInUnitField(GridPosToIndex(GetIndexes(moveLocation->targetPos), mapSize.x));
	}
}

void DisableChild(const flecs::entity entity)
{
	if (entity.has<ChildData>())
	{
		const ChildData* childData = entity.get<ChildData>();
		ecs.entity(childData->childID).disable();
	}
}


void MoveTankBullet(const flecs::entity entity, TankBullet& tankBullet, ShotObjectData& shotObjectData)
{
	flecs::entity target = ecs.entity(shotObjectData.targetID);
	if (!target.has<Dead>())
	{
		MoveEntity(entity, shotObjectData.moveData);
		if (shotObjectData.moveData.reachedTarget)
		{
			AddScore(target);
			GetWorld()->DisableSprite(shotObjectData.targetID);		
			RemoveUnitFromUnitField(target);
			DisableChild(target);
			target.add<Dead>();
			target.disable();

			GetWorld()->DisableSprite(entity.id());
			entity.add<Dead>();
			entity.disable();
		}
	}
	else
	{
		GetWorld()->DisableSprite(entity.id());
		entity.add<Dead>();
		entity.disable();
	}
}

void MoveArtilleryBullet(const flecs::entity entity, ArtilleryBullet& artilleryBullet, ShotObjectData& shotObjectData)
{
	MoveCurveEntity(entity, shotObjectData.moveData);
	if (shotObjectData.moveData.reachedTarget)
	{

		World* world = GetWorld();
		for (auto it : ecs.filter(filterPlayersAndBuildings))
		{
			for (auto index : it)
			{
				flecs::entity& currentEntity = it.entity(index);
				float3 entityPos = make_float3(world->sprite[currentEntity.id()]->currPos);
				entityPos.y = 0;
				float3 targetPos = shotObjectData.moveData.targetPos;
				targetPos.y = 0;
				float dist = abs(length(entityPos - targetPos));
				if (dist <= 32)
				{
					AddScore(currentEntity);
					RemoveUnitFromUnitField(currentEntity);
					DisableChild(currentEntity);
					world->DisableSprite(it.entity(index).id());
					currentEntity.add<Dead>();
					currentEntity.disable();
				}
			}
		}
		world->DisableSprite(entity.id());
		entity.add<Dead>();
		entity.disable();
		//Spawn model for blocking terrain
		//Check if there is already a gab there
		//TODO: Change this so it gets the terrain and removes part of it
		pathfinder.SetCostOnCostField(GridPosToIndex(GetIndexes(shotObjectData.moveData.targetPos), mapSize.x));
		uint spriteID = world->LoadSprite("assets/tile00.vox");
		world->DestroyTerrain(spriteID, shotObjectData.moveData.targetPos.x, 8, shotObjectData.moveData.targetPos.z);
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

void PatrollEntity(const flecs::entity entity, MovePathFinding& movePathfinding, PatrollData& patrollData)
{
	if (movePathfinding.reachedTarget && !entity.has<Dead>())
	{
		movePathfinding.target = patrollData.targets[patrollData.index];
		movePathfinding.reachedTarget = false;
		patrollData.index++;
		if (patrollData.index >= patrollData.targets.size())
		{
			patrollData.index = 0;
		}
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
