#include "unit_manager.h"
#include <cassert>
#include <algorithm>

UnitManager &UnitManager::Instance()
{
    static UnitManager instance;
    return instance;
}

Unit *UnitManager::CreateUnit(Unit::Type type)
{
    int id = static_cast<int>(units_.size());
    Unit *u = new Unit(type, id);
    units_.push_back(u);
    return u;
}

void UnitManager::TickAll()
{
    for (Unit *u : Instance().units_) {
        if (u) u->tick();
    }
}

// Exposed C-linkage function for game loop integration
void CallUnitTicks()
{
    UnitManager::TickAll();
}

void UnitManager::SelectUnit(int id)
{
    if (id < 0 || id >= static_cast<int>(units_.size())) {
        selected_unit_id_ = -1;
        return;
    }
    selected_unit_id_ = id;
}

void UnitManager::ClearSelection()
{
    selected_unit_id_ = -1;
}

void UnitManager::FormGroup(int leader_id, int size, Unit::FormationStyle style)
{
    Unit *leader = GetUnitById(leader_id);
    if (leader == nullptr || size <= 0) return;

    int slot = 0;
    for (Unit *u : units_) {
        if (u == nullptr) continue;
        if (u == leader) {
            u->SetFormation(leader_id, 0, style);
            continue;
        }

        if (slot >= size) {
            u->ClearFormation();
            continue;
        }

        u->SetFormation(leader_id, slot + 1, style);
        ++slot;
    }
}

Unit *UnitManager::GetUnitById(int id) const
{
    if (id < 0 || id >= static_cast<int>(Instance().units_.size())) return nullptr;
    return Instance().units_[id];
}
