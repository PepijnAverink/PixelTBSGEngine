#include "precomp.h"
#include "mygame.h"
#include "GameFunctions.h"
#include <iostream>
#include "scripts\Imgui\imgui.h"
#include "particles/ExplotionParticleSystem.h"

Game* game = new MyGame();


void MyGame::Init()
{
	//ShowCursor(false);
	world = GetWorld();
	units.Init(*world);
	tileLoader.Init();
	outlines.Init(*world);

	for (int i = 0; i < 20; ++i)
	{
		outlineSprties[i] = CloneSprite(outlines.selectionOutline);
		world->ScaleSprite(outlineSprties[i], make_uint3(1, 1, 1));
	}


	ifstream myfile("assets/highscore/score.txt");
	if (myfile.is_open())
	{
		string line;
		string result = "";
		while (getline(myfile, line))
		{
			result.append(line);
		}
		highScore = std::stoi(result);
		myfile.close();
	}
	else
	{
		highScore = 0;
		ofstream myfile("assets/highscore/score.txt");
		if (myfile.is_open())
		{
			myfile << to_string(highScore);
			myfile.close();
		}
	}

	AddComponentsToFlecsWorld();
	tileLoader.LoadTile("assets/Maps/Level1.json");
	heightMap = tileLoader.GetHeightMap();
	mapSize = make_int2(tileLoader.grid.width, tileLoader.grid.height);
	pathfinder.arrow = world->LoadSprite("assets/Arrow.vox");
	pathfinder.point = world->LoadSprite("assets/Point.vox");
	pathfinder.moveingPoint = world->LoadSprite("assets/MovingUnit.vox");
	SpawnWorld();
	


	//uint id = world->LoadSprite("assets/transparency/transparency_0x0.vox");
	//world->MoveSpriteTo(id, 200, 32, 190);
	//world->LoadTerainFromSprite(id, 180, 32, 180);
	//world->LoadTerainFromSprite(id, 190, 32, 180);
	//world->LoadTerainFromSprite(id, 200, 32, 180);
	//world->LoadTerainFromSprite(id, 210, 32, 180);

	/*for (uint x = 0; x < 4; x++)
	{
		for (uint y = 0; y < 6; y++)
		{
			string s = "assets/transparency/transparency_" + std::to_string(x) + "x" + std::to_string(y) + ".vox";
			uint id = world->LoadSprite(s.c_str());
			world->MoveSpriteTo(id, 600 + x * 10, 20, 200 + y * 10);
		}
	}*/


	camera = Camera();
	camera.SetPositionAndLookat(make_float3(752, 128, 752), make_float3(600, 1, 600));

	pathfinder.SetCostField(tileLoader.GetCostField());
	pathfinder.SetMapSize(make_int2(tileLoader.grid.width, tileLoader.grid.height));
	//VisualizeFlowField(make_float3(390, 16, 210));

	float3 startPos = make_float3(640, 16, 720);
	for (int i = 0; i < 1; ++i)
	{
		int indexX = i * 16;
		SpawnFastTank(1, startPos + make_float3(indexX, 0, 0));
		SpawnNormalTank(1, startPos + make_float3(indexX, 0, 16));
		SpawnArtilleryTank(1, startPos + make_float3(indexX, 0, 32));
	}

	//SpawnPatrollingTank(2, { make_float3(208,16,352),make_float3(352,16,352) });
	//SpawnPatrollingTank(2, { make_float3(352,16,352), make_float3(208,16,352) });
	//SpawnPatrollingTank(2, { make_float3(352,16,208),make_float3(352,16,352) });
	//SpawnPatrollingTank(2, { make_float3(352,16,352), make_float3(352,16,208) });
	//
	//SpawnPatrollingTank(2, { make_float3(400,16,208), make_float3(640,16,512) });
	//SpawnPatrollingAtrilleryTank(2, { make_float3(416,16,208), make_float3(656,16,512) });
	//
	//SpawnPatrollingAtrilleryTank(2, { make_float3(208,16,416), make_float3(384,16,416) });
	//SpawnPatrollingAtrilleryTank(2, { make_float3(208,16,432), make_float3(384,16,432) });
	//
	//
	//SpawnNormalTank(2, { make_float3(352,16,784) });
	//SpawnNormalTank(2, { make_float3(352,16,544) });
	//SpawnNormalTank(2, { make_float3(208,16,784) });
	//SpawnNormalTank(2, { make_float3(208,16,544) });
	//SpawnNormalTank(2, { make_float3(496,16,784) });
	//SpawnNormalTank(2, { make_float3(496,16,544) });
	//SpawnNormalTank(2, { make_float3(672,16,320) });
	//SpawnNormalTank(2, { make_float3(624,16,336) });
	//SpawnNormalTank(2, { make_float3(576,16,320) });
	//
	//
	//SpawnNormalTank(2, { make_float3(480,16,480) });
	//SpawnNormalTank(2, { make_float3(448,16,480) });
	//SpawnNormalTank(2, { make_float3(480,16,448) });
	//
	//
	//SpawnArtilleryTank(2, { 390,16,226 });
	//SpawnArtilleryTank(2, { 210,16,390 });
	//SpawnArtilleryTank(2, { 210,16,210 });
}

