#include "precomp.h"
#include "mygame.h"

Game* game = new MyGame();

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void MyGame::Init()
{
	ShowCursor(false);

	// init deer flock
//	GetWorld()->LoadSprite("assets/deer.vox");
//	for (int i = 0; i < 50; i++)
//	{
//		GetWorld()->CloneSprite(0);
//		dx[i] = RandomUInt() % 1000 + 1, dz[i] = i * 20 + 10;
//		df[i] = RandomUInt() % (GetWorld()->SpriteFrameCount(0) * 4);
//	}


	uint cloneCount = 7;
	uint startPos = 50;
	GetWorld()->LoadSprite("assets/tile00.vox");
	GetWorld()->MoveSpriteTo(0, startPos, 1, 10);
//	GetWorld()->ScaleSprite(0, {4, 14, 4});
	uint pos = startPos;
	for (uint i = 0; i < cloneCount; i++)
	{
		pos += (i + 1) * 16;

		uint idx = GetWorld()->CloneSprite(0);
		GetWorld()->ScaleSprite(idx, {i + 2, i + 2, i + 2});
		GetWorld()->MoveSpriteTo(idx, pos, 1, 10);
	}
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
static float rot = 0.0f;
void MyGame::Tick( float deltaTime )
{
	// This function gets called once per frame by the template code.
	World* world = GetWorld();
	world->Print( "Hello World!", 280, 128, 5, 1 );
	world->SetCameraMatrix( mat4::LookAt( make_float3( 512, 128, 512 ), make_float3( 384, 128, 0 ) ) );

//	// deer
//	for (int i = 0; i < 50; i++)
//	{
//		world->MoveSpriteTo(i, dx[i], 1, dz[i]);
//		world->SetSpriteFrame(i, df[i] >> 3);
//		if (++df[i] == world->SpriteFrameCount(0) * 8) df[i] = 0;
//		if (--dx[i] < 15) dx[i] = 990;
//	}
}