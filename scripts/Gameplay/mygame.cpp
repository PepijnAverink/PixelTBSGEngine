#include "precomp.h"
#include "mygame.h"
#include "GameFunctions.h"
#include <iostream>
#include "scripts\Imgui\imgui.h"

Game* game = new MyGame();


void MyGame::Init()
{
	//ShowCursor(false);
	world = GetWorld();
	units.Init(*world);
	terrain.Init(*world);
	outlines.Init(*world);

	for (int i = 0; i < 20; ++i)
	{
		outlineSprties[i] = CloneSprite(outlines.selectionOutline);
		world->ScaleSprite(outlineSprties[i], make_uint3(1, 1, 1));
	}

	AddComponentsToFlecsWorld();
	SpawnWorld();

	SpawnTank(units.recon, 1, { 370,3,370 });

	SpawnEntity(units.aAirMissile, 2, { 390,3,210 });
	SpawnEntity(units.aAirMissile, 2, { 210,3,390 });
	SpawnEntity(units.aAirMissile, 2, { 210,3,210 });

	camera = Camera();
	camera.SetPositionAndLookat(make_float3(500, 128, 500), make_float3(300, 1, 300));
}

void Tmpl8::MyGame::AddComponentsToFlecsWorld()
{
	ecs.component<MoveLocation>();
	ecs.component<MoveAttack>();
	ecs.component<Rotation>();
	ecs.component<Player1>();
	ecs.component<Player2>();
	ecs.component<Dead>();
	ecs.system<MoveLocation>("MoveEntity").each(GameplayFunctions::MoveEntity);
	ecs.system<MoveLocation>("OnMoveStart").kind(flecs::Disabled).each(GameplayFunctions::OnMoveStart);
	ecs.system<MoveAttack>("MoveAttackEntity").each(GameplayFunctions::MoveAttackEntity);
	ecs.system<Rotation>("OnRotationStart").kind(flecs::Disabled).each(GameplayFunctions::OnRotationStart);
	ecs.system<Rotation>("RotateEntity").each(GameplayFunctions::RotateEntity);
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
	for (int i =0; i < 350; ++i)
	{
		if (keys[i])
		{
			KeyPressed(i);
		}
	}

	//Outline for selection
	SetOutlineSelectedUnits();

	//float3 direction = make_float3(keys[GLFW_KEY_UP] - keys[GLFW_KEY_DOWN], 0, keys[GLFW_KEY_LEFT] - keys[GLFW_KEY_RIGHT]);
	float3 direction = make_float3((mousePos.y < 20 ? 1 : 0) - (mousePos.y > 700 ? 1 : 0), 0, (mousePos.x < 20 ? 1 : 0) - (mousePos.x > 1260 ? 1 : 0));
	camera.Movement(direction, deltaTime);
	ecs.progress();
}

void Tmpl8::MyGame::MouseUp(int button)
{
	keys[button] = false;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		uint objectHit = HandleMouseRaycasting();
		if (objectHit > 0 && objectHit < world->sprite.size())
		{
			float3 endPos = make_float3(world->sprite[objectHit]->currPos);
			float dist = abs(length(startPos - endPos));

			std::vector<uint> unitsInArea = GetFriendlyUnitInArea(startPos, endPos);
			if (dist > 5 && unitsInArea.size() > 0)
			{
				selectedUnits = unitsInArea;
			}
			else if (IsFriendlyUnit(objectHit))
			{
				selectedUnits.clear();
				selectedUnits.push_back(objectHit);
			}
			else if (IsEnemyUnit(objectHit))
			{
				SetSelectedTanksAttackTarget(objectHit);
				std::cout << "Works" << std::endl;
			}
			else if (IsMoveableTerrain(objectHit))
			{
				SetSelectedTanksMoveLocation(make_float3(world->sprite[objectHit]->currPos));
			}
		}
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		selectedUnits.clear();
	}
}

void Tmpl8::MyGame::MouseDown(int button)
{
	keys[button] = true;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		uint objectHit = HandleMouseRaycasting();
		if (objectHit > 0 && objectHit < world->sprite.size())
		{
			//if (IsEnemyUnit(objectHit))
			//{
				//Can Add cursor indicator of enemy unit
			//}
			
			startPos = make_float3(world->sprite[objectHit]->currPos);
		}
	}
}

void Tmpl8::MyGame::MouseScrolling(float xAxis, float yAxis)
{
	camera.Zooming(-yAxis);
}

void Tmpl8::MyGame::KeyUp(int key)
{
	keys[key] = false;
}

void Tmpl8::MyGame::KeyDown(int key)
{
	keys[key] = true;
	
	if (keys[GLFW_KEY_LEFT_CONTROL])
	{
		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
		{
			unitBindings[key - GLFW_KEY_0].bindedUnits = selectedUnits;
		}
	}
	else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
	{
		selectedUnits = unitBindings[key - GLFW_KEY_0].bindedUnits;
	}
}

void Tmpl8::MyGame::KeyPressed(int key)
{

}

