#pragma once
#include "flecs.h"
#include "GameData.h"
#include <vector>
namespace Tmpl8
{
class MyGame : public Game
{
public:
	// game flow methods
	void Init();
	void AddComponentsToFlecsWorld();
	void SpawnWorld();
	void Tick( float deltaTime );
	void Shutdown() { /* implement if you want to do something on exit */ }
	// input handling
	void MouseUp(int button) { mouseClicked = false; }
	void MouseDown(int button) { mouseClicked = true; }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void KeyUp( int key ) { keys[key] = false; }
	void KeyDown(int key) { keys[key] = true; }

	//Raycasting
	uint HandleMouseRaycasting();

	//GameplayFunctions
	//void SetTankTarget(float3 target);
	void SetSelectedTanksTarget(float3 target);


	flecs::entity SpawnEntity(uint unit, int playerID = 0, float3 location = make_float3(0,0,0));
	// data members
	int2 mousePos;
	bool mouseClicked;

private:
	const uint gridXSize = 10;
	const uint gridZSize = 10;
	vector<uint> grid;
	uint index = 0;
	//flecs::entity tank;
	Units units;
	Terrain terrain;
	World* world;

	//Input
	int keys[640];

};

} // namespace Tmpl8