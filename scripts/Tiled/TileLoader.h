#pragma once
#include "tileson.hpp"

namespace Tmpl8
{
	enum class TileType { Terrain = 0, Building, Setdress };

	struct TileData
	{
		uint tile;
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
		uint GetID(string name);
		TileType GetTileType(int ID);
		int GetHeight(int ID);
        Grid grid;
		vector<int> GetHeightMap() { return heightMap; };



	private:
		vector<int> heightMap;
		uint terrain[42];

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
			34,
			35,
			36,
			37
		};

		set<int> secondHeight =
		{
			38,
			39,
			40,
			41
		};

		string terrainNames[42] =
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
			"GrassSlope_270",
			"GrassSlope",
			"GrassSlope_90",
			"GrassSlope_180",
			"GrassSlope_Second_270",
			"GrassSlope_Second",
			"GrassSlope_Second_90",
			"GrassSlope_Second_180",
		};
    };
}