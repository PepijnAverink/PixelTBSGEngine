#include <precomp.h>
#include "Pathfinder.h"
#include <queue>

vector<int> Tmpl8::Pathfinder::CalculateFlowField(const int2 target)
{

    vector<int> integrationField = CalculateIntegrationField(target);
    vector<int> flowField = vector<int>(integrationField.size());


    int smallestNeibor = 65535;
    int smallestNeiborIndex = -1;
    for (int i = 0; i < flowField.size(); ++i)
    {
        vector<int> neibors = GetNeibors(i);
        smallestNeibor = 65535;
        smallestNeiborIndex = -1;
        for (int ii = 0; ii < neibors.size(); ++ii)
        {
            if (integrationField[neibors[ii]] < smallestNeibor)
            {
                smallestNeibor = integrationField[neibors[ii]];
                smallestNeiborIndex = ii;
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

        vector<int> neibors = GetNeibors(currentID);

        for (int currentNeiborID : neibors)
        {
            int neiborCost = costField[currentNeiborID];
            if (neiborCost != 255)
            {
                int endNodeCost = neiborCost + integrationField[currentID];
                if (integrationField[currentNeiborID] > endNodeCost)
                {
                    if (!CheckIfContains(currentNeiborID, openList))
                    {
                        openList.push_back(currentNeiborID);
                    }

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
    vector<int> neibors = { target - mapSize.y, target + mapSize.y, target - 1, target + 1 };
    vector<int> result = vector<int>();
    for (int index : neibors)
    {
        if (index >= 0 && index < mapSize.x * mapSize.y)
        {
            result.push_back(index);
        }
    }


    return result;
}

const vector<int>* Tmpl8::Pathfinder::GetFlowFlield(const int2 target)
{
    if (flowfields.find(target) != flowfields.end())
    {
        return &flowfields[target];
    }
    else
    {
        flowfields.insert(std::pair<int2, vector<int>>(target, CalculateFlowField(target)));
        return &flowfields[target];
    }
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
