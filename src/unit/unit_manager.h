#pragma once

#include <vector>
#include "unit.h"

class UnitManager {
public:
    static UnitManager &Instance();
    Unit *CreateUnit(Unit::Type type);
    Unit *GetUnitById(int id) const;
    const std::vector<Unit*> &GetUnits() const { return units_; }
    void SelectUnit(int id);
    void ClearSelection();
    int GetSelectedUnitId() const { return selected_unit_id_; }
    bool HasSelectedUnit() const { return selected_unit_id_ >= 0; }
    static void TickAll();

private:
    UnitManager() = default;
    std::vector<Unit*> units_;
    int selected_unit_id_ = -1;
};
