#include <precomp.h>
#include "Pathfinder.h"
#include <queue>

vector<int> Tmpl8::Pathfinder::CalculateFlowField(const int2 target)
{

    vector<int> integrationField = CalculateIntegrationField(target);
    vector<int> flowField = vector<int>(integrationField.size());


    int smallestNeibor;
    int smallestNeiborIndex;
    for (int i = 0; i < flowField.size(); ++i)
    {
        smallestNeibor = 65535;
        smallestNeiborIndex = -1;
        vector<int> neibors = GetNeibors(i);
        if (integrationField[i] != 65535)
        {
            for (int ii = 0; ii < neibors.size(); ++ii)
            {
                if (integrationField[neibors[ii]] < smallestNeibor)
                {
                    smallestNeibor = integrationField[neibors[ii]];
                    smallestNeiborIndex = ii;
                }
            }
        }
        flowField[i] = smallestNeiborIndex != -1 ? neibors[smallestNeiborIndex] : -1;
        
    }
    return flowField;
}

vector<int> Tmpl8::Pathfinder::CalculateIntegrationField(const int2 target)
{

    vector<int> integrationField = vector<int>(costField.size(), 65535);
    int targetID = GridPosToIndex(target, mapSize.x);
    integrationField[targetID] = 0;
    list<int> openList = { targetID };

    while (openList.size() > 0)
    {
        int currentID = openList.front();
        openList.pop_front();

        vector<int> neibors = GetSquareNeibors(currentID);

        for (int currentNeiborID : neibors)
        {
            int neiborCost = costField[currentNeiborID] + unitField[currentNeiborID];
            int endNodeCost = neiborCost + integrationField[currentID];
            if (integrationField[currentNeiborID] > endNodeCost)
            {
                if (!CheckIfContains(currentNeiborID, openList))
                {
                    openList.push_back(currentNeiborID);
                }
                if (costField[currentNeiborID] < 255)
                {
                    integrationField[currentNeiborID] = endNodeCost;
                }
            }      
        }
    }

    return integrationField;
}

void Tmpl8::Pathfinder::AddUnitsToCostField(const vector<flecs::entity> entities)
{
    World* world = GetWorld();
    for (flecs::entity entity : entities)
    {
        int2 indexes = GetIndexes(make_float3(world->sprite[entity.id()]->currPos));
        costField[GridPosToIndex(indexes, mapSize.x)] = 255;
    }
}

vector<int> Tmpl8::Pathfinder::GetNeibors(const int target)
{
    int2 gridPos = IndexToGridPos(target, mapSize.x);
    vector<int2> neibors = { 
        make_int2(gridPos.x + 1,gridPos.y), make_int2(gridPos.x - 1,gridPos.y),make_int2(gridPos.x,gridPos.y + 1),make_int2(gridPos.x, gridPos.y - 1),
        make_int2(gridPos.x + 1,gridPos.y + 1),make_int2(gridPos.x - 1,gridPos.y + 1),
        make_int2(gridPos.x + 1, gridPos.y - 1),make_int2(gridPos.x - 1, gridPos.y - 1)
    };
    vector<int> result = vector<int>();
    for (int2 indexes : neibors)
    {
        if (indexes.x >= 0 && indexes.x < mapSize.x && indexes.y >= 0 && indexes.y < mapSize.y)
        {
            result.push_back(GridPosToIndex(indexes,mapSize.x));
        }
    }


    return result;
}

vector<int> Tmpl8::Pathfinder::GetSquareNeibors(const int target)
{
    int2 gridPos = IndexToGridPos(target, mapSize.x);
    vector<int2> neibors = {
        make_int2(gridPos.x,gridPos.y + 1),
        make_int2(gridPos.x + 1,gridPos.y), make_int2(gridPos.x - 1,gridPos.y),
        make_int2(gridPos.x, gridPos.y - 1)
    };
    vector<int> result = vector<int>();
    for (int2 indexes : neibors)
    {
        if (indexes.x >= 0 && indexes.x < mapSize.x && indexes.y >= 0 && indexes.y < mapSize.y)
        {
            result.push_back(GridPosToIndex(indexes, mapSize.x));
        }
    }


    return result;
}

vector<int> Tmpl8::Pathfinder::GetFlowFlield(const int2 target)
{
    return CalculateFlowField(target);
}

void Tmpl8::Pathfinder::VisualizeFlowField(float3 target)
{
    const vector<int> flowField = GetFlowFlield(GetIndexes(target));
    if (arrows.empty())
    {
        arrows = vector<uint>(flowField.size());
        for (int i = 0; i < mapSize.x; ++i)
        {
            for (int ii = 0; ii < mapSize.y; ++ii)
            {
                int index = i * mapSize.y + ii;
                uint3 spriteSpawnPos = make_uint3(i * 16 + (10 * 16), 32, ii * 16 + (10 * 16));
                arrows[index] = GetWorld()->CloneSprite(arrow);
                GetWorld()->SetSpritePivot(arrows[index], 8, 0, 8);
                GetWorld()->MoveSpriteTo(arrows[index], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
            }
        }
    }

    for (int i = 0; i < flowField.size(); ++i)
    {
        if (flowField[i] == -1)
        {
            GetWorld()->DisableSprite(arrows[i]);
        }
        else
        {
            switch (i - flowField[i])
            {
            case 1:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(90));
                break;
            }
            case -1:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(270));
                break;
            }
            case 31:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(315));
                break;
            }
            case 32:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(0));
                break;
            }
            case 33:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(45));
                break;
            }
            case -31:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(135));
                break;
            }
            case -32:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(180));
                break;
            }
            case -33:
            {
                GetWorld()->RotateSprite(arrows[i], 0, 1, 0, DegreesToRadians(225));
                break;
            }
            }
        }
    }
}

