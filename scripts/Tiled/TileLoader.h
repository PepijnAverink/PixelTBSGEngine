#pragma once
#include "tileson.hpp"

namespace Tmpl8
{
	enum class TileType { Terrain = 0, Building, Setdress };

	struct TileData
	{
		int tile;
		TileType tileType;
		float rotation;
	};

	struct Grid
	{
		vector<TileData> tiledatasLayer1;
		vector<TileData> tiledatasLayer2;
		int width;
		int height;
	};

    class TileLoader
    {
    public:
        void Init();
        Grid LoadTile(string filePath);
		vector<int> GetHeightMap() { return heightMap; };
		vector<int> GetCostField() { return costField; };
		uint GetID(string name);
        Grid grid;

	private:
		TileType GetTileType(int ID);
		int GetHeight(int ID);
		int GetCost(int ID);

		vector<int> heightMap;
		vector<int> costField;
		uint terrain[36];

		set<int> buildingIDs =
		{
			6,
			13,
			20,
			27
		};

		set<int> setDressIDs =
		{
			25,
			26,
			32,
			33
		};

		set<int> firstHeight =
		{
			34
		};

		set<int> secondHeight =
		{
			35
		};

		map<int, int> terrainCost =
		{
			{ 0, 1 },
			{ 1, 1 },
			{ 2, 1 },
			{ 3, 255 },
			{ 4, 255 },
			{ 5, 255 },
			{ 6, 255 },
			{ 7, 1 },
			{ 8, 1 },
			{ 9, 1 },
			{ 10, 255 },
			{ 11, 255 },
			{ 12, 255 },
			{ 13, 255 },
			{ 14, 1 },
			{ 15, 1 },
			{ 16, 1 },
			{ 17, 255 },
			{ 18, 255 },
			{ 19, 255 },
			{ 20, 255 },
			{ 21, 255 },
			{ 22, 255 },
			{ 23, 255 },
			{ 24, 255 },
			{ 25, 255 },
			{ 26, 255 },
			{ 27, 255 },
			{ 28, 255 },
			{ 29, 255 },
			{ 30, 255 },
			{ 31, 255 },
			{ 32, 255 },
			{ 33, 1 },
			{ 34, 1 },
			{ 35, 1 }
		};

		string terrainNames[36] =
		{
			"Road_Corner_180",
			"Road_90",
			"Road_Corner_270",
			"Beach_InnerCorner",
			"Beach",
			"Beach_InnerCorner_90",
			"Base",
			"Road",
			"Grass",
			"Road_180",
			"Beach_270",
			"Sea",
			"Beach_90",
			"City",
			"Road_Corner_90",
			"Road_270",
			"Road_Corner",
			"Beach_InnerCorner_270",
			"Beach_180",
			"Beach_InnerCorner_180",
			"QG",
			"Beach_OuterCorner_180",
			"Beach_OuterCorner_270",
			"River_180",
			"River_270",
			"Mountain_Low",
			"Mountain_High",
			"Airport",
			"Beach_OuterCorner_90",
			"Beach_OuterCorner",
			"River_90",
			"River",
			"Forest",
			"Bridge",
			"Grass_Height1",
			"Grass_Height2",
		};
    };
}