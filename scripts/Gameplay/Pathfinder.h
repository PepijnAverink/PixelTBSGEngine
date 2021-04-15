#pragma once
#include <vector>
#include <map>
#include "flecs.h"

namespace Tmpl8
{
    class Pathfinder
    {
    public:
        inline void SetCostField(const vector<int> newCostField) { costField = newCostField; };
        inline void SetMapSize(const int2 newMapSize) { mapSize = newMapSize; };
        const vector<int>* GetFlowFlield(const int2 target);
    private:
        vector<int> CalculateFlowField(const int2 target);
        vector<int> CalculateIntegrationField(const int2 target);
        void AddUnitsToCostField(const vector<flecs::entity> entities);
        vector<int> GetNeibors(const int target);
        bool CheckIfContains(const int index, const list<int> list);
        vector<int> costField;
        map<int2, vector<int>> flowfields = map<int2,vector<int>>();
        int2 mapSize;
    };
}