void Tmpl8::MyGame::AddComponentsToFlecsWorld()
{
	ecs.component<MoveLocation>();
	ecs.component<MovePathFinding>();
	ecs.component<PatrollData>();
	ecs.component<MoveAttack>();
	ecs.component<Rotation>();
	ecs.component<Player1>();
	ecs.component<Player2>();
	ecs.component<Dead>();
	ecs.component<ChildData>();
	ecs.component<TankBullet>();
	ecs.component<ArtilleryBullet>();
	ecs.component<Building>();
	ecs.component<Health>();
	ecs.system<Rotation>("RotateEntity").kind(flecs::OnUpdate).each(GameplayFunctions::RotateEntity);
	ecs.system<MoveLocation>("OnAddMoveLocation").kind(flecs::OnAdd).each(GameplayFunctions::OnAddMoveLocation);
	ecs.system<MoveLocation>("MoveEntity").kind(flecs::OnUpdate).each(GameplayFunctions::MoveEntity);
	ecs.system<MovePathFinding>("MoveEntityOverPath").kind(flecs::OnUpdate).each(GameplayFunctions::MoveUnitOverPath);
	ecs.system<MoveAttack>("MoveAttackEntity").kind(flecs::OnUpdate).each(GameplayFunctions::MoveAttackEntity);
	ecs.system<WeaponData>("WeaponUpdate").kind(flecs::OnUpdate).each(GameplayFunctions::WeaponUpdate);
	ecs.system<TankBullet, ShotObjectData>("MoveTankBullet").kind(flecs::OnUpdate).each(GameplayFunctions::MoveTankBullet);
	ecs.system<ArtilleryBullet, ShotObjectData>("MoveArtilleryBullet").kind(flecs::OnUpdate).each(GameplayFunctions::MoveArtilleryBullet);
	ecs.system<ChildData>("MoveChild").kind(flecs::OnUpdate).each(GameplayFunctions::HandleChilds);
	ecs.system<MovePathFinding,PatrollData>("PatrolEntity").kind(flecs::OnUpdate).each(GameplayFunctions::PatrollEntity);
}

void Tmpl8::MyGame::SpawnWorld()
{
	for (int i = 0; i < tileLoader.grid.height; ++i)
	{
		for (int ii = 0; ii < tileLoader.grid.width; ++ii)
		{
			int index = i * tileLoader.grid.width + ii;
			uint3 spriteSpawnPos = make_uint3(i * 16 + (10 * 16), 32, ii * 16 + (10 * 16));
			SpawnTile(tileLoader.grid.tiledatasLayer1[index],make_int3(i,0,ii));
			SpawnTile(tileLoader.grid.tiledatasLayer2[index],make_int3(i,1,ii));
		}
	}
}

