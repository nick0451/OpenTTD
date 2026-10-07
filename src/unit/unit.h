#pragma once

#include "stdint.h"
#include <cmath>
#include <utility>
#include <vector>

class Unit {
public:
    enum class Type { Infantry, Armor, Artillery, Air, Naval };
    enum class FormationStyle { None, Column, Wedge, Line };
    enum class TacticalState { Advance, Hold, Retreat };
    enum class Owner { Player, Enemy };

    Unit(Type type, int id);
    ~Unit();

    int get_id() const;
    Type get_type() const;
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetHeading() const { return heading_; }
    int GetHealth() const { return health_; }
    int GetSupply() const { return supply_; }
    bool IsAlive() const { return health_ > 0; }
    bool IsMoving() const;
    bool IsUnderSupplyPressure() const;
    TacticalState GetTacticalState() const { return tactical_state_; }
    Owner GetOwner() const { return owner_; }
    void SetOwner(Owner owner) { owner_ = owner; }
    int GetFrontlinePressure() const;
    Unit *FindNearestEnemy() const;
    Unit *FindBestThreatTarget() const;
    float EvaluateTargetPriority(const Unit *target) const;

    void tick();

    // RTS-style orders
    void SetMoveTarget(int wx, int wy);
    void SetTacticalState(TacticalState state);
    void SetFormation(int leader_id, int formation_index, FormationStyle style = FormationStyle::Column);
    void ClearFormation();
    bool HasFormation() const;
    void SetBasePosition(int wx, int wy);
    bool HasBase() const;
    void SetAssignedObjective(int wx, int wy);
    bool HasAssignedObjective() const;
    void SetAttackTarget(int target_unit_id);
    void ClearAttackTarget();
    bool HasMoveTarget() const;
    bool HasAttackTarget() const;
    int GetRoleAttackWeight() const;
    int GetRoleDefenseWeight() const;
    void TakeDamage(int amount);
    void Stop();

private:
    float GetMoveSpeed() const;
    float GetTerrainPenaltyAt(int wx, int wy) const;
    float GetAttackRange() const;
    int GetAttackDamage() const;
    float GetSupplyPenalty() const;
    float GetSupplyLinePenalty() const;
    int GetTownDefenseRating(int town_x, int town_y) const;
    std::vector<std::pair<int, int>> BuildRoute(int target_x, int target_y) const;
    bool ShouldReturnToBase() const;
    bool AtBase() const;
    float GetFormationOffsetX() const;
    float GetFormationOffsetY() const;

    int id_;
    Type type_;
    Owner owner_ = Owner::Player;
    float x_, y_;
    float heading_;
    int move_target_x_ = -1;
    int move_target_y_ = -1;
    int assigned_objective_x_ = -1;
    int assigned_objective_y_ = -1;
    int attack_target_id_ = -1;
    int attack_cooldown_ = 0;
    int base_x_ = -1;
    int base_y_ = -1;
    int formation_leader_id_ = -1;
    int formation_index_ = 0;
    FormationStyle formation_style_ = FormationStyle::None;
    TacticalState tactical_state_ = TacticalState::Advance;
    std::vector<std::pair<int, int>> route_points_;
    int route_index_ = 0;
    int health_;
    int supply_ = 100;
};
