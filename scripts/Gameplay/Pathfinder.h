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
        inline void SetCostOnCostField(const int pos) { costField[pos] = 255; };
        inline void RemoveCostOnCostField(const int pos) { costField[pos] = 1; };
        inline void SetMapSize(const int2 newMapSize) { mapSize = newMapSize; unitField = vector<int>(newMapSize.x * newMapSize.y); };
        inline void SetUnitInUnitField(const int oldPos, const int newPos) { if (oldPos > 0 && oldPos < unitField.size()) { unitField[oldPos] = 0; } if (newPos > 0 && newPos < unitField.size()) { unitField[newPos] = 255; } };
        inline void SetUnitInUnitField(const int pos) { if (pos > 0 && pos < unitField.size()) { unitField[pos] = 255; } };
        inline void SetUnitInUnitFieldWithAmount(const int pos,const int amount) { if (pos > 0 && pos < unitField.size()) { unitField[pos] = amount; } };
        inline void RemoveUnitInUnitField(const int pos) { if (pos > 0 && pos < unitField.size()) { unitField[pos] = 0; } };
        vector<int> GetFlowFlield(const int2 target);
        void VisualizeFlowField(float3 target);
        void VisualizeUnitField();
        const vector <float3> GetTargetsForUnit(float3 target, int index);

        //Debug
        uint arrow;
        uint point;
        //Move to private. is only public for testing
        vector<int> unitField;
        vector<int> CalculateIntegrationField(const int2 target);
    private:
        vector<int> CalculateFlowField(const int2 target);
        void AddUnitsToCostField(const vector<flecs::entity> entities);
        vector<int> GetNeibors(const int target);
        vector<int> GetSquareNeibors(const int target);
        bool CheckIfContains(const int index, const list<int> list);
        bool CheckIfContains(const int index, const vector<int> list);
        vector<int> costField;
        map<int2, vector<int>> flowfields = map<int2,vector<int>>();
        int2 mapSize;

        //Debug
        vector<uint> arrows;
        vector<uint> points;


    };
}