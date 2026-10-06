#include "unit.h"
#include "unit_manager.h"
#include <algorithm>

Unit::Unit(Type type, int id)
: id_(id), type_(type), x_(0), y_(0), heading_(0), health_(100) {}

Unit::~Unit() {}

int Unit::get_id() const { return id_; }

Unit::Type Unit::get_type() const { return type_; }

float Unit::GetMoveSpeed() const {
    switch (type_) {
        case Type::Infantry: return 0.8f;
        case Type::Armor:    return 1.5f;
        case Type::Artillery:return 0.7f;
        case Type::Air:      return 2.2f;
        case Type::Naval:    return 1.1f;
        default:            return 1.0f;
    }
}

float Unit::GetAttackRange() const {
    switch (type_) {
        case Type::Infantry: return 10.0f;
        case Type::Armor:    return 14.0f;
        case Type::Artillery:return 18.0f;
        case Type::Air:      return 20.0f;
        case Type::Naval:    return 16.0f;
        default:            return 12.0f;
    }
}

int Unit::GetAttackDamage() const {
    switch (type_) {
        case Type::Infantry: return 8;
        case Type::Armor:    return 14;
        case Type::Artillery:return 18;
        case Type::Air:      return 12;
        case Type::Naval:    return 10;
        default:            return 10;
    }
}

float Unit::GetTerrainPenaltyAt(int wx, int wy) const {
    const int tile_x = wx / 32;
    const int tile_y = wy / 32;
    const int pseudo = (tile_x * 13 + tile_y * 17) % 10;

    if (pseudo >= 7) return 2.4f;
    if (pseudo >= 4) return 1.6f;
    return 1.0f;
}

float Unit::GetSupplyPenalty() const {
    if (supply_ >= 75) return 1.0f;
    if (supply_ >= 40) return 1.2f;
    if (supply_ >= 20) return 1.5f;
    return 1.9f;
}

bool Unit::IsUnderSupplyPressure() const {
    return supply_ < 50;
}

bool Unit::ShouldReturnToBase() const {
    return base_x_ >= 0 && base_y_ >= 0 && (health_ < 60 || supply_ < 20);
}

bool Unit::AtBase() const {
    return base_x_ >= 0 && base_y_ >= 0 && std::abs(static_cast<int>(x_) - base_x_) <= 4 && std::abs(static_cast<int>(y_) - base_y_) <= 4;
}

float Unit::GetFormationOffsetX() const {
    if (formation_style_ == FormationStyle::None) return 0.0f;

    const float spread = 18.0f + static_cast<float>(formation_index_) * 12.0f;
    switch (formation_style_) {
        case FormationStyle::Column:
            return std::cos(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 1.57079632679f)) * spread;
        case FormationStyle::Wedge:
            return std::cos(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 0.8f)) * spread;
        case FormationStyle::Line:
            return std::cos(heading_) * spread;
        default:
            return 0.0f;
    }
}

float Unit::GetFormationOffsetY() const {
    if (formation_style_ == FormationStyle::None) return 0.0f;

    const float spread = 18.0f + static_cast<float>(formation_index_) * 12.0f;
    switch (formation_style_) {
        case FormationStyle::Column:
            return std::sin(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 1.57079632679f)) * spread;
        case FormationStyle::Wedge:
            return std::sin(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 0.8f)) * spread;
        case FormationStyle::Line:
            return std::sin(heading_) * spread;
        default:
            return 0.0f;
    }
}

bool Unit::IsMoving() const {
    return HasMoveTarget();
}

