#include "precomp.h"
#include "TileLoader.h"

void TileLoader::Init()
{
    World* world = GetWorld();

    int arraySize = sizeof(terrainNames) / sizeof(terrainNames[0]);
    for(int i =0; i < arraySize; ++i)
    {
        TileType tileType = GetTileType(i);
        switch (tileType)
        {
            case TileType::Setdress:
            {
                terrain[i] = world->LoadSprite(("assets/SetDressing/" + terrainNames[i] + ".vox").c_str());
                break;
            }
            case TileType::Building:
            {
                terrain[i] = world->LoadSprite(("assets/Buildings/" + terrainNames[i] + ".vox").c_str());
                break;
            }
            case TileType::Terrain:
            {
                terrain[i] = world->LoadBigTile(("assets/Terrain/16x16x16/" + terrainNames[i] + ".vox").c_str());
                break;
            }
        }
    }
}

const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const unsigned FLIPPED_VERTICALLY_FLAG = 0x40000000;
const unsigned FLIPPED_DIAGONALLY_FLAG = 0x20000000;

Grid TileLoader::LoadTile(string filePath)
{
    grid.tiledatasLayer1 = std::vector<TileData>();
    grid.tiledatasLayer2 = std::vector<TileData>();
    tson::Tileson t;
    std::unique_ptr<tson::Map> map = t.parse(fs::path(filePath));

    if (map->getStatus() == tson::ParseStatus::OK)
    {

        grid.width = map->getSize().x;
        grid.height = map->getSize().y;
        heightMap = vector<int>(map->getSize().x * map->getSize().y);
        costField = vector<int>(map->getSize().x * map->getSize().y);
        for (auto& terrainLayer : map->getLayers())
        {
            int index = 0;
            for (auto& obj : terrainLayer.getData())
            {
                unsigned ID = obj;
                TileData tileData = TileData();
                tileData.tile = -1;
                if (ID != 0)
                {
                    tileData.rotation = 0;
                    tileData.rotation = (ID & FLIPPED_HORIZONTALLY_FLAG) ? 90 : tileData.rotation;
                    tileData.rotation = (ID & FLIPPED_VERTICALLY_FLAG) ? 180 : tileData.rotation;
                    tileData.rotation = (ID & FLIPPED_DIAGONALLY_FLAG) ? 270 : tileData.rotation;

                    // Clear the flags
                    ID &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

                    ID -= 1;
                    tileData.tile = terrain[ID];
                    tileData.tileType = GetTileType(ID);
                    costField[index] = GetCost(ID);
                    if (terrainLayer.getName() == "Tile Layer 2")
                    {
                        heightMap[index] = GetHeight(ID);
                    }
                }
                if (terrainLayer.getName() == "Tile Layer 1")
                {
                    grid.tiledatasLayer1.push_back(tileData);
                }
                else if (terrainLayer.getName() == "Tile Layer 2")
                {
                    grid.tiledatasLayer2.push_back(tileData);
                }

                index++;
            }
        }
    }
    return grid;
}

uint Tmpl8::TileLoader::GetID(string name)
{
    int arraySize = sizeof(terrainNames) / sizeof(terrainNames[0]);
    for (uint i = 0; i < arraySize; ++i)
    {
        if (terrainNames[i] == name)
        {
            return terrain[i];
        }
    }
    return 0;
}

TileType Tmpl8::TileLoader::GetTileType(int ID)
{
    if (buildingIDs.find(ID) != buildingIDs.end())
    {
        return TileType::Building;
    }
    else if (setDressIDs.find(ID) != setDressIDs.end())
    {
        return TileType::Setdress;
    }
    return TileType::Terrain;
}

int Tmpl8::TileLoader::GetHeight(int ID)
{
    if (firstHeight.find(ID) != firstHeight.end())
    {
        return 2;
    }
    else if (secondHeight.find(ID) != secondHeight.end())
    {
        return 4;
    }

    return 0;
}

int Tmpl8::TileLoader::GetCost(int ID)
{
    return terrainCost[ID];
}