uint Tmpl8::MyGame::HandleMouseRaycasting()
{
	float2 screenResolution = make_float2(1280,720);
	float fovX = PI / 5.0f;

	float mx = (float)(((screenResolution.x - mousePos.x) - screenResolution.x * 0.5) * (1.0 / screenResolution.x) * fovX);
	float my = (float)(((screenResolution.y - mousePos.y) - screenResolution.y * 0.5) * (1.0 / screenResolution.y) * fovX);
	float3 cameraForward = world->GetCameraViewDir();
	float3 cameraRight = cross(cameraForward, camera.GetCameraUp());
	float3 dx = cameraRight * mx;
	float3 dy = camera.GetCameraUp() * my;

	float3 dir = normalize(cameraForward + (dx + dy) * 2.0);

	uint raycastHit = world->RayCast(camera.GetPos(), dir);

	return raycastHit;
}

void Tmpl8::MyGame::SetUnitMoveLocation(float3 target, flecs::entity& unit)
{
	unit.enable<MoveLocation>();
	float3 tankPos = make_float3(world->sprite[unit.id()]->currPos);
	target.y = tankPos.y;
	MoveLocation* moveData = unit.get_mut<MoveLocation>();
	moveData->target = target;
	moveData->reachedTarget = false;
	unit.modified<MoveLocation>();
}

void Tmpl8::MyGame::SetUnitMoveLocationAndRotation(float3 target, flecs::entity& unit)
{
	unit.enable<MoveLocation>();
	float3 tankPos = make_float3(world->sprite[unit.id()]->currPos);
	target.y = tankPos.y;
	MoveLocation* moveData = unit.get_mut<MoveLocation>();
	moveData->target = target;
	moveData->reachedTarget = false;
	unit.modified<MoveLocation>();

	unit.enable<Rotation>();
	float3 dir = normalize(moveData->target - moveData->currentPos);
	float targetRot = atan2(dir.z, dir.x);
	Rotation* rotation = unit.get_mut<Rotation>();
	rotation->target = RadiansToDegrees(targetRot);
	rotation->reachedTarget = false;
	unit.modified<Rotation>();
}

void Tmpl8::MyGame::SetUnitAttackTarget(uint target, flecs::entity& unit)
{
	if (unit.has<MoveAttack>())
	{
		unit.enable<MoveAttack>();
		MoveAttack* moveData = unit.get_mut<MoveAttack>();
		moveData->target = target;
		unit.modified<MoveAttack>();
	}
}


void Tmpl8::MyGame::SetSelectedTanksMoveLocation(float3 target)
{
	for (uint unit : selectedUnits)
	{	
		ecs.entity(unit).disable<MoveAttack>();
		SetUnitMoveLocationAndRotation(target, ecs.entity(unit));
	}
}

void Tmpl8::MyGame::SetSelectedTanksAttackTarget(uint target)
{
	for (uint unit : selectedUnits)
	{
		SetUnitAttackTarget(target, ecs.entity(unit));
	}
}

void Tmpl8::MyGame::SetOutlineSelectedUnits()
{
	for (int i = 0; i < 20; ++i)
	{
		if (selectedUnits.size() > i)
		{
			MoveSpriteTo(outlineSprties[i], world->sprite[selectedUnits[i]]->currPos + make_int3(-15, 3, -15));
		}
		else
		{
			world->DisableSprite(outlineSprties[i]);
		}
	}
}

bool Tmpl8::MyGame::IsFriendlyUnit(uint unitID)
{
	for (auto it : ecs.filter(filterPlayer1))
	{
		for (auto index : it)
		{
			if (it.entity(index).id() == unitID)
			{
				return true;
			}
		}
	}

	return false;
}

vector<uint> Tmpl8::MyGame::GetFriendlyUnitInArea(float3 start, float3 end)
{
	vector<uint> friendlyUnitsInArea = vector<uint>();
	for (auto it : ecs.filter(filterPlayer1))
	{
		for (auto index : it)
		{
			float2 diff = make_float2(start.x - end.x, start.z - end.z);
			float2 size = make_float2(abs(diff.x), abs(diff.y));
			float2 middle = make_float2(start.x - (diff.x * 0.5f), start.z - (diff.y * 0.5f));

			int3 spritePos = world->sprite[it.entity(index).id()]->currPos;
			if (GameplayFunctions::AABB(make_float2(spritePos.x, spritePos.z), make_float2(22, 22), middle, size))
			{
				friendlyUnitsInArea.push_back(it.entity(index).id());
			}
		}
	}

	return friendlyUnitsInArea;
}


bool Tmpl8::MyGame::IsMoveableTerrain(uint terrainID)
{
	for (auto terrain : grid)
	{
		if (terrain == terrainID)
		{
			return true;
		}
	}

	return false;
}

bool Tmpl8::MyGame::IsEnemyUnit(uint enemyID)
{
	for (auto it : ecs.filter(filterPlayer2))
	{
		for (auto index : it)
		{
			if (it.entity(index).id() == enemyID)
			{
				return true;
			}
		}
	}

	return false;
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

flecs::entity Tmpl8::MyGame::SpawnTank(uint unit, int playerID, float3 location)
{
	flecs::entity currentUnit = SpawnEntity(units.recon, playerID, location);
	world->SetSpritePivot(currentUnit.id(), 8, 0, 6);
	currentUnit.add<MoveLocation>()
		.set<MoveLocation>({ location, 37, location })
		.disable<MoveLocation>()
		.add<MoveAttack>()
		.set<MoveAttack>({ 0,100 })
		.disable<MoveAttack>()
		.add< Rotation>()
		.set<Rotation>({0,300,0})
		.disable< Rotation>()
		.add<WeaponData>()
		.set<WeaponData>({ 0, 100,120,100,0,units.bazooka,0,5,0,100 });

	return currentUnit;
}
