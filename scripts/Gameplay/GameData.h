#pragma once
#include "flecs.h"
namespace Tmpl8
{

	flecs::world ecs;
	vector<int> heightMap;
	vector<int> costField;
	int2 mapSize;

	struct Units
	{
		uint recon;
		uint vTB;
		uint rocket;
		uint artillery;
		uint bazooka;
		uint infantery;
		uint aAirMissile;
		uint dCA;
		uint tank;
		uint megaTank;
		uint head;
		uint tankBottom1;
		uint tankBottom2;


		void Init(World& world)
		{
			recon = world.LoadSprite("assets/Units/TanksAndWar_Source-21-Recon.vox");
			vTB = world.LoadSprite("assets/Units/TanksAndWar_Source-22-VTB.vox");
			rocket = world.LoadSprite("assets/Units/TanksAndWar_Source-23-Rocket.vox");
			artillery = world.LoadSprite("assets/Units/TanksAndWar_Source-25-Artillery.vox");
			bazooka = world.LoadSprite("assets/Units/TanksAndWar_Source-26-Bazooka.vox");
			infantery = world.LoadSprite("assets/Units/TanksAndWar_Source-27-Infantery.vox");
			aAirMissile = world.LoadSprite("assets/Units/TanksAndWar_Source-29-A-AirMissile.vox");
			dCA = world.LoadSprite("assets/Units/TanksAndWar_Source-30-D.C.A..vox");
			tank = world.LoadSprite("assets/Units/TanksAndWar_Source-33-Tank.vox");
			megaTank = world.LoadSprite("assets/Units/TanksAndWar_Source-35-MegaTank.vox");
			head = world.LoadSprite("assets/Units/TanksAndWar_Source-16-Head.vox");
			tankBottom1 = world.LoadSprite("assets/Units/TanksAndWar_Source-24-TankBottom1.vox");
			tankBottom2 = world.LoadSprite("assets/Units/TanksAndWar_Source-36-TankBottom2.vox");
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
		const vector<int>* flowField;
		bool reachedTarget;
	};

	struct MoveLocation
	{
		float3 target;
		float speed;
		float3 currentPos;
		bool reachedTarget;
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
		float dmg; //not in use now
	};

	struct WeaponData
	{
		float currentRot;
		float speed;
		float fireAngle;
		float fireRange;
		float dmg; //not in use now
		uint shotObjectID;
		float targetRot;
		float reloadTime;
		float currentReloadTime;
		float shotObjectSpeed;
	};

	//Tags
	struct Player1 {};

	struct Player2 {};

	struct Dead {};

	//Filters

	auto filterPlayer1 = flecs::filter(ecs)
		.include<Player1>()
		.include_kind(flecs::MatchAll);

	auto filterPlayer2 = flecs::filter(ecs)
		.include<Player2>()
		.include_kind(flecs::MatchAll);

}