void Tmpl8::MyGame::SpawnTile(TileData tileData, int3 indexes)
{
	if (tileData.tile >= 0)
	{
		uint3 spawnPos = make_uint3(indexes.x + 10, indexes.y, indexes.z + 10);
		switch (tileData.tileType)
		{
		case TileType::Setdress:
		case TileType::Building:
		{
			int height = heightMap[GridPosToIndex(make_int2(indexes.x,indexes.z), mapSize.x)];
			uint3 spriteSpawnPos = make_uint3(indexes.x * 16 + (10 * 16), ((indexes.y + 1) *16) + height, indexes.z * 16 + (10 * 16));		
			world->LoadTerainFromBigTile(tileLoader.GetID("Grass"), spawnPos.x, spawnPos.y, spawnPos.z);

			world->LoadTerainFromSprite(tileData.tile, spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
			flecs::entity building =  ecs.entity();
			building.add<Building>()
			.add<Health>()
			.set<Health>({4,4})
			.add<Player2>();

			BuildingData buildingData;
			buildingData.entity = building;
			buildingData.pos = make_float3(spriteSpawnPos);
			buildings.push_back(buildingData);
			if (tileData.tile == 20)
			{
				headquarters = building;
			}
			break;
		}
		case TileType::Terrain:
		{
			world->LoadTerainFromBigTile(tileData.tile, spawnPos.x, spawnPos.y, spawnPos.z);
			break;
		}
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

	world->UpdateParticleSystems(deltaTime / 1000.0f);

	ImGui::Text(("Score: " + to_string(score)).c_str());
	if (!gameLose)
	{
		if (headquarters.has<Dead>() || gameWon)
		{
			if (gameWon == false)
			{
				score += 1000;
			}

			ImGui::Text(("You win the game you Score: " + to_string(score)).c_str());
			ImGui::Text(("Your highScore: " + to_string(highScore)).c_str());
			gameWon = true;
		}
	}
	
	if (!gameWon)
	{
		if (hasLost() || gameLose)
		{
			ImGui::Text(("You lose the game you Score: " + to_string(score)).c_str());
			ImGui::Text(("Your highScore: " + to_string(highScore)).c_str());
			gameLose = true;
		}
	}	

	if (score > highScore)
	{
		highScore = score;
		ofstream myfile("assets/highscore/score.txt");
		if (myfile.is_open())
		{
			myfile << to_string(highScore);
			myfile.close();
		}
	}


	//Outline for selection
	SetOutlineSelectedUnits();
	//pathfinder.VisualizeUnitField();
	//pathfinder.VisualizeCostField();

	float3 direction = make_float3((keys[GLFW_KEY_UP] || keys[GLFW_KEY_W]) - (keys[GLFW_KEY_DOWN] || keys[GLFW_KEY_S]), 0, (keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_A]) - (keys[GLFW_KEY_RIGHT] || keys[GLFW_KEY_D]));
	//float3 direction = make_float3((mousePos.y < 20 ? 1 : 0) - (mousePos.y > 700 ? 1 : 0), 0, (mousePos.x < 20 ? 1 : 0) - (mousePos.x > 1260 ? 1 : 0));
	camera.Movement(direction, deltaTime);
	ecs.progress();
}

void Tmpl8::MyGame::MouseUp(int button)
{
	keys[button] = false;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		float3 endPos = GetMousePosInWorld();
		endPos.y = round(endPos.y);
		
		float dist = abs(length(startPos - endPos));
		std::vector<uint> unitsInArea = GetFriendlyUnitInArea(startPos, endPos);
		if (dist > 5 && unitsInArea.size() > 0)
		{
			selectedUnits = unitsInArea;
			return;
		}
		
		
		int buildingID = IsEnemyBuilding(endPos);

		if (buildingID >= 0)
		{
			SetSelectedTanksAttackTarget(buildings[buildingID].entity.id());
			std::cout << "Works Building" << std::endl;
			return;
		}
		else
		{
			uint objectHit = HandleMouseRaycastingTopDown(endPos);
			if (objectHit > 0 && objectHit < world->sprite.size())
			{
				if (IsFriendlyUnit(objectHit))
				{
					selectedUnits.clear();
					selectedUnits.push_back(objectHit);
					return;
				}
				else if (IsEnemyUnit(objectHit))
				{
					SetSelectedTanksAttackTarget(objectHit);
					std::cout << "Works" << std::endl;
					return;
				}
			}

		}
		SetSelectedTanksMoveLocation(endPos);
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
		//uint objectHit = HandleMouseRaycasting();
		//if (objectHit > 0 && objectHit < world->sprite.size())
		//{
		//	//if (IsEnemyUnit(objectHit))
		//	//{
		//		//Can Add cursor indicator of enemy unit
		//	//}
		//	
		//	startPos = make_float3(world->sprite[objectHit]->currPos);
		//}

		startPos = GetMousePosInWorld();
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

	float mx = (float)(((screenResolution.x - mousePos.x) - screenResolution.x * 0.5f) * (1.0f / screenResolution.x) * fovX * 0.5f);
	float my = (float)(((screenResolution.y - mousePos.y) - screenResolution.y * 0.5f) * (1.0f / screenResolution.x) * fovX * 0.5f);
	float3 cameraForward = world->GetCameraViewDir();
	float3 cameraRight = cross(cameraForward, camera.GetCameraUp());
	float3 dx = cameraRight * mx;
	float3 dy = camera.GetCameraUp() * my;

	float3 dir = normalize(cameraForward + (dx + dy) * 2.0f);

	uint raycastHit = world->RayCast(camera.GetPos(), dir);

	return raycastHit;
}

uint Tmpl8::MyGame::HandleMouseRaycastingTopDown(float3 pos)
{
	float3 rayCastPos = pos;
	rayCastPos.y += 64;
	return world->RayCast(rayCastPos, make_float3(0,-1,0));
}

float3 Tmpl8::MyGame::GetMousePosInWorld()
{
	float2 screenResolution = make_float2(1280, 720);
	float2 newMousePos = (mousePos / screenResolution) * 2 - 1;
	newMousePos.x *= -1;
	float3 newDir = PaniniProjection(newMousePos, PI / 5.0f, 0.15f);

	float3 camPos = camera.GetPos();
	float aspectRatio = 1280 / 720;
	float3 p0 = TransformPosition(make_float3(-aspectRatio, 1, 3), world->GetCameraMatrix());
	float3 p1 = TransformPosition(make_float3(aspectRatio, 1, 3), world->GetCameraMatrix());
	float3 p2 = TransformPosition(make_float3(-aspectRatio, -1, 3), world->GetCameraMatrix());

	const float3 D = newDir.z * normalize((p1 + p2) * 0.5f - camPos) +
		newDir.x * normalize(p1 - p0) +
		newDir.y * normalize(p2 - p0);


	float numberOfSteps = (camPos.y - 16) / abs(D.y);

	float3 locationInWorld = camPos + (D * numberOfSteps);

	return GetEntityPos(GetIndexes(locationInWorld),16);
}

void Tmpl8::MyGame::SetUnitMoveLocation(float3 target, flecs::entity& unit)
{
	float3 tankPos = make_float3(world->sprite[unit.id()]->currPos);
	target.y = tankPos.y;
	MoveLocation* moveData = unit.get_mut<MoveLocation>();
	moveData->startPos = tankPos;
	moveData->targetPos = target;
	moveData->reachedTarget = false;
	moveData->progress = 0;
	unit.modified<MoveLocation>();
}

void Tmpl8::MyGame::SetUnitMoveLocationAndRotation(float3 target, flecs::entity& unit)
{
	float3 tankPos = make_float3(world->sprite[unit.id()]->currPos);
	target.y = tankPos.y;
	MoveLocation* moveData = unit.get_mut<MoveLocation>();
	moveData->startPos = tankPos;
	moveData->targetPos = target;
	moveData->reachedTarget = false;
	moveData->progress = 0;
	unit.modified<MoveLocation>();

	float3 dir = normalize(moveData->targetPos - moveData->startPos);
	float targetRot = atan2(dir.z, dir.x);
	Rotation* rotation = unit.get_mut<Rotation>();
	rotation->target = RadiansToDegrees(targetRot);
	rotation->reachedTarget = false;
	unit.modified<Rotation>();
}

void Tmpl8::MyGame::SetUnitMovePath(float3 target, uint unitID)
{
	flecs::entity& unit = ecs.entity(unitID);
	MovePathFinding* movePathfFinding = unit.get_mut<MovePathFinding>();
	movePathfFinding->target = target;
	movePathfFinding->reachedTarget = false;
}

void Tmpl8::MyGame::SetUnitAttackTarget(uint target, uint unitID)
{
	flecs::entity& unit = ecs.entity(unitID);
	if (unit.has<MoveAttack>())
	{
		//int3 targetPos = world->sprite[target]->currPos;
		//int2 indexes = GetIndexes(make_float3(targetPos));
		//recalculate point to go
		//float2 newTarget = GetEntityPos(indexes);
		//SetUnitMovePath(make_float3(newTarget.x, targetPos.y, newTarget.y), unitID);
		MoveAttack* moveAttack = unit.get_mut<MoveAttack>();
		moveAttack->target = target;
		unit.modified<MoveAttack>();

		WeaponData* weaponData = ecs.entity(unit.get<ChildData>()->childID).get_mut<WeaponData>();
		weaponData->target = target;
	}
}


void Tmpl8::MyGame::SetSelectedTanksMoveLocation(float3 target)
{
	const vector<float3> newTarget = pathfinder.GetTargetsForUnit(target, selectedUnits.size());
	for (int i = 0; i < selectedUnits.size();++i)
	{	
		if (ecs.entity(selectedUnits[i]).has<Dead>())
		{
			selectedUnits.erase(selectedUnits.begin() + i);
			--i;
		}
		else
		{
			//ecs.entity(selectedUnits[i]).disable<MoveAttack>();
			SetUnitMovePath(newTarget[i], selectedUnits[i]);
			//pathfinder.VisualizeFlowField(newTarget[i]);
		}
	}
}

void Tmpl8::MyGame::SetSelectedTanksAttackTarget(uint target)
{
	for (uint unit : selectedUnits)
	{
		SetUnitAttackTarget(target, unit);
	}
}

void Tmpl8::MyGame::SetOutlineSelectedUnits()
{
	for (int i = 0; i < 20; ++i)
	{
		if (selectedUnits.size() > i && !ecs.entity(selectedUnits[i]).has<Dead>())
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

bool Tmpl8::MyGame::IsEnemyUnit(uint enemyID)
{
	for (auto& it : ecs.filter(filterPlayer2))
	{
		for (auto& index : it)
		{
			if (it.entity(index).id() == enemyID)
			{
				return true;
			}
		}
	}

	return false;
}

int Tmpl8::MyGame::IsEnemyBuilding(float3 raycastPos)
{
	for (int i = 0; i < buildings.size(); ++i)
	{
		float dist = abs(length(buildings[i].pos - raycastPos));
		if (dist <= 16)
		{
			return i;
		}
	}

	return -1;
}

bool Tmpl8::MyGame::hasLost()
{

	for (auto it : ecs.filter(filterPlayer1))
	{
		for (auto index : it)
		{
			if (!it.entity(index).has<Dead>())
			{
				return false;
			}
		}
	}
	return true;
}

float3 Tmpl8::MyGame::PaniniProjection(float2 tc, const float fov, const float d)
{
	const float d2 = d * d;
	{
		const float fo = PI * 0.5f - fov * 0.5f;
		const float f = cos(fo) / sin(fo) * 2.0f;
		const float f2 = f * f;
		const float b = (sqrt(max(0.f, (d + d2) * (d + d2) * (f2 + f2 * f2))) - (d * f + f)) / (d2 + d2 * f2 - 1);
		tc *= b;
	}
	const float h = tc.x, v = tc.y, h2 = h * h;
	const float k = h2 / ((d + 1) * (d + 1)), k2 = k * k;
	const float discr = max(0.f, k2 * d2 - (k + 1) * (k * d2 - 1));
	const float cosPhi = (-k * d + sqrt(discr)) / (k + 1.f);
	const float S = (d + 1) / (d + cosPhi), tanTheta = v / S;
	float sinPhi = sqrt(max(0.f, 1 - cosPhi * cosPhi));
	if (tc.x < 0.0) sinPhi *= -1;
	const float s = rsqrtf(1 + tanTheta * tanTheta);
	return make_float3(sinPhi, tanTheta, cosPhi) * s;
}

flecs::entity Tmpl8::MyGame::SpawnEntity(uint unit, uint playerID /* = 0*/, float3 location /* = float3 (0,0,0) */)
{
	uint spriteID = world->CloneSprite(unit);
	world->MoveSpriteTo(spriteID, location.x, location.y, location.z);
	flecs::entity entity = ecs.entity(spriteID);
	if (playerID == 1) { entity.add<Player1>(); }
	else if (playerID == 2) { entity.add<Player2>(); }

	return entity;
}

flecs::entity Tmpl8::MyGame::SpawnUnitWithChild(uint top, uint bottom, float speed, uint playerID, float3 location, float3 offset)
{
	flecs::entity currentUnit = SpawnEntity(bottom, playerID, location);
	flecs::entity childUnit = SpawnEntity(top, 0, location + offset);
	//top.add_childof(currentUnit);

	world->SetSpritePivot(childUnit.id(), 8, 0, 6);
	childUnit.add<Rotation>()
		.set<Rotation>({ 0,300,0,true });

	//flecs::entity currentUnit = SpawnEntity(units.recon, playerID, location);
	world->SetSpritePivot(currentUnit.id(), 8, 0, 6);
	currentUnit.add<MoveLocation>()
		.set<MoveLocation>({ speed, location, location,location, true, 0 })
		.add< Rotation>()
		.set<Rotation>({ 0,300,0,true })
		.add<MovePathFinding>()
		.set<MovePathFinding>({ location ,location ,true,true })
		.add<ChildData>()
		.set<ChildData>({ (uint)childUnit.id(), offset });

	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnFastTank(uint playerID, float3 location)
{
	flecs::entity currentUnit = SpawnUnitWithChild(units.reconTop, units.reconBottom, 0.8f, playerID, location, make_float3(0, 9, 0));
	currentUnit.add<MoveAttack>()
		.set<MoveAttack>({ 0,48 })
		.add<Health>()
		.set<Health>({ 4, 4 });

	ecs.entity(currentUnit.get<ChildData>()->childID)
		.add<WeaponData>()
		.set<WeaponData>({ 0, 48,units.bullet,0,1.5f,0, 1,BulletType::Bullet_Tank,playerID });
	pathfinder.SetUnitInUnitField(GridPosToIndex(GetIndexes(location), mapSize.x));
	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnUnit(uint unit, uint playerID, float3 location)
{
	flecs::entity currentUnit = SpawnEntity(unit, playerID, location);
	world->SetSpritePivot(currentUnit.id(), 8, 0, 6);
	currentUnit.add<MoveLocation>()
		.set<MoveLocation>({ 0.5f, location, location,location, true, 0 })
		.add< Rotation>()
		.set<Rotation>({ 0,600,0,true })
		.add<MovePathFinding>()
		.set<MovePathFinding>({ location ,location ,true,true });

	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnNormalTank(uint playerID, float3 location)
{
	flecs::entity currentUnit = SpawnUnitWithChild(units.tank, units.tankBottom2,0.6f, playerID, location, make_float3(0, 6, 0));
	currentUnit.add<MoveAttack>()
		.set<MoveAttack>({ 0,48 })
		.add<Health>()
		.set<Health>({ 6, 6 });

	ecs.entity(currentUnit.get<ChildData>()->childID)
	.add<WeaponData>()
	.set<WeaponData>({ 0, 48,units.bullet,0,1.5f,0, 1,BulletType::Bullet_Tank,playerID });
	pathfinder.SetUnitInUnitField(GridPosToIndex(GetIndexes(location), mapSize.x));
	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnArtilleryTank(uint playerID, float3 location)
{
	flecs::entity currentUnit = SpawnUnitWithChild(units.artilleryTop, units.artilleryBottom, 0.4f, playerID, location,make_float3(0, 6, 0));
	currentUnit.add<MoveAttack>()
		.set<MoveAttack>({ 0,80 })
		.add<Health>()
		.set<Health>({2, 2});
	ecs.entity(currentUnit.get<ChildData>()->childID)
		.add<WeaponData>()
		.set<WeaponData>({ 0, 80,units.bullet,0,5,0, 1,BulletType::Bullet_Artillery,playerID });
	pathfinder.SetUnitInUnitField(GridPosToIndex(GetIndexes(location), mapSize.x));
	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnPatrollingTank(uint playerID, float3 location, vector<float3> patrolPoints)
{
	flecs::entity currentUnit = SpawnNormalTank(2, location);
	currentUnit.add<PatrollData>()
		.set<PatrollData>({ patrolPoints , 0 });
	currentUnit.get_mut<MovePathFinding>()->reachedTarget = true;
	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnPatrollingTank(uint playerID, vector<float3> patrolPoints)
{
	flecs::entity currentUnit = SpawnNormalTank(2, patrolPoints[0]);
	currentUnit.add<PatrollData>()
		.set<PatrollData>({ patrolPoints , 0 });
	currentUnit.get_mut<MovePathFinding>()->reachedTarget = true;
	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnPatrollingAtrilleryTank(uint playerID, float3 location, vector<float3> patrolPoints)
{
	flecs::entity currentUnit = SpawnArtilleryTank(2, location);
	currentUnit.add<PatrollData>()
		.set<PatrollData>({ patrolPoints , 0 });
	currentUnit.get_mut<MovePathFinding>()->reachedTarget = true;
	return currentUnit;
}

flecs::entity Tmpl8::MyGame::SpawnPatrollingAtrilleryTank(uint playerID, vector<float3> patrolPoints)
{
	flecs::entity currentUnit = SpawnArtilleryTank(2, patrolPoints[0]);
	currentUnit.add<PatrollData>()
		.set<PatrollData>({ patrolPoints , 0 });
	currentUnit.get_mut<MovePathFinding>()->reachedTarget = true;
	return currentUnit;
}