void Unit::tick() {
    if (attack_cooldown_ > 0) {
        --attack_cooldown_;
    }

    if (base_x_ >= 0 && base_y_ >= 0) {
        const int dist = std::abs(static_cast<int>(x_) - base_x_) + std::abs(static_cast<int>(y_) - base_y_);
        if (dist > 180) {
            supply_ = std::max(0, supply_ - 2);
        } else if (dist < 60) {
            supply_ = std::min(100, supply_ + 1);
        }
    }

    if (health_ > 0 && health_ < 100 && AtBase()) {
        health_ = std::min(100, health_ + 2);
        supply_ = std::min(100, supply_ + 2);
    }

    if (IsUnderSupplyPressure() && !AtBase()) {
        health_ = std::max(0, health_ - 1);
    }

    if (formation_leader_id_ >= 0 && formation_leader_id_ != id_) {
        Unit *leader = UnitManager::Instance().GetUnitById(formation_leader_id_);
        if (leader != nullptr && leader->IsAlive()) {
            const float target_x = leader->GetX() + GetFormationOffsetX();
            const float target_y = leader->GetY() + GetFormationOffsetY();
            move_target_x_ = static_cast<int>(target_x);
            move_target_y_ = static_cast<int>(target_y);
        }
    }

    if (tactical_state_ == TacticalState::Retreat && base_x_ >= 0 && base_y_ >= 0) {
        SetMoveTarget(base_x_, base_y_);
        ClearAttackTarget();
    }

    if (health_ < 35 && tactical_state_ == TacticalState::Advance) {
        SetTacticalState(TacticalState::Retreat);
    }

    if (attack_target_id_ >= 0) {
        Unit *target = UnitManager::Instance().GetUnitById(attack_target_id_);
        if (target == nullptr || !target->IsAlive()) {
            ClearAttackTarget();
        } else {
            const float dx = target->GetX() - x_;
            const float dy = target->GetY() - y_;
            const float dist = std::sqrt(dx * dx + dy * dy);
            const float attack_range = GetAttackRange();

            if (dist <= attack_range) {
                const float target_angle = std::atan2(dy, dx);
                const float delta = std::atan2(std::sin(target_angle - heading_), std::cos(target_angle - heading_));
                const float turn_rate = 0.20f;
                if (std::abs(delta) > 0.01f) {
                    heading_ += std::copysign(std::min(std::abs(delta), turn_rate), delta);
                }

                if (attack_cooldown_ <= 0) {
                    target->TakeDamage(GetAttackDamage());
                    attack_cooldown_ = 20;
                }
                return;
            }

            SetMoveTarget(static_cast<int>(target->GetX()), static_cast<int>(target->GetY()));
            ClearFormation();
        }
    }

    if (ShouldReturnToBase()) {
        SetMoveTarget(base_x_, base_y_);
        ClearFormation();
    }

    if (!HasMoveTarget()) return;

    const float dx = move_target_x_ - x_;
    const float dy = move_target_y_ - y_;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist <= 1.0f) {
        if (base_x_ >= 0 && base_y_ >= 0 && move_target_x_ == base_x_ && move_target_y_ == base_y_) {
            health_ = std::min(100, health_ + 10);
        }
        Stop();
        return;
    }

    const float target_angle = std::atan2(dy, dx);
    const float delta = std::atan2(std::sin(target_angle - heading_), std::cos(target_angle - heading_));
    const float turn_rate = 0.18f;

    if (std::abs(delta) > 0.01f) {
        heading_ += std::copysign(std::min(std::abs(delta), turn_rate), delta);
    }

    const float terrain_penalty = GetTerrainPenaltyAt(static_cast<int>(x_), static_cast<int>(y_));
    const float supply_penalty = GetSupplyPenalty();
    const float speed = (GetMoveSpeed() / terrain_penalty) / supply_penalty;
    const float step_x = std::cos(heading_) * speed;
    const float step_y = std::sin(heading_) * speed;

    if (std::sqrt(step_x * step_x + step_y * step_y) >= dist) {
        x_ = static_cast<float>(move_target_x_);
        y_ = static_cast<float>(move_target_y_);
        Stop();
        return;
    }

    x_ += step_x;
    y_ += step_y;
}

void Unit::SetMoveTarget(int wx, int wy) {
    move_target_x_ = wx;
    move_target_y_ = wy;
}

void Unit::SetTacticalState(TacticalState state) {
    tactical_state_ = state;
}

void Unit::SetFormation(int leader_id, int formation_index, FormationStyle style) {
    formation_leader_id_ = leader_id;
    formation_index_ = formation_index;
    formation_style_ = style;
}

void Unit::ClearFormation() {
    formation_leader_id_ = -1;
    formation_index_ = 0;
    formation_style_ = FormationStyle::None;
}

bool Unit::HasFormation() const {
    return formation_leader_id_ >= 0 && formation_style_ != FormationStyle::None;
}

void Unit::SetBasePosition(int wx, int wy) {
    base_x_ = wx;
    base_y_ = wy;
    if (health_ < 60 && (base_x_ >= 0 && base_y_ >= 0)) {
        SetMoveTarget(base_x_, base_y_);
    }
}

bool Unit::HasBase() const {
    return base_x_ >= 0 && base_y_ >= 0;
}

void Unit::SetAttackTarget(int target_unit_id) {
    attack_target_id_ = target_unit_id;
    move_target_x_ = -1;
    move_target_y_ = -1;
}

void Unit::ClearAttackTarget() {
    attack_target_id_ = -1;
    attack_cooldown_ = 0;
}

bool Unit::HasMoveTarget() const {
    return move_target_x_ >= 0 && move_target_y_ >= 0;
}

bool Unit::HasAttackTarget() const {
    return attack_target_id_ >= 0;
}

void Unit::TakeDamage(int amount) {
    health_ = std::max(0, health_ - amount);
    if (health_ <= 0) {
        ClearAttackTarget();
        ClearFormation();
        Stop();
    }
}

void Unit::Stop() {
    move_target_x_ = -1;
    move_target_y_ = -1;
}