void Tmpl8::Pathfinder::VisualizeUnitField()
{

    if (points.empty())
    {
        points = vector<uint>(unitField.size());
        moveingPoints = vector<uint>(unitField.size());
        for (int i = 0; i < mapSize.x; ++i)
        {
            for (int ii = 0; ii < mapSize.y; ++ii)
            {
                int index = i * mapSize.y + ii;
                uint3 spriteSpawnPos = make_uint3(i * 16 + (10 * 16), 32, ii * 16 + (10 * 16));
                points[index] = GetWorld()->CloneSprite(point);
                GetWorld()->SetSpritePivot(points[index], 8, 0, 8);
                GetWorld()->MoveSpriteTo(points[index], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);

                moveingPoints[index] = GetWorld()->CloneSprite(moveingPoint);
                GetWorld()->SetSpritePivot(moveingPoints[index], 8, 0, 8);
                GetWorld()->MoveSpriteTo(moveingPoints[index], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
            }
        }
    }
    for (int i = 0; i < unitField.size(); ++i)
    {
        if (unitField[i] == 100)
        {
            GetWorld()->EnableSprite(points[i]);
            int2 gridPos = IndexToGridPos(i, mapSize.x);
            uint3 spriteSpawnPos = make_uint3(gridPos.x * 16 + (10 * 16), 32, gridPos.y * 16 + (10 * 16));
            GetWorld()->MoveSpriteTo(points[i], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
            GetWorld()->DisableSprite(moveingPoints[i]);
        }
        else if (unitField[i] > 0)
        {
            GetWorld()->EnableSprite(moveingPoints[i]);
            int2 gridPos = IndexToGridPos(i, mapSize.x);
            uint3 spriteSpawnPos = make_uint3(gridPos.x * 16 + (10 * 16), 32, gridPos.y * 16 + (10 * 16));
            GetWorld()->MoveSpriteTo(moveingPoints[i], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
            GetWorld()->DisableSprite(points[i]);
        }
        else
        {
            GetWorld()->DisableSprite(points[i]);
            GetWorld()->DisableSprite(moveingPoints[i]);
        }
    }
}

void Tmpl8::Pathfinder::VisualizeCostField()
{

    if (nonPassableObjects.empty())
    {
        nonPassableObjects = vector<uint>(costField.size());
        for (int i = 0; i < mapSize.x; ++i)
        {
            for (int ii = 0; ii < mapSize.y; ++ii)
            {
                int index = i * mapSize.y + ii;
                uint3 spriteSpawnPos = make_uint3(i * 16 + (10 * 16), 32, ii * 16 + (10 * 16));
                nonPassableObjects[index] = GetWorld()->CloneSprite(nonPassableObject);
                GetWorld()->SetSpritePivot(nonPassableObjects[index], 8, 0, 8);
                GetWorld()->MoveSpriteTo(nonPassableObjects[index], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
            }
        }
    }
    for (int i = 0; i < costField.size(); ++i)
    {
        if (costField[i] == 255)
        {
            GetWorld()->EnableSprite(nonPassableObjects[i]);
            int2 gridPos = IndexToGridPos(i, mapSize.x);
            uint3 spriteSpawnPos = make_uint3(gridPos.x * 16 + (10 * 16), 32, gridPos.y * 16 + (10 * 16));
            GetWorld()->MoveSpriteTo(nonPassableObjects[i], spriteSpawnPos.x, spriteSpawnPos.y, spriteSpawnPos.z);
        }
        else
        {
            GetWorld()->DisableSprite(nonPassableObjects[i]);
        }
    }
}

const vector <float3> Tmpl8::Pathfinder::GetTargetsForUnit(float3 target, int index)
{
    vector <float3> targets;
    int2 indexes = GetIndexes(target);
    int numberOfTargets = 0;
    vector<int> searchTargets;
    int targetIndex = GridPosToIndex(indexes, mapSize.x);
    if (costField[targetIndex] < 255 && unitField[targetIndex] < 255)
    {
        float2 newPos = GetEntityPos(indexes);
        targets.push_back(make_float3(newPos.x,target.y, newPos.y));
        ++numberOfTargets;
    }
    searchTargets.push_back(targetIndex);
    for (int i = 0; i < searchTargets.size(); ++i)
    {
        vector<int> neibors = GetNeibors(searchTargets[i]);
        for (int ii = 0; ii < neibors.size(); ++ii)
        {
            if (numberOfTargets >= index)
            {
                return targets;
            }

            if (!CheckIfContains(neibors[ii], searchTargets))
            {
                if (costField[neibors[ii]] < 255 && unitField[neibors[ii]] < 255)
                {
                    int2 gridPos = IndexToGridPos(neibors[ii], mapSize.x);
                    float2 newPos = GetEntityPos(gridPos);
                    targets.push_back(make_float3(newPos.x, target.y, newPos.y));
                    ++numberOfTargets;
                }
                searchTargets.push_back(neibors[ii]);
            }
        }
    }
    return targets;
}

bool Tmpl8::Pathfinder::CheckIfContains(const int index, const list<int> list)
{
    for (int currentIndex : list)
    {
        if (currentIndex == index)
        {
            return true;
        }
    }

    return false;
}

bool Tmpl8::Pathfinder::CheckIfContains(const int index, const vector<int> list)
{
    for (int currentIndex : list)
    {
        if (currentIndex == index)
        {
            return true;
        }
    }

    return false;
}
