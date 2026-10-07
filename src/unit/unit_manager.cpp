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
    if (Instance().units_.empty()) {
        Instance().SpawnDemoBattleScenario();
    }

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

void UnitManager::SpawnDemoBattleScenario()
{
    for (Unit *u : units_) {
        delete u;
    }
    units_.clear();

    const int player_base_x = 80;
    const int player_base_y = 120;
    const int enemy_base_x = 560;
    const int enemy_base_y = 420;

    for (int i = 0; i < 5; ++i) {
        Unit *u = CreateUnit(Unit::Type::Infantry);
        u->SetOwner(Unit::Owner::Player);
        u->SetBasePosition(player_base_x + (i * 26), player_base_y + ((i % 2) * 18));
        u->SetMoveTarget(player_base_x + 120 + (i * 20), player_base_y + 140 + ((i % 3) * 16));
        u->SetTacticalState(Unit::TacticalState::Advance);
    }

    for (int i = 0; i < 4; ++i) {
        Unit *u = CreateUnit(Unit::Type::Armor);
        u->SetOwner(Unit::Owner::Enemy);
        u->SetBasePosition(enemy_base_x - (i * 28), enemy_base_y - ((i % 2) * 18));
        u->SetMoveTarget(enemy_base_x - 120, enemy_base_y - 120);
        u->SetTacticalState(Unit::TacticalState::Advance);
    }
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
