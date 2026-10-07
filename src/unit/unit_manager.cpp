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
    town_objectives_.clear();

    const int player_base_x = 80;
    const int player_base_y = 120;
    const int enemy_base_x = 560;
    const int enemy_base_y = 420;
    objective_x_ = 320;
    objective_y_ = 240;

    AddTownObjective(140, 160, 22, 18);
    AddTownObjective(320, 240, 30, 24);
    AddTownObjective(520, 370, 28, 20);

    for (int i = 0; i < 5; ++i) {
        Unit *u = CreateUnit(Unit::Type::Infantry);
        u->SetOwner(Unit::Owner::Player);
        u->SetBasePosition(player_base_x + (i * 26), player_base_y + ((i % 2) * 18));
        u->SetTacticalState(Unit::TacticalState::Advance);
        u->SetAssignedObjective(140 + (i % 2) * 20, 160 + (i % 3) * 16);
    }

    for (int i = 0; i < 4; ++i) {
        Unit *u = CreateUnit(Unit::Type::Armor);
        u->SetOwner(Unit::Owner::Enemy);
        u->SetBasePosition(enemy_base_x - (i * 28), enemy_base_y - ((i % 2) * 18));
        u->SetTacticalState(Unit::TacticalState::Advance);
        u->SetAssignedObjective(520 - (i % 2) * 26, 370 - (i % 3) * 18);
    }

    AssignDefensiveRoles();
}

void UnitManager::AddTownObjective(int x, int y, int defense_value, int resource_value)
{
    TownObjective objective;
    objective.x = x;
    objective.y = y;
    objective.defense_value = defense_value;
    objective.resource_value = resource_value;
    town_objectives_.push_back(objective);
}

void UnitManager::AssignDefensiveRoles()
{
    for (Unit *u : units_) {
        if (u == nullptr) continue;

        int best_town_index = -1;
        int best_distance = std::numeric_limits<int>::max();
        for (size_t i = 0; i < town_objectives_.size(); ++i) {
            const TownObjective &objective = town_objectives_[i];
            const int distance = std::abs(static_cast<int>(u->GetX()) - objective.x) + std::abs(static_cast<int>(u->GetY()) - objective.y);
            if (distance < best_distance) {
                best_distance = distance;
                best_town_index = static_cast<int>(i);
            }
        }

        if (best_town_index >= 0) {
            const TownObjective &objective = town_objectives_[best_town_index];
            u->SetAssignedObjective(objective.x, objective.y);
            if (u->GetRoleDefenseWeight() >= 2) {
                u->SetTacticalState(Unit::TacticalState::Hold);
            }
        }
    }
}

void UnitManager::SetScenarioObjective(int x, int y)
{
    objective_x_ = x;
    objective_y_ = y;

    for (Unit *u : units_) {
        if (u == nullptr) continue;
        if (u->GetOwner() == Unit::Owner::Player || u->GetOwner() == Unit::Owner::Enemy) {
            if (!u->HasAttackTarget() && !u->HasMoveTarget()) {
                u->SetMoveTarget(x, y);
            }
        }
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
