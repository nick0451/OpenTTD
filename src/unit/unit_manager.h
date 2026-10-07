#pragma once

#include <vector>
#include "unit.h"

class UnitManager {
public:
    static UnitManager &Instance();
    Unit *CreateUnit(Unit::Type type);
    Unit *GetUnitById(int id) const;
    const std::vector<Unit*> &GetUnits() const { return units_; }
    struct TownObjective {
        int x = 0;
        int y = 0;
        int defense_value = 20;
        int resource_value = 0;
    };

    void SelectUnit(int id);
    void ClearSelection();
    void SpawnDemoBattleScenario();
    void AddTownObjective(int x, int y, int defense_value, int resource_value = 0);
    void SetScenarioObjective(int x, int y);
    int GetObjectiveX() const { return objective_x_; }
    int GetObjectiveY() const { return objective_y_; }
    const std::vector<TownObjective> &GetTownObjectives() const { return town_objectives_; }
    void AssignDefensiveRoles();
    void FormGroup(int leader_id, int size = 4, Unit::FormationStyle style = Unit::FormationStyle::Column);
    int GetSelectedUnitId() const { return selected_unit_id_; }
    bool HasSelectedUnit() const { return selected_unit_id_ >= 0; }
    static void TickAll();

private:
    UnitManager() = default;
    std::vector<Unit*> units_;
    std::vector<TownObjective> town_objectives_;
    int selected_unit_id_ = -1;
    int objective_x_ = 320;
    int objective_y_ = 240;
};
