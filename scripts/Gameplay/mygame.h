#pragma once
#include "flecs.h"
#include "GameData.h"
#include <vector>
#include "Camera.h"
#include "scripts\Tiled\TileLoader.h"
namespace Tmpl8
{
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
	void SetUnitAttackTarget(uint target,flecs::entity& unit);
	void SetSelectedTanksMoveLocation(float3 target);
	void SetSelectedTanksAttackTarget(uint target);
	void SetOutlineSelectedUnits();
	vector <uint> GetFriendlyUnitInArea(float3 start, float3 end);
	bool IsFriendlyUnit(uint unitID);
	bool IsMoveableTerrain(uint terrainID);
	bool IsEnemyUnit(uint enemyID);

	
	flecs::entity SpawnEntity(uint unit, int playerID = 0, float3 location = make_float3(0,0,0));
	flecs::entity SpawnTank(uint unit, int playerID = 0, float3 location = make_float3(0,0,0));


private:
	float2 mousePos;
	vector<uint> selectedUnits;
	uint outlineSprties[20];
	BindingUnits unitBindings[10];
	Camera camera;

	vector<uint> grid;
	uint index = 0;
	
	//Sprites aka voxels
	Units units;
	TileLoader tileLoader;
	Outlines outlines;

	World* world;

	//Selecting Units
	float3 startPos;
	bool keys[350];
};

} // namespace Tmpl8