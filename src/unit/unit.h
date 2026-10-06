#pragma once

#include "stdint.h"
#include <cmath>

class Unit {
public:
    enum class Type { Infantry, Armor, Artillery, Air, Naval };

    Unit(Type type, int id);
    ~Unit();

    int get_id() const;
    Type get_type() const;
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetHeading() const { return heading_; }
    int GetHealth() const { return health_; }
    bool IsAlive() const { return health_ > 0; }
    bool IsMoving() const;

    void tick();

    // RTS-style orders
    void SetMoveTarget(int wx, int wy);
    void SetBasePosition(int wx, int wy);
    bool HasBase() const;
    void SetAttackTarget(int target_unit_id);
    void ClearAttackTarget();
    bool HasMoveTarget() const;
    bool HasAttackTarget() const;
    void TakeDamage(int amount);
    void Stop();

private:
    float GetMoveSpeed() const;
    float GetTerrainPenaltyAt(int wx, int wy) const;
    bool ShouldReturnToBase() const;
    bool AtBase() const;

    int id_;
    Type type_;
    float x_, y_;
    float heading_;
    int move_target_x_ = -1;
    int move_target_y_ = -1;
    int attack_target_id_ = -1;
    int attack_cooldown_ = 0;
    int base_x_ = -1;
    int base_y_ = -1;
    int health_;
};
