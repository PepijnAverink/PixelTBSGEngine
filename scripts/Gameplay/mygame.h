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
	void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void KeyUp( int key ) { keys[key] = false; }
	void KeyDown(int key) { keys[key] = true; }

	//Raycasting
	void HandleMouseRaycasting();


	flecs::entity SpawnEntity(uint unit, float3 location = make_float3(0,0,0));
	// data members
	int2 mousePos;

private:
	const uint gridXSize = 10;
	const uint gridZSize = 10;
	vector<uint> grid;
	uint index = 0;
	flecs::world ecs;
	flecs::entity tank;
	Units units;
	Terrain terrain;
	World* world;

	//Input
	int keys[640];

};

} // namespace Tmpl8