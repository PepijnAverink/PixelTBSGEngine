#pragma once
#include "flecs.h"
#include "GameData.h"
#include <vector>
#include <queue>
#include "Camera.h"
#include "scripts\Tiled\TileLoader.h"
#include "Pathfinder.h"

namespace Tmpl8
{
	static Pathfinder pathfinder;

class MyGame : public Game
{
public:
	// game flow methods
	void Init();
	void AddComponentsToFlecsWorld();
	void SpawnWorld();
	void SpawnTile(TileData tileData, int3 indexes);
	void Tick( float deltaTime );
	void Shutdown() { /* implement if you want to do something on exit */ }
	// input handling
	void MouseUp(int button);
	void MouseDown(int button);
	void MouseMove(int x, int y) { mousePos = make_float2(x, y); };
	void MouseScrolling(float xAxis, float yAxis);
	void KeyUp(int key);
	void KeyDown(int key);
	void KeyPressed(int key);

	//Raycasting
	uint HandleMouseRaycasting();
	uint HandleMouseRaycastingTopDown(float3 pos);
	float3 GetMousePosInWorld();

	//GameplayFunctions
	void SetUnitMoveLocation(float3 target,flecs::entity& unit);
	void SetUnitMoveLocationAndRotation(float3 target,flecs::entity& unit);
	void SetUnitMovePath(float3 target,uint unit);
	void SetUnitAttackTarget(uint target,uint unit);
	void SetSelectedTanksMoveLocation(float3 target);
	void SetSelectedTanksAttackTarget(uint target);
	void SetOutlineSelectedUnits();
	vector <uint> GetFriendlyUnitInArea(float3 start, float3 end);
	bool IsFriendlyUnit(uint unitID);
	bool IsEnemyUnit(uint enemyID);
	int IsEnemyBuilding(float3 raycastPos);
	bool hasLost();
	float3 PaniniProjection(float2 tc, const float fov, const float d);
	
	flecs::entity SpawnEntity(uint unit, uint playerID = 0, float3 location = make_float3(0,0,0));
	flecs::entity SpawnUnit(uint unit, uint playerID = 0, float3 location = make_float3(0,0,0));
	flecs::entity SpawnUnitWithChild(uint top, uint bottom, float speed = 0.5f, uint playerID = 0, float3 location = make_float3(0,0,0), float3 offset = make_float3(0, 0, 0));
	flecs::entity SpawnFastTank(uint playerID = 0, float3 location = make_float3(0,0,0));
	flecs::entity SpawnNormalTank(uint playerID = 0, float3 location = make_float3(0,0,0));
	flecs::entity SpawnArtilleryTank(uint playerID = 0, float3 location = make_float3(0,0,0));
	flecs::entity SpawnPatrollingTank(uint playerID, float3 location, vector<float3> patrolPoints);
	flecs::entity SpawnPatrollingTank(uint playerID, vector<float3> patrolPoints);
	flecs::entity SpawnPatrollingAtrilleryTank(uint playerID, float3 location, vector<float3> patrolPoints);
	flecs::entity SpawnPatrollingAtrilleryTank(uint playerID, vector<float3> patrolPoints);


private:

	float2 mousePos;
	vector<uint> selectedUnits;
	uint outlineSprties[20];
	BindingUnits unitBindings[10];
	Camera camera;
	
	Units units;
	TileLoader tileLoader;
	Outlines outlines;

	World* world;

	flecs::entity headquarters;
	bool gameWon;
	bool gameLose;
	int highScore;

	//Selecting Units
	float3 startPos;
	bool keys[350];
};

} // namespace Tmpl8