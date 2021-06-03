#pragma once
#include "flecs.h"
namespace Tmpl8
{

	flecs::world ecs;
	vector<int> heightMap;
	int2 mapSize;
	int score;

	//Enums

	enum BulletType { Bullet_Tank = 0, Bullet_Artillery };

	struct Units
	{
		uint recon;
		uint vTB;
		uint rocket;
		uint artillery;
		uint artilleryTop;
		uint artilleryBottom;
		uint bazooka;
		uint infantery;
		uint aAirMissile;
		uint dCA;
		uint tank;
		uint megaTank;
		uint head;
		uint tankBottom1;
		uint tankBottom2;
		uint reconBottom;
		uint reconTop;
		uint bullet;


		void Init(World& world)
		{
			recon = world.LoadSprite("assets/Units/TanksAndWar_Source-21-Recon.vox");
			vTB = world.LoadSprite("assets/Units/TanksAndWar_Source-22-VTB.vox");
			rocket = world.LoadSprite("assets/Units/TanksAndWar_Source-23-Rocket.vox");
			artillery = world.LoadSprite("assets/Units/TanksAndWar_Source-25-Artillery.vox");
			artilleryTop = world.LoadSprite("assets/Units/TanksAndWar_Source-Artillery-Top.vox");
			artilleryBottom = world.LoadSprite("assets/Units/TanksAndWar_Source-Artillery-Bottom.vox");
			bazooka = world.LoadSprite("assets/Units/TanksAndWar_Source-26-Bazooka.vox");
			infantery = world.LoadSprite("assets/Units/TanksAndWar_Source-27-Infantery.vox");
			aAirMissile = world.LoadSprite("assets/Units/TanksAndWar_Source-29-A-AirMissile.vox");
			dCA = world.LoadSprite("assets/Units/TanksAndWar_Source-30-D.C.A..vox");
			tank = world.LoadSprite("assets/Units/TanksAndWar_Source-33-Tank.vox");
			megaTank = world.LoadSprite("assets/Units/TanksAndWar_Source-35-MegaTank.vox");
			head = world.LoadSprite("assets/Units/TanksAndWar_Source-16-Head.vox");
			tankBottom1 = world.LoadSprite("assets/Units/TanksAndWar_Source-24-TankBottom1.vox");
			tankBottom2 = world.LoadSprite("assets/Units/TanksAndWar_Source-36-TankBottom2.vox");
			reconBottom = world.LoadSprite("assets/Units/TanksAndWar_Source-Recon-Bottom.vox");
			reconTop = world.LoadSprite("assets/Units/TanksAndWar_Source-Recon-Top.vox");
			bullet = world.LoadSprite("assets/Bullet.vox");
		}

	};

	struct Outlines
	{
		uint selectionOutline;
		uint unitOutline;

		void Init(World& world)
		{
			selectionOutline = world.LoadSprite("assets/Outline/SelectionOutline.vox");
			unitOutline = world.LoadSprite("assets/Outline/UnitOutline.vox");
		}
	};

	struct BindingUnits
	{
		vector<uint> bindedUnits;
	};

	struct Health
	{
		int health;
		int maxHealth;
	};

	struct Rotation
	{
		float currentRotation;
		float speed;
		float target;
		bool reachedTarget;
	};

	struct MovePathFinding
	{
		float3 target;
		float3 oldPos;
		bool reachedTarget;
		bool setOldPosUnitCost;
	};

	struct MoveLocation
	{
		float speed;
		float3 startPos;
		float3 currentPos;
		float3 targetPos;
		bool reachedTarget;
		float progress;
	};

	struct PatrollData
	{
		vector<float3> targets;
		int index;
	};

	struct MoveAttack
	{
		uint target;
		float shootingRange;
	};

	struct ShotObjectData
	{
		MoveLocation moveData;
		uint targetID;
		uint playerID;
	};

	struct WeaponData
	{
		float currentRot;
		float fireRange;
		uint shotObjectID;
		float targetRot;
		float reloadTime;
		float currentReloadTime;
		float shotObjectSpeed;
		BulletType bulletType;
		uint playerID;
		uint target;
	};

	struct ChildData
	{
		uint childID;
		float3 offset;
	};

	//Tags
	struct Player1 {};

	struct Player2 {};

	struct Dead {};

	struct Building {};

	struct TankBullet {};

	struct ArtilleryBullet {};

	//Filters

	auto filterPlayer1 = flecs::filter(ecs)
		.exclude<Building>()
		.include<Player1>()
		.include_kind(flecs::MatchAll);

	auto filterPlayer2 = flecs::filter(ecs)
		.exclude<Building>()
		.include<Player2>()
		.include_kind(flecs::MatchAll);

	auto filterPlayers = flecs::filter(ecs)
		.exclude<Building>()
		.include<Player1>()
		.include<Player2>()
		.include_kind(flecs::MatchAny);

	auto filterPlayersAndBuildings = flecs::filter(ecs)
		.include<Building>()
		.include<Player1>()
		.include<Player2>()
		.include_kind(flecs::MatchAny);

	auto filterBuilding = flecs::filter(ecs)
		.include<Building>()
		.include_kind(flecs::MatchAll);

	auto filterPlayer1Building = flecs::filter(ecs)
		.include<Player1>()
		.include<Building>()
		.include_kind(flecs::MatchAll);

	auto filterPlayer2Building = flecs::filter(ecs)
		.include<Player2>()
		.include<Building>()
		.include_kind(flecs::MatchAll);